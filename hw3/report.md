# CN HW3 Report

b09902004 資工三 郭懷元

## Notation

- `cwnd`: Current transmit window size
- `thres`: Threshold of switching from "slow start" to "congestion avoidance"
- `next_seq_num`: The next sequence number that `receiver` is expecting

## Sender
```mermaid
flowchart TB
  A["Initialize an empty queue. Enqueue the video's metadata (e.g. resolution)"]
  B["Release resources and end the process"]
  A-->Streaming
  Streaming-->B
  subgraph Streaming
    direction TB
    C1["Fill queue with segments of a frame until queue size > <code>cwnd</code>"]
    C2["Send the first <code>cwnd</code> segments in queue"]
    C3["Receive ACK with timeout"]
    C4-1["Verify that the segment comes from <code>agent</code>"]
    C4-2["Adjust <code>cwnd</code> and <code>thres</code>"]
    C5["Dequeue until the first element
    with <code>seqNum</code> > <code>ackNum</code>"]
    C5alt["<b>Finish streaming</b>"]
    C6["Adjust <code>cwnd</code> based on
    the current congestion control mode"]
    
    C1-->C2-->C3;

    C3-."Timeout".->C4-2-->C1;
    C3-."Received segment".->C4-1;

    C4-1-."Is FINACK".->C5alt;
    C4-1-."Is normal ACK".->C5;
    
    C5-."# segments ACKed < <code>cwnd</code>".->C3;
    C5-."# segments ACKed == <code>cwnd</code>".->C6-->C1;
  end
```
---

<div style="page-break-before: always;"></div>

## Receiver
```mermaid
flowchart TB
  A["Initialize a fixed-size buffer"]
  B["Release resources and end the process"]
  A-->Streaming
  Streaming-->B
  subgraph Streaming
    direction TB
    C1["Receive a data segment"]
    C2-1["Construct
    ACK #<code>next_seq_num-1</code>"]
    C2-2["Construct FINACK"]
    C2-3["Construct
    ACK <code>next_seq_num</code>"]
    C3["Send the constructed segment"]
    C4-1["<b>Finish streaming</b>"]
    
    C1-."Out of order
    Data corrupted
    Full buffer".->C2-1;
    C1-."Is FIN segment".->C2-2;
    C1-."Is data segment".->C2-3;
    C2-1 & C2-2 & C2-3-->C3;
    C3-."Received FIN segment
    and buffer not full".->C4-1;
    C3-."Buffer not full".->C1;
    C3-."Buffer full".->C4-2-->C1;

    subgraph C4-2 [Flush buffer]
    direction TB
      D1-1["Unpack data #1 and #2
      as metadata"]
      D1-2["Unpack other data
      as partial video frame"]
      D2["If the frame is completely filled,
      display the frame"]
      D1-2-->D2
    end

  end
```
---

<div style="page-break-before: always;"></div>

## Agent

```mermaid
flowchart TB
  A["Receive a segment"]
  B1["Forward to <code>sender</code>"]
  B2["Forward to <code>receiver</code>"]
  
  D["Release resources
  and end the process"]

  C["Select the next step
  based on <code>error_rate</code>"]
  C1["Corrupt payload"]
  C2["Drop segment"]

  A-."Comes from <code>receiver</code>".->B1;
  B1-."Is ACK".->A;
  B1-."Is FINACK".->D;

  A-."Comes from <code>sender</code>".->C;
  C-->C1 & B2 & C2;
  C1-->B2-->A;  
```
---