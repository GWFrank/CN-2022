# CN HW2 Report

## File transferring workflow

### Notations

"File segment" is like a "block" of file contents, with a designed size limit of 1 MiB.

### Workflow of `put <filename>`

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: filename
    note over c: Check file existence<br/>in client_dir
    alt file exists
        loop
            note over c: Try to read a file segment
            alt read > 0 B
                c->>s: file segment
                note over s: Write file segment
            end
        end
    else file doesn't exist
        note over c: print file doesn't exist message
    end
```

On alternative paths (e.g. if read more than 0 bytes), a "control message" is also sent, in order to indicate which path we are taking.

### Workflow of `get <filename>`

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: filename
    note over s: Check file existence <br/> in server_dir/<username>
    alt file exists
        loop
            note over s: Try to read a file segment
            alt read > 0 B
                s->>c: file segment
                note over c: Write file segment
            end
        end
    else file doesn't exist
        note over c: print file doesn't exist message
    end
```

On alternative paths (e.g. if read more than 0 bytes), a "control message" is also sent, in order to indicate which path we are taking.

### Details on file segment transferring

```mermaid
sequenceDiagram
    participant tx as sender
    participant rx as receiver
    tx ->> rx: segment length in uint64
    tx ->> rx: file segment raw bytes
```

A `uint64` number is packed into a `char` array in a little-endian way.

## Video streaming workflow

## What is SIGPIPE? Is it possible that SIGPIPE is sent to your process? If so, how do you handle it?

SIGPIPE is sent when the other end breaks the connection, and the default handler for SIGPIPE will end the process.

I used `send()` with `MSG_NOSIGNAL` flag to avoid generating SIGPIPE, as I implemented IO multiplexing with pthreads, and it's basically impossible to determine which thread the SIGPIPE is for. I used the return value from `send()` to know if an error occurs, then gracefully close the socket to end the connection.

## Is blocking I/O equal to synchronized I/O? Please give some examples to explain it.

