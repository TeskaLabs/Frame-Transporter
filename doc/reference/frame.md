# libft reference - Memory Frames and Vectors

The memory frame is a fixed-length continuos memory buffer which is managed by a libft \(see memory pool\). It is typically aligned with underlying [system memory pages](https://en.wikipedia.org/wiki/Page_%28computer_memory%29) and the capacity \(aka size\) is an integer multiple of the memory page size. The default capacity of the memory frame is 20KiB respectively five 4KiB memory pages. Memory frames are used as buffers for storing data that are send to and received from network.

The content of a memory frame _can_ be organized using _vectors_. Vectors provide [scatter/gather mechanism](https://en.wikipedia.org/wiki/Vectored_I/O) which means that vectors divide a memory space of the assigned frame into smaller pieces. Each memory frame can contain series of vectors. Vectors can overlaps each other, have gaps between each others and/or be organised in random non-continuos manner. Vector provide a convinient functionality for effective parsing and building data from/to wire protocols.

### Schema: Memory frame and its vectors

```asciiart
Memory frame
+---------------------------------------------------------------+
| |- Vector -|                                                  |
|       |----- Vector ----------|                               |
|                                         |--- Vector ---|      |
+---------------------------------------------------------------+
^ Frame data                                                    ^ Frame data + capacity
```

## `struct ft_frame`- Memory frame class

### Memory coordinates

#### size\_t capacity \[read-only attribute\]

Specify how much data can fit into a frame in bytes. Typically 20KiB.

Capacity is read-only value that is provided during frame construction time.

#### uint8\_t \* data \[read-only attribute\]

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

