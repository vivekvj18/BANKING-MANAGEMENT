// include/common.h
#ifndef COMMON_H
#define COMMON_H

// --- System Call Headers ---
#include <stdio.h>      // For perror() only
#include <stdlib.h>     // For exit(), atoi(), atof
#include <unistd.h>     // For open, read, write, lseek, close, fork
#include <fcntl.h>      // For fcntl() (locking) and file flags
#include <string.h>     // For strcmp, strcpy, memset, strncpy
#include <sys/socket.h> // For socket programming
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>  // For inet_addr, inet_pton
#include <errno.h>      // For errno
#include <sys/types.h>  // For lseek
#include <pthread.h>    // For threads

// --- Project-Specific Definitions ---
#define PORT 8080
#define MAX_BUFFER 1024

// --- File Paths ---
#define USER_FILE "data/users.dat"
#define ACCOUNT_FILE "data/accounts.dat"
#define LOAN_FILE "data/loans.dat"
#define FEEDBACK_FILE "data/feedback.dat"
#define TRANSACTION_FILE "data/transactions.dat"
#define TRANSFER_LOG_FILE "data/transfer_log.dat" // <-- THIS WAS THE MISSING LINE

// --- Data Structures ---
typedef enum {
    CUSTOMER,
    EMPLOYEE,
    MANAGER,
    ADMINISTRATOR
} UserRole;

typedef struct {
    int userId;
    char password[50];
    UserRole role;
    int isActive;
    char firstName[50];
    char lastName[50];
    char phone[15];
    char email[100];
    char address[256];
} User;

typedef struct {
    int accountId;
    int ownerUserId;
    char accountNumber[20];
    double balance;
    int isActive;
} Account;

typedef enum {
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER_OUT,
    TRANSFER_IN
} TransactionType;

typedef struct {
    int transactionId;
    int accountId;
    int userId;
    TransactionType type;
    double amount;
    double newBalance;
    char otherPartyAccountNumber[20]; 
} Transaction;

typedef enum {
    PENDING,
    PROCESSING,
    APPROVED,
    REJECTED
} LoanStatus;

typedef struct {
    int loanId;
    int userId;
    int accountIdToDeposit;
    double amount;
    LoanStatus status;
    int assignedToEmployeeId;
} Loan;

typedef struct {
    int feedbackId;
    int userId;
    char feedbackText[256];
    int isReviewed;
} Feedback;


// --- ADDED: New Structs for Write-Ahead Log ---
typedef enum {
    LOG_START,
    LOG_COMMIT
} LogStatus;

typedef struct {
    long transferId;  // A unique ID for this specific transfer
    int fromAccountId;
    int toAccountId;
    double amount;
    LogStatus status;
} TransferLog;
// --- END ADDED ---

#endif // COMMON_H