// src/shared.c
#include "shared.h"
#include "model.h"
#include "utils.h"

// --- FIX: NEW VALIDATION HELPERS ---

// Helper to check for '@' and '.'
int is_email_valid(const char* email) {
    if (strstr(email, "@") != NULL && strstr(email, ".") != NULL) {
        return 1;
    }
    return 0;
}

// Helper to check if email is unique (Fixes Uniqueness Validation)
int is_email_unique(const char* email) {
    int fd = open(USER_FILE, O_RDONLY);
    if (fd == -1) return 1; // File doesn't exist, so it's unique
    
    set_file_lock(fd, F_RDLCK);
    User user;
    while(read(fd, &user, sizeof(User)) == sizeof(User)) {
        if (my_strcmp(user.email, email) == 0) {
            set_file_lock(fd, F_UNLCK);
            close(fd);
            return 0; // Found a duplicate
        }
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);
    return 1; // No duplicates
}

// Helper to read a string and validate it
// Fixes: Empty Input & Buffer Overflow
int get_valid_string(int client_socket, char* dest, int max_len) {
    char buffer[MAX_BUFFER];
    while(1) {
        int read_status = read_client_input(client_socket, buffer, MAX_BUFFER);
        if (read_status <= 0) {
            strcpy(dest, ""); // Set dest to empty on disconnect
            return -1; // Client disconnected
        }

        if (my_strcmp(buffer, "") == 0) {
            write_string(client_socket, "Input cannot be empty. Try again: ");
            continue;
        }

        if (strlen(buffer) >= max_len) {
            char msg[100];
            sprintf(msg, "Input is too long (max %d chars). Try again: ", max_len - 1);
            write_string(client_socket, msg);
            continue;
        }

        // It's valid
        strcpy(dest, buffer);
        return 0;
    }
}

// Helper for email (adds format + uniqueness checks)
int get_valid_email(int client_socket, char* dest, int max_len) {
    while(1) {
        if (get_valid_string(client_socket, dest, max_len) == -1) return -1; // Disconnected

        if (!is_email_valid(dest)) {
            write_string(client_socket, "Invalid email format. Try again: ");
            continue;
        }

        if (!is_email_unique(dest)) {
            write_string(client_socket, "Email is already in use. Try again: ");
            continue;
        }

        // It's valid and unique
        return 0;
    }
}

// --- Shared Handler Functions ---

void handle_view_my_details(int client_socket, User user) {
    char buffer[512];
    write_string(client_socket, "\n--- Your Personal Details ---\n");
    sprintf(buffer, "User ID: %d\n", user.userId); write_string(client_socket, buffer);
    sprintf(buffer, "Name: %s %s\n", user.firstName, user.lastName); write_string(client_socket, buffer);
    sprintf(buffer, "Phone: %s\n", user.phone); write_string(client_socket, buffer);
    sprintf(buffer, "Email: %s\n", user.email); write_string(client_socket, buffer);
    sprintf(buffer, "Address: %s\n", user.address); write_string(client_socket, buffer);
    write_string(client_socket, "------------------------------\n");
}

void handle_change_password(int client_socket, int userId) {
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter new password: ");
    
    // --- FIX: Use get_valid_string to check for empty/long passwords ---
    if (get_valid_string(client_socket, buffer, 50) == -1) return; // Disconnected
    
    int record_num = find_user_record(userId);
    if (record_num == -1) { write_string(client_socket, "Error: User not found.\n"); return; }
    
    int fd = open(USER_FILE, O_RDWR);
    if (fd == -1) { write_string(client_socket, "Error: Could not access user data.\n"); return; }
    
    set_record_lock(fd, record_num, sizeof(User), F_WRLCK);
    User user;
    lseek(fd, record_num * sizeof(User), SEEK_SET);

    // --- FIX: Check read() failure ---
    if (read(fd, &user, sizeof(User)) != sizeof(User)) {
        write_string(client_socket, "Error: Failed to read user record.\n");
        set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
        close(fd);
        return;
    }

    strcpy(user.password, buffer);
    lseek(fd, record_num * sizeof(User), SEEK_SET);

    // --- FIX: Check write() failure ---
    if (write(fd, &user, sizeof(User)) != sizeof(User)) {
        write_string(STDOUT_FILENO, "FATAL: Failed to write password to disk.\n");
    }
    
    set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
    close(fd);
    write_string(client_socket, "Password changed successfully.\n");
}

void handle_add_user(int client_socket, UserRole role_to_add) {
    char buffer[MAX_BUFFER]; // Temp buffer
    User new_user;
    new_user.role = role_to_add;
    new_user.isActive = 1;

    // --- FIX: Lock file *before* getting ID to prevent race condition ---
    int fd_user = open(USER_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd_user == -1) { 
        perror("Error opening user file");
        write_string(client_socket, "Error opening user file.\n"); 
        return; 
    }
    set_file_lock(fd_user, F_WRLCK); // Lock the *entire file*
    
    new_user.userId = get_next_user_id();

    write_string(client_socket, "Enter new user's password: ");
    if (get_valid_string(client_socket, new_user.password, 50) == -1) {
        set_file_lock(fd_user, F_UNLCK); close(fd_user); return; // Disconnected
    }
    
    write_string(client_socket, "Enter user's First Name: ");
    if (get_valid_string(client_socket, new_user.firstName, 50) == -1) {
        set_file_lock(fd_user, F_UNLCK); close(fd_user); return;
    }
    
    write_string(client_socket, "Enter user's Last Name: ");
    if (get_valid_string(client_socket, new_user.lastName, 50) == -1) {
        set_file_lock(fd_user, F_UNLCK); close(fd_user); return;
    }
    
    write_string(client_socket, "Enter user's Phone: ");
    if (get_valid_string(client_socket, new_user.phone, 15) == -1) {
        set_file_lock(fd_user, F_UNLCK); close(fd_user); return;
    }
    
    write_string(client_socket, "Enter user's Email: ");
    if (get_valid_email(client_socket, new_user.email, 100) == -1) {
        set_file_lock(fd_user, F_UNLCK); close(fd_user); return;
    }
    
    write_string(client_socket, "Enter user's Address: ");
    if (get_valid_string(client_socket, new_user.address, 256) == -1) {
        set_file_lock(fd_user, F_UNLCK); close(fd_user); return;
    }

    if (write(fd_user, &new_user, sizeof(User)) != sizeof(User)) {
        write_string(client_socket, "FATAL: Failed to write new user to disk.\n");
    }
    set_file_lock(fd_user, F_UNLCK); // Release the lock
    close(fd_user);

    if (role_to_add == CUSTOMER) {
        Account new_account;
        new_account.accountId = new_user.userId; 
        new_account.ownerUserId = new_user.userId;
        new_account.balance = 0.0;
        new_account.isActive = 1;
        sprintf(new_account.accountNumber, "SB-%d", new_user.userId); 

        int fd_acct = open(ACCOUNT_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd_acct == -1) { write_string(client_socket, "Error opening account file.\n"); return; }
        
        set_file_lock(fd_acct, F_WRLCK);
        if (write(fd_acct, &new_account, sizeof(Account)) != sizeof(Account)) {
             write_string(client_socket, "FATAL: Failed to write new account to disk.\n");
        }
        set_file_lock(fd_acct, F_UNLCK);
        close(fd_acct);
        
        sprintf(buffer, "User created successfully. New User ID: %d, Account No: SB-%d\n", new_user.userId, new_user.userId);
        write_string(client_socket, buffer);
    } else {
        sprintf(buffer, "User created successfully. New User ID: %d\n", new_user.userId);
        write_string(client_socket, buffer);
    }
}

void handle_modify_user_details(int client_socket, int admin_mode) {
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter User ID to modify: ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) return;
    
    int target_user_id = atoi(buffer);
    if(target_user_id <= 0) { write_string(client_socket, "Invalid User ID.\n"); return; }
    
    int record_num = find_user_record(target_user_id);
    if (record_num == -1) { write_string(client_socket, "User not found.\n"); return; }

    int fd = open(USER_FILE, O_RDWR);
    if (fd == -1) { write_string(client_socket, "Error accessing user data.\n"); return; }
    
    set_record_lock(fd, record_num, sizeof(User), F_WRLCK);
    lseek(fd, record_num * sizeof(User), SEEK_SET);
    User user;
    
    if (read(fd, &user, sizeof(User)) != sizeof(User)) {
        write_string(client_socket, "Error: Failed to read user record.\n");
        set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
        close(fd);
        return;
    }

    if (!admin_mode && user.role != CUSTOMER) {
        write_string(client_socket, "Permission denied. Employees can only modify customers.\n");
        set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return;
    }

    write_string(client_socket, "Enter new password (or 'skip'): ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
    if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
        if(strlen(buffer) >= 50) { write_string(client_socket, "Password too long. Skipped.\n"); }
        else { strcpy(user.password, buffer); }
    }
    
    write_string(client_socket, "Enter new First Name (or 'skip'): ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
    if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
        if(strlen(buffer) >= 50) { write_string(client_socket, "Name too long. Skipped.\n"); }
        else { strcpy(user.firstName, buffer); }
    }
    
    write_string(client_socket, "Enter new Last Name (or 'skip'): ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
    if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
        if(strlen(buffer) >= 50) { write_string(client_socket, "Name too long. Skipped.\n"); }
        else { strcpy(user.lastName, buffer); }
    }
    
    write_string(client_socket, "Enter new Phone (or 'skip'): ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
    if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
        if(strlen(buffer) >= 15) { write_string(client_socket, "Phone too long. Skipped.\n"); }
        else { strcpy(user.phone, buffer); }
    }
    
    write_string(client_socket, "Enter new Email (or 'skip'): ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
    if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
        if(strlen(buffer) >= 100) { write_string(client_socket, "Email too long. Skipped.\n"); }
        else if (!is_email_valid(buffer)) { write_string(client_socket, "Invalid email format. Skipped.\n"); }
        else if (!is_email_unique(buffer)) { write_string(client_socket, "Email already in use. Skipped.\n"); }
        else { strcpy(user.email, buffer); }
    }
    
    write_string(client_socket, "Enter new Address (or 'skip'): ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
    if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
        if(strlen(buffer) >= 256) { write_string(client_socket, "Address too long. Skipped.\n"); }
        else { strcpy(user.address, buffer); }
    }

    if (admin_mode) {
        write_string(client_socket, "Enter new role (0=CUST, 1=EMP, 2=MAN, 3=ADMIN) (or 'skip'): ");
        if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) { set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return; }
        if (my_strcmp(buffer, "skip") != 0 && my_strcmp(buffer, "") != 0) {
            int new_role = atoi(buffer);
            if(new_role >= 0 && new_role <= 3) { user.role = new_role; }
            else { write_string(client_socket, "Invalid role. Skipped.\n"); }
        }
    }

    lseek(fd, record_num * sizeof(User), SEEK_SET);
    if (write(fd, &user, sizeof(User)) != sizeof(User)) {
        write_string(STDOUT_FILENO, "FATAL: Failed to write modified user to disk.\n");
    }
    set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
    close(fd);
    write_string(client_socket, "User details modified successfully.\n");
}

void handle_set_account_status(int client_socket, int admin_mode) {
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter User ID to modify status: ");
    if(read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) return;
    int target_user_id = atoi(buffer);
    if(target_user_id <= 0) { write_string(client_socket, "Invalid User ID.\n"); return; }

    write_string(client_socket, "Enter status (1=Active, 0=Deactivated): ");
    if(read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) return;
    int new_status = atoi(buffer);
    if (new_status != 0 && new_status != 1) {
        write_string(client_socket, "Invalid status. Must be 0 or 1.\n");
        return;
    }

    int user_rec_num = find_user_record(target_user_id);
    if (user_rec_num == -1) { write_string(client_socket, "User not found.\n"); return; }
    
    int fd_user = open(USER_FILE, O_RDWR);
    if(fd_user == -1) { write_string(client_socket, "Error accessing user data.\n"); return; }

    set_record_lock(fd_user, user_rec_num, sizeof(User), F_WRLCK);
    lseek(fd_user, user_rec_num * sizeof(User), SEEK_SET);
    User user; 
    
    if(read(fd_user, &user, sizeof(User)) != sizeof(User)) {
        write_string(client_socket, "Error reading user record.\n");
        set_record_lock(fd_user, user_rec_num, sizeof(User), F_UNLCK);
        close(fd_user);
        return;
    }

    if (!admin_mode && user.role != CUSTOMER) {
        write_string(client_socket, "Permission denied. Managers can only modify customers.\n");
        set_record_lock(fd_user, user_rec_num, sizeof(User), F_UNLCK); close(fd_user); return;
    }
    user.isActive = new_status;
    lseek(fd_user, user_rec_num * sizeof(User), SEEK_SET);

    if(write(fd_user, &user, sizeof(User)) != sizeof(User)) {
        write_string(STDOUT_FILENO, "FATAL: Failed to write user status.\n");
    }
    set_record_lock(fd_user, user_rec_num, sizeof(User), F_UNLCK);
    close(fd_user);

    int acct_rec_num = find_account_record_by_id(target_user_id);
    if (acct_rec_num != -1) {
        int fd_acct = open(ACCOUNT_FILE, O_RDWR);
        if (fd_acct == -1) { write_string(client_socket, "User status updated, but couldn't open account file.\n"); return; }
        
        set_record_lock(fd_acct, acct_rec_num, sizeof(Account), F_WRLCK); 
        Account account;
        lseek(fd_acct, acct_rec_num * sizeof(Account), SEEK_SET); 

        if(read(fd_acct, &account, sizeof(Account)) != sizeof(Account)) {
            write_string(client_socket, "Error reading account record.\n");
        } else {
            account.isActive = new_status;
            lseek(fd_acct, acct_rec_num * sizeof(Account), SEEK_SET);
            if(write(fd_acct, &account, sizeof(Account)) != sizeof(Account)) {
                write_string(STDOUT_FILENO, "FATAL: Failed to write account status.\n");
            }
        }
        set_record_lock(fd_acct, acct_rec_num, sizeof(Account), F_UNLCK);
        close(fd_acct);
    }
    
    write_string(client_socket, "User and their account updated successfully.\n");
}