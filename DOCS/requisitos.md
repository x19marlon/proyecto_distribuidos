# Requisitos

## Requisitos funcionales

### Eventos

| ID | Requisito |
|---|---|
| RF-01 | El sistema debe permitir crear, consultar, actualizar y eliminar (CRUD) eventos de la universidad. |
| RF-02 | Cada evento tiene un **nombre**, un **lugar** (dónde se realiza), un **precio** y **una o más fechas**. |
| RF-03 | Un evento puede ser **gratuito** o **pago**. Los eventos pagos tienen un precio por cupo que se le cobra al estudiante. |
| RF-04 | El lugar de un evento es un **lugar de la universidad** (edificio o teatro) o un **lugar externo**. |
| RF-05 | Si el lugar es de la universidad, debe estar registrado en la base de datos, y el límite de reservas del evento es la **capacidad** del lugar. |
| RF-06 | Si el lugar es externo, se guarda como una dirección en texto libre en el evento, y el límite de reservas se define manualmente en el evento. |
| RF-07 | Cada fecha de un evento lleva su propio contador de **reservas** (cupos ocupados), que nunca puede superar el límite del evento. |

### Lugares

| ID | Requisito |
|---|---|
| RF-08 | El sistema debe permitir el CRUD de lugares de la universidad (edificios y teatros), cada uno con nombre, tipo y capacidad. |
| RF-09 | Un lugar que está siendo usado por un evento no se puede eliminar. |

### Estudiantes

| ID | Requisito |
|---|---|
| RF-10 | Cuando un estudiante hace una reserva, su información debe quedar registrada en el sistema (si aún no lo está). |
| RF-11 | Información del estudiante: nombre, código de estudiante, tipo de documento, número de documento, carrera, departamento, semestre, correo institucional, teléfono. |

### Consultar y reservar

| ID | Requisito |
|---|---|
| RF-12 | Un estudiante puede consultar los eventos de un **mes** (todas las fechas de eventos que caen en ese mes). |
| RF-13 | Para cada fecha de un evento, el estudiante puede consultar las **reservas de los cupos**: cupos ocupados, límite y cupos disponibles. |
| RF-14 | Un estudiante puede reservar **uno o más cupos** en una fecha de un evento. |
| RF-15 | Un estudiante puede tener reservas en **cualquier cantidad** de eventos (N eventos), pero como máximo una reserva activa por evento. |
| RF-16 | Un estudiante puede consultar todas sus reservas (evento, fecha, lugar, cantidad de cupos). |
| RF-17 | Un estudiante puede **modificar una reserva**: cambiar la **cantidad de cupos** y/o pasarla a **otra fecha** del mismo evento. |
| RF-18 | Una modificación solo se acepta si la fecha destino tiene suficientes cupos disponibles. En eventos pagos se cobra o se reembolsa la diferencia de precio. |
| RF-19 | Un estudiante puede cancelar una reserva; los cupos se liberan y, en eventos pagos, se reembolsa lo pagado. |
| RF-19a | La cancelación es **asíncrona**: primero el sistema acepta la solicitud y responde de inmediato con un mensaje indicando que la cancelación está en proceso; después realiza la cancelación y actualiza la capa de datos. |
| RF-19b | El estudiante puede consultar el estado de una solicitud de cancelación (`PENDIENTE`, `CONFIRMADA`, `FALLIDA`). |

### Transacciones

| ID | Requisito |
|---|---|
| RF-20 | Cada reserva, modificación y cancelación queda registrada como una transacción, administrada por la clase `GestorTransacciones`. |
| RF-21 | Un estudiante puede consultar su historial de transacciones. |

## Requisitos no funcionales

| ID | Requisito |
|---|---|
| RNF-01 | El sistema está dividido en tres capas, en este orden: Consumidores → Servicios → Persistencia de Datos. Cada una corre como un proceso separado. |
| RNF-02 | **Operación síncrona:** toda solicitud **excepto la cancelación** (consultar, reservar, modificar, CRUD) es una petición/respuesta bloqueante. El consumidor espera hasta que el cambio quede confirmado en persistencia y recibe el resultado final (éxito o error) en la misma llamada. No existe un estado de "aceptado, se procesa después". |
| RNF-03 | Las reservas o modificaciones concurrentes nunca pueden superar el límite de una fecha de evento. |
| RNF-04 | Reintentar una solicitud (por ejemplo, después de un timeout) nunca debe reservar, modificar ni cobrar dos veces. |
| RNF-05 | Los cambios que afectan varios registros (reserva, contadores de una o dos fechas, saldo, transacción) son atómicos: se aplican todos o ninguno. |
| RNF-06 | **Cancelación asíncrona:** una cancelación aceptada no se puede perder, aunque el proceso de servicios falle antes de aplicarla, y debe aplicarse exactamente una vez. |
| RNF-07 | **Tiempo de transacción < 1 s:** toda operación (y la aceptación de una cancelación) termina de extremo a extremo en menos de un segundo para al menos el 95 % de las solicitudes (p95). Ver [indicadores-desempeno.md](indicadores-desempeno.md). |
| RNF-08 | **Visibilidad del desempeño:** el sistema registra el tiempo de cada solicitud en cada capa, y el administrador puede ver los indicadores de desempeño (tiempos de respuesta, throughput, errores, cancelaciones pendientes, verificaciones de consistencia). |
| RNF-09 | **Evaluación del desempeño:** el desempeño se evalúa con escenarios de prueba de carga definidos, de forma cuantitativa (indicadores) y cualitativa (capacidad de respuesta, estabilidad, recuperación). |
| RNF-10 | **Seguridad – autenticación y autorización:** los usuarios y los procesos internos se autentican; los estudiantes solo pueden ver y cambiar sus propios datos y reservas; las operaciones de administración requieren el rol de administrador. Ver [modelos-sistema.md](modelos-sistema.md#4-modelo-de-seguridad). |
| RNF-11 | **Seguridad – confidencialidad e integridad:** todos los canales de red van cifrados (CurveZMQ); los datos personales (en especial el número de documento) se enmascaran en las respuestas a quienes no son administradores y nunca se escriben en los logs. |
| RNF-12 | **Tolerancia a fallos:** la caída o la pérdida de mensajes de cualquier proceso o canal individual no debe causar operaciones duplicadas, perdidas o aplicadas a medias. Ver [modelos-sistema.md](modelos-sistema.md#3-modelo-de-fallos). |
| RNF-13 | **Comunicación:** toda la comunicación entre procesos usa ZeroMQ: petición–respuesta (`REQ`/`ROUTER`/`DEALER` con un broker Majordomo) para las operaciones de reserva y publicación–suscripción para las cancelaciones. Ver [comunicacion-zeromq.md](comunicacion-zeromq.md). En la entrega 1 el `GestorTransacciones` cumple el papel del broker ([implementacion.md](implementacion.md)). |
| RNF-14 | **Despliegue:** el sistema corre en Kubernetes sobre 4 máquinas (1 plano de control + 3 nodos worker). Ver [despliegue-kubernetes.md](despliegue-kubernetes.md). |
| RNF-15 | **Evaluación de desempeño del curso:** se comparan una arquitectura base y una arquitectura escalable con 3, 5 y 8 clientes simultáneos (3: 2 de consulta + 1 de consulta y reserva; 5: 3 de consulta + 2 de reserva y modificación; 8: 4 de consulta + 4 de reserva y modificación). Se mide el tiempo promedio de respuesta y su desviación estándar para consultas y para reservas, y las transacciones por minuto (TPM) de consultas y de reservas, medidas en el gestor de transacciones o en la BD. Los resultados se presentan en una tabla y en gráficos en función del número de clientes, comparando ambas arquitecturas, con comentarios en el informe. Ver [plan-pruebas.md](plan-pruebas.md#10-pruebas-de-desempeño-del-curso-arquitectura-base-vs-escalable). |
