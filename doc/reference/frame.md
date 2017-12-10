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

|-------------+-------------+-------------|
^             ^             ^             ^ 
Offset        Position      Limit         Capacity
```

The location of the vector within a frame is defined by vector's O_ffset_ and _Capacity_. Vector cannot cross low and high boundary of the frame. Vector defines also its _Position_ and _Limit_. Read/write operations happen at the _Position_, which is increased as the operation finishes.

It is postulated that: 0 &lt;= _Position_ &lt;= _Limit_ &lt;= _Capacity_. It means that:

* _Position_ can be in range of 0 to _Capacity_ but lower or equal to _Limit_.
* _Limit_ can be in range of 0 to _Capacity_ but higher of equal to _Position_.
* If _Position_ equals to _Limit_, no futher read/write operations are possible because we hit the end of a vector.

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

### Read & write lifecycle

Here we present a typical lifecycle of the memory frames with vectors on the network protocol, that utilizes a concept of fixed headers and variable-length bodies \(such as HTTP/2 or SPDY\).

#### Step \#1: Prepare a frame for reading fixed-length header from network

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
^ Position = 0  
                ^ Capacity = 8
```

The frame is prepared so that it contains a one vector, that points at the begin of the frame. The capacity of vector is set to a known size of the protocol header \(in this example it is 8 bytes\).

#### Step \#2: A fixed-length header is received

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
                ^ Position = 8
                ^ Capacity = 8
```

The protocol header is received e.g. by libft sockets. The Position is equal Limit. Now the length of the body needs to be parsed from data.

#### Step \#3: Flip of the vector to prepare for reading

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
^ Position = 0
                ^ Capacity = 8
```

The _flip_ set Position to 0 \(Limit stays the same because previous value of Position was 8\).

#### Step \#4: Parse a header

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
............ >> ^ Position = 8
                ^ Capacity = 8
```

Iterate thru a vector and use load/store family of functions to parse the header data from the frame. For this example let's assume, that we extracted length of the body to be 32 bytes.

#### Step \#5: Prepare a frame for reading variable-length body from network

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
                ^ Position = 8
                ^ Capacity = 8

                | ----------------- vector -------------------|
                ^ Offset = 8
                                                              ^ Limit = 32
                ^ Position = 0
                                                              ^ Capacity = 32
```

Add a second vector with capacity 32 bytes \(the value received from parser of the header data\).

#### Step \#6: A body is received

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
                ^ Position = 8
                ^ Capacity = 8

                | ----------------- vector -------------------|
                ^ Offset = 8
                                                              ^ Limit = 32
                                                              ^ Position = 32
                                                              ^ Capacity = 32
```

The protocol header is received e.g. by libft sockets. The Position is equal Limit. Now the length of the body needs to be parsed from data.

#### Step \#7: The frame is flipped and read is complete

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
^ Position = 0
                ^ Capacity = 8

                | ----------------- vector -------------------|
                ^ Offset = 8
                                                              ^ Limit = 32
                ^ Position = 0
                                                              ^ Capacity = 32
```

The frame is flipped again and the read cycle is over. The frame is prepared for any upstream processing e.g. by application layer  or by an higher level protocol such as HTTP or TLS. Equally it is ready for being send over the network. The later will be demonstrated in the next steps of this example.

#### Step \#8: Send the frame over the network

```
+------------------------------------------------------------------------------
|---- vector ---|
^ Offset = 0
                ^ Limit = 8
                ^ Position = 8
                ^ Capacity = 8

                | ----------------- vector -------------------|
                ^ Offset = 8
                                                              ^ Limit = 32
                                                              ^ Position = 32
                                                              ^ Capacity = 32
```

Because the frame has been correctly prepared, libft sent the frame at once. libft iterated over all vectors and sent data from each vector based on its _Position_ and _Limit_.

### Vector methods

#### `bool ft_vec_forward(struct ft_vec *, size_t position_delta)`

Move a _Position_ forward by `position_delta` bytes. `position_delta`_can be a negative value, in that case, a Position_ will be moved backward.

Returns `true` if _Position_ has been changed correctly, `false` for error such as exceeding vector _Limit_ or _Capacity_.

#### bool`ft_vec_set_position(struct ft_vec *, size_t position)`

Set a _Position_ value. Negative values are not accepted.

Returns `true` if _Position_ has been changed correctly, `false` for error such as exceeding vector _Limit_ or _Capacity_.

#### `void ft_vec_flip(struct ft_vec *)`

Set _Limit_ to a current value of _Position_ and reset _Position_ to 0.

Flip function is used to flip the vector from "writing" to "reading" and vice verse. After a sequence of writes is used to fill the vector, flip will set the limit of the vector to the current position and reset the position to zero. This has the effect of making a future read from the vector read all of what was written into the vector and no more.

#### `void ft_vec_trim(struct ft_vec *)`

Set _Limit_ and _Capacity_ to a current _Position_.

#### `bool ft_vec_extend(struct ft_vec *, size_t size)`

Add a value of `size` to a currect Capacity of the vector. It means that the vector will be extended by a provided value.

Returns `true` if _Position_ has been changed correctly, `false` for error such as exceeding capacity of the frame.

#### `bool ft_vec_sprintf(struct ft_vec *, const char * format, ...)`

Print a [formatted string](https://en.wikipedia.org/wiki/Printf_format_string) into a vector.

Returns `true` if successful, `false` when error has been observed \(e.g. result doesn't fit into a frame\).

#### `bool ft_vec_vsprintf(struct ft_vec *, const char * format, va_list ap)`

Print a [formatted string](https://en.wikipedia.org/wiki/Printf_format_string) into a vector \(variable argument style\).

Returns `true` if successful, `false` when error has been observed \(e.g. result doesn't fit into a frame\).

#### `bool ft_vec_strcat(struct ft_vec *, const char * text)`

Append a `text` to a NUL-terminated string in the a vector in a `strcat()` way and move _Position_ at the end of the resulting string.

Returns `true` if successful, `false` when error has been observed \(e.g. result doesn't fit into a frame\).

#### `bool ft_vec_cat(struct ft_vec *, const void * data, size_t data_len)`

Copy bytes from `data` \(length is defined by`data_len` \) into the a vector and move _Position_ by `data_len`.

Returns `true` if successful, `false` when error has been observed \(e.g. result doesn't fit into a frame\).

#### `size_t ft_vec_remaining(struct ft_vec *)`

Returns a difference between _Position_ and _Limit_. It means how many more data can fit into a vector or how many data can be still read from a vector.

#### `void * ft_vec_ptr(struct ft_vec *)`

...

#### `void * ft_vec_begin_ptr(struct ft_vec *)`

...

#### `void * ft_vec_end_ptr(struct ft_vec *)`

...

#### `void ft_vec_advance_ptr(struct ft_vec *, void * ptr)`

...

### Vector attributes

#### `uint16_t offset` \[read-only attribute\]

A location of the vector within the frame.

#### `uint16_t capacity` \[read-only attribute\]

A size of the vector.

#### `uint16_t position` \[read-only attribute\]

The position is the index of the next byte to be read or written.

#### `uint16_t limit` \[read-only attribute\]

The limit is the index of the first byte that should not be read or written.

#### `struct ft_frame * frame` \[read-only attribute\]

A pointer to a frame that owns a vector.

