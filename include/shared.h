// include/shared.h
#ifndef SHARED_H
#define SHARED_H

#include "common.h"

// --- Shared Handler Functions ---
void handle_view_my_details(int client_socket, User user);
void handle_change_password(int client_socket, int userId);
void handle_add_user(int client_socket, UserRole role_to_add);
void handle_modify_user_details(int client_socket, int admin_mode);
void handle_set_account_status(int client_socket, int admin_mode);

#endif // SHARED_H