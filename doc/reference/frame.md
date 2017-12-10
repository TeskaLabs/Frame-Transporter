# libft reference - Memory Frames and Vectors

The memory frame is a fixed-length continuos memory buffer which is managed by a libft \(see memory pool\). It is typically aligned with underlying [system memory pages](https://en.wikipedia.org/wiki/Page_%28computer_memory%29) and the capacity \(aka size\) is an integer multiple of the memory page size. The default capacity of the memory frame is 20KiB respectively five 4KiB memory pages. Memory frames are used as buffers for storing data that are send to and received from network.

The content of a memory frame _can_ be organized using _vectors_. Vectors provide [scatter/gather mechanism](https://en.wikipedia.org/wiki/Vectored_I/O) which means that vectors divide a memory space of the assigned frame into smaller pieces. Each memory frame can contain series of vectors. Vectors can overlaps each other, have gaps between each others and/or be organised in random non-continuos manner. Vector provide a convinient functionality for effective parsing and building data from/to wire protocols.

### Schema: Memory frame with vectors

```asciidoc
Memory frame
+------------------------------------------------------------+
|          |- 1st Vector -|                                  |
|               |---- 2nd Vector ----|                       |
|                                           |-- 3rd Vector --|
|- 4th Vector -|                                             |
+------------------------------------------------------------+
^ Frame data                                                 ^ Frame data + capacity
```

### Schema: Vector

```asciidoc
Vector
|-------+-------+-------|
O       P       L       C
^       ^       ^       ^
|       |       |       Capacity
|       |       Limit (cursor)
|       Position (cursor)
Offset
```

The location of the vector within a frame is defined by vector's **O**ffset and **C**apacity. Vector cannot cross low and high boundary of the frame. Vector defines also its cursor **P**osition and **L**imit. Read/write operations happen at the cursor **P**osition, which is increased as the operation finishes.

It is postulated that: 0 &lt;= **P** &lt;= **L** &lt;= **C**. It means that:

* **P**osition can be in range of 0 to **C**apacity but lower or equal to **L**imit.
* **L**imit can be in range of 0 to **C**apacity but higher of equal to **P**osition.
* If **P**osition equals to **L**imit, no futher read/write operations are possible because the cursor is at the end of a vector.

## `struct ft_frame`- Memory frame class

### Memory coordinates

#### `size_t capacity` \[read-only attribute\]

Specify how much data can fit into a frame in bytes. Typically 20KiB.

Capacity is read-only value that is provided during frame construction time.

#### `uint8_t * data` \[read-only attribute\]

A pointer to a frame memory space as an array of bytes.

### Type

#### `enum ft_frame_type type` \[read-write attribute\]

Each memory frame carries information about its type. This information can be used for faster dispatching of the frame. The type is a number and can be changed by an application.

Known memory frame types:

* `FT_FRAME_TYPE_FREE` - a memory frame is not used, a frame is returned back to pool. If you encounter frames of this type, it is likely an incorrect implementation \(use after release\).
* `FT_FRAME_TYPE_END_OF_STREAM` - a special frame that signalizes an end of a stream. There are no data in this frame.
* `FT_FRAME_TYPE_RAW_DATA` - unspecified \(raw\) data are present in the frame.
* `FT_FRAME_TYPE_LOG` - the frame carries logging information

### Peer address

#### `struct sockaddr_storage addr` \[read-write attribute\]

#### `socklen_t addrlen` \[read-write attribute\]

The memory frame can be received of the network from a peer. The peer address is stored in `addr` and `addrlen` attributes.

**WARNING:** This is obsolete part and will be removed in future releases of libft.

## `struct ft_vec`- Memory frame vector class

Vector objects are stored at the end of the frame so that available capacity of a given frame is a bit smaller if vectors are used.

### Vector coordinates

#### `uint16_t offset` \[read-only attribute\]

A location of the vector within the frame.

#### `uint16_t capacity` \[read-only attribute\]

A size of the vector.

#### `uint16_t position` \[read-only attribute\]

The position is the index of the next byte to be read or written.

#### `uint16_t limit` \[read-only attribute\]

The limit is the index of the first byte that should not be read or written.





