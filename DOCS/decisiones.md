# Decisiones

## Tomadas

| # | Decisión | Motivo |
|---|---|---|
| D1 | Tres capas en orden: Consumidores → Servicios → Persistencia de Datos | Requisito del proyecto |
| D2 | Las clases principales son `Estudiante` y `Transaccion`, más `Evento`, `FechaEvento`, `Lugar`, `Reserva` y `SolicitudCancelacion` | Requisito del proyecto |
| D3 | Cada capa solo llama a la capa directamente debajo | Separación de responsabilidades; las capas se pueden desplegar de forma independiente |
| D4 | Los eventos tienen CRUD completo | Requisito del proyecto |
| D5 | Cada fecha de evento guarda sus `reservas` actuales y comparte el límite del evento | Requisito del proyecto |
| D6 | Los edificios/teatros de la universidad son registros `Lugar`; el límite del evento es la `capacidad` del lugar | Requisito del proyecto |
| D7 | Los lugares externos son texto libre en el evento, con una `capacidadExterna` manual | Decisión del equipo |
| D8 | Algunos eventos son pagos y otros gratuitos (`precio = 0`) | Requisito del proyecto |
| D9 | Campos del estudiante: nombre, código de estudiante, tipo + número de documento, carrera, departamento, semestre, correo, teléfono | Requisito del proyecto |
| D10 | Un estudiante puede tener reservas en N eventos, como máximo una reserva activa por evento | Requisito del proyecto |
| D11 | Un evento puede tener una o más fechas | Requisito del proyecto |
| D12 | Una reserva tiene N cupos; los estudiantes consultan los eventos por mes y pueden modificar los cupos y/o la fecha de una reserva | Requisito del proyecto |
| D13 | Todas las operaciones son petición/respuesta síncrona, excepto la cancelación | Requisito del proyecto |
| D14 | `Reserva` guarda el estado actual; `Transaccion` es el registro inmutable de cada cambio | Facilita consultar la reserva actual sin perder el historial |
| D15 | La cancelación es asíncrona: aceptar → responder "en proceso" → un trabajador la aplica en la capa de datos | Requisito del proyecto |
| D16 | Las cancelaciones aceptadas se guardan en la base de datos (`solicitud_cancelacion`) antes de responder, y luego se anuncian por ZeroMQ PUB/SUB; los trabajadores las toman y un barrido recupera las notificaciones perdidas | ZeroMQ PUB/SUB no es durable; una cancelación aceptada debe sobrevivir a una caída (RNF-06) |
| D17 | Meta de tiempo de transacción: p95 < 1 s de extremo a extremo (cancelación: aceptación) | Requisito del proyecto |
| D18 | Cada capa registra los tiempos de las solicitudes por `idSolicitud`; el administrador puede ver los indicadores de desempeño | Requisito del proyecto |
| D19 | Modelo de fallos: los procesos se caen (sin comportamiento bizantino), los canales pueden perder/retrasar/duplicar mensajes; un timeout significa "desconocido" | Base del diseño de reintentos + idempotencia |
| D20 | El `idEstudiante` de una operación siempre sale del token autenticado, nunca del cuerpo de la solicitud | Impide actuar sobre datos de otros estudiantes |
| D21 | Toda la comunicación entre procesos usa ZeroMQ: REQ → ROUTER (broker Majordomo) → DEALER para las operaciones de reserva; PUB/SUB a través de un proxy XSUB/XPUB para las cancelaciones | Decisión del equipo |
| D22 | La tolerancia a fallos y la escalabilidad usan los patrones de confiabilidad de ZeroMQ: Lazy Pirate (clientes), Paranoid Pirate (heartbeats), Majordomo (broker, balanceo de carga), Binary Star (alta disponibilidad del broker) | Decisión del equipo |
| D23 | Despliegue en Kubernetes: 4 máquinas, 1 plano de control + 3 nodos worker | Decisión del equipo |
| D24 | Base de datos: PostgreSQL con CloudNativePG (primaria + réplicas, failover automático) | Encaja con Kubernetes, transacciones atómicas, failover |
| D25 | Cifrado y autenticación interna con CurveZMQ (reemplaza TLS) | ZeroMQ no usa TLS |
| D26 | Todo el proyecto (documentación, diagramas, nombres de clases, operaciones, códigos de error y código) se escribe en español | Requisito del curso |
| D27 | La clase de servicio que administra reservas, modificaciones, cancelaciones y recargas se llama `GestorTransacciones` (servicio ZeroMQ `transacciones`, pod `gestor-transacciones`) | Requisito del proyecto |
| D28 | Para las métricas del curso, `modificarReserva` cuenta como operación de reserva (tiempos y TPM), y además se reporta desglosada | Los escenarios de 5 y 8 clientes mezclan reservas y modificaciones |
| D29 | Arquitectura base = Fase 0 (una réplica de cada componente); arquitectura escalable = Fase 1 (réplicas de gestores y trabajadores de persistencia, PostgreSQL con réplica) | Permite comparar en el mismo clúster cambiando solo las réplicas |
| D30 | Tiempos de respuesta medidos del lado del consumidor; TPM de consultas en la capa de persistencia y TPM de reservas en la BD (tabla `transacciones`) | El enunciado permite medir en el gestor de transacciones o en la BD; las consultas no pasan por `GestorTransacciones` |
| D31 | Los clientes de los escenarios del curso funcionan en ciclo cerrado, sin tiempo de reflexión: 2 min de calentamiento + 10 min de medición, 3 repeticiones | Mide la capacidad real con pocos clientes y da resultados repetibles |

## Pendientes

Llenar la columna "Decisión" cuando el equipo se ponga de acuerdo.

| # | Pregunta | Opciones | Decisión |
|---|---|---|---|
| O1 | Lenguaje de programación | Java, Python, Go, Node.js | |
| O2 | Comunicación síncrona entre capas | **Decidido: ZeroMQ** (D21) | ✔ |
| O3 | Almacenamiento | **Decidido: PostgreSQL** (D24) | ✔ |
| O4 | Control de concurrencia | Bloqueo de fila, bloqueo optimista, actualización condicional, escritor único (ver [aspectos-distribuidos.md](aspectos-distribuidos.md)) | |
| O5 | Tipo de consumidor | CLI, interfaz de escritorio, web | |
| O6 | ¿Cómo se pagan los eventos pagos? | Saldo prepagado del estudiante con recargas (diseño actual), pago registrado sin saldo | |
| O7 | ¿Dos eventos pueden usar el mismo lugar en la misma fecha y hora? | Permitido, bloqueado | |
| O8 | ¿El `departamento` es texto libre o una lista fija (y la `carrera` está ligada a un departamento)? | Texto libre, tabla de catálogo | |
| O9 | ¿Qué fase de tolerancia a fallos entra en el alcance? (ver [tolerancia-fallos-escalabilidad.md](tolerancia-fallos-escalabilidad.md#6-fases-de-implementación)) | Al menos Fase 1 (es la arquitectura escalable, D29); Fase 2 opcional | Fase 1 mínimo |
| O10 | ¿Máximo de cupos por reserva? | Sin límite (solo disponibilidad), máximo fijo (p. ej. 5) | |
| O11 | ¿Plazo para modificar/cancelar? | Hasta la fecha del evento, N horas antes | |
| O12 | ¿Un administrador puede mover una fecha que ya tiene reservas? | Bloqueado, permitido (las reservas siguen a la fecha) | |
| O13 | Cola durable para las cancelaciones | **Decidido:** tabla en la base de datos + ZeroMQ PUB/SUB (D16) | ✔ |
| O14 | Mientras una cancelación está pendiente, ¿se puede modificar la reserva? | Bloqueado con `CANCELACION_PENDIENTE` (diseño actual), permitido (la cancelación aplica al último estado) | |
| O15 | ¿Cómo se entera el estudiante del resultado final? | Consulta `consultarEstadoSolicitud` (diseño actual), notificación push / correo | |
| O16 | Herramienta de métricas (ver [recoleccion-metricas.md](recoleccion-metricas.md)) | Vía A: registros de tiempo JSON + script de análisis; Vía B: A + Prometheus, Grafana, exportadores, OpenTelemetry/Jaeger (recomendado: A desde el inicio, B antes de las pruebas de fallos) | |
| O17 | Herramienta de pruebas de carga | Script propio en el lenguaje del proyecto con un proceso por cliente (recomendado: solo son 3–8 clientes y hablan ZeroMQ), Locust con cliente ZeroMQ, k6 | |
| O18 | ¿Basta con p95 < 1 s, o debe ser p99 / todas las transacciones? | p95, p99, máximo | |
| O19 | ¿Cómo se autentican los usuarios? | SSO/LDAP de la universidad, tabla propia de usuarios/contraseñas, sin autenticación (solo demo) | |
| O20 | ¿Cómo se autentican los procesos internos? | Llaves CurveZMQ + manejador ZAP (propuesta), solo aislamiento de red | |
| O21 | ¿Cifrar los datos personales en reposo? | Solo la columna del número de documento, todo el disco, no | |
| O22 | Balanceador de carga | **Decidido:** broker Majordomo (D22) | ✔ |
| O23 | Failover de la base de datos | **Decidido:** automático con CloudNativePG (D24) | ✔ |
| O24 | ¿Usar caché para los listados? | Redis con TTL corto, sin caché | |
| O25 | Distribución de Kubernetes | k3s (propuesta), kubeadm | |
| O26 | Acceso externo al broker-servicios | NodePort (propuesta), MetalLB | |
| O27 | ¿Pares de brokers Binary Star desde el inicio o en la fase 2? | Fase 1: un solo broker + reinicios; Fase 2: Binary Star | |
| O28 | Serialización de los mensajes | JSON (propuesta), MessagePack, Protobuf | |
