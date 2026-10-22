# Distributed Systems Concerns

Because the layers run as separate processes on a network, the following
problems must be handled explicitly.

## Concurrency

**Problem:** two students try to take the last spots of an event date at the
same time (by reserving or by modifying a reservation to more spots / another
date). Both read the same `reservations` value, both see enough room, and both
succeed → overbooking.

**Options (pick one, record in [decisions.md](decisions.md)):**

| Approach | How |
|---|---|
| Database transaction + row lock | `SELECT ... FOR UPDATE` on student and event rows inside `commitTransaction` |
| Optimistic locking | Each record has a `version`; update fails if the version changed, service retries |
| Conditional update | `UPDATE event_date SET reservations = reservations + ? WHERE id = ? AND reservations + ? <= <limit>` and check affected rows |
| Single-writer | Serialize all writes through one thread/queue in the persistence process |

### Modifications that touch two dates

Moving a reservation from date D1 to D2 changes two `EVENT_DATE` rows, the
reservation, the balance and inserts a transaction. All of it must be one
atomic unit: if D2 has no room, D1 keeps its spots and the reservation is left
unchanged.

If row locks are used, always lock rows in a fixed order (e.g. by `id`) so that
one student moving D1 → D2 while another moves D2 → D1 cannot deadlock.

## Synchronous operation

Every operation except cancellation is synchronous (see
[architecture.md](architecture.md#communication)):

- A service replies only after persistence has committed or rejected the change.
- The consumer never sees an intermediate state; a reservation is either fully
  modified or not modified at all.
- Because callers block, every call needs a timeout, and a timeout means the
  outcome is unknown → retry with the same `requestId`.

## Asynchronous cancellation

Cancellation is accepted first and applied later by a worker, which brings its
own problems:

| Problem | Handling |
|---|---|
| Services crash after replying `ACCEPTED` but before applying | The queue is **durable**: the request is stored before the reply is sent, so it is still there after a restart. |
| Queue unavailable when the student cancels | Reply `QUEUE_UNAVAILABLE`; nothing is accepted, the student can retry. Never reply `ACCEPTED` without storing first. |
| Worker crashes after the data layer commit but before acknowledging | The message is delivered again (**at-least-once**). `applyCancellation` checks whether a transaction with that `requestId` already exists and does nothing if so, so the effect is **exactly once**. |
| Persistence down when the worker runs | Leave the message in the queue and retry with backoff. |
| Reservation no longer cancellable when the worker runs (date passed, already cancelled) | Record a `FAILED` transaction with the reason and acknowledge. |
| Student retries the cancel request | Same `requestId` → same `ACCEPTED` answer, no second message. |
| Student modifies the reservation while the cancellation is pending | Rejected with `CANCELLATION_PENDING` (see decision O14). |
| Student reads data while a cancellation is pending | They see the reservation still active, marked "cancellation pending". The spots become available to others only after the worker commits (**eventual consistency** for this operation only). |
| Order of requests | One worker, or several workers partitioned by `reservationId`, so two requests for the same reservation are never applied in parallel. |

## Idempotency and retries

**Problem:** the consumer sends a request, the service processes it, but the
response is lost. The consumer retries → the student is charged twice.

**Solution:** every write carries a client-generated `requestId`. Persistence
stores it with a unique constraint. If the same `requestId` arrives again, the
service returns the stored result instead of executing again.

## Partial failures

| Failure | Expected behavior |
|---|---|
| Persistence down | Service returns `PERSISTENCE_UNAVAILABLE`; no partial data written |
| Service down | Consumer times out and retries with the same `requestId` (with backoff) |
| Crash in the middle of a commit | Atomic commit guarantees all-or-nothing; a `PENDING` transaction left behind is resolved on restart (marked `FAILED` or completed) |
| Network timeout | Treat outcome as unknown → retry with same `requestId` |

## Consistency

- Writes that change more than one record (reservation, date counters, balance,
  transaction) must be atomic in the persistence layer.
- `EventDate.reservations` must always equal the sum of `spots` of active
  reservations on that date; both are updated in the same atomic unit.
- Reads may be slightly stale if replicas/caches are introduced later; balance
  and reservation-limit checks must always use the authoritative store.

## Timeouts

Every network call has a timeout. Since transactions must take less than 1 s
(see [performance-indicators.md](performance-indicators.md)), **(proposal)**:
1.5 s consumer→service, 700 ms service→persistence. Never wait forever.

## Logging and traceability

Log every request at every layer with its `requestId`, operation, result, start
and end time, and duration. This makes it possible to follow one operation
across processes and is the source of the performance indicators.

## Possible extensions

- Replicate the persistence layer (primary/backup) for fault tolerance.
- Run multiple service instances behind a load balancer (requires that services
  are stateless, which this design already assumes).
- Run several cancellation workers (partitioned by `reservationId`) if the
  queue grows.
- Notify the student (email/push) when the worker finishes a cancellation,
  instead of making them poll `getRequestStatus`.

The full design for these extensions is in
[fault-tolerance-scalability.md](fault-tolerance-scalability.md).
