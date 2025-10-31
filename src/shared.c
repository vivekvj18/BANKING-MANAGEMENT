// src/shared.c
#include "shared.h"
#include "model.h"
#include "utils.h"

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
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int record_num = find_user_record(userId);
    if (record_num == -1) { write_string(client_socket, "Error: User not found.\n"); return; }
    int fd = open(USER_FILE, O_RDWR);
    if (fd == -1) { write_string(client_socket, "Error: Could not access user data.\n"); return; }
    set_record_lock(fd, record_num, sizeof(User), F_WRLCK);
    User user;
    lseek(fd, record_num * sizeof(User), SEEK_SET);
    read(fd, &user, sizeof(User));
    strcpy(user.password, buffer);
    lseek(fd, record_num * sizeof(User), SEEK_SET);
    write(fd, &user, sizeof(User));
    set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
    close(fd);
    write_string(client_socket, "Password changed successfully.\n");
}

void handle_add_user(int client_socket, UserRole role_to_add) {
    char buffer[MAX_BUFFER];
    User new_user;
    new_user.userId = get_next_user_id();
    new_user.role = role_to_add;
    new_user.isActive = 1;

    write_string(client_socket, "Enter new user's password: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(new_user.password, buffer);
    
    write_string(client_socket, "Enter user's First Name: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(new_user.firstName, buffer);
    
    write_string(client_socket, "Enter user's Last Name: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(new_user.lastName, buffer);
    
    write_string(client_socket, "Enter user's Phone: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(new_user.phone, buffer);
    
    write_string(client_socket, "Enter user's Email: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(new_user.email, buffer);
    
    write_string(client_socket, "Enter user's Address: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    strcpy(new_user.address, buffer);

    int fd_user = open(USER_FILE, O_WRONLY | O_APPEND);
    if (fd_user == -1) { write_string(client_socket, "Error opening user file.\n"); return; }
    set_file_lock(fd_user, F_WRLCK);
    write(fd_user, &new_user, sizeof(User));
    set_file_lock(fd_user, F_UNLCK);
    close(fd_user);

    if (role_to_add == CUSTOMER) {
        Account new_account;
        new_account.accountId = new_user.userId; 
        new_account.ownerUserId = new_user.userId;
        new_account.balance = 0.0;
        new_account.isActive = 1;
        sprintf(new_account.accountNumber, "SB-%d", new_user.userId); 

        int fd_acct = open(ACCOUNT_FILE, O_WRONLY | O_APPEND);
        if (fd_acct == -1) { write_string(client_socket, "Error opening account file.\n"); return; }
        set_file_lock(fd_acct, F_WRLCK);
        write(fd_acct, &new_account, sizeof(Account));
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
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int target_user_id = atoi(buffer);
    int record_num = find_user_record(target_user_id);
    if (record_num == -1) { write_string(client_socket, "User not found.\n"); return; }

    int fd = open(USER_FILE, O_RDWR);
    set_record_lock(fd, record_num, sizeof(User), F_WRLCK);
    lseek(fd, record_num * sizeof(User), SEEK_SET);
    User user;
    read(fd, &user, sizeof(User));

    if (!admin_mode && user.role != CUSTOMER) {
        write_string(client_socket, "Permission denied. Employees can only modify customers.\n");
        set_record_lock(fd, record_num, sizeof(User), F_UNLCK); close(fd); return;
    }

    write_string(client_socket, "Enter new password (or 'skip'): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    if (my_strcmp(buffer, "skip") != 0) { strcpy(user.password, buffer); }
    
    write_string(client_socket, "Enter new First Name (or 'skip'): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    if (my_strcmp(buffer, "skip") != 0) { strcpy(user.firstName, buffer); }
    
    write_string(client_socket, "Enter new Last Name (or 'skip'): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    if (my_strcmp(buffer, "skip") != 0) { strcpy(user.lastName, buffer); }
    
    write_string(client_socket, "Enter new Phone (or 'skip'): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    if (my_strcmp(buffer, "skip") != 0) { strcpy(user.phone, buffer); }
    
    write_string(client_socket, "Enter new Email (or 'skip'): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    if (my_strcmp(buffer, "skip") != 0) { strcpy(user.email, buffer); }
    
    write_string(client_socket, "Enter new Address (or 'skip'): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    if (my_strcmp(buffer, "skip") != 0) { strcpy(user.address, buffer); }

    if (admin_mode) {
        write_string(client_socket, "Enter new role (0=CUST, 1=EMP, 2=MAN, 3=ADMIN) (or 'skip'): ");
        read_client_input(client_socket, buffer, MAX_BUFFER);
        if (my_strcmp(buffer, "skip") != 0) { user.role = atoi(buffer); }
    }

    lseek(fd, record_num * sizeof(User), SEEK_SET);
    write(fd, &user, sizeof(User));
    set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
    close(fd);
    write_string(client_socket, "User details modified successfully.\n");
}

void handle_set_account_status(int client_socket, int admin_mode) {
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter User ID to modify status: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int target_user_id = atoi(buffer);
    write_string(client_socket, "Enter status (1=Active, 0=Deactivated): ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int new_status = atoi(buffer);

    int user_rec_num = find_user_record(target_user_id);
    if (user_rec_num == -1) { write_string(client_socket, "User not found.\n"); return; }
    
    int fd_user = open(USER_FILE, O_RDWR);
    set_record_lock(fd_user, user_rec_num, sizeof(User), F_WRLCK);
    lseek(fd_user, user_rec_num * sizeof(User), SEEK_SET);
    User user; read(fd_user, &user, sizeof(User));
    if (!admin_mode && user.role != CUSTOMER) {
        write_string(client_socket, "Permission denied. Managers can only modify customers.\n");
        set_record_lock(fd_user, user_rec_num, sizeof(User), F_UNLCK); close(fd_user); return;
    }
    user.isActive = new_status;
    lseek(fd_user, user_rec_num * sizeof(User), SEEK_SET);
    write(fd_user, &user, sizeof(User));
    set_record_lock(fd_user, user_rec_num, sizeof(User), F_UNLCK);
    close(fd_user);

    int acct_rec_num = find_account_record_by_id(target_user_id);
    if (acct_rec_num != -1) {
        int fd_acct = open(ACCOUNT_FILE, O_RDWR);
        if (fd_acct == -1) { write_string(client_socket, "User status updated, but couldn't open account file.\n"); return; }
        
        set_record_lock(fd_acct, acct_rec_num, sizeof(Account), F_WRLCK); 
        Account account;
        lseek(fd_acct, acct_rec_num * sizeof(Account), SEEK_SET); 
        read(fd_acct, &account, sizeof(Account));
        account.isActive = new_status;
        lseek(fd_acct, acct_rec_num * sizeof(Account), SEEK_SET);
        write(fd_acct, &account, sizeof(Account));
        set_record_lock(fd_acct, acct_rec_num, sizeof(Account), F_UNLCK);
        close(fd_acct);
    }
    
    write_string(client_socket, "User and their account updated successfully.\n");
}