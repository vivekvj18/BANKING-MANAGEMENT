// include/controller.h
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "common.h"

// --- Main Client Handler (The "Router") ---
void* handle_client(void* client_socket_ptr);

#endif // CONTROLLER_H