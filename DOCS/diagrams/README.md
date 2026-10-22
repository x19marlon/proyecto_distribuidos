# UML Diagrams

UML diagrams of the system, written in [PlantUML](https://plantuml.com/).
The `.puml` files are the source of truth; regenerate images from them after
any change.

| File | UML diagram | Shows |
|---|---|---|
| [deployment.puml](deployment.puml) | Deployment | Devices, execution environments, artifacts and communication paths (TLS, RPC, SQL) across the 3 machines |
| [components.puml](components.puml) | Component | Components of each layer, their provided/required interfaces and dependencies |
| [class.puml](class.puml) | Class | Domain model: `Student`, `Place`, `Event`, `EventDate`, `Reservation`, `Transaction`, `CancellationRequest`, `TimingRecord`, enumerations, multiplicities and constraints |
| [seq-01-consult-month.puml](seq-01-consult-month.puml) | Sequence | Consult events of a month and the spots of an event (sync) |
| [seq-02-reserve.puml](seq-02-reserve.puml) | Sequence | Reserve N spots on a date (sync, atomic conditional update) |
| [seq-03-modify.puml](seq-03-modify.puml) | Sequence | Modify a reservation's spots and/or date (sync, two dates in one atomic unit) |
| [seq-04-cancel-async.puml](seq-04-cancel-async.puml) | Sequence | Cancel a reservation: accept, worker applies it, student checks status (async) |
| [seq-05-retry-lost-reply.puml](seq-05-retry-lost-reply.puml) | Sequence | Failure: reply lost, retry with same `requestId` (idempotency) |
| [seq-06-create-event.puml](seq-06-create-event.puml) | Sequence | Admin creates an event with its dates (authorization + validation) |

## How to view / render

Any of these work:

- **VS Code:** install the "PlantUML" extension (jebbs.plantuml), open a `.puml`
  file and press `Alt+D` for a live preview. Needs Java + Graphviz, or set the
  extension to use a PlantUML server.
- **IntelliJ / PyCharm:** "PlantUML Integration" plugin.
- **Command line** (needs Java):

  ```bash
  java -jar plantuml.jar -tsvg DOCS/diagrams/*.puml   # SVG next to each file
  java -jar plantuml.jar -tpng DOCS/diagrams/*.puml   # PNG
  ```

- **Online:** paste the file content at <https://www.plantuml.com/plantuml>.

Related text documentation: [architecture](../architecture.md),
[domain model](../domain-model.md), [flows](../transaction-flow.md),
[system models](../system-models.md).
