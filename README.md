# Eezydep
Eezydep is an event-driven HTTP reverse-proxy written in C for Linux systems, built arount non-blocking sockets and `epoll`.
It routes incoming HTTP requests to configurable backend services based on the `Host` HTTP header. Each proxied connection is managed through an explicit state machine, with buffered bidirectional I/O, connection timeouts based on state and synchronous backend connection handling.

**Status:** In development. Eezydep currently implements the subset of HTTP/1.x required for host-based reverse proxying and is not yet intended for production use.

## Features
- Event-driven networking using Linux `epoll`
- Non-blocking client and backend sockets
- Host-based HTTP routing
- Asynchronous backend connection establishment
- Explicit per-connection state machine
- Bidirectional buffered I/O
- Partial read/write handling
- Connection and state-specific timeouts
- Configurable backend routes
- Signal-driven clean shutdown

## Architecture & State Machine
```mermaid
flowchart LR
    C[Client]

    subgraph E[Eezydep]
        EL[epoll Event Loop]
        L[Listening Socket]
        PC[Proxy Connection]
        RT[Route Table]
    end

    B[Backend Service]

    C -->|connect| L

    L -->|ready event| EL
    EL -->|accept| PC

    PC -->|register FDs| EL
    EL -->|read/write readiness| PC

    PC -->|Host lookup| RT
    RT -->|backend address| PC

    C <-->|HTTP traffic| PC
    PC <-->|HTTP traffic| B
```
```mermaid
stateDiagram-v2
    [*] --> READ_HEADER: listen socket accept
    READ_HEADER --> CONNECT_BACKEND: headers parsed
    READ_HEADER --> CLOSING: invalid request / timeout / disconnect

    CONNECT_BACKEND --> PIPING: connect complete
    CONNECT_BACKEND --> CLOSING: connect error / timeout

    PIPING --> CLOSING: EOF / error / timeout / shutdown

```

A single event loop is run, multiplexing listening, client, and backend
sockets using `epoll`.

Each accepted connection is represented by a `proxy_conn_t` structure that
stores connection state, socket descriptors, timestamps, request data, and
per-direction write buffers.

The state machine determines which operations are valid at each stage of the
connection lifecycle.

## Connection lifecycle

A proxied connection moves through the following states:

### `READ_HEADER`

Eezydep reads HTTP headers from the client until a complete header block is
available. The `Host` header is extracted the corresponding backend route is retrieved from a configuration file.

### `CONNECT_BACKEND`

A non-blocking connection is initiated to the selected backend. Completion is
detected through `epoll`.

### `PIPING`

Once connected, traffic is forwarded bidirectionally between the client and
backend.

Because socket writes may complete only partially, unsent data is retained in
per-direction buffers and retried when the destination becomes writable.

### `CLOSING`

Socket descriptors are removed from the event loop and all resources associated
with the connection are released.

## Routing Configuration
Routes map HTTP hostnames to backend addresses.

Example:

```text
api.eezydep.org 127.0.0.1:8080
auth.eezydep.org 127.0.0.1:9000
```
a request containing `Host: api.eezydep.org` will be forwarded to `127.0.0.1:8080`

## Current limitations

- Linux only due to the use of `epoll`
- HTTP/1.x only
- No TLS termination [in progress]
- HTTP parsing implements only the subset required for host-based routing
- Each hostname currently maps to a single backend
- No load balancing between backend instances

## Roadmap

Near-term development priorities:

- [ ] Complete bounded-buffer backpressure
- [ ] Expand integration and stress testing
- [ ] Add zero-downtime route reload via `SIGHUP`
- [ ] Add backend health checks
- [ ] Add structured logging and metrics
- [ ] Add TLS termination

Long term, Eezydep is intended to serve as the networking layer of a lightweight
deployment platform for self-hosted applications.
