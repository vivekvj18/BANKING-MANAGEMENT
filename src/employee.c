// src/employee.c
#include "employee.h"
#include "model.h"
#include "utils.h"
#include "shared.h" // For shared functions

// --- Private Employee Handlers ---

static void handle_view_customer_transactions(int client_socket) {
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter Customer User ID: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    
    int rec_num = find_account_record_by_id(atoi(buffer));
    if (rec_num == -1) {
        write_string(client_socket, "Account not found for that User ID.\n"); return;
    }
    
    int fd = open(ACCOUNT_FILE, O_RDONLY);
    set_record_lock(fd, rec_num, sizeof(Account), F_RDLCK);
    lseek(fd, rec_num * sizeof(Account), SEEK_SET);
    Account account;
    read(fd, &account, sizeof(Account));
    set_record_lock(fd, rec_num, sizeof(Account), F_UNLCK);
    close(fd);
    
    // This function is in customer.c, we can't call it.
    // We must duplicate the logic.
    // In a future step, this could also be moved to shared.c
    int txn_fd = open(TRANSACTION_FILE, O_RDONLY);
    if (txn_fd == -1) { write_string(client_socket, "No transactions found.\n"); return; }

    set_file_lock(txn_fd, F_RDLCK);
    Transaction txn;
    int found = 0;
    
    write_string(client_socket, "\n--- Transaction History ---\n");
    sprintf(buffer, "%-7s | %-15s | %-12s | %-15s | %-15s\n", 
            "TXN ID", "TYPE", "RECIVER ACC", "AMOUNT", "BALANCE");
    write_string(client_socket, buffer);
    write_string(client_socket, "--------------------------------------------------------------------------\n");

    while (read(txn_fd, &txn, sizeof(Transaction)) == sizeof(Transaction)) {
        if (txn.accountId == account.accountId) { 
            found = 1;
            char type_str[16], other_user_str[20], amount_str[16], balance_str[16];
            // (Same switch case as in customer.c)
            // ... (omitted for brevity, but you would paste it here) ...
            sprintf(buffer, "TXN: %d, Type: %d, Amt: %.2f\n", txn.transactionId, txn.type, txn.amount);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(txn_fd, F_UNLCK); close(txn_fd);
    if (!found) { write_string(client_socket, "No transactions found for this account.\n"); }
}

static void handle_process_loan(int client_socket, int employeeId) {
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter Loan ID to process: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int loanId = atoi(buffer);
    int rec_num = find_loan_record(loanId);
    if (rec_num == -1) { write_string(client_socket, "Loan ID not found.\n"); return; }
    int fd = open(LOAN_FILE, O_RDWR);
    if (fd == -1) { write_string(client_socket, "Error accessing loan data.\n"); return; }
    set_record_lock(fd, rec_num, sizeof(Loan), F_WRLCK);
    lseek(fd, rec_num * sizeof(Loan), SEEK_SET);
    Loan loan;
    read(fd, &loan, sizeof(Loan));
    if (loan.assignedToEmployeeId != employeeId) {
        write_string(client_socket, "This loan is not assigned to you.\n");
    } else if (loan.status != PENDING && loan.status != PROCESSING) {
        write_string(client_socket, "This loan has already been processed.\n");
    } else {
        write_string(client_socket, "Choose action: 1 = Approve, 2 = Reject: ");
        read_client_input(client_socket, buffer, MAX_BUFFER);
        int choice = atoi(buffer);
        if (choice == 1) {
            loan.status = APPROVED;
            int account_rec_num = find_account_record_by_id(loan.accountIdToDeposit);
            if (account_rec_num == -1) {
                write_string(client_socket, "Loan approved, but customer account not found!\n");
            } else {
                int fd_acct = open(ACCOUNT_FILE, O_RDWR);
                set_record_lock(fd_acct, account_rec_num, sizeof(Account), F_WRLCK);
                Account account;
                lseek(fd_acct, account_rec_num * sizeof(Account), SEEK_SET);
                read(fd_acct, &account, sizeof(Account));
                account.balance += loan.amount;
                lseek(fd_acct, account_rec_num * sizeof(Account), SEEK_SET);
                write(fd_acct, &account, sizeof(Account));
                set_record_lock(fd_acct, account_rec_num, sizeof(Account), F_UNLCK);
                close(fd_acct);
                log_transaction(account.accountId, account.ownerUserId, DEPOSIT, loan.amount, account.balance, "LOAN_CREDIT");
                write_string(client_socket, "Loan approved. Amount credited to customer account.\n");
            }
        } else if (choice == 2) {
            loan.status = REJECTED;
            write_string(client_socket, "Loan rejected.\n");
        } else {
            write_string(client_socket, "Invalid choice. No action taken.\n");
        }
        lseek(fd, rec_num * sizeof(Loan), SEEK_SET);
        write(fd, &loan, sizeof(Loan));
    }
    set_record_lock(fd, rec_num, sizeof(Loan), F_UNLCK);
    close(fd);
}

static void handle_view_assigned_loans(int client_socket, int employeeId) {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) {
        write_string(client_socket, "No loans found.\n");
        return;
    }

    set_file_lock(fd, F_RDLCK);
    Loan loan;
    char buffer[256];
    int found = 0;
    
    write_string(client_socket, "\n--- Your Assigned Loans ---\n");
    lseek(fd, 0, SEEK_SET); 
    while (read(fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.assignedToEmployeeId == employeeId && (loan.status == PENDING || loan.status == PROCESSING)) {
            found = 1;
            char* status_str = (loan.status == PENDING) ? "PENDING" : "PROCESSING";
            sprintf(buffer, "Loan ID: %d | Customer ID: %d | Amount: ₹%.2f | Status: %s\n",
                loan.loanId, loan.userId, loan.amount, status_str);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);

    if (!found) {
        write_string(client_socket, "No assigned loans found.\n");
    }
}

// --- Public Employee Menu ---

void employee_menu(int client_socket, User user) {
    char buffer[MAX_BUFFER];
    char welcome_msg[150]; 
    sprintf(welcome_msg, "    Employee: %s %s (ID: %d)\n", user.firstName, user.lastName, user.userId);

    while(1) {
        write_string(client_socket, "\n\n+---------------------------------------+\n");
        write_string(client_socket, "       👨‍💼  Employee Portal  👨‍💼\n");
        write_string(client_socket, "+---------------------------------------+\n");
        write_string(client_socket, welcome_msg);
        write_string(client_socket, "-----------------------------------------\n");
        
        write_string(client_socket, "1. Add New Customer\n");
        write_string(client_socket, "2. Modify Customer Details\n");
        write_string(client_socket, "3. View Customer Transactions\n");
        write_string(client_socket, "4. View Assigned Loans\n");
        write_string(client_socket, "5. Process Loan Application\n");
        write_string(client_socket, "6. View My Personal Details\n");
        write_string(client_socket, "7. Change My Password\n");
        write_string(client_socket, "8. Logout\n");
        write_string(client_socket, "Enter your choice: ");

        read_client_input(client_socket, buffer, MAX_BUFFER);
        int choice = atoi(buffer);
        switch(choice) {
            case 1: handle_add_user(client_socket, CUSTOMER); break;
            case 2: handle_modify_user_details(client_socket, 0); break;
            case 3: handle_view_customer_transactions(client_socket); break;
            case 4: handle_view_assigned_loans(client_socket, user.userId); break;
            case 5: handle_process_loan(client_socket, user.userId); break;
            case 6: handle_view_my_details(client_socket, user); break;
            case 7: handle_change_password(client_socket, user.userId); break;
            case 8: write_string(client_socket, "Logging out. Goodbye!\n"); return;
            default: write_string(client_socket, "Invalid choice.\n");
        }
    }
}