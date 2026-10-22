# Requirements

## Functional requirements

### Events

| ID | Requirement |
|---|---|
| RF-01 | The system must allow creating, reading, updating and deleting (CRUD) university events. |
| RF-02 | Each event has a **name**, a **spot** (where it happens), a **price** and **one or more dates**. |
| RF-03 | An event can be **free** or **paid**. Paid events have a price per spot that is charged to the student. |
| RF-04 | The spot of an event is either a **university place** (building or theater) or an **external place**. |
| RF-05 | If the spot is a university place, the place must be registered in the database, and the event's reservation limit is the place's **capacity**. |
| RF-06 | If the spot is an external place, it is stored as a free-text address on the event, and the reservation limit is set manually on the event. |
| RF-07 | Each date of an event keeps its own **reservations** counter (spots taken), which can never exceed the event's limit. |

### Places

| ID | Requirement |
|---|---|
| RF-08 | The system must allow CRUD of university places (buildings and theaters), each with a name, type and capacity. |
| RF-09 | A place that is used by an event cannot be deleted. |

### Students

| ID | Requirement |
|---|---|
| RF-10 | When a student makes a reservation, their information must be registered in the system (if not already registered). |
| RF-11 | Student information: name, student ID, government ID type, government ID, career, department, semester, institutional email, phone. |

### Consulting and reserving

| ID | Requirement |
|---|---|
| RF-12 | A student can consult the events of a given **month** (every event date that falls in that month). |
| RF-13 | For each event date, a student can consult the **reservations of the spots**: spots taken, limit and spots available. |
| RF-14 | A student can reserve **one or more spots** for one date of an event. |
| RF-15 | A student can have reservations in **any number** of events (N events), but at most one active reservation per event. |
| RF-16 | A student can consult all their reservations (event, date, spot, number of spots). |
| RF-17 | A student can **modify a reservation**: change the **number of spots** and/or move it to **another date** of the same event. |
| RF-18 | A modification is accepted only if the target date has enough spots available. For paid events, the price difference is charged or refunded. |
| RF-19 | A student can cancel a reservation; the spots are released and, for paid events, the amount paid is refunded. |
| RF-19a | Cancellation is **asynchronous**: the system first accepts the request and immediately answers with a message saying the cancellation is in progress; afterwards it performs the cancellation and updates the data layer. |
| RF-19b | The student can consult the status of a cancellation request (`PENDING`, `COMMITTED`, `FAILED`). |

### Transactions

| ID | Requirement |
|---|---|
| RF-20 | Every reservation, modification and cancellation is recorded as a transaction. |
| RF-21 | A student can consult their transaction history. |

## Non-functional requirements

| ID | Requirement |
|---|---|
| RNF-01 | The system is split into three layers, in order: Consumers → Services → Data Persistence. Each runs as a separate process. |
| RNF-02 | **Synchronous operation:** every request **except cancellation** (consult, reserve, modify, CRUD) is a blocking request/response. The consumer waits until the change is committed in persistence and gets the final result (success or error) in the same call. There is no "accepted, processing later" state. |
| RNF-03 | Concurrent reservations or modifications must never exceed the limit of an event date. |
| RNF-04 | Retrying a request (e.g. after a timeout) must never reserve, modify or charge twice. |
| RNF-05 | Changes that touch several records (reservation, counters of one or two dates, balance, transaction) are atomic: all are applied or none. |
| RNF-06 | **Asynchronous cancellation:** an accepted cancellation must not be lost, even if the services process crashes before applying it, and must be applied exactly once. |
| RNF-07 | **Transaction time < 1 s:** every operation (and the acceptance of a cancellation) completes end-to-end in under one second for at least 95 % of requests (p95). See [performance-indicators.md](performance-indicators.md). |
| RNF-08 | **Performance visibility:** the system records the time of every request in every layer, and the admin can see the performance indicators (response times, throughput, errors, queue state, consistency checks). |
| RNF-09 | **Performance evaluation:** performance is evaluated with defined load-test scenarios, quantitatively (indicators) and qualitatively (responsiveness, stability, recovery). |
