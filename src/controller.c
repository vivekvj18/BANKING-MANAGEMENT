// src/controller.c
#include "controller.h"
#include "model.h"  
#include "utils.h"  

// --- Include all the new role-specific controllers ---
#include "admin.h"
#include "manager.h"
#include "employee.h"
#include "customer.h"

// --- Session Management Globals ---
#define MAX_SESSIONS 10000 
int activeUserIds[MAX_SESSIONS];
int activeUserCount = 0;
pthread_mutex_t sessionMutex = PTHREAD_MUTEX_INITIALIZER;


// --- Main Client Handler (The "Router") ---
void* handle_client(void* client_socket_ptr) {
    int client_socket = *(int*)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[MAX_BUFFER];
    User user;
    user.userId = -1; 
    int roleChoice = 0;
    int loginSuccess = 0; 

    write_string(client_socket, "\n\n        🏦  Welcome to the Bank  🏦\n");
    write_string(client_socket, "-----------------------------------------\n");
    write_string(client_socket, "   Please select your role to log in:\n");
    write_string(client_socket, "-----------------------------------------\n");
    write_string(client_socket, "   1. Administrator\n");
    write_string(client_socket, "   2. Manager\n");
    write_string(client_socket, "   3. Employee\n");
    write_string(client_socket, "   4. Customer\n");
    write_string(client_socket, "-----------------------------------------\n");

    while(1) {
        
        write_string(client_socket, "Enter choice (1-4): ");
        read_client_input(client_socket, buffer, MAX_BUFFER);
        roleChoice = atoi(buffer);
        if (roleChoice >= 1 && roleChoice <= 4) break;
        else write_string(client_socket, "Invalid choice. Please try again.\n");
    }

    int userIdInput;
    char password[50];
    write_string(client_socket, "Enter User ID: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    userIdInput = atoi(buffer);
    write_string(client_socket, "Enter Password: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(password, buffer);

    // --- Authentication (from model.c) ---
    user = check_login(userIdInput, password);

    // --- Verification Logic ---
    if (user.userId <= 0) { 
        if (user.userId == -2) {
            write_string(STDOUT_FILENO, "Login failed (deactivated)\n");
            write_string(client_socket, "Account is deactivated. Contact your bank.\n");
        } else {
            write_string(STDOUT_FILENO, "Login failed (invalid credentials)\n");
            write_string(client_socket, "Invalid User ID or Password.\n");
        }
    } else {
        // --- Role Mismatch Check ---
        UserRole selectedRole;
        switch(roleChoice) {
            case 1: selectedRole = ADMINISTRATOR; break;
            case 2: selectedRole = MANAGER; break;
            case 3: selectedRole = EMPLOYEE; break;
            case 4: selectedRole = CUSTOMER; break;
        }

        if (user.role != selectedRole) {
            write_string(STDOUT_FILENO, "Login failed: Role mismatch.\n");
            write_string(client_socket, "Login failed: Your User ID does not match the selected role.\n");
            user.userId = 0; // Invalidate user
        }
    } 

    // --- Session Management ---
    if (user.userId > 0) {
        pthread_mutex_lock(&sessionMutex);
        int alreadyLoggedIn = 0;
        for (int i = 0; i < activeUserCount; i++) {
            if (activeUserIds[i] == user.userId) {
                alreadyLoggedIn = 1;
                break;
            }
        }

        if (alreadyLoggedIn) {
            pthread_mutex_unlock(&sessionMutex);
            write_string(STDOUT_FILENO, "Login failed: User already logged in.\n");
            write_string(client_socket, "ERROR: This user is already logged in elsewhere.\n");
            user.userId = 0; 
        } else if (activeUserCount >= MAX_SESSIONS) {
            pthread_mutex_unlock(&sessionMutex);
            write_string(STDOUT_FILENO, "Login failed: Server full.\n");
            write_string(client_socket, "ERROR: Server is currently full. Please try again later.\n");
            user.userId = 0; 
        } else {
            // --- Success ---
            activeUserIds[activeUserCount] = user.userId;
            activeUserCount++;
            pthread_mutex_unlock(&sessionMutex);
            loginSuccess = 1; 

            write_string(STDOUT_FILENO, "Login success, session added.\n");
            write_string(client_socket, "Login Successful!\n"); 

            // --- ROUTING LOGIC ---
            // Route the client to the correct menu
            switch (user.role) {
                case CUSTOMER: customer_menu(client_socket, user); break;
                case EMPLOYEE: employee_menu(client_socket, user); break;
                case MANAGER: manager_menu(client_socket, user); break;
                case ADMINISTRATOR: admin_menu(client_socket, user); break;
            }
        }
    }

    // --- Cleanup ---
    if (loginSuccess == 1) {
        pthread_mutex_lock(&sessionMutex);
        for (int i = 0; i < activeUserCount; i++) {
            if (activeUserIds[i] == user.userId) {
                activeUserIds[i] = activeUserIds[activeUserCount - 1];
                activeUserCount--;
                write_string(STDOUT_FILENO, "Session removed.\n");
                break;
            }
        }
        pthread_mutex_unlock(&sessionMutex);
    }

    close(client_socket);
    write_string(STDOUT_FILENO, "Client session ended.\n");
    return NULL;
}