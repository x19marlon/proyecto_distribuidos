# Architecture

## Overview

The system follows a strict three-layer architecture. Each layer only talks to
the layer directly below it:

```mermaid
flowchart LR
    subgraph L1[1. Consumers]
        C1[Student client]
        C2[Admin / event organizer client]
    end

    subgraph L2[2. Services]
        S1[Student service]
        S3[Event service]
        S2[Transaction service]
        Q[[Cancellation queue]]
        W[Cancellation worker]
    end

    subgraph L3[3. Data Persistence]
        P1[(Students)]
        P3[(Events, dates and places)]
        P2[(Reservations and transactions)]
    end

    C1 --> S1
    C1 --> S3
    C1 --> S2
    C2 --> S1
    C2 --> S3
    C2 --> S2
    S1 --> P1
    S3 --> P3
    S2 --> P2
    S2 -. validates student .-> S1
    S2 -. checks event and limit .-> S3
    S2 -- enqueue cancellation --> Q
    Q -- dequeue --> W
    W --> P2
```

| Layer | Role | Knows about |
|---|---|---|
| **Consumers** | Entry point for users. Builds requests, shows results. No business rules. | Services layer API only |
| **Services** | Business logic: student and event/place CRUD, monthly event listing, reserving / modifying / cancelling spots, availability and balance checks, transactions. | Persistence layer API only |
| **Data Persistence** | Stores and retrieves `Student`, `Place`, `Event`, `EventDate`, `Reservation` and `Transaction` records durably. No business rules. | Its own storage |

Rules:

1. A consumer **never** talks directly to persistence.
2. Persistence **never** calls up into services or consumers.
3. Services may call other services (e.g. the transaction service asks the
   student service whether a student exists).

## Communication

Each layer runs as its own process (possibly on different machines), so every
arrow above is a network call.

### Synchronous operations (everything except cancellation)

The caller blocks until the layer below answers. A chain looks like this:

```mermaid
sequenceDiagram
    participant C as Consumer
    participant S as Service
    participant P as Persistence
    C->>S: request (blocks)
    S->>P: request (blocks)
    P-->>S: committed / error
    S-->>C: final result / error
```

When the response arrives, the change is already committed (or definitely not
applied).

### Asynchronous operation: cancellation

Cancelling a reservation does **not** wait for the data layer. The transaction
service validates the request, stores it in a durable **cancellation queue** and
answers immediately with `ACCEPTED` ("your cancellation is being processed").
A **cancellation worker**, running in the services layer, later takes the
request from the queue and applies it to the data layer.

```mermaid
sequenceDiagram
    participant C as Consumer
    participant S as Transaction service
    participant Q as Cancellation queue
    participant W as Cancellation worker
    participant P as Persistence
    C->>S: cancelReservation (blocks briefly)
    S->>Q: enqueue (durable)
    S-->>C: ACCEPTED: "cancellation in progress"
    Note over C: consumer is free
    W->>Q: take next request
    W->>P: apply cancellation (atomic)
    P-->>W: committed / error
    W->>Q: acknowledge (remove)
    C->>S: getRequestStatus (later)
    S-->>C: PENDING / COMMITTED / FAILED
```

Both the queue and the worker belong to the services layer; the consumer still
only talks to services, and only services talk to persistence.

**(proposal)** Use TCP sockets, RPC (gRPC / Java RMI / Python `xmlrpc`) or
HTTP+JSON. Whatever is chosen, every message should carry:

| Field | Purpose |
|---|---|
| `requestId` | Unique ID used for idempotency and log correlation |
| `operation` | Name of the action, e.g. `CREATE_TRANSACTION` |
| `payload` | Operation-specific data |
| `timestamp` | When the request was produced |

Responses carry `requestId`, a `status` (`OK` / `ERROR`, or `ACCEPTED` for an
asynchronous cancellation), and either a `result`
or an `error` with a code and message.

## Deployment view

```mermaid
flowchart TB
    subgraph M1[Machine A]
        Consumer1[Consumer process]
    end
    subgraph M2[Machine B]
        Service1[Services process]
    end
    subgraph M3[Machine C]
        Persistence1[Persistence process]
        DB[(Storage)]
    end
    Consumer1 -- network --> Service1
    Service1 -- network --> Persistence1
    Persistence1 --- DB
```

For development, all three processes can run on one machine on different ports.
Addresses and ports should come from configuration, never be hard-coded.
