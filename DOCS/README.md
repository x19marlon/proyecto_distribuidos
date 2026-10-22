# Project Documentation

Distributed transaction system for university events.

Students consult the events of a month, reserve spots on an event date, and can
modify (number of spots and/or date) or cancel their reservations. Each
reservation, modification or cancellation is recorded as a **transaction**.
All operations are synchronous except cancellation, which is accepted
immediately and applied asynchronously.

The system is split into three layers that run as independent processes and
talk to each other over the network:

```
Consumers  ──►  Services  ──►  Data Persistence
```

## Index

| Document | Contents |
|---|---|
| [requirements.md](requirements.md) | Functional and non-functional requirements |
| [architecture.md](architecture.md) | The three layers, how they communicate, deployment view |
| [system-models.md](system-models.md) | Architectural, interaction, failure and security models applied to the project |
| [domain-model.md](domain-model.md) | `Student`, `Event`, `EventDate`, `Place`, `Reservation` and `Transaction` entities |
| [layers.md](layers.md) | Responsibilities, operations and tables of each layer |
| [transaction-flow.md](transaction-flow.md) | Step-by-step flows: consult by month, reserve, modify, cancel, event CRUD |
| [distributed-concerns.md](distributed-concerns.md) | Consistency, concurrency, failures, idempotency |
| [fault-tolerance-scalability.md](fault-tolerance-scalability.md) | Components and phases to make the system fault tolerant and scalable |
| [performance-indicators.md](performance-indicators.md) | Performance indicators, the < 1 s target, load-test scenarios |
| [metrics-collection.md](metrics-collection.md) | Tools and strategies to obtain the performance indicators |
| [test-plan.md](test-plan.md) | How we test functionality, concurrency, failures, security and performance |
| [decisions.md](decisions.md) | Decisions taken and decisions still open |
| [diagrams/](diagrams/README.md) | UML diagrams (PlantUML): deployment, component, class and sequence diagrams |

## Conventions

- Diagrams are written in [Mermaid](https://mermaid.js.org/) so they render on GitHub.
- Anything marked **(proposal)** is a suggestion that the team has not yet confirmed.
  Record the final choice in [decisions.md](decisions.md).
