#include "sticky_note.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <userspace-descriptors/descriptor.h>

typedef uint32_t u32;
typedef uint8_t u8;

struct sticky_note {
    u8 const* data;
    u32 size;
    u32 cursor;
};
static int sticky_note_close(struct userspace_descriptor* descriptor);
static ssize_t sticky_note_read(struct userspace_descriptor* descriptor, void* buf, size_t size);
static ssize_t sticky_note_write(struct userspace_descriptor* descriptor, void const* buf, size_t size);
static int sticky_note_fstat(struct userspace_descriptor* descriptor, struct stat*);

int sticky_note_init(void)
{
    struct sticky_note* handle = malloc(sizeof(struct sticky_note));
    if (!handle)
        return -1;
    *handle = (struct sticky_note) {
        .data = 0,
        .cursor = 0,
        .size = 0,
    };
    return userspace_descriptor_register((struct userspace_descriptor){
        .user = handle,
        .read = sticky_note_read,
        .write = sticky_note_write,
        .close = sticky_note_close,
        .fstat = sticky_note_fstat,
    });
}

static int sticky_note_close(struct userspace_descriptor* descriptor)
{
    struct sticky_note* handle = descriptor->user;
    if (handle->data)
        free((u8*)handle->data);
    free(descriptor->user);
    return 0;
}

static ssize_t sticky_note_read(struct userspace_descriptor* descriptor, void* buf, size_t size)
{
    struct sticky_note* handle = descriptor->user;
    if (handle->size == 0)
        return 0;
    if (size > handle->size - handle->cursor) {
        size = handle->size - handle->cursor;
    }
    memcpy(buf, &handle->data[handle->cursor], size);
    handle->cursor += size;
    handle->cursor %= handle->size;
    return size;
}

static ssize_t sticky_note_write(struct userspace_descriptor* descriptor, void const* buf, size_t size)
{
    (void)buf;
    (void)size;
    struct sticky_note* handle = descriptor->user;
    if (!handle->data) {
        handle->data = malloc(size);
        if (!handle->data)
            return -1;
    }
    void* new_data = realloc((void*)handle->data, size);
    if (!new_data)
        return -1;
    handle->data = new_data;
    handle->size = size;
    memcpy((u8*)handle->data, buf, size);
    return size;
}

static int sticky_note_fstat(struct userspace_descriptor* descriptor, struct stat* st)
{
    struct sticky_note* handle = descriptor->user;
    *st = (struct stat) {
        .st_size = handle->size,
    };
    return 0;
}
