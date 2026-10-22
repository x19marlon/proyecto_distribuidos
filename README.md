# proyecto_distribuidos

Proyecto final de Sistemas Distribuidos – equipo **crotolamo que gico**.

## ¿Qué es?

Un sistema distribuido de transacciones para los eventos de la universidad. Los
estudiantes pueden:

- consultar los eventos de un mes y los cupos disponibles de cada fecha;
- reservar uno o más cupos en una fecha de un evento;
- modificar su reserva (cantidad de cupos o fecha);
- cancelar su reserva (la cancelación se procesa de forma asíncrona).

Cada reserva, modificación o cancelación queda registrada como una transacción,
administrada por la clase `GestorTransacciones`.

## Arquitectura

Tres capas que corren como procesos separados y se comunican con **ZeroMQ**:

```
Consumidores  ──►  Servicios  ──►  Persistencia de Datos
```

El sistema se despliega en **Kubernetes** (1 plano de control + 3 workers) y usa
**PostgreSQL** como base de datos.

## Implementación (C++ orientado a objetos)

Por ahora el sistema permite **consultar los eventos de un mes** y **reservar cupos**,
sobre la parte síncrona de la arquitectura básica ([Figura 1](DOCS/diagramas/diagrama_proyecto.png)).
El detalle está en [DOCS/implementacion.md](DOCS/implementacion.md). Cada componente es un proceso:

| Capa | Proceso | Clase principal | ZeroMQ |
|---|---|---|---|
| Consumidores | `cliente` | `Cliente`, `ConsolaCliente` | REQ (Lazy Pirate) al gestor |
| Servicios de Negocio | `gestor-transacciones` | `GestorTransacciones` | ROUTER para clientes, ROUTER para procesadores |
| Servicios de Negocio | `procesador-rmc` (1 o más) | `ProcesadorRMC` (reservas y consultas) | DEALER con latidos (Paranoid Pirate) |
| Servicios de Almacenamiento | `persistencia` | `ServicioPersistencia`, `BaseDatos`, `AlmacenamientoArchivo` | REP |

```
src/comun/          Mensaje, ClienteConfiable (Lazy Pirate), Servicio (clase base), utilidades
src/consumidores/   cliente del estudiante
src/servicios/      gestor/ y rmc/
src/persistencia/   modelo (Estudiante, Evento, FechaEvento, Lugar, Reserva, Transaccion),
                    base de datos y bitácora en disco
scripts/            iniciar y detener el sistema
```

### Compilar

```bash
sudo apt install g++ make libzmq3-dev cppzmq-dev
make
```

### Ejecutar

```bash
scripts/iniciar_sistema.sh base          # o "escalable" (3 procesadores RMC)
bin/cliente EST-1                        # menú del estudiante (EST-1, EST-2 y EST-3 ya existen)
scripts/detener_sistema.sh
```

Los eventos de ejemplo están en noviembre y diciembre de 2026 (consulta el mes `2026-11`).

## Integrantes

- mariananvv16
- martinezm-oe
- Nick_07
- x19marlon

## Documentación

Toda la documentación del diseño está en [DOCS/](DOCS/README.md).
