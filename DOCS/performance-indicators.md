# Performance Indicators

The system must let the team **see how it performs** and **decide** what to
change. This document lists the indicators (KPIs) to measure, how to measure
them, the target for each one, and which decision each indicator supports.

## Main target: transaction time < 1 second

| Operation type | What "transaction time" means | Target |
|---|---|---|
| Synchronous writes: `reserve`, `modifyReservation`, `topUp`, CRUD | From the moment the consumer sends the request until it receives the final result (committed or error) | **< 1 s** |
| Synchronous reads: `listEventsByMonth`, `getEvent`, `listStudentReservations`, history | Same as above | **< 1 s** |
| Asynchronous `cancelReservation` – acceptance | Until the consumer receives `ACCEPTED` | **< 1 s** |
| Asynchronous `cancelReservation` – completion | From `ACCEPTED` until the worker commits it in the data layer | **< 1 s** under normal load (proposal); never more than 10 s |

"< 1 s" is measured as the **95th percentile (p95)**: at least 95 % of
transactions finish in under one second. The maximum is also tracked so that
outliers are visible. **(proposal)** Stricter goal: p99 < 1 s.

## How times are measured

Every layer records, for each request, its `requestId`, operation, start time,
end time, result (`OK` / error code / `ACCEPTED`) and the layer name. Because
the same `requestId` travels through all layers, one transaction's time can be
split by layer:

Example of how a `reserve()` that takes 420 ms splits up:

| Segment | Time |
|---|---|
| Network consumer → service | 30 ms |
| Service validation and calls to other services | 150 ms |
| Persistence atomic commit | 180 ms |
| Service builds response | 30 ms |
| Network service → consumer | 30 ms |
| **End-to-end** | **420 ms** |

- **End-to-end time** = consumer receives response − consumer sends request.
- **Service time** = service sends response − service receives request.
- **Persistence time** = same inside persistence.
- **Network time** ≈ end-to-end − service time (and service time − persistence time).

All clocks report in milliseconds. Comparisons across machines use durations
measured on the same machine, never timestamps from two different clocks.

## Indicator list

### 1. Response time (latency)

| ID | Indicator | How it is calculated | Target | Decision it supports |
|---|---|---|---|---|
| L-01 | End-to-end time per operation (p50, p95, p99, max) | Consumer-side durations grouped by operation | p95 < 1 s | Whether the system meets the main requirement |
| L-02 | % of transactions under 1 s | `count(time < 1000 ms) / count(all)` | ≥ 95 % | Same; easy number to report |
| L-03 | Service-layer time per operation | Service-side durations | p95 < 600 ms | Optimize business logic or service-to-service calls |
| L-04 | Persistence time per operation | Persistence-side durations | p95 < 300 ms | Add indexes, change storage (O3), change locking strategy (O4) |
| L-05 | Network time per hop | End-to-end − inner layer time | p95 < 100 ms | Change protocol (O2), move processes closer |
| L-06 | Cancellation acceptance time | Time to `ACCEPTED` | p95 < 1 s | Queue choice (O13) |
| L-07 | Cancellation completion time | `committedAt − acceptedAt` | p95 < 1 s, max < 10 s | Number of workers |

### 2. Throughput and load

| ID | Indicator | How it is calculated | Target | Decision it supports |
|---|---|---|---|---|
| T-01 | Requests per second, per operation | Count per second | Measured under load test | Capacity planning |
| T-02 | Max sustained throughput with p95 < 1 s | Load test, increase load until L-01 breaks | Documented value | How many students the system supports at once |
| T-03 | Concurrent users / open connections | Active connections at the service | — | Thread pool / connection pool size |
| T-04 | Reservations per minute at peak | Count of `RESERVATION` transactions | — | Scaling the services layer (more instances) |

### 3. Asynchronous cancellation queue

| ID | Indicator | How it is calculated | Target | Decision it supports |
|---|---|---|---|---|
| Q-01 | Queue depth | Messages waiting | Near 0; alert if > 50 | Add workers |
| Q-02 | Age of the oldest pending message | `now − acceptedAt` of oldest | < 10 s | Worker stuck or persistence down |
| Q-03 | Worker retries | Count of re-attempts | Low | Persistence stability |
| Q-04 | Duplicate deliveries skipped | `applyCancellation` found existing `requestId` | Tracked | Confirms idempotency works |
| Q-05 | Failed cancellations | `FAILED` cancellation transactions, by reason | Low | Validation at acceptance is missing a case |

### 4. Errors and reliability

| ID | Indicator | How it is calculated | Target | Decision it supports |
|---|---|---|---|---|
| E-01 | Error rate per operation | Errors / total | Business errors are normal; technical errors < 1 % | Where to focus fixes |
| E-02 | Errors by code | Count per error code | — | e.g. many `NOT_ENOUGH_SPOTS` → bigger place or more dates |
| E-03 | Timeouts per hop | Requests that hit the timeout | < 0.5 % | Tune timeouts, find slow layer |
| E-04 | Retries with the same `requestId` | Repeated `requestId`s received | Tracked | Network quality; confirms idempotency |
| E-05 | Availability per layer | Uptime / total time (health check every N s) | ≥ 99 % during tests | Replication (O9) |
| E-06 | `PERSISTENCE_UNAVAILABLE` / `QUEUE_UNAVAILABLE` count | Count | 0 in normal operation | Fault tolerance |

### 5. Concurrency

| ID | Indicator | How it is calculated | Target | Decision it supports |
|---|---|---|---|---|
| C-01 | Lock wait time / optimistic-lock retries | From persistence | Low | Choice of concurrency strategy (O4) |
| C-02 | Deadlocks | Count | 0 | Lock ordering is correct |
| C-03 | Conflicts on the same event date | Concurrent writes rejected or retried per date | Tracked | Popular events need special handling |

### 6. Consistency (must always be zero)

Checked by a periodic job and after every load test.

| ID | Indicator | Rule | Target |
|---|---|---|---|
| K-01 | Overbooked dates | `EventDate.reservations > limit` | 0 |
| K-02 | Counter mismatch | `EventDate.reservations ≠ sum(spots of ACTIVE reservations)` | 0 |
| K-03 | Negative balances | `Student.balance < 0` | 0 |
| K-04 | Duplicated effects | More than one transaction per `requestId` | 0 |
| K-05 | Lost cancellations | `ACCEPTED` cancellations with no transaction and not in the queue | 0 |

### 7. Resources

| ID | Indicator | Target | Decision it supports |
|---|---|---|---|
| R-01 | CPU % per process | < 70 % sustained | Add instances / bigger machine |
| R-02 | Memory per process | Stable (no growth over time) | Memory leaks |
| R-03 | Persistence connections in use | < pool size | Pool size |
| R-04 | Storage size and growth | Tracked | Archiving old transactions |

### 8. Usage (business)

Not about speed, but needed to make decisions about events.

| ID | Indicator | Decision it supports |
|---|---|---|
| U-01 | Occupancy per event date (`reservations / limit`) | Add dates or move to a bigger place |
| U-02 | Events and reservations per month | Plan load peaks |
| U-03 | Cancellation rate (`cancellations / reservations`) | Policies such as a deadline to cancel (O11) |
| U-04 | Modification rate and most common change (spots vs date) | Which dates are preferred |
| U-05 | Revenue and refunds for paid events | Pricing |

## Qualitative evaluation

Besides numbers, each test round records a short qualitative assessment:

| Aspect | Question to answer | Scale |
|---|---|---|
| Responsiveness | Does the consumer feel instant? Any visible waits? | Good / Acceptable / Poor |
| Stability | Did any process crash or need a restart? | Good / Acceptable / Poor |
| Behavior under load | Did times degrade gradually or suddenly? | Good / Acceptable / Poor |
| Recovery | After stopping a layer and restarting it, did everything recover with no lost or duplicated data? | Good / Acceptable / Poor |
| Clarity of errors | Were error messages clear to the student? | Good / Acceptable / Poor |

## How the performance is shown

**(proposal)** The admin consumer has a **performance view** that shows, for a
chosen time window:

- p50 / p95 / p99 / max time per operation, and % under 1 s, with the 1 s line
  marked.
- Requests per second and error rate.
- Cancellation queue depth and oldest message age.
- Consistency checks (K-01 … K-05) as green/red.
- Occupancy of upcoming event dates.

The data comes from the services layer operation `getMetrics(from, to)`, which
aggregates the timing records of all three layers by `requestId`. Tool options
are listed in [decisions.md](decisions.md) (O16).

## Load test scenarios

To evaluate the indicators, run these scenarios and record the results:

| # | Scenario | Main indicators |
|---|---|---|
| S1 | Normal load: N students consulting and reserving across many events | L-01, L-02, T-01 |
| S2 | Rush: many students reserving the last spots of the same date at once | L-01, C-01–C-03, K-01, K-02 |
| S3 | Many modifications moving reservations between two dates in both directions | C-02, K-02, L-01 |
| S4 | Burst of cancellations | Q-01, Q-02, L-06, L-07 |
| S5 | Stop persistence for 30 s during load, then restart | E-03, E-06, Q-03, K-04, K-05, recovery |
| S6 | Stop the services process right after cancellations are accepted | K-05, Q-04 |
| S7 | Increase load until p95 > 1 s | T-02 |

## Results template

| Date | Scenario | Load | p50 | p95 | p99 | max | % < 1 s | Errors % | Queue max | Consistency | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| | | | | | | | | | | | |
