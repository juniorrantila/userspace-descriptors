#pragma once
#include <sys/stat.h>
#include <sys/types.h>

struct userspace_descriptor {
    void* user;
    ssize_t (*read)(struct userspace_descriptor*, void*, size_t);
    ssize_t (*write)(struct userspace_descriptor*, void const*, size_t);
    int (*close)(struct userspace_descriptor*);
    int (*fstat)(struct userspace_descriptor*, struct stat*);
};

int userspace_descriptor_register(struct userspace_descriptor);
