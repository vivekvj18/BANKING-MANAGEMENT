// include/utils.h
#ifndef UTILS_H
#define UTILS_H

#include "common.h"

// --- I/O and String Functions ---
void write_string(int fd, const char* str);
int my_strcmp(const char* s1, const char* s2);
void read_client_input(int client_socket, char* buffer, int size);

// --- Locking Functions ---
int set_file_lock(int fd, int lock_type);
int set_record_lock(int fd, int record_num, int record_size, int lock_type);

#endif // UTILS_H