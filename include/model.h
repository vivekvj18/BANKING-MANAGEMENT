// include/model.h
#ifndef MODEL_H
#define MODEL_H

#include "common.h"

// --- Record-Finding Functions ---
int find_user_record(int userId);
int find_account_record_by_id(int userId);
int find_loan_record(int loanId);
int find_feedback_record(int feedbackId);

// --- Authentication ---
User check_login(int userId, char* password);

// --- Data Creation/Update Functions ---
void log_transaction(int accountId, int userId, TransactionType type, double amount, double newBalance, const char* otherPartyAccount);

// --- ID Generation Functions ---
int get_next_user_id();
int get_next_loan_id();
int get_next_feedback_id();
int get_next_transaction_id();

// --- ADDED: Transfer Log Prototypes ---
long get_next_transfer_id();
void write_transfer_log(TransferLog* log_entry);
void perform_recovery_check();
// --- END ADDED ---

#endif // MODEL_H