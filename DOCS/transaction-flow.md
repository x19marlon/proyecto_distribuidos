# Flows

All flows are **synchronous** except **cancel a reservation**: each arrow is a
blocking request, and the consumer gets the final result in the response. A
write is only reported as successful after persistence has committed it.

Cancellation is **asynchronous**: the system accepts it, answers right away
that the cancellation is in progress, and applies it to the data layer
afterwards.

## Student workflow

```mermaid
flowchart TD
    A[Choose a month] --> B[listEventsByMonth]
    B --> C[Pick an event date]
    C --> D[getEvent: see reservations / available spots per date]
    D --> E{Already has a reservation<br/>for this event?}
    E -- No --> F[reserve N spots on a date]
    E -- Yes --> G{What to do?}
    G -- Change spots and/or date --> H[modifyReservation]
    G -- Drop it --> I[cancelReservation]
    F --> J[See updated reservation]
    H --> J
    I --> K[Message: cancellation in progress]
    K --> L[getRequestStatus later]
    L --> J
```

## Consult events of a month

1. Consumer sends `listEventsByMonth(year, month)`.
2. Event service asks persistence for all `EventDate`s between the first and the
   last day of the month (`listEventDatesBetween`), joined with their event and
   place.
3. Response: one row per date with event name, date, spot (place name or external
   address), price, `reservations`, `limit` and `available`.

Read-only, no `requestId` needed.

## Consult the reservations of spots

`getEvent(eventId)` returns every date of the event with:

| Field | Meaning |
|---|---|
| `date` | Date and time of this occurrence |
| `reservations` | Spots already taken |
| `limit` | Place capacity or `externalCapacity` |
| `available` | `limit − reservations` |

The student's own reservations come from `listStudentReservations(studentId)`.

## Reserve spots

If the student is not yet in the system, the consumer first calls
`createStudent` with their information. Then:

```mermaid
sequenceDiagram
    actor S as Student
    participant C as Consumer
    participant TS as Transaction service
    participant SS as Student service
    participant ES as Event service
    participant P as Persistence

    S->>C: Reserve N spots on date D of event E
    C->>C: generate requestId
    C->>TS: reserve(requestId, studentId, D, N)
    TS->>P: findTransactionByRequestId(requestId)
    alt already processed
        P-->>TS: existing transaction
        TS-->>C: existing result (no new effect)
    else new request
        TS->>SS: getStudent(studentId)
        SS-->>TS: Student
        TS->>ES: getEventDate(D)
        ES-->>TS: EventDate + Event (limit, price)
        TS->>TS: validate (active, not past, N >= 1, available >= N,<br/>no active reservation for E, balance >= N × price)
        alt validation fails
            TS-->>C: ERROR (code)
        else validation ok
            TS->>P: applyReservationChange(tx, new reservation,<br/>[D: +N], balance −N×price)
            Note over P: re-checks reservations + N <= limit<br/>inside the atomic write
            P-->>TS: COMMITTED
            TS-->>C: Reservation + Transaction
        end
    end
    C-->>S: Show result
```

## Modify a reservation

The student can change the number of spots, the date, or both. The new date
must belong to the same event.

```mermaid
sequenceDiagram
    actor S as Student
    participant C as Consumer
    participant TS as Transaction service
    participant P as Persistence

    S->>C: Change reservation R to M spots on date D2
    C->>C: generate requestId
    C->>TS: modifyReservation(requestId, R, newSpots=M, newEventDateId=D2)
    TS->>P: findTransactionByRequestId(requestId)
    alt already processed
        P-->>TS: existing transaction
        TS-->>C: existing result
    else new request
        TS->>P: getReservation(R), getEventDate(D1), getEventDate(D2), getEvent
        P-->>TS: data
        TS->>TS: validate (owner, ACTIVE, D2 same event, dates not past,<br/>M >= 1, enough spots, balance for extra cost, something changes)
        alt validation fails
            TS-->>C: ERROR (code)
        else validation ok
            TS->>P: applyReservationChange(tx, R{date=D2, spots=M},<br/>[D1: −N, D2: +M], balance −(M−N)×price)
            Note over P: all-or-nothing: if D2 has no room<br/>the old reservation stays intact
            P-->>TS: COMMITTED
            TS-->>C: updated Reservation + Transaction
        end
    end
    C-->>S: Show updated reservation
```

Cases:

| Change | Date deltas | Balance |
|---|---|---|
| Only spots, N → M on D | D: `+(M − N)` | `−(M − N) × price` (refund when M < N) |
| Only date, D1 → D2 | D1: `−N`, D2: `+N` | no change |
| Both | D1: `−N`, D2: `+M` | `−(M − N) × price` |

If the change fails for any reason, the original reservation (date, spots,
amount paid) remains exactly as it was.

## Cancel a reservation (asynchronous)

```mermaid
sequenceDiagram
    actor S as Student
    participant C as Consumer
    participant TS as Transaction service
    participant Q as Cancellation queue
    participant W as Cancellation worker
    participant P as Persistence

    S->>C: Cancel reservation R
    C->>C: generate requestId
    C->>TS: cancelReservation(requestId, R)
    TS->>TS: already queued or processed?
    alt yes
        TS-->>C: same answer as the first time
    else new request
        TS->>P: getReservation(R) (read only)
        P-->>TS: Reservation
        TS->>TS: validate (owner, ACTIVE, date not passed,<br/>no cancellation pending)
        alt validation fails
            TS-->>C: ERROR (code)
        else validation ok
            TS->>Q: enqueue {requestId, R, studentId}
            Q-->>TS: stored
            TS-->>C: ACCEPTED: "Your cancellation is in progress"
            C-->>S: Show "cancellation in progress"
        end
    end

    Note over W,P: later, independently of the student
    W->>Q: take next request
    W->>P: applyCancellation(tx, R)
    Note over P: atomic: reservation CANCELLED, date −spots,<br/>refund amountPaid, insert CANCELLATION (COMMITTED)
    alt ok
        P-->>W: COMMITTED
        W->>Q: acknowledge
    else persistence down
        P--xW: error
        W->>W: retry with backoff (message stays in queue)
    else no longer applicable
        P-->>W: reservation not ACTIVE / date passed
        W->>P: insert CANCELLATION transaction (FAILED, reason)
        W->>Q: acknowledge
    end

    S->>C: Check my cancellation
    C->>TS: getRequestStatus(requestId)
    TS-->>C: PENDING / COMMITTED / FAILED
```

Steps:

1. **Accept.** The service does quick checks and puts the request in the durable
   queue. Nothing is written to the data layer yet.
2. **Reply.** The student immediately gets `ACCEPTED` with the message that the
   cancellation is in progress. The reservation is still shown, marked as
   "cancellation pending".
3. **Create the cancellation.** The worker applies it to the data layer in one
   atomic step: reservation `CANCELLED`, spots released on the date, refund for
   paid events, `CANCELLATION` transaction stored as `COMMITTED`.
4. **Result.** `getRequestStatus(requestId)` returns `COMMITTED`, or `FAILED`
   with the reason.

While the cancellation is pending, the spots are still taken, and modifying or
cancelling the same reservation again is rejected with `CANCELLATION_PENDING`.

## Event CRUD

| Operation | Flow |
|---|---|
| Create | Validate spot (exactly one of `placeId` / `externalAddress`; place exists; `externalCapacity > 0` if external) and at least one date; create event and its dates with `reservations = 0`. |
| Read | Return event plus each date's `reservations`, `limit` and `available`. |
| Update | If the spot changes, the new limit must be `>= reservations` of every date. Changing `price` does not affect existing reservations' `amountPaid`. |
| Add / change a date | Insert a date, or move a date that has no reservations (proposal). |
| Delete a date | Cancel and refund its reservations; the event must keep at least one date. |
| Delete event | Cancel and refund every active reservation atomically, then delete the event and its dates. |

## Top up balance

1. Admin consumer sends `topUp(requestId, studentId, amount)`.
2. Service validates `amount > 0` and that the student exists.
3. Persistence atomically inserts a `TOP_UP` transaction and increments `balance`.

## Query history

`listTransactions(studentId)` or `listTransactions(reservationId)` is
read-only: Consumer → Transaction service → Persistence.
