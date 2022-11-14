# CN HW2 Report

b09902004 資工三 郭懷元

## File transferring workflow

### Notations

"File segment" is like a "block" of file contents, with a designed size limit of 1 MiB.

### Workflow of `put <filename>`

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: filename
    note over c: Check file existence
    loop Until EOF reached
        note over c: Try to read a file segment
        alt Read > 0 B
            c->>s: file segment
            note over s: Write file segment
        end
    end
```

On alternative paths (e.g. if read more than 0 bytes), a "control message" is also sent in order to indicate which path we are taking.

<div style="page-break-before: always;"></div>

### Workflow of `get <filename>`

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: filename
    note over s: Check file existence
    loop Until EOF reached
        note over s: Try to read a file segment
        alt Read > 0 B
            s->>c: file segment
            note over c: Write file segment
        end
    end
```

On alternative paths, a "control message" is also sent in order to indicate which path we are taking.

### Details on file segment transferring

```mermaid
sequenceDiagram
    participant tx as sender
    participant rx as receiver
    tx ->> rx: segment length in uint64
    tx ->> rx: file segment raw bytes
```

A `uint64` number is packed into a `char` array in a little-endian way.

---

<div style="page-break-before: always;"></div>

## Video streaming workflow

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: video filename
    note over s: Check existence & file extension
    note over s: Open video
    s->>c: video resolution & frame time
    loop Until end of video reached (a 0-sized frame)
        note over s: Read a frame
        s->>c: frame size
        s->>c: raw frame bytes
        note over c: Display the received frame
        note over c: Wait until [ESC] is pressed <br/> or frame time has passed
        c->>s: pressed key
        note over c,s: Leave loop if [ESC] is pressed
    end
```

On alternative paths, a "control message" is also sent in order to indicate which path we are taking.

---

<div style="page-break-before: always;"></div>

## What is SIGPIPE? Is it possible that SIGPIPE is sent to your process? If so, how do you handle it?

SIGPIPE is sent when the other end breaks the connection, and the default handler for SIGPIPE will end the process.

I used `send()` with `MSG_NOSIGNAL` flag to avoid generating SIGPIPE, as I implemented IO multiplexing with pthreads, and it's basically impossible to determine which thread the SIGPIPE is for. I used the return value from `send()` to know if an error occurs, then gracefully close the socket to end the connection.

---

## Is blocking I/O equal to synchronized I/O? Please give some examples to explain it.

> Refs:
> 
> - [POSIX.1-2017 Definitions](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html)
> - [Synchronizing I/O (The GNU C Library)](https://www.gnu.org/software/libc/manual/html_node/Synchronizing-I_002fO.html)
> - [淺談I/O Model. 前言 | by Carl | Medium](https://medium.com/@clu1022/%E6%B7%BA%E8%AB%87i-o-model-32da09c619e6)
>
> Note: In this section, blocking, non-blocking, synchronized, synchronous, and asynchronous are used with the definition in POSIX.

### Blocking & Synchronized I/O

Blocking I/O doesn't equal to synchronized I/O. *Blocking* only implies that the function won't return until the request has been fulfilled, but *synchronized* requires guarantee of data integrity.

For example in normal file outputs on UNIX & UNIX-like OS's, *synchronized* typically means the data is written onto the **physical device**, and can be enforced using `sync()` for the whole filesystem. *blocking* means the data is written to the kernel's **buffer cache**, and can be enabled on a per-file-descriptor basis.

In the case of TCP socket I/O, calling `send()` in blocking mode doesn't mean that the data is immediately sent. The kernel will usually wait until enough data is in its TCP buffer, then send the packet out, to reduce the header overhead of many tiny TCP packets.

<div style="page-break-before: always;"></div>

### Synchronous & Asynchronous I/O

Another concept is synchronous and asynchronous, which can be explained through the following diagrams.

#### Asynchronous I/O

I/O operation won't cause the user thread to be blocked. Their execution is decoupled.

```mermaid
sequenceDiagram
    participant t as thread
    participant k as kernel
    participant b as buffer cache
    participant d as disk
    t ->> k: Call aio_read()
    note over k: Read request is queued
    k ->> t: Return
    par
        note over t: Continue execution
    and
        d ->> b: Read data to<br/>buffer cache
        b ->> k: Copy data to<br/>thread's buffer
    end
    k ->> t: Notify the operation<br/>has finished
```

<div style="page-break-before: always;"></div>

#### Synchronous I/O

A non-blocking but synchronous I/O call can still block the user thread.

```mermaid
sequenceDiagram
    participant t as thread
    participant k as kernel
    participant b as buffer cache
    participant d as disk
    t ->> k: Call read()
    note over k: Requested data<br/>not in buffer cache
    alt if non-blocking
        k -->> t: Return with error
    end
    d ->> b: Read data to<br/>buffer cache
    alt if non-blocking
        t -->> k: Call read() again
    end
    b ->> k: Copy data to<br/>thread's buffer<br/>(Thread is blocked)
    k ->> t: Return
```

---
