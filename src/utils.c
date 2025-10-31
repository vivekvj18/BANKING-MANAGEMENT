// src/utils.c
#include "utils.h"

// --- I/O and String Functions ---

void write_string(int fd, const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    write(fd, str, len);
}

int my_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// --- FIX: Returns the read_size to detect disconnects (0) or errors (-1)
int read_client_input(int client_socket, char* buffer, int size) {
    int read_size = read(client_socket, buffer, size - 1);
    if (read_size > 0) {
        buffer[read_size] = '\0';
        if (buffer[read_size - 1] == '\n') {
            buffer[read_size - 1] = '\0';
        }
    }
    // Return the status of the read operation
    return read_size;
}

// --- Locking Functions ---
// (Unchanged)
int set_file_lock(int fd, int lock_type) {
    struct flock fl;
    fl.l_type = lock_type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl file lock");
        return -1;
    }
    return 0;
}

int set_record_lock(int fd, int record_num, int record_size, int lock_type) {
    struct flock fl;
    fl.l_type = lock_type;
    fl.l_whence = SEEK_SET;
    fl.l_start = record_num * record_size;
    fl.l_len = record_size;
    fl.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl record lock");
        return -1;
    }
    return 0;
}