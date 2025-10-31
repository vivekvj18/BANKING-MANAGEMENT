// src/admin.c
#include "admin.h"
#include "model.h"
#include "utils.h"
#include "shared.h" // For shared functions

// --- Public Admin Menu ---

void admin_menu(int client_socket, User user) {
    char buffer[MAX_BUFFER];
    char welcome_msg[100];
    sprintf(welcome_msg, "    Administrator: %s (ID: %d)\n", user.firstName, user.userId);
    while(1) {
        write_string(client_socket, "\n\n+---------------------------------------+\n");
        write_string(client_socket, "     🛡️  System Administration  🛡️\n");
        write_string(client_socket, "+---------------------------------------+\n");
        write_string(client_socket, welcome_msg);
        write_string(client_socket, "-----------------------------------------\n");
        
        write_string(client_socket, "1. Add User (Employee/Manager/Customer)\n");
        write_string(client_socket, "2. Modify User Details (Password/Role/KYC)\n");
        write_string(client_socket, "3. Activate/Deactivate Any User & Account\n"); 
        write_string(client_socket, "4. View My Personal Details\n");
        write_string(client_socket, "5. Change My Password\n");
        write_string(client_socket, "6. Logout\n");
        write_string(client_socket, "+---------------------------------------+\n");
        write_string(client_socket, "Enter your choice: ");
        
        // --- FIX: Check for disconnect or empty input ---
        int read_status = read_client_input(client_socket, buffer, MAX_BUFFER);
        if (read_status <= 0) {
            write_string(STDOUT_FILENO, "Client disconnected.\n");
            return; // Exit menu
        }
        if (my_strcmp(buffer, "") == 0) {
            write_string(client_socket, "Invalid choice. Please enter a number.\n");
            continue; // Re-show menu
        }
        // --- END FIX ---
        
        int choice = atoi(buffer);
        switch(choice) {
            case 1: 
                write_string(client_socket, "Enter role (0=CUST, 1=EMP, 2=MAN): ");
                // --- FIX: Add check for role input ---
                if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) break;
                if(my_strcmp(buffer, "") == 0) continue;
                
                int role = atoi(buffer);
                if (role >= 0 && role <= 2) handle_add_user(client_socket, (UserRole)role);
                else write_string(client_socket, "Invalid role.\n");
                break;
            case 2: handle_modify_user_details(client_socket, 1); break;
            case 3: handle_set_account_status(client_socket, 1); break;
            case 4: handle_view_my_details(client_socket, user); break;
            case 5: handle_change_password(client_socket, user.userId); break;
            case 6: write_string(client_socket, "Logging out. Goodbye!\n"); return;
            default: write_string(client_socket, "Invalid choice.\n");
        }
    }
}