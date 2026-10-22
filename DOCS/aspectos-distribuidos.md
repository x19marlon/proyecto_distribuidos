# Aspectos de Sistemas Distribuidos

Como las capas corren como procesos separados en una red, los siguientes
problemas se deben manejar de forma explícita.

## Concurrencia

**Problema:** dos estudiantes intentan tomar los últimos cupos de una fecha de
evento al mismo tiempo (reservando, o modificando una reserva a más cupos / a
otra fecha). Ambos leen el mismo valor de `reservas`, ambos ven espacio
suficiente y ambos tienen éxito → sobrecupo.

**Opciones (elegir una y registrarla en [decisiones.md](decisiones.md)):**

| Enfoque | Cómo |
|---|---|
| Transacción de BD + bloqueo de fila | `SELECT ... FOR UPDATE` sobre las filas del estudiante y de la fecha dentro de `aplicarCambioReserva` |
| Bloqueo optimista | Cada registro tiene una `version`; la actualización falla si la versión cambió y el servicio reintenta |
| Actualización condicional | `UPDATE fecha_evento SET reservas = reservas + ? WHERE id = ? AND reservas + ? <= <limite>` y revisar las filas afectadas |
| Escritor único | Serializar todas las escrituras en un solo hilo/cola dentro del proceso de persistencia |

### Modificaciones que tocan dos fechas

Pasar una reserva de la fecha D1 a la D2 cambia dos filas de `FECHA_EVENTO`, la
reserva y el saldo, e inserta una transacción. Todo debe ser una sola unidad
atómica: si D2 no tiene espacio, D1 conserva sus cupos y la reserva queda igual.

Si se usan bloqueos de fila, siempre se bloquean en un orden fijo (p. ej. por
`id`) para que un estudiante que mueve D1 → D2 mientras otro mueve D2 → D1 no
produzcan un interbloqueo (deadlock).

## Operación síncrona

Toda operación excepto la cancelación es síncrona (ver
[arquitectura.md](arquitectura.md#comunicación)):

- Un servicio responde solo después de que la persistencia confirmó o rechazó el cambio.
- El consumidor nunca ve un estado intermedio; una reserva queda completamente
  modificada o no se modifica.
- Como quien llama queda bloqueado, toda llamada necesita un timeout, y un
  timeout significa que el resultado es desconocido → reintentar con el mismo
  `idSolicitud`.

## Cancelación asíncrona

La cancelación primero se acepta y después la aplica un trabajador, lo que trae
sus propios problemas:

| Problema | Manejo |
|---|---|
| Servicios se cae después de responder `ACEPTADA` pero antes de publicar | La solicitud se **guardó** como `PENDIENTE` antes de responder; el barrido de los trabajadores la encuentra y la aplica. |
| Persistencia no disponible cuando el estudiante cancela | Responder `PERSISTENCIA_NO_DISPONIBLE`; no se acepta nada y el estudiante puede reintentar. Nunca responder `ACEPTADA` sin guardar antes. |
| ZeroMQ `PUB`/`SUB` pierde la notificación (sin suscriptor, suscriptor tardío, reinicio del proxy, high-water mark) | El barrido (cada 2 s) toma las solicitudes `PENDIENTE` con más de 2 s. |
| Varios trabajadores reciben la misma notificación | Solo uno gana la **toma** en la base de datos (`PENDIENTE → EN_PROCESO`); los demás la ignoran. |
| Un trabajador se cae después de tomarla y antes de aplicarla | El bloqueo de 30 s vence y el barrido la vuelve a tomar. |
| Un trabajador la aplica dos veces (p. ej. retomada después de una confirmación lenta) | `aplicarCancelacion` revisa si ya existe una transacción con ese `idSolicitud` y en ese caso no hace nada, así el efecto ocurre **exactamente una vez**. |
| Persistencia caída cuando corre el trabajador | La solicitud sigue `PENDIENTE` / el bloqueo vence; se aplica cuando vuelve la persistencia. |
| La reserva ya no se puede cancelar cuando corre el trabajador (fecha pasada, ya cancelada) | Registrar una transacción `FALLIDA` con el motivo; la solicitud queda `FALLIDA`. |
| El estudiante reintenta la cancelación | Mismo `idSolicitud` → misma respuesta `ACEPTADA`, sin segunda solicitud. |
| El estudiante modifica la reserva mientras la cancelación está pendiente | Se rechaza con `CANCELACION_PENDIENTE` (ver decisión O14). |
| El estudiante consulta datos mientras la cancelación está pendiente | Ve la reserva todavía activa, marcada "cancelación pendiente". Los cupos quedan libres para otros solo cuando el trabajador confirma (**consistencia eventual** solo para esta operación). |
| Orden de las solicitudes | Como máximo una cancelación pendiente por reserva (restricción única), así que los trabajadores nunca necesitan coordinar el orden. |

## Idempotencia y reintentos

**Problema:** el consumidor envía una solicitud, el servicio la procesa, pero la
respuesta se pierde. El consumidor reintenta → al estudiante se le cobra dos veces.

**Solución:** toda escritura lleva un `idSolicitud` generado por el cliente. La
persistencia lo guarda con una restricción única. Si el mismo `idSolicitud`
llega otra vez, el servicio devuelve el resultado guardado en lugar de ejecutar
de nuevo.

## Fallos parciales

| Fallo | Comportamiento esperado |
|---|---|
| Persistencia caída | El servicio devuelve `PERSISTENCIA_NO_DISPONIBLE`; no se escriben datos parciales |
| Servicio caído | El consumidor agota el timeout y reintenta con el mismo `idSolicitud` (con espera creciente) |
| Caída en medio de una confirmación | La confirmación atómica garantiza todo o nada; una transacción `PENDIENTE` que quede se resuelve al reiniciar (marcada `FALLIDA` o completada) |
| Timeout de red | Tratar el resultado como desconocido → reintentar con el mismo `idSolicitud` |

## Consistencia

- Las escrituras que cambian más de un registro (reserva, contadores de fechas,
  saldo, transacción) deben ser atómicas en la capa de persistencia.
- `FechaEvento.reservas` debe ser siempre igual a la suma de `cupos` de las
  reservas activas en esa fecha; ambos se actualizan en la misma unidad atómica.
- Las lecturas pueden estar levemente desactualizadas si más adelante se usan
  réplicas o caché; las verificaciones de saldo y de límite de reservas siempre
  deben usar el almacenamiento principal.

## Timeouts

Toda llamada por red tiene un timeout. Como las transacciones deben tardar menos
de 1 s (ver [indicadores-desempeno.md](indicadores-desempeno.md)), **(propuesta)**:
1 s por intento consumidor→servicios (3 intentos) y 300 ms por intento
servicios→persistencia (2 intentos), siguiendo el patrón Lazy Pirate
([comunicacion-zeromq.md](comunicacion-zeromq.md#3-timeouts-y-opciones-de-socket)).
Nunca esperar indefinidamente.

## Logs y trazabilidad

Registrar cada solicitud en cada capa con su `idSolicitud`, operación,
resultado, hora de inicio y fin, y duración. Esto permite seguir una operación a
través de los procesos y es la fuente de los indicadores de desempeño.

## Posibles extensiones

- Replicar la base de datos (primaria + réplicas en espera) para tolerancia a fallos.
- Correr varias réplicas de cada servicio registradas en el broker Majordomo
  (requiere servicios sin estado, que este diseño ya asume).
- Correr varios trabajadores de cancelaciones si crecen las cancelaciones pendientes.
- Notificar al estudiante (correo/push) cuando el trabajador termine una
  cancelación, en lugar de que tenga que consultar `consultarEstadoSolicitud`.

El diseño completo de estas extensiones está en
[tolerancia-fallos-escalabilidad.md](tolerancia-fallos-escalabilidad.md).
