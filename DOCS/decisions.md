# Decisions

## Decided

| # | Decision | Reason |
|---|---|---|
| D1 | Three layers in order: Consumers → Services → Data Persistence | Project requirement |
| D2 | Core classes are `Student` and `Transaction`, plus `Event`, `EventDate`, `Place` and `Reservation` | Project requirement |
| D3 | Each layer only calls the layer directly below it | Separation of concerns; layers can be deployed independently |
| D4 | Events support full CRUD | Project requirement |
| D5 | Each event date stores its current `reservations` and shares the event's limit | Project requirement |
| D6 | University buildings/theaters are `Place` records; the event limit is the place's `capacity` | Project requirement |
| D7 | External spots are free text on the event, with a manual `externalCapacity` | Team decision |
| D8 | Some events are paid and some free (`price = 0`) | Project requirement |
| D9 | Student fields: name, student ID, government ID type + number, career, department, semester, email, phone | Project requirement |
| D10 | A student can have reservations in N events, at most one active reservation per event | Project requirement |
| D11 | An event can have one or more dates | Project requirement |
| D12 | A reservation holds N spots; students consult events by month and can modify a reservation's spots and/or date | Project requirement |
| D13 | All operations are synchronous request/response, except cancellation | Project requirement |
| D15 | Cancellation is asynchronous: accept → reply "in progress" → worker applies it to the data layer | Project requirement |
| D16 | Accepted cancellations are stored in a durable queue before replying | An accepted cancellation must survive a crash (RNF-06) |
| D14 | `Reservation` holds the current state; `Transaction` is the immutable log of each change | Keeps the current booking easy to query while keeping full history |
| D17 | Transaction time target: p95 < 1 s end-to-end (cancellation: acceptance) | Project requirement |
| D18 | Every layer records request timings by `requestId`; admin can view performance indicators | Project requirement |
| D19 | Failure model: processes crash (no Byzantine behavior), channels may lose/delay/duplicate messages; timeouts mean "unknown" | Basis for retries + idempotency design |
| D20 | `studentId` for an operation always comes from the authenticated token, never from the request body | Prevents acting on other students' data |

## Open

Fill in the "Decision" column once the team agrees.

| # | Question | Options | Decision |
|---|---|---|---|
| O1 | Programming language | Java, Python, Go, Node.js | |
| O2 | Synchronous communication between layers | Sockets, RPC (gRPC / RMI / xmlrpc), HTTP+JSON | |
| O3 | Storage | Files, SQLite, PostgreSQL/MySQL | |
| O4 | Concurrency control | Row locks, optimistic locking, conditional update, single writer (see [distributed-concerns.md](distributed-concerns.md)) | |
| O5 | Consumer type | CLI, desktop GUI, web | |
| O6 | How are paid events paid? | Prepaid student balance with top-ups (current design), payment recorded without balance | |
| O7 | Can two events use the same place at the same date/time? | Allowed, blocked | |
| O8 | Is `department` free text or a fixed list (and is `career` linked to a department)? | Free text, catalog table | |
| O9 | Which fault-tolerance phase is in scope? (see [fault-tolerance-scalability.md](fault-tolerance-scalability.md#6-implementation-phases)) | Phase 1 (proposal), Phase 2, Phase 3 | |
| O10 | Max spots per reservation? | No limit (only availability), fixed max (e.g. 5) | |
| O11 | Deadline to modify/cancel? | Until the event date, N hours before | |
| O12 | Can an admin move a date that already has reservations? | Blocked, allowed (reservations follow the date) | |
| O13 | Durable queue for cancellations | RabbitMQ, ZeroMQ + log file, Redis list, `pending_cancellation` table in a separate store | |
| O14 | While a cancellation is pending, can the reservation be modified? | Blocked with `CANCELLATION_PENDING` (current design), allowed (cancellation applies to the latest state) | |
| O15 | How does the student learn the final result? | Polls `getRequestStatus` (current design), push notification / email | |
| O16 | Metrics tool (see [metrics-collection.md](metrics-collection.md)) | Track A: JSON timing records + analysis script; Track B: A + Prometheus, Grafana, exporters, OpenTelemetry/Jaeger (recommended: A from the start, B before failure tests) | |
| O17 | Load-testing tool | JMeter, Locust, k6, custom script | |
| O18 | Is p95 < 1 s enough, or must it be p99 / every transaction? | p95, p99, max | |
| O19 | How do users authenticate? | University SSO/LDAP, own user/password table, no auth (demo only) | |
| O20 | How do internal processes authenticate? | Mutual TLS, shared secret, network isolation only | |
| O21 | Encrypt personal data at rest? | Government ID column only, whole disk, no | |
| O22 | Load balancer | HAProxy, Nginx, client-side list of instances | |
| O23 | Database failover | Manual (repmgr), automatic (Patroni + etcd) | |
| O24 | Use a cache for listings? | Redis with short TTL, no cache | |
