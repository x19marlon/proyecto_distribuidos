# Documentación del Proyecto

Sistema distribuido de transacciones para eventos universitarios.

Los estudiantes consultan los eventos de un mes, reservan cupos en una fecha de
un evento, y pueden modificar (cantidad de cupos y/o fecha) o cancelar sus
reservas. Cada reserva, modificación o cancelación queda registrada como una
**transacción**, administrada por la clase **`GestorTransacciones`**.
Todas las operaciones son síncronas excepto la cancelación, que se acepta de
inmediato y se aplica de forma asíncrona.

El sistema está dividido en tres capas que corren como procesos independientes
y se comunican por la red con ZeroMQ:

```
Consumidores  ──►  Servicios  ──►  Persistencia de Datos
```

## Índice

| Documento | Contenido |
|---|---|
| [requisitos.md](requisitos.md) | Requisitos funcionales y no funcionales |
| [arquitectura.md](arquitectura.md) | Las tres capas, cómo se comunican, vista de despliegue |
| [comunicacion-zeromq.md](comunicacion-zeromq.md) | Patrones ZeroMQ (REQ/ROUTER/DEALER, PUB/SUB), patrones de confiabilidad, formato de mensajes, CurveZMQ |
| [despliegue-kubernetes.md](despliegue-kubernetes.md) | Despliegue en Kubernetes con 4 máquinas (1 plano de control + 3 workers) |
| [modelos-sistema.md](modelos-sistema.md) | Modelos arquitectónico, de interacción, de fallos y de seguridad aplicados al proyecto |
| [modelo-dominio.md](modelo-dominio.md) | Entidades `Estudiante`, `Evento`, `FechaEvento`, `Lugar`, `Reserva`, `Transaccion`, `SolicitudCancelacion` |
| [capas.md](capas.md) | Responsabilidades, operaciones y tablas de cada capa |
| [flujos.md](flujos.md) | Flujos paso a paso: consultar por mes, reservar, modificar, cancelar, CRUD de eventos |
| [aspectos-distribuidos.md](aspectos-distribuidos.md) | Consistencia, concurrencia, fallos, idempotencia |
| [tolerancia-fallos-escalabilidad.md](tolerancia-fallos-escalabilidad.md) | Componentes y fases para que el sistema sea tolerante a fallos y escalable |
| [indicadores-desempeno.md](indicadores-desempeno.md) | Indicadores de desempeño, la meta de < 1 s, escenarios de carga |
| [recoleccion-metricas.md](recoleccion-metricas.md) | Herramientas y estrategias para obtener los indicadores |
| [plan-pruebas.md](plan-pruebas.md) | Cómo probamos funcionalidad, concurrencia, fallos, seguridad y desempeño |
| [decisiones.md](decisiones.md) | Decisiones tomadas y decisiones pendientes |
| [diagramas/](diagramas/README.md) | Diagramas UML (PlantUML): despliegue, componentes, clases y secuencia |

## Convenciones

- Los diagramas dentro de los `.md` están en [Mermaid](https://mermaid.js.org/) para que GitHub los muestre.
- Los diagramas UML están en PlantUML dentro de [diagramas/](diagramas/README.md).
- Todo lo marcado como **(propuesta)** es una sugerencia que el equipo aún no ha confirmado.
  La decisión final se registra en [decisiones.md](decisiones.md).
