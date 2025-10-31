// src/customer.c
#include "customer.h"
#include "model.h"
#include "utils.h"
#include "shared.h" // For shared functions

// --- FIX: NEW VALIDATION HELPER ---
static int is_valid_amount(const char *str)
{
    int dots = 0;
    if (my_strcmp(str, "") == 0)
        return 0; // Empty
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == '.')
        {
            dots++;
        }
        else if (str[i] < '0' || str[i] > '9')
        {
            return 0; // Not a digit
        }
        if (dots > 1)
            return 0; // More than one dot
    }
    return 1;
}

// --- Private Customer Handlers ---

static void handle_view_transaction_history(int client_socket, int userId)
{
    int fd = open(TRANSACTION_FILE, O_RDONLY);
    if (fd == -1)
    {
        write_string(client_socket, "No transactions found.\n");
        return;
    }

    set_file_lock(fd, F_RDLCK);
    Transaction txn;
    char buffer[256];
    int found = 0;

    write_string(client_socket, "\n--- Transaction History ---\n");
    sprintf(buffer, "%-7s | %-15s | %-12s | %-15s | %-15s\n",
            "TXN ID", "TYPE", "RECIVER ACC", "AMOUNT", "BALANCE");
    write_string(client_socket, buffer);
    write_string(client_socket, "--------------------------------------------------------------------------\n");

    while (read(fd, &txn, sizeof(Transaction)) == sizeof(Transaction))
    {
        if (txn.accountId == userId)
        {
            found = 1;
            char type_str[16], other_user_str[20], amount_str[16], balance_str[16];
            switch (txn.type)
            {
            case DEPOSIT:
                strcpy(type_str, "CREDITED");
                strcpy(other_user_str, "---");
                sprintf(amount_str, "₹%.2f", txn.amount);
                break;
            case WITHDRAWAL:
                strcpy(type_str, "DEBITED");
                strcpy(other_user_str, "---");
                sprintf(amount_str, "₹%.2f", txn.amount);
                break;
            case TRANSFER_OUT:
                strcpy(type_str, "DEBITED");
                sprintf(other_user_str, "%s", txn.otherPartyAccountNumber);
                sprintf(amount_str, "₹%.2f", txn.amount);
                break;
            case TRANSFER_IN:
                strcpy(type_str, "CREDITED");
                sprintf(other_user_str, "%s", txn.otherPartyAccountNumber);
                sprintf(amount_str, "₹%.2f", txn.amount);
                break;
            default:
                strcpy(type_str, "UNKNOWN");
                strcpy(other_user_str, "---");
                sprintf(amount_str, "₹%.2f", txn.amount);
            }
            sprintf(balance_str, "₹%.2f", txn.newBalance);
            sprintf(buffer, "%-7d | %-15s | %-12s | %-15s | %-15s\n",
                    txn.transactionId, type_str, other_user_str, amount_str, balance_str);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);
    if (!found)
    {
        write_string(client_socket, "No transactions found for this account.\n");
    }
}

static void handle_view_balance(int client_socket, int userId)
{
    int record_num = find_account_record_by_id(userId);
    if (record_num == -1)
    {
        write_string(client_socket, "Error: Account not found.\n");
        return;
    }
    int fd = open(ACCOUNT_FILE, O_RDONLY);
    if (fd == -1)
    {
        write_string(client_socket, "Error: Could not access account data.\n");
        return;
    }

    set_record_lock(fd, record_num, sizeof(Account), F_RDLCK);
    lseek(fd, record_num * sizeof(Account), SEEK_SET);
    Account account;

    // --- FIX: Check read() failure ---
    if (read(fd, &account, sizeof(Account)) != sizeof(Account))
    {
        write_string(client_socket, "Error: Could not read account data.\n");
    }
    else
    {
        char buffer[100];
        sprintf(buffer, "Balance for account %s: ₹%.2f\n", account.accountNumber, account.balance);
        write_string(client_socket, buffer);
    }

    set_record_lock(fd, record_num, sizeof(Account), F_UNLCK);
    close(fd);
}

static void handle_deposit(int client_socket, int userId)
{
    char buffer[MAX_BUFFER];
    double amount;
    write_string(client_socket, "Enter amount to deposit: ");

    // --- FIX: Check for disconnect and invalid data type ---
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0)
        return;
    if (!is_valid_amount(buffer))
    {
        write_string(client_socket, "Invalid amount. Please enter numbers only.\n");
        return;
    }
    amount = atof(buffer);
    if (amount <= 0.01)
    {
        write_string(client_socket, "Amount must be positive.\n");
        return;
    }

    int record_num = find_account_record_by_id(userId);
    if (record_num == -1)
    {
        write_string(client_socket, "Error: Account not found.\n");
        return;
    }
    int fd = open(ACCOUNT_FILE, O_RDWR);
    if (fd == -1)
    {
        write_string(client_socket, "Error: Could not access account data.\n");
        return;
    }

    set_record_lock(fd, record_num, sizeof(Account), F_WRLCK);
    Account account;
    lseek(fd, record_num * sizeof(Account), SEEK_SET);

    // --- FIX: Check read() failure ---
    if (read(fd, &account, sizeof(Account)) != sizeof(Account))
    {
        write_string(client_socket, "Error: Could not read account data.\n");
        set_record_lock(fd, record_num, sizeof(Account), F_UNLCK);
        close(fd);
        return;
    }

    account.balance += amount;
    lseek(fd, record_num * sizeof(Account), SEEK_SET);

    // --- FIX: Check write() failure ---
    if (write(fd, &account, sizeof(Account)) != sizeof(Account))
    {
        write_string(STDOUT_FILENO, "FATAL: Failed to write deposit to disk.\n");
    }

    set_record_lock(fd, record_num, sizeof(Account), F_UNLCK);
    close(fd);

    log_transaction(account.accountId, account.ownerUserId, DEPOSIT, amount, account.balance, "---");
    sprintf(buffer, "Deposit successful. New balance: ₹%.2f\n", account.balance);
    write_string(client_socket, buffer);
}

static void handle_withdraw(int client_socket, int userId)
{
    char buffer[MAX_BUFFER];
    double amount;
    write_string(client_socket, "Enter amount to withdraw: ");

    // --- FIX: Check for disconnect and invalid data type ---
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0)
        return;
    if (!is_valid_amount(buffer))
    {
        write_string(client_socket, "Invalid amount. Please enter numbers only.\n");
        return;
    }
    amount = atof(buffer);
    if (amount <= 0.01)
    {
        write_string(client_socket, "Amount must be positive.\n");
        return;
    }

    int record_num = find_account_record_by_id(userId);
    if (record_num == -1)
    {
        write_string(client_socket, "Error: Account not found.\n");
        return;
    }
    int fd = open(ACCOUNT_FILE, O_RDWR);
    if (fd == -1)
    {
        write_string(client_socket, "Error: Could not access account data.\n");
        return;
    }

    set_record_lock(fd, record_num, sizeof(Account), F_WRLCK);
    Account account;
    lseek(fd, record_num * sizeof(Account), SEEK_SET);

    // --- FIX: Check read() failure ---
    if (read(fd, &account, sizeof(Account)) != sizeof(Account))
    {
        write_string(client_socket, "Error: Could not read account data.\n");
    }
    else if (amount > account.balance)
    {
        write_string(client_socket, "Insufficient funds.\n");
    }
    else
    {
        account.balance -= amount;
        lseek(fd, record_num * sizeof(Account), SEEK_SET);

        // --- FIX: Check write() failure ---
        if (write(fd, &account, sizeof(Account)) != sizeof(Account))
        {
            write_string(STDOUT_FILENO, "FATAL: Failed to write withdrawal to disk.\n");
        }
        else
        {
            log_transaction(account.accountId, account.ownerUserId, WITHDRAWAL, amount, account.balance, "---");
            sprintf(buffer, "Withdrawal successful. New balance: ₹%.2f\n", account.balance);
            write_string(client_socket, buffer);
        }
    }
    set_record_lock(fd, record_num, sizeof(Account), F_UNLCK);
    close(fd);
}

// --- MODIFIED: handle_transfer_funds ---
static void handle_transfer_funds(int client_socket, int senderUserId) {
    char buffer[MAX_BUFFER];
    char receiver_user_id_str[20];
    double amount;

    write_string(client_socket, "Enter User ID to transfer to: ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) return;
    strcpy(receiver_user_id_str, buffer);
    int receiverUserId = atoi(receiver_user_id_str);
    if(receiverUserId <= 0) { write_string(client_socket, "Invalid User ID.\n"); return; }

    write_string(client_socket, "Enter amount to transfer: ");
    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0) return;
    if (!is_valid_amount(buffer)) {
        write_string(client_socket, "Invalid amount. Please enter numbers only.\n");
        return;
    }
    amount = atof(buffer);
    if (amount <= 0.01) { write_string(client_socket, "Amount must be positive.\n"); return; }

    int sender_rec_num = find_account_record_by_id(senderUserId);
    int receiver_rec_num = find_account_record_by_id(receiverUserId);

    if (sender_rec_num == -1) {
        write_string(client_socket, "Error: Your account could not be found.\n"); return;
    }
    if (receiver_rec_num == -1) {
        write_string(client_socket, "Error: Recipient User ID not found.\n"); return;
    }
    if (sender_rec_num == receiver_rec_num) {
        write_string(client_socket, "Cannot transfer funds to your own account.\n"); return;
    }

    // --- ADDED: Log intent *before* locking ---
    long t_id = get_next_transfer_id();
    TransferLog log_entry;
    log_entry.transferId = t_id;
    log_entry.fromAccountId = senderUserId;
    log_entry.toAccountId = receiverUserId;
    log_entry.amount = amount;
    log_entry.status = LOG_START;
    write_transfer_log(&log_entry);
    // --- END ADDED ---

    int fd = open(ACCOUNT_FILE, O_RDWR);
    if (fd == -1)
    {
        write_string(client_socket, "Error accessing account data.\n");
        return;
    }

    int rec1 = (sender_rec_num < receiver_rec_num) ? sender_rec_num : receiver_rec_num;
    int rec2 = (sender_rec_num > receiver_rec_num) ? sender_rec_num : receiver_rec_num;
    set_record_lock(fd, rec1, sizeof(Account), F_WRLCK);
    set_record_lock(fd, rec2, sizeof(Account), F_WRLCK);

    Account sender_account, receiver_account;
    lseek(fd, sender_rec_num * sizeof(Account), SEEK_SET);

    if (read(fd, &sender_account, sizeof(Account)) != sizeof(Account))
    {
        write_string(client_socket, "Error: Failed to read sender account.\n");
        set_record_lock(fd, rec1, sizeof(Account), F_UNLCK);
        set_record_lock(fd, rec2, sizeof(Account), F_UNLCK);
        close(fd);
        return;
    }

    lseek(fd, receiver_rec_num * sizeof(Account), SEEK_SET);
    if (read(fd, &receiver_account, sizeof(Account)) != sizeof(Account))
    {
        write_string(client_socket, "Error: Failed to read receiver account.\n");
        set_record_lock(fd, rec1, sizeof(Account), F_UNLCK);
        set_record_lock(fd, rec2, sizeof(Account), F_UNLCK);
        close(fd);
        return;
    }


    int transfer_succeeded = 0; // Flag to check if we should commit




    if (sender_account.balance < amount)
    {
        write_string(client_socket, "Insufficient funds.\n");
    }
    else if (!receiver_account.isActive)
    { // --- FIX: State Validation ---
        write_string(client_socket, "Error: The recipient's account is deactivated.\n");
    }
    else
    {
        // --- MODIFIED: Perform Atomic Write ---
        sender_account.balance -= amount;
        lseek(fd, sender_rec_num * sizeof(Account), SEEK_SET);
        if(write(fd, &sender_account, sizeof(Account)) != sizeof(Account)) {
            write_string(STDOUT_FILENO, "FATAL: Transfer debit write failed.\n");
        } else {
            receiver_account.balance += amount;
            lseek(fd, receiver_rec_num * sizeof(Account), SEEK_SET);
            if(write(fd, &receiver_account, sizeof(Account)) != sizeof(Account)) {
                write_string(STDOUT_FILENO, "FATAL: Transfer credit write failed.\n");
            } else {
                transfer_succeeded = 1; // Mark as success
            }
        }
    }

    set_record_lock(fd, rec1, sizeof(Account), F_UNLCK);
    set_record_lock(fd, rec2, sizeof(Account), F_UNLCK);
    close(fd);
// --- MODIFIED: Only log and notify if the commit was successful ---
    if(transfer_succeeded) {
        // --- ADDED: Log Commit ---
        log_entry.status = LOG_COMMIT;
        write_transfer_log(&log_entry);
        // --- END ADDED ---

        log_transaction(receiver_account.accountId, receiver_account.ownerUserId, TRANSFER_IN, amount, receiver_account.balance, sender_account.accountNumber);
        log_transaction(sender_account.accountId, sender_account.ownerUserId, TRANSFER_OUT, amount, sender_account.balance, receiver_account.accountNumber);
        write_string(client_socket, "Transfer successful.\n");
    } else {
        // Don't log commit, so recovery will roll it back
        write_string(client_socket, "Transfer failed. Please check logs or try again.\n");
    }
}

static void handle_apply_loan(int client_socket, int userId)
{
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter loan amount (e.g., 50000): ");

    if (read_client_input(client_socket, buffer, MAX_BUFFER) <= 0)
        return;
    if (!is_valid_amount(buffer))
    {
        write_string(client_socket, "Invalid amount. Please enter numbers only.\n");
        return;
    }
    double amount = atof(buffer);
    if (amount <= 0.01)
    {
        write_string(client_socket, "Amount must be positive.\n");
        return;
    }

    int rec_num = find_account_record_by_id(userId);
    if (rec_num == -1)
    {
        write_string(client_socket, "Error: Your account could not be found.\n");
        return;
    }

    Loan new_loan;
    new_loan.loanId = get_next_loan_id();
    new_loan.userId = userId;
    new_loan.accountIdToDeposit = userId;
    new_loan.amount = amount;
    new_loan.status = PENDING;
    new_loan.assignedToEmployeeId = 0;

    int fd_loan = open(LOAN_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_loan == -1)
    {
        write_string(client_socket, "Error submitting loan application.\n");
        return;
    }

    set_file_lock(fd_loan, F_WRLCK);
    if (write(fd_loan, &new_loan, sizeof(Loan)) != sizeof(Loan))
    {
        write_string(client_socket, "Error saving loan application.\n");
    }
    else
    {
        sprintf(buffer, "Loan application (ID: %d) submitted. Status: PENDING\n", new_loan.loanId);
        write_string(client_socket, buffer);
    }
    set_file_lock(fd_loan, F_UNLCK);
    close(fd_loan);
}

static void handle_view_loan_status(int client_socket, int userId)
{
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1)
    {
        write_string(client_socket, "No loan applications found.\n");
        return;
    }
    set_file_lock(fd, F_RDLCK);
    Loan loan;
    char buffer[256];
    int found = 0;
    write_string(client_socket, "\n--- Your Loan Applications ---\n");
    while (read(fd, &loan, sizeof(Loan)) == sizeof(Loan))
    {
        if (loan.userId == userId)
        {
            found = 1;
            char *status_str;
            switch (loan.status)
            {
            case PENDING:
                status_str = "PENDING";
                break;
            case PROCESSING:
                status_str = "PROCESSING";
                break;
            case APPROVED:
                status_str = "APPROVED";
                break;
            case REJECTED:
                status_str = "REJECTED";
                break;
            default:
                status_str = "UNKNOWN";
            }
            sprintf(buffer, "Loan ID: %d | Amount: ₹%.2f | Status: %s\n",
                    loan.loanId, loan.amount, status_str);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);
    if (!found)
    {
        write_string(client_socket, "No loan applications found.\n");
    }
}

static void handle_add_feedback(int client_socket, int userId)
{
    char buffer[MAX_BUFFER];
    write_string(client_socket, "Enter your feedback: ");

    // --- FIX: Check for disconnect/empty/long ---
    if (get_valid_string(client_socket, buffer, 256) == -1)
        return;

    Feedback new_feedback;
    new_feedback.feedbackId = get_next_feedback_id();
    new_feedback.userId = userId;
    strncpy(new_feedback.feedbackText, buffer, 255);
    new_feedback.feedbackText[255] = '\0';
    new_feedback.isReviewed = 0;

    int fd = open(FEEDBACK_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        write_string(client_socket, "Error submitting feedback.\n");
        return;
    }

    set_file_lock(fd, F_WRLCK);
    if (write(fd, &new_feedback, sizeof(Feedback)) != sizeof(Feedback))
    {
        write_string(client_socket, "Error saving feedback.\n");
    }
    else
    {
        write_string(client_socket, "Feedback submitted successfully. Thank you!\n");
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);
}

static void handle_view_feedback_status(int client_socket, int userId)
{
    int fd = open(FEEDBACK_FILE, O_RDONLY);
    if (fd == -1)
    {
        write_string(client_socket, "No feedback history found.\n");
        return;
    }
    set_file_lock(fd, F_RDLCK);
    Feedback feedback;
    char buffer[512];
    int found = 0;
    write_string(client_socket, "\n--- Your Feedback History ---\n");
    while (read(fd, &feedback, sizeof(Feedback)) == sizeof(Feedback))
    {
        if (feedback.userId == userId)
        {
            found = 1;
            char *status_str = (feedback.isReviewed) ? "Reviewed" : "Pending Review";
            sprintf(buffer, "ID: %d | Status: %s | Feedback: %.50s...\n",
                    feedback.feedbackId, status_str, feedback.feedbackText);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);
    if (!found)
    {
        write_string(client_socket, "No feedback history found.\n");
    }
}

// --- Public Customer Menu ---

void customer_menu(int client_socket, User user)
{
    char buffer[MAX_BUFFER];
    char welcome_msg[100];
    sprintf(welcome_msg, "    Welcome, %s! (User ID: %d)\n", user.firstName, user.userId);

    while (1)
    {
        write_string(client_socket, "\n\n+---------------------------------------+\n");
        write_string(client_socket, "       🏦  Customer Dashboard  🏦\n");
        write_string(client_socket, "+---------------------------------------+\n");
        write_string(client_socket, welcome_msg);
        write_string(client_socket, "-----------------------------------------\n");

        write_string(client_socket, " 1. View Balance\n");
        write_string(client_socket, " 2. Deposit Money\n");
        write_string(client_socket, " 3. Withdraw Money\n");
        write_string(client_socket, " 4. Transfer Funds\n");
        write_string(client_socket, " 5. View Transaction History\n");
        write_string(client_socket, " 6. Apply for Loan\n");
        write_string(client_socket, " 7. View Loan Status\n");
        write_string(client_socket, " 8. View My Personal Details\n");
        write_string(client_socket, " 9. Add Feedback\n");
        write_string(client_socket, " 10. View Feedback Status\n");
        write_string(client_socket, " 11. Change Password\n");
        write_string(client_socket, " 12. Logout\n");
        write_string(client_socket, "+---------------------------------------+\n");
        write_string(client_socket, "Enter your choice: ");

        // --- FIX: Check for disconnect or empty input ---
        int read_status = read_client_input(client_socket, buffer, MAX_BUFFER);
        if (read_status <= 0)
        {
            write_string(STDOUT_FILENO, "Client disconnected.\n");
            return; // Exit menu
        }
        if (my_strcmp(buffer, "") == 0)
        {
            write_string(client_socket, "Invalid choice. Please enter a number.\n");
            continue; // Re-show menu
        }
        // --- END FIX ---

        int choice = atoi(buffer);
        switch (choice)
        {
        case 1:
            handle_view_balance(client_socket, user.userId);
            break;
        case 2:
            handle_deposit(client_socket, user.userId);
            break;
        case 3:
            handle_withdraw(client_socket, user.userId);
            break;
        case 4:
            handle_transfer_funds(client_socket, user.userId);
            break;
        case 5:
            handle_view_transaction_history(client_socket, user.userId);
            break;
        case 6:
            handle_apply_loan(client_socket, user.userId);
            break;
        case 7:
            handle_view_loan_status(client_socket, user.userId);
            break;
        case 8:
            handle_view_my_details(client_socket, user);
            break;
        case 9:
            handle_add_feedback(client_socket, user.userId);
            break;
        case 10:
            handle_view_feedback_status(client_socket, user.userId);
            break;
        case 11:
            handle_change_password(client_socket, user.userId);
            break;
        case 12:
            write_string(client_socket, "Logging out. Goodbye!\n");
            return;
        default:
            write_string(client_socket, "Invalid choice.\n");
        }
    }
}