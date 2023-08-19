#include "sticky_note.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int write_note(int fd, char const*);
static int read_note(int fd, void*, void(*)(void*, char const*));
static void log_note(void* context, char const* message);

int main(void)
{
    int fd = sticky_note_init();
    if (fd < 0) {
        perror("sticky_note_init");
        return -1;
    }

    int times_read = 0;

    if (write_note(fd, "Hello") < 0) {
        perror("write_note");
        return -1;
    }
    if (read_note(fd, &times_read, log_note) < 0) {
        perror("read_note");
        return -1;
    }
    if (read_note(fd, &times_read, log_note) < 0) {
        perror("read_note");
        return -1;
    }

    if (write_note(fd, "World") < 0) {
        perror("write_note");
        return -1;
    }
    if (read_note(fd, &times_read, log_note) < 0) {
        perror("read_note");
        return -1;
    }
    if (read_note(fd, &times_read, log_note) < 0) {
        perror("read_note");
        return -1;
    }

    close(fd);

    return 0;
}

static void log_note(void* context, char const* message)
{
    int* called = context;
    *called += 1;
    printf("%d: %s\n", *called, message);
}

static int write_note(int fd, char const* message)
{
    if (write(fd, message, strlen(message)) < 0)
        return -1;
    return 0;
}

static int read_note(int fd, void* context, void(*callback)(void*, char const*))
{
    struct stat st;
    if (fstat(fd, &st) < 0)
        return -1;
    size_t size = st.st_size;
    void* data = calloc(1, size + 1);
    if (!data)
        goto fi_0;
    if (read(fd, data, size) < 0)
        goto fi_1;

    callback(context, data);
    free(data);
    return 0;

fi_1: free(data);
fi_0: return -1;
}
