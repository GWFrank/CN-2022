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
    
    note over c: Check file existence<br/>in client_dir
    
    break File doesn't exist
        c --> s: Print file doesn't exist message
    end
    
    loop Until EOF reached
        note over c: Try to read a file segment
        alt Read > 0 B
            c->>s: file segment
            note over s: Write file segment
        end
    end
```

On alternative paths (e.g. if read more than 0 bytes), a "control message" is also sent in order to indicate which path we are taking.

### Workflow of `get <filename>`

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: filename
    note over s: Check file existence <br/> in server_dir
    break File doesn't exist
        c --> s: Print file doesn't exist message
    end
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

<div style="page-break-before: always;"></div>

## Video streaming workflow

```mermaid
sequenceDiagram
    participant c as client
    participant s as server
    c->>s: video filename
    note over s: Check file existence<br/>in server_dir
    break File doesn't exist
        c --> s: Print file doesn't exist message
    end
    note over s: Check file extension
    break Isn't an mpg file
        c --> s: Print invalid file message
    end
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

<div style="page-break-before: always;"></div>

## What is SIGPIPE? Is it possible that SIGPIPE is sent to your process? If so, how do you handle it?

SIGPIPE is sent when the other end breaks the connection, and the default handler for SIGPIPE will end the process.

I used `send()` with `MSG_NOSIGNAL` flag to avoid generating SIGPIPE, as I implemented IO multiplexing with pthreads, and it's basically impossible to determine which thread the SIGPIPE is for. I used the return value from `send()` to know if an error occurs, then gracefully close the socket to end the connection.

## Is blocking I/O equal to synchronized I/O? Please give some examples to explain it.

> Refs:
> 
> - [POSIX.1-2017 Definitions](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html)
> - [Synchronizing I/O (The GNU C Library)](https://www.gnu.org/software/libc/manual/html_node/Synchronizing-I_002fO.html)
> - [淺談I/O Model. 前言 | by Carl | Medium](https://medium.com/@clu1022/%E6%B7%BA%E8%AB%87i-o-model-32da09c619e6)

Blocking I/O doesn't equal to synchronized I/O. In fact, at least in Linux, most I/O operations are probably not synchronized. A blocking I/O operation only guarantees the kernel to finish writing to the kernel's **buffer cache**. A synchronized I/O operation requires the data to be written onto the **physical device**.

For example, calling `write()` in blocking-mode means the data will be written to the buffer cache, but it might not be written onto the hard drive for some time, or until calls to functions like `sync()`.

Another concept is "synchronous/asynchronous", which can be explained through the following diagram.

```mermaid
sequenceDiagram
    participant p as process
    participant k as kernel
    alt Blocking I/O
        p ->> k: Call write()
        note over k: Start writing data <br/> into kernel's buffer cache
        note over k: Still writing...
        note over k: Finish writing
        k ->> p: Return normally
    else Non-blocking I/O
        p ->> k: Call write()
        note over k: Start writing data <br/> into kernel's buffer cache
        k ->> p: Return with error
        note over k: Still writing...
        p ->> k: Call write()
        k ->> p: Return with error
        note over k: Finish writing
        p ->> k: Call write()
        k ->> p: Return normally
    else Asynchronous I/O
        p ->> k: Call async version of write()
        note over k: Add write request to a queue
        k ->> p: Return normally
        note over p: Continue execution
        note over p,k: A few moments later...
        note over k: Finish the write request
        k ->> p: Notify the requesting process
    end
```
