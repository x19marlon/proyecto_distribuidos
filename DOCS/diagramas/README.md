# Diagramas UML

Diagramas UML del sistema, escritos en [PlantUML](https://plantuml.com/).
Los archivos `.puml` son la fuente de verdad; las imágenes se regeneran a partir
de ellos después de cualquier cambio.

| Archivo | Diagrama UML | Muestra |
|---|---|---|
| [despliegue.puml](despliegue.puml) | Despliegue | Clúster Kubernetes (cp-1 + worker-1..3): pods, enlaces ZeroMQ (CurveZMQ), NodePorts |
| [componentes.puml](componentes.puml) | Componentes | Componentes de cada capa con sus sockets ZeroMQ: brokers Majordomo (ROUTER), workers DEALER, proxy PUB/SUB |
| [clases.puml](clases.puml) | Clases | Modelo de dominio: `Estudiante`, `Lugar`, `Evento`, `FechaEvento`, `Reserva`, `Transaccion`, `SolicitudCancelacion`, `RegistroTiempo`, la clase de servicio `GestorTransacciones`, enumeraciones, multiplicidades y restricciones |
| [sec-01-consultar-mes.puml](sec-01-consultar-mes.puml) | Secuencia | Consultar los eventos de un mes y los cupos de un evento (síncrono) |
| [sec-02-reservar.puml](sec-02-reservar.puml) | Secuencia | Reservar N cupos en una fecha (síncrono, actualización condicional atómica) |
| [sec-03-modificar.puml](sec-03-modificar.puml) | Secuencia | Modificar los cupos y/o la fecha de una reserva (síncrono, dos fechas en una sola unidad atómica) |
| [sec-04-cancelar-asincrono.puml](sec-04-cancelar-asincrono.puml) | Secuencia | Cancelar una reserva: guardar, ACEPTADA, notificación PUB/SUB, toma, barrido, estado (asíncrono). Muestra los brokers explícitamente |
| [sec-05-reintento-respuesta-perdida.puml](sec-05-reintento-respuesta-perdida.puml) | Secuencia | Fallo: se pierde la respuesta, se reintenta con el mismo `idSolicitud` (idempotencia) |
| [sec-06-crear-evento.puml](sec-06-crear-evento.puml) | Secuencia | El administrador crea un evento con sus fechas (autorización + validación) |

En las secuencias 1, 2, 3, 5 y 6, cada flecha entre consumidor, servicios y
persistencia es una llamada lógica; físicamente pasa por el broker Majordomo de
esa capa, como se dibuja en la secuencia 4
([comunicacion-zeromq.md](../comunicacion-zeromq.md)).

## Cómo verlos / generarlos

Cualquiera de estas opciones sirve:

- **VS Code:** instalar la extensión "PlantUML" (jebbs.plantuml), abrir un
  `.puml` y presionar `Alt+D` para la vista previa. Necesita Java + Graphviz, o
  configurar la extensión para usar un servidor PlantUML.
- **IntelliJ / PyCharm:** plugin "PlantUML Integration".
- **Línea de comandos** (necesita Java):

  ```bash
  java -jar plantuml.jar -tsvg DOCS/diagramas/*.puml   # SVG junto a cada archivo
  java -jar plantuml.jar -tpng DOCS/diagramas/*.puml   # PNG
  ```

- **En línea:** pegar el contenido del archivo en <https://www.plantuml.com/plantuml>.

Documentación relacionada: [arquitectura](../arquitectura.md),
[modelo de dominio](../modelo-dominio.md), [flujos](../flujos.md),
[modelos del sistema](../modelos-sistema.md).

## Figura 1 del enunciado

[diagrama_proyecto.png](diagrama_proyecto.png): Arquitectura Básica del Sistema, su parte síncrona (consulta y reserva) se implementa en la entrega 1 (ver [implementacion.md](../implementacion.md)).
