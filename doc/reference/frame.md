# libft reference - Memory Frames and Vectors

The memory frame is a fixed-length memory space managed by a libft \(see memory pool\). It is typically aligned with underlying [system memory pages](https://en.wikipedia.org/wiki/Page_%28computer_memory%29) and the size is an integer multiple of the memory page size. The default size of the memory frame is 20KiB respectively five 4KiB memory pages.Memory frames are used for storing data that are send to and received from network.

## `struct ft_frame`- Memory frame class

#### `enum ft_frame_type type` \[read-write attribute\] - Type of the memory frame

Each memory frame carries information about its type. This information can be used for faster dispatching of the frame. The type is a number and can be changed by an application.

Known memory frame types:

* `FT_FRAME_TYPE_FREE` - a memory frame is not used, a frame is returned back to pool. If you encounter frames of this type, it is likely an incorrect implementation \(use after release\).
* `FT_FRAME_TYPE_END_OF_STREAM` - a special frame that signalizes an end of a stream. There are no data in this frame.
* `FT_FRAME_TYPE_RAW_DATA` - unspecified \(raw\) data are present in the frame.
* `FT_FRAME_TYPE_LOG` - the frame carries logging information





