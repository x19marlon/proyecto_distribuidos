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

## Integrantes

- mariananvv16
- martinezm-oe
- Nick_07
- x19marlon

## Documentación

Toda la documentación del diseño está en [DOCS/](DOCS/README.md).
