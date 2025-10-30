// src/model.c
#include "model.h"
#include "utils.h" // For set_file_lock

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
    int fd = open(TRANSACTION_FILE, O_WRONLY | O_APPEND);
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
// (These came from the old common_utils.c)

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