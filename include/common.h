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
    int accountId;         // Unique ID for the account (matches ownerUserId)
    int ownerUserId;       // ID of the user who owns this
    char accountNumber[20]; // The string "SB-2", "SB-6" etc.
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
    int accountId;      // Which account this transaction belongs to
    int userId;         // Which user performed this
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
    int accountIdToDeposit; // The accountId (== userId) to put the money in
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

#endif // COMMON_H