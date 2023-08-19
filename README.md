# userspace-descriptors

**userspace-descriptors** is an API for creating custom file descriptors in userspace

## Build instructions

### Setup:

```sh
meson build
```

### Build:

```sh
ninja -C build
```

## Usage

```c

#include <userspace-descriptors/descriptor.h>

struct custom_fd {
    int some_data;
};
static int custom_fd_close(struct userspace_descriptor* descriptor);

int custom_fd_init(void)
{
    void* data = malloc(sizeof(struct custom_fd));
    if (!data)
        return -1;
    return userspace_descriptor_register((struct userspace_descriptor) {
        .user  = data,
        .read  = 0,
        .write = 0,
        .close = custom_fd_close,
        .fstat = 0,
    });
}

static int custom_fd_close(struct userspace_descriptor* descriptor)
{
    free(descriptor->user);
    return 0;
}

int main(void)
{
    int fd = custom_fd_init();
    if (fd < 0) {
        perror("custom_fd_init");
        return -1;
    }
    close(fd);
}

```
