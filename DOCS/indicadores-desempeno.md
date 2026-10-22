# Indicadores de Desempeño

El sistema debe permitir al equipo **ver cómo se desempeña** y **decidir** qué
cambiar. Este documento lista los indicadores (KPI) a medir, cómo medirlos, la
meta de cada uno y qué decisión apoya cada indicador.

## Meta principal: tiempo de transacción < 1 segundo

| Tipo de operación | Qué significa "tiempo de transacción" | Meta |
|---|---|---|
| Escrituras síncronas: `reservar`, `modificarReserva`, `recargarSaldo`, CRUD | Desde que el consumidor envía la solicitud hasta que recibe el resultado final (confirmado o error) | **< 1 s** |
| Lecturas síncronas: `listarEventosPorMes`, `obtenerEvento`, `listarReservasEstudiante`, historial | Igual que arriba | **< 1 s** |
| `cancelarReserva` asíncrona – aceptación | Hasta que el consumidor recibe `ACEPTADA` | **< 1 s** |
| `cancelarReserva` asíncrona – finalización | Desde `ACEPTADA` hasta que el trabajador la confirma en la capa de datos | **< 1 s** con carga normal (propuesta); nunca más de 10 s |

"< 1 s" se mide como el **percentil 95 (p95)**: al menos el 95 % de las
transacciones terminan en menos de un segundo. También se registra el máximo
para que se vean los valores atípicos. **(propuesta)** Meta más estricta: p99 < 1 s.

## Cómo se miden los tiempos

Cada capa registra, por cada solicitud, su `idSolicitud`, operación, hora de
inicio, hora de fin, resultado (`OK` / código de error / `ACEPTADA`) y el nombre
de la capa. Como el mismo `idSolicitud` pasa por todas las capas, el tiempo de una
transacción se puede dividir por capa:

Ejemplo de cómo se reparte un `reservar()` que tarda 420 ms:

| Tramo | Tiempo |
|---|---|
| Red consumidor → servicio | 30 ms |
| Validación del servicio y llamadas a otros servicios | 150 ms |
| Confirmación atómica en persistencia | 180 ms |
| El servicio arma la respuesta | 30 ms |
| Red servicio → consumidor | 30 ms |
| **De extremo a extremo** | **420 ms** |

- **Tiempo de extremo a extremo** = el consumidor recibe la respuesta − el consumidor envía la solicitud.
- **Tiempo del servicio** = el servicio envía la respuesta − el servicio recibe la solicitud.
- **Tiempo de persistencia** = lo mismo dentro de la persistencia.
- **Tiempo de red** ≈ extremo a extremo − tiempo del servicio (y tiempo del servicio − tiempo de persistencia).

Todos los relojes reportan en milisegundos. Las comparaciones entre máquinas usan
duraciones medidas en la misma máquina, nunca marcas de tiempo de dos relojes distintos.

## Lista de indicadores

### 1. Tiempo de respuesta (latencia)

| ID | Indicador | Cómo se calcula | Meta | Decisión que apoya |
|---|---|---|---|---|
| L-01 | Tiempo de extremo a extremo por operación (p50, p95, p99, máx) | Duraciones del lado del consumidor agrupadas por operación | p95 < 1 s | Si el sistema cumple el requisito principal |
| L-02 | % de transacciones por debajo de 1 s | `count(tiempo < 1000 ms) / count(todas)` | ≥ 95 % | Lo mismo; un número fácil de reportar |
| L-03 | Tiempo de la capa de servicios por operación | Duraciones del lado de servicios | p95 < 600 ms | Optimizar la lógica de negocio o las llamadas entre servicios |
| L-04 | Tiempo de persistencia por operación | Duraciones del lado de persistencia | p95 < 300 ms | Agregar índices, cambiar la estrategia de bloqueo (O4) |
| L-05 | Tiempo de red por salto | Extremo a extremo − tiempo de la capa interna | p95 < 100 ms | Ajustar la serialización (O28), acercar los procesos |
| L-06 | Tiempo de aceptación de una cancelación | Tiempo hasta `ACEPTADA` | p95 < 1 s | Velocidad para guardar la solicitud (persistencia) |
| L-07 | Tiempo de finalización de una cancelación | `confirmadaEn − aceptadaEn` | p95 < 1 s, máx < 10 s | Cantidad de trabajadores |
| L-08 | **Tiempo promedio y desviación estándar de las consultas** (requisito del curso) | Media y desviación estándar muestral de `listarEventosPorMes` + `obtenerEvento` (lado del consumidor, solo `OK`) | Se compara base vs escalable | Si la arquitectura escalable reduce el tiempo de consulta |
| L-09 | **Tiempo promedio y desviación estándar de las reservas** (requisito del curso) | Igual, para `reservar` + `modificarReserva` (también desglosado) | Se compara base vs escalable | Si la arquitectura escalable reduce el tiempo de reserva |

### 2. Throughput y carga

| ID | Indicador | Cómo se calcula | Meta | Decisión que apoya |
|---|---|---|---|---|
| T-01 | Solicitudes por segundo, por operación | Conteo por segundo | Se mide en la prueba de carga | Planificación de capacidad |
| T-02 | Throughput máximo sostenido con p95 < 1 s | Prueba de carga, subir la carga hasta que L-01 se rompa | Valor documentado | Cuántos estudiantes soporta el sistema a la vez |
| T-03 | Usuarios concurrentes / conexiones abiertas | Conexiones activas en el servicio | — | Tamaño de los pools de hilos y de conexiones |
| T-04 | Reservas por minuto en el pico | Conteo de transacciones `RESERVA` | — | Escalar la capa de servicios (más instancias) |
| T-05 | **TPM de consultas** (requisito del curso) | Consultas procesadas con éxito por minuto, contadas en la capa de persistencia | Se compara base vs escalable | Capacidad de lectura de cada arquitectura |
| T-06 | **TPM de reservas** (requisito del curso) | Transacciones `RESERVA` + `MODIFICACION` `CONFIRMADA` por minuto, contadas en la BD | Se compara base vs escalable | Capacidad de escritura de cada arquitectura |

### 3. Cancelaciones asíncronas (solicitudes pendientes)

| ID | Indicador | Cómo se calcula | Meta | Decisión que apoya |
|---|---|---|---|---|
| Q-01 | Cancelaciones pendientes | Filas de `solicitud_cancelacion` en `PENDIENTE` o `EN_PROCESO` | Cerca de 0; alerta si > 50 | Agregar trabajadores de cancelaciones |
| Q-02 | Antigüedad de la cancelación pendiente más vieja | `ahora − aceptadaEn` de la `PENDIENTE` más vieja | < 10 s | Trabajador detenido o persistencia caída |
| Q-03 | Recuperaciones del barrido | Solicitudes aplicadas por el barrido en lugar de por la notificación PUB/SUB (notificaciones perdidas) | Bajo | Estabilidad del PUB/SUB y del proxy |
| Q-04 | Tomas duplicadas descartadas | Trabajadores que perdieron la toma, o `aplicarCancelacion` encontró un `idSolicitud` existente | Se registra | Confirma que la idempotencia funciona |
| Q-05 | Cancelaciones fallidas | Transacciones de cancelación `FALLIDA`, por motivo | Bajo | A la validación de la aceptación le falta un caso |

### 4. Errores y confiabilidad

| ID | Indicador | Cómo se calcula | Meta | Decisión que apoya |
|---|---|---|---|---|
| E-01 | Tasa de errores por operación | Errores / total | Los errores de negocio son normales; errores técnicos < 1 % | Dónde concentrar las correcciones |
| E-02 | Errores por código | Conteo por código de error | — | p. ej. muchos `CUPOS_INSUFICIENTES` → lugar más grande o más fechas |
| E-03 | Timeouts por salto | Solicitudes que llegaron al timeout | < 0,5 % | Ajustar timeouts, encontrar la capa lenta |
| E-04 | Reintentos con el mismo `idSolicitud` | `idSolicitud` repetidos recibidos | Se registra | Calidad de la red; confirma la idempotencia |
| E-05 | Disponibilidad por capa | Tiempo arriba / tiempo total (health check cada N s) | ≥ 99 % durante las pruebas | Fase de tolerancia a fallos (O9) |
| E-06 | Conteo de `PERSISTENCIA_NO_DISPONIBLE` y cambios de broker | Conteo | 0 en operación normal | Tolerancia a fallos |

### 5. Concurrencia

| ID | Indicador | Cómo se calcula | Meta | Decisión que apoya |
|---|---|---|---|---|
| C-01 | Tiempo de espera de bloqueos / reintentos de bloqueo optimista | Desde la persistencia | Bajo | Elección de la estrategia de concurrencia (O4) |
| C-02 | Interbloqueos (deadlocks) | Conteo | 0 | El orden de los bloqueos es correcto |
| C-03 | Conflictos en la misma fecha de evento | Escrituras concurrentes rechazadas o reintentadas por fecha | Se registra | Los eventos populares necesitan manejo especial |

### 6. Consistencia (siempre debe ser cero)

Se verifica con un job periódico y después de cada prueba de carga.

| ID | Indicador | Regla | Meta |
|---|---|---|---|
| K-01 | Fechas con sobrecupo | `FechaEvento.reservas > limite` | 0 |
| K-02 | Contador descuadrado | `FechaEvento.reservas ≠ suma(cupos de las reservas ACTIVA)` | 0 |
| K-03 | Saldos negativos | `Estudiante.saldo < 0` | 0 |
| K-04 | Efectos duplicados | Más de una transacción por `idSolicitud` | 0 |
| K-05 | Cancelaciones perdidas | Cancelaciones `ACEPTADA` sin fila en `solicitud_cancelacion`, o atascadas en `PENDIENTE`/`EN_PROCESO` > 60 s | 0 |

### 7. Recursos

| ID | Indicador | Meta | Decisión que apoya |
|---|---|---|---|
| R-01 | % de CPU por proceso | < 70 % sostenido | Agregar instancias / máquina más grande |
| R-02 | Memoria por proceso | Estable (sin crecer en el tiempo) | Fugas de memoria |
| R-03 | Conexiones de persistencia en uso | < tamaño del pool | Tamaño del pool |
| R-04 | Tamaño del almacenamiento y su crecimiento | Se registra | Archivar transacciones antiguas |

### 8. Uso (negocio)

No es sobre velocidad, pero hace falta para tomar decisiones sobre los eventos.

| ID | Indicador | Decisión que apoya |
|---|---|---|
| U-01 | Ocupación por fecha de evento (`reservas / limite`) | Agregar fechas o pasar a un lugar más grande |
| U-02 | Eventos y reservas por mes | Planear los picos de carga |
| U-03 | Tasa de cancelación (`cancelaciones / reservas`) | Políticas como un plazo para cancelar (O11) |
| U-04 | Tasa de modificación y cambio más común (cupos vs fecha) | Qué fechas se prefieren |
| U-05 | Ingresos y reembolsos de los eventos pagos | Precios |

## Evaluación cualitativa

Además de los números, cada ronda de pruebas registra una evaluación cualitativa corta:

| Aspecto | Pregunta a responder | Escala |
|---|---|---|
| Capacidad de respuesta | ¿El consumidor se siente instantáneo? ¿Hay esperas visibles? | Buena / Aceptable / Mala |
| Estabilidad | ¿Algún proceso se cayó o necesitó reinicio? | Buena / Aceptable / Mala |
| Comportamiento bajo carga | ¿Los tiempos empeoraron de forma gradual o de golpe? | Buena / Aceptable / Mala |
| Recuperación | Después de detener una capa y reiniciarla, ¿todo se recuperó sin datos perdidos ni duplicados? | Buena / Aceptable / Mala |
| Claridad de los errores | ¿Los mensajes de error fueron claros para el estudiante? | Buena / Aceptable / Mala |

## Cómo se muestra el desempeño

**(propuesta)** El consumidor administrador tiene una **vista de desempeño** que
muestra, para una ventana de tiempo elegida:

- Tiempo p50 / p95 / p99 / máx por operación y % por debajo de 1 s, con la línea
  de 1 s marcada.
- Solicitudes por segundo y tasa de errores.
- Cancelaciones pendientes y antigüedad de la más vieja.
- Verificaciones de consistencia (K-01 … K-05) en verde/rojo.
- Ocupación de las próximas fechas de eventos.

Los datos vienen de la operación `obtenerMetricas(desde, hasta)` de la capa de
servicios, que agrega por `idSolicitud` los registros de tiempo de las tres capas.
Las opciones de herramientas están en [decisiones.md](decisiones.md) (O16) y en
[recoleccion-metricas.md](recoleccion-metricas.md).

## Escenarios de prueba del curso

Los escenarios obligatorios (3, 5 y 8 clientes simultáneos, en la arquitectura
base y en la escalable) con L-08, L-09, T-05 y T-06 están definidos en
[plan-pruebas.md](plan-pruebas.md#10-pruebas-de-desempeño-del-curso-arquitectura-base-vs-escalable),
junto con la tabla de resultados y los gráficos que van en el informe.

## Escenarios de prueba de carga adicionales

Para evaluar los indicadores, correr estos escenarios y registrar los resultados:

| # | Escenario | Indicadores principales |
|---|---|---|
| S1 | Carga normal: N estudiantes consultando y reservando en muchos eventos | L-01, L-02, T-01 |
| S2 | Avalancha: muchos estudiantes reservando a la vez los últimos cupos de la misma fecha | L-01, C-01–C-03, K-01, K-02 |
| S3 | Muchas modificaciones moviendo reservas entre dos fechas en ambos sentidos | C-02, K-02, L-01 |
| S4 | Ráfaga de cancelaciones | Q-01, Q-02, L-06, L-07 |
| S5 | Detener la persistencia 30 s durante la carga y luego reiniciarla | E-03, E-06, Q-03, K-04, K-05, recuperación |
| S6 | Detener el proceso de servicios justo después de aceptar cancelaciones | K-05, Q-04 |
| S7 | Subir la carga hasta que p95 > 1 s | T-02 |

## Plantilla de resultados

Para los escenarios del curso se usa la tabla de
[plan-pruebas.md](plan-pruebas.md#104-tabla-de-resultados-va-en-el-informe).
Para las pruebas adicionales:

| Fecha | Escenario | Carga | p50 | p95 | p99 | máx | % < 1 s | Errores % | Máx. cancel. pendientes | Consistencia | Notas |
|---|---|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | | | |
