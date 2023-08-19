#include "descriptor.h"
#include <stdint.h>
#include <sys/errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/syslimits.h>

typedef uint32_t u32;
typedef uintptr_t uptr;

#define CUSTOM_FD_BIT ((uint32_t)(1 << 30))

typedef struct {
    struct userspace_descriptor descriptor;

    bool is_used;
} Descriptor;
static int descriptor_init_slot(u32 slot, struct userspace_descriptor);
static int descriptor_init_new_slot(struct userspace_descriptor);
static Descriptor descriptor_invalid = { 0 };

static Descriptor g_descriptors[OPEN_MAX];
static u32 g_descriptors_size = 0;
static u32 g_descriptors_capacity = OPEN_MAX;

#ifndef NDEBUG
#define ASSERT(cond, func, line)                                        \
   do {                                                                 \
       if (!(cond)) {                                                   \
           fprintf(stderr, "%s:%d: ASSERT failed: (%s) from %s:%d\n", __FUNCTION__, __LINE__, #cond, func, line); \
       }                                                                \
   } while(0)
#else
#define ASSERT(cond, func, line) do{ (void)!(cond); (void)func; (void)line; } while(0)
#endif


static Descriptor* descriptor_from_slot(u32 slot)
{
    return &g_descriptors[slot];
}

#define descriptor_from_fd(fd) descriptor_from_fd_impl(fd, __FUNCTION__, __LINE__)
static Descriptor* descriptor_from_fd_impl(int fd, char const* func, int line)
{
    ASSERT(fd & CUSTOM_FD_BIT, func, line);
    u32 slot = (u32)(fd & ~CUSTOM_FD_BIT);
    if (slot >= g_descriptors_size)
        return errno=EOVERFLOW, NULL;
    return descriptor_from_slot(slot);
}

int userspace_descriptor_register(struct userspace_descriptor self)
{
    for (u32 id = 0; id < g_descriptors_size; id++) {
        Descriptor* descriptor = descriptor_from_fd(id | CUSTOM_FD_BIT);
        if (!descriptor->is_used) {
            return descriptor_init_slot(id, self);
        }
    }
    return descriptor_init_new_slot(self);
}

static int do_descriptor_init(Descriptor* self, struct userspace_descriptor descriptor);
static int descriptor_init_slot(u32 slot, struct userspace_descriptor descriptor)
{
    Descriptor* self = descriptor_from_slot(slot);
    if (do_descriptor_init(self, descriptor) < 0)
        return -1;
    slot |= CUSTOM_FD_BIT;
    return (int)slot;
}

static int descriptor_init_new_slot(struct userspace_descriptor descriptor)
{
    if (g_descriptors_size >= g_descriptors_capacity) {
        errno = ENFILE;
        return -1;
    }
    u32 id = g_descriptors_size++;
    int fd = descriptor_init_slot(id, descriptor);
    if (fd < 0) {
        g_descriptors_size--;
        return -1;
    }
    return fd;
}

static int do_descriptor_init(Descriptor* self, struct userspace_descriptor descriptor)
{

    *self = (Descriptor) {
        .descriptor = descriptor,
        .is_used = true,
    };
    return 0;
}

static int descriptor_deinit(Descriptor* self)
{
    int result = 0;
    if (self->descriptor.close) {
        result = self->descriptor.close(&self->descriptor);
    }
    *self = descriptor_invalid;
    return result;
}

static int do_close(int fd);
int close(int fd)
{
    static int (*og_close)(int fd);

    if (fd & CUSTOM_FD_BIT)
        return do_close(fd);

    if (!og_close) {
        void* handle = dlsym(RTLD_NEXT, "close");
        if (!handle) {
            return errno = ENOSYS, -1;
        }
        og_close = (int(*)(int))handle;
    }
    return og_close(fd);
}

static int do_close(int fd)
{
    assert(fd & CUSTOM_FD_BIT);
    return descriptor_deinit(descriptor_from_fd(fd));
}

static ssize_t do_read(int fd, void* buf, size_t size);
ssize_t read(int fd, void* buf, size_t size)
{
    static ssize_t (*og_read)(int fd, void* buf, size_t size);

    if (fd & CUSTOM_FD_BIT)
        return do_read(fd, buf, size);

    if (!og_read) {
        void* handle = dlsym(RTLD_NEXT, "read");
        if (!handle) {
            return errno = ENOSYS, -1;
        }
        og_read = (ssize_t(*)(int, void*, size_t))handle;
    }
    return og_read(fd, buf, size);
}

static ssize_t do_read(int fd, void* buf, size_t size)
{
    assert(fd & CUSTOM_FD_BIT);
    Descriptor* self = descriptor_from_fd(fd);
    if (!self)
        return -1;
    if (self->descriptor.read)
        return self->descriptor.read(&self->descriptor, buf, size);
    return errno=ENOSYS, -1;
}

static ssize_t do_write(int fd, void const* buf, size_t size);
ssize_t write(int fd, void const* buf, size_t size)
{
    static ssize_t (*og_write)(int fd, void const* buf, size_t size);

    if (fd & CUSTOM_FD_BIT)
        return do_write(fd, buf, size);

    if (!og_write) {
        void* handle = dlsym(RTLD_NEXT, "write");
        if (!handle) {
            return errno = ENOSYS, -1;
        }
        og_write = (ssize_t(*)(int, void const*, size_t))handle;
    }
    return og_write(fd, buf, size);
}

static ssize_t do_write(int fd, void const* buf, size_t size)
{
    assert(fd & CUSTOM_FD_BIT);
    Descriptor* self = descriptor_from_fd(fd);
    if (!self)
        return -1;
    if (self->descriptor.write)
        return self->descriptor.write(&self->descriptor, buf, size);
    return errno=ENOSYS, -1;
}

static int do_fstat(int, struct stat*);
int fstat(int fd, struct stat* st)
{
    static int (*og_fstat)(int fd, struct stat* st);
    if (fd & CUSTOM_FD_BIT)
        return do_fstat(fd, st);

    if (!og_fstat) {
        void* handle = dlsym(RTLD_NEXT, "fstat");
        if (!handle) {
            return errno = ENOSYS, -1;
        }
        og_fstat = (int(*)(int, struct stat*))handle;
    }
    return og_fstat(fd, st);
}

static int do_fstat(int fd, struct stat* st)
{
    assert(fd & CUSTOM_FD_BIT);
    Descriptor* self = descriptor_from_fd(fd);
    if (!self)
        return -1;
    if (self->descriptor.fstat)
        return self->descriptor.fstat(&self->descriptor, st);
    return errno=ENOSYS, -1;
}
