# libft reference - Memory Frames and Vectors

Memory frame is a fixed-length memory space managed by a libft \(see memory pool\). It is typically aligned with underlying [system memory pages](https://en.wikipedia.org/wiki/Page_%28computer_memory%29) and the size is an integer multiple of the memory page size. The default size of the memory frame is 20KiB respectively five 4KiB memory pages.Memory frames are used for storing data that are send to and received from network.

## `struct ft_frame`- Memory frame class





