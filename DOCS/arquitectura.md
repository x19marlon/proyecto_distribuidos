# Arquitectura

## Visión general

El sistema sigue una arquitectura estricta de tres capas. Cada capa solo se
comunica con la capa que está directamente debajo:

```mermaid
flowchart LR
    subgraph L1[1. Consumidores]
        C1[Cliente estudiante]
        C2[Cliente administrador / organizador]
    end

    subgraph L2[2. Servicios]
        S1[Gestor de Estudiantes]
        S3[Gestor de Eventos]
        S2[GestorTransacciones]
        Q[[Proxy de cancelaciones PUB/SUB]]
        W[Trabajador de cancelaciones]
    end

    subgraph L3[3. Persistencia de Datos]
        P1[(Estudiantes)]
        P3[(Eventos, fechas y lugares)]
        P2[(Reservas y transacciones)]
    end

    C1 --> S1
    C1 --> S3
    C1 --> S2
    C2 --> S1
    C2 --> S3
    C2 --> S2
    S1 --> P1
    S3 --> P3
    S2 --> P2
    S2 -. valida estudiante .-> S1
    S2 -. consulta evento y límite .-> S3
    S2 -- guarda solicitud --> P2
    S2 -- publica cancelación --> Q
    Q -- suscripción --> W
    W --> P2
```

| Capa | Rol | Conoce |
|---|---|---|
| **Consumidores** | Punto de entrada de los usuarios. Arma las solicitudes y muestra los resultados. Sin reglas de negocio. | Solo la API de la capa de servicios |
| **Servicios** | Lógica de negocio: CRUD de estudiantes, eventos y lugares, listado de eventos por mes, reservar / modificar / cancelar cupos, verificación de disponibilidad y saldo, transacciones (`GestorTransacciones`). | Solo la API de la capa de persistencia |
| **Persistencia de Datos** | Guarda y recupera de forma durable los registros de `Estudiante`, `Lugar`, `Evento`, `FechaEvento`, `Reserva`, `Transaccion` y `SolicitudCancelacion`. Sin reglas de negocio. | Su propio almacenamiento |

Reglas:

1. Un consumidor **nunca** habla directamente con la persistencia.
2. La persistencia **nunca** llama hacia arriba a servicios o consumidores.
3. Los servicios pueden llamar a otros servicios (por ejemplo, `GestorTransacciones`
   le pregunta al Gestor de Estudiantes si un estudiante existe).

## Comunicación

Cada capa corre como su propio proceso (posiblemente en máquinas distintas), así
que cada flecha de arriba es una llamada por la red.

### Operaciones síncronas (todo excepto la cancelación)

Quien llama queda bloqueado hasta que la capa de abajo responde. Una cadena se ve así:

```mermaid
sequenceDiagram
    participant C as Consumidor
    participant S as Servicio
    participant P as Persistencia
    C->>S: solicitud (bloquea)
    S->>P: solicitud (bloquea)
    P-->>S: confirmado / error
    S-->>C: resultado final / error
```

Cuando llega la respuesta, el cambio ya está confirmado (o definitivamente no
se aplicó).

### Operación asíncrona: cancelación

Cancelar una reserva **no** espera a que la cancelación se aplique.
`GestorTransacciones` valida la solicitud, la **guarda** en la capa de datos como
una solicitud de cancelación `PENDIENTE`, responde de inmediato con `ACEPTADA`
("tu cancelación está en proceso") y **publica** una notificación. Un
**trabajador de cancelaciones** suscrito a esas notificaciones toma la solicitud
y aplica la cancelación en la capa de datos.

```mermaid
sequenceDiagram
    participant C as Consumidor
    participant S as GestorTransacciones
    participant X as Proxy de cancelaciones (PUB/SUB)
    participant W as Trabajador de cancelaciones
    participant P as Persistencia
    C->>S: cancelarReserva (bloquea poco tiempo)
    S->>P: guarda solicitud de cancelación (PENDIENTE)
    S-->>C: ACEPTADA: "cancelación en proceso"
    S->>X: PUB cancelacion {idSolicitud}
    Note over C: el consumidor queda libre
    X->>W: SUB recibe la notificación
    W->>P: toma la solicitud + aplica la cancelación (atómico)
    P-->>W: confirmada / fallida
    C->>S: consultarEstadoSolicitud (después)
    S-->>C: PENDIENTE / CONFIRMADA / FALLIDA
```

PUB/SUB puede perder notificaciones, así que los trabajadores también revisan la
capa de datos buscando solicitudes pendientes: la solicitud guardada es la
garantía y la notificación es el camino rápido. El proxy y el trabajador
pertenecen a la capa de servicios; el consumidor sigue hablando solo con
servicios, y solo los servicios hablan con persistencia.

### Tecnología: ZeroMQ

Todos los enlaces usan ZeroMQ ([comunicacion-zeromq.md](comunicacion-zeromq.md)):

| Enlace | Patrón |
|---|---|
| Consumidores → Servicios | `REQ` → broker Majordomo (`ROUTER`/`ROUTER`) → trabajadores de servicio `DEALER` |
| Servicios → Persistencia | Igual, a través de un broker interno de persistencia |
| GestorTransacciones → Trabajadores de cancelaciones | `PUB` → proxy `XSUB`/`XPUB` → `SUB` |

La tolerancia a fallos usa los patrones de confiabilidad de ZeroMQ (Lazy Pirate,
Paranoid Pirate, Majordomo, Binary Star); ver
[tolerancia-fallos-escalabilidad.md](tolerancia-fallos-escalabilidad.md).

Todo mensaje de solicitud lleva:

| Campo | Propósito |
|---|---|
| `idSolicitud` | ID único usado para idempotencia y para correlacionar logs |
| `operacion` | Nombre de la acción, p. ej. `reservar` |
| `token` | Token de sesión firmado (identidad y rol) |
| `datos` | Datos propios de la operación |
| `marcaTiempo` | Momento en que se creó la solicitud |
| `traceparent` | Contexto de traza para trazabilidad distribuida |

Las respuestas llevan `idSolicitud`, un `estado` (`OK` / `ERROR`, o `ACEPTADA`
para una cancelación asíncrona), y un `resultado` o un `error` con código y
mensaje.

## Vista de despliegue

El sistema corre en **Kubernetes con 4 máquinas: 1 plano de control y 3 nodos
worker** ([despliegue-kubernetes.md](despliegue-kubernetes.md)). Los
consumidores corren fuera del clúster.

```mermaid
flowchart TB
    subgraph Fuera[Fuera del clúster]
        Consumidor1[Procesos consumidores]
    end
    subgraph Cluster[Clúster Kubernetes]
        CP[cp-1<br/>plano de control]
        subgraph W1[worker-1]
            A1[brokers, servicios,<br/>trabajadores de persistencia, instancia BD]
        end
        subgraph W2[worker-2]
            A2[brokers, servicios,<br/>trabajadores de persistencia, instancia BD]
        end
        subgraph W3[worker-3]
            A3[servicios, trabajadores de persistencia,<br/>instancia BD, monitoreo]
        end
    end
    Consumidor1 -- ZeroMQ REQ --> W1 & W2
    CP -. administra .- W1 & W2 & W3
```

Para desarrollo, todos los procesos pueden correr en una sola máquina (o en un
Kubernetes local como k3d/kind) en puertos distintos. Las direcciones y puertos
salen de la configuración, nunca quedan escritos en el código.
