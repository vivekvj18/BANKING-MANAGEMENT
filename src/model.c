// src/model.c
#include "common.h" // <-- This is required
#include "model.h"
#include "utils.h" 

// --- Record-Finding Functions ---
int find_user_record(int userId) {
    int fd = open(USER_FILE, O_RDONLY);
    if (fd == -1) { perror("open user file"); return -1; }
    set_file_lock(fd, F_RDLCK);
    User user;
    int record_num = 0;
    while (read(fd, &user, sizeof(User)) == sizeof(User)) {
        if (user.userId == userId) {
            set_file_lock(fd, F_UNLCK); close(fd); return record_num;
        }
        record_num++;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return -1;
}

int find_account_record_by_id(int userId) {
    int fd = open(ACCOUNT_FILE, O_RDONLY);
    if (fd == -1) { perror("open account file"); return -1; }
    set_file_lock(fd, F_RDLCK);
    Account account;
    int record_num = 0;
    while (read(fd, &account, sizeof(Account)) == sizeof(Account)) {
        if (account.accountId == userId) {
            set_file_lock(fd, F_UNLCK); close(fd); return record_num;
        }
        record_num++;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return -1;
}

int find_loan_record(int loanId) {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) { perror("open loan file"); return -1; }
    set_file_lock(fd, F_RDLCK);
    Loan loan;
    int record_num = 0;
    while (read(fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.loanId == loanId) {
            set_file_lock(fd, F_UNLCK); close(fd); return record_num;
        }
        record_num++;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return -1;
}

int find_feedback_record(int feedbackId) {
    int fd = open(FEEDBACK_FILE, O_RDONLY);
    if (fd == -1) { perror("open feedback file"); return -1; }
    set_file_lock(fd, F_RDLCK);
    Feedback feedback;
    int record_num = 0;
    while (read(fd, &feedback, sizeof(Feedback)) == sizeof(Feedback)) {
        if (feedback.feedbackId == feedbackId) {
            set_file_lock(fd, F_UNLCK); close(fd); return record_num;
        }
        record_num++;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return -1;
}

// --- Login Function ---
User check_login(int userId, char* password) {
    User user_to_find;
    user_to_find.userId = 0; 
    int record_num = find_user_record(userId);
    if (record_num == -1) { return user_to_find; }

    int fd = open(USER_FILE, O_RDONLY);
    if (fd == -1) { user_to_find.userId = -1; return user_to_find; }

    set_record_lock(fd, record_num, sizeof(User), F_RDLCK);
    lseek(fd, record_num * sizeof(User), SEEK_SET);
    
    User user;
    if (read(fd, &user, sizeof(User)) == sizeof(User)) {
        if (user.userId == userId && my_strcmp(user.password, password) == 0) {
            if (user.isActive) {
                user_to_find = user;
            } else {
                user_to_find.userId = -2;
            }
        }
    }
    set_record_lock(fd, record_num, sizeof(User), F_UNLCK);
    close(fd);
    return user_to_find;
}

// --- Transaction Functions ---
void log_transaction(int accountId, int userId, TransactionType type, double amount, double newBalance, const char* otherPartyAccount) {
    int fd = open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) { perror("Could not open transaction file"); return; }

    set_file_lock(fd, F_WRLCK);

    Transaction txn;
    txn.transactionId = get_next_transaction_id();
    txn.accountId = accountId;
    txn.userId = userId;
    txn.type = type;
    txn.amount = amount;
    txn.newBalance = newBalance;
    strcpy(txn.otherPartyAccountNumber, otherPartyAccount);

    write(fd, &txn, sizeof(Transaction));
    set_file_lock(fd, F_UNLCK);
    close(fd);
}

// --- ID Generation Functions ---
int get_next_user_id() {
    int fd = open(USER_FILE, O_RDONLY);
    if (fd == -1) { return 1; }

    set_file_lock(fd, F_RDLCK);
    if (lseek(fd, -sizeof(User), SEEK_END) == -1) {
        set_file_lock(fd, F_UNLCK); close(fd); return 1;
    }
    User last_user;
    if (read(fd, &last_user, sizeof(User)) == sizeof(User)) {
        set_file_lock(fd, F_UNLCK); close(fd);
        return last_user.userId + 1;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return 1;
}

int get_next_loan_id() {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) { return 1; } 

    set_file_lock(fd, F_RDLCK);
    if (lseek(fd, -sizeof(Loan), SEEK_END) == -1) {
        set_file_lock(fd, F_UNLCK); close(fd); return 1;
    }
    Loan last_loan;
    if (read(fd, &last_loan, sizeof(Loan)) == sizeof(Loan)) {
        set_file_lock(fd, F_UNLCK); close(fd);
        return last_loan.loanId + 1;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return 1; 
}

int get_next_feedback_id() {
    int fd = open(FEEDBACK_FILE, O_RDONLY);
    if (fd == -1) { return 1; } 

    set_file_lock(fd, F_RDLCK);
    if (lseek(fd, -sizeof(Feedback), SEEK_END) == -1) {
        set_file_lock(fd, F_UNLCK); close(fd); return 1;
    }
    Feedback last_feedback;
    if (read(fd, &last_feedback, sizeof(Feedback)) == sizeof(Feedback)) {
        set_file_lock(fd, F_UNLCK); close(fd);
        return last_feedback.feedbackId + 1;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return 1; 
}

int get_next_transaction_id() {
    int fd = open(TRANSACTION_FILE, O_RDONLY);
    if (fd == -1) { return 1; } 

    set_file_lock(fd, F_RDLCK);
    if (lseek(fd, -sizeof(Transaction), SEEK_END) == -1) {
        set_file_lock(fd, F_UNLCK); close(fd); return 1;
    }
    Transaction last_txn;
    if (read(fd, &last_txn, sizeof(Transaction)) == sizeof(Transaction)) {
        set_file_lock(fd, F_UNLCK); close(fd);
        return last_txn.transactionId + 1;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return 1; 
}

// --- ADDED: Atomicity & Recovery Functions ---

long get_next_transfer_id() {
    int fd = open(TRANSFER_LOG_FILE, O_RDONLY);
    if (fd == -1) { return 1; } 

    set_file_lock(fd, F_RDLCK);
    if (lseek(fd, -sizeof(TransferLog), SEEK_END) == -1) {
        set_file_lock(fd, F_UNLCK); close(fd); return 1;
    }
    TransferLog last_log;
    if (read(fd, &last_log, sizeof(TransferLog)) == sizeof(TransferLog)) {
        set_file_lock(fd, F_UNLCK); close(fd);
        return last_log.transferId + 1;
    }
    set_file_lock(fd, F_UNLCK); close(fd);
    return 1; 
}

void write_transfer_log(TransferLog* log_entry) {
    int fd = open(TRANSFER_LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("FATAL: Failed to open transfer log");
        return;
    }
    set_file_lock(fd, F_WRLCK);
    if (write(fd, log_entry, sizeof(TransferLog)) != sizeof(TransferLog)) {
        perror("FATAL: Failed to write to transfer log");
    }
    set_file_lock(fd, F_UNLCK);
    close(fd);
}

void perform_recovery_check() {
    int log_fd = open(TRANSFER_LOG_FILE, O_RDONLY);
    if (log_fd == -1) {
        write_string(STDOUT_FILENO, "No transfer log found. Skipping recovery.\n");
        return;
    }
    
    set_file_lock(log_fd, F_RDLCK);

    #define MAX_PENDING_TXS 1000
    TransferLog pending[MAX_PENDING_TXS];
    int pending_count = 0;
    TransferLog entry;

    // --- Step 1: Find all incomplete transactions ---
    while(read(log_fd, &entry, sizeof(TransferLog)) == sizeof(TransferLog)) {
        if (entry.status == LOG_START) {
            if(pending_count < MAX_PENDING_TXS) {
                pending[pending_count++] = entry;
            } else {
                write_string(STDOUT_FILENO, "ERROR: Too many pending transactions to recover.\n");
                break;
            }
        } else if (entry.status == LOG_COMMIT) {
            for (int i = 0; i < pending_count; i++) {
                if (pending[i].transferId == entry.transferId) {
                    pending[i] = pending[pending_count - 1]; // Swap with last
                    pending_count--;
                    break;
                }
            }
        }
    }
    set_file_lock(log_fd, F_UNLCK);
    close(log_fd);

    // --- Step 2: Rollback any transactions still in the pending list ---
    if (pending_count == 0) {
        write_string(STDOUT_FILENO, "Recovery check clean. No incomplete transfers found.\n");
        return;
    }

    char buffer[256];
    sprintf(buffer, "WARNING: Found %d incomplete transfers. Rolling back...\n", pending_count);
    write_string(STDOUT_FILENO, buffer);

    int acct_fd = open(ACCOUNT_FILE, O_RDWR);
    if (acct_fd == -1) {
        write_string(STDOUT_FILENO, "FATAL: Cannot open account file for recovery.\n");
        return;
    }

    for (int i = 0; i < pending_count; i++) {
        TransferLog* failed_tx = &pending[i];
        
        int sender_rec_num = find_account_record_by_id(failed_tx->fromAccountId);
        if (sender_rec_num == -1) continue; 

        set_record_lock(acct_fd, sender_rec_num, sizeof(Account), F_WRLCK);
        
        Account sender_account;
        lseek(acct_fd, sender_rec_num * sizeof(Account), SEEK_SET);
        if (read(acct_fd, &sender_account, sizeof(Account)) != sizeof(Account)) {
             write_string(STDOUT_FILENO, "ERROR: Could not read account for rollback.\n");
             set_record_lock(acct_fd, sender_rec_num, sizeof(Account), F_UNLCK);
             continue;
        }
        
        // REFUND THE MONEY
        sender_account.balance += failed_tx->amount;
        
        lseek(acct_fd, sender_rec_num * sizeof(Account), SEEK_SET);
        if (write(acct_fd, &sender_account, sizeof(Account)) != sizeof(Account)) {
            write_string(STDOUT_FILENO, "FATAL: Could not write rollback.\n");
        } else {
            log_transaction(sender_account.accountId, sender_account.ownerUserId, DEPOSIT, failed_tx->amount, sender_account.balance, "ROLLBACK_FAIL");
            
            sprintf(buffer, "Rolled back %f from user %d.\n", failed_tx->amount, failed_tx->fromAccountId);
            write_string(STDOUT_FILENO, buffer);
        }
        
        set_record_lock(acct_fd, sender_rec_num, sizeof(Account), F_UNLCK);
    }
    close(acct_fd);
}
// --- END ADDED ---