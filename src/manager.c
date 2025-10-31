// src/manager.c
#include "manager.h"
#include "model.h"
#include "utils.h"
#include "shared.h" // For shared functions

// --- Private Manager Handlers ---

static void handle_assign_loan(int client_socket) {
    int fd = open(LOAN_FILE, O_RDWR);
    if (fd == -1) {
        write_string(client_socket, "No loans found.\n");
        return;
    }

    set_file_lock(fd, F_RDLCK);
    Loan loan;
    char buffer[256];
    int found = 0;
    
    write_string(client_socket, "\n--- Unassigned Loans (Status: PENDING) ---\n");
    lseek(fd, 0, SEEK_SET); 
    while (read(fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.assignedToEmployeeId == 0 && loan.status == PENDING) {
            found = 1;
            sprintf(buffer, "Loan ID: %d | Customer ID: %d | Amount: ₹%.2f\n",
                loan.loanId, loan.userId, loan.amount);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(fd, F_UNLCK);

    if (!found) {
        write_string(client_socket, "No unassigned loans found.\n");
        close(fd);
        return;
    }

    write_string(client_socket, "Enter Loan ID to assign: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int loanId = atoi(buffer);

    write_string(client_socket, "Enter Employee ID to assign to: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int employeeId = atoi(buffer);

    int emp_rec_num = find_user_record(employeeId);
    if (emp_rec_num == -1) {
        write_string(client_socket, "Employee not found.\n");
        close(fd);
        return;
    }

    int loan_rec_num = find_loan_record(loanId);
    if (loan_rec_num == -1) {
        write_string(client_socket, "Loan not found.\n");
        close(fd);
        return;
    }

    set_record_lock(fd, loan_rec_num, sizeof(Loan), F_WRLCK);
    lseek(fd, loan_rec_num * sizeof(Loan), SEEK_SET);
    read(fd, &loan, sizeof(Loan));
    
    if (loan.assignedToEmployeeId != 0 || loan.status != PENDING) {
         write_string(client_socket, "Loan cannot be assigned (already assigned or processed).\n");
    } else {
        loan.assignedToEmployeeId = employeeId;
        loan.status = PROCESSING; 
        lseek(fd, loan_rec_num * sizeof(Loan), SEEK_SET);
        write(fd, &loan, sizeof(Loan));
        write_string(client_socket, "Loan assigned successfully.\n");
    }

    set_record_lock(fd, loan_rec_num, sizeof(Loan), F_UNLCK);
    close(fd);
}

static void handle_review_feedback(int client_socket) {
    int fd = open(FEEDBACK_FILE, O_RDWR);
    if (fd == -1) {
        write_string(client_socket, "No feedback found.\n");
        return;
    }

    set_file_lock(fd, F_RDLCK);
    Feedback feedback;
    char buffer[512];
    int found = 0;
    
    write_string(client_socket, "\n--- Unreviewed Feedback ---\n");
    lseek(fd, 0, SEEK_SET); 
    while (read(fd, &feedback, sizeof(Feedback)) == sizeof(Feedback)) {
        if (feedback.isReviewed == 0) {
            found = 1;
            sprintf(buffer, "ID: %d | User: %d | Feedback: %.100s...\n",
                feedback.feedbackId, feedback.userId, feedback.feedbackText);
            write_string(client_socket, buffer);
        }
    }
    set_file_lock(fd, F_UNLCK);

    if (!found) {
        write_string(client_socket, "No unreviewed feedback found.\n");
        close(fd);
        return;
    }

    write_string(client_socket, "Enter Feedback ID to mark as reviewed: ");
    read_client_input(client_socket, buffer, MAX_BUFFER);
    int feedbackId = atoi(buffer);

    int rec_num = find_feedback_record(feedbackId);
    if (rec_num == -1) {
        write_string(client_socket, "Feedback ID not found.\n");
        close(fd);
        return;
    }

    set_record_lock(fd, rec_num, sizeof(Feedback), F_WRLCK);
    lseek(fd, rec_num * sizeof(Feedback), SEEK_SET);
    read(fd, &feedback, sizeof(Feedback));

    if (feedback.isReviewed == 1) {
        write_string(client_socket, "Feedback already marked as reviewed.\n");
    } else {
        feedback.isReviewed = 1; 
        lseek(fd, rec_num * sizeof(Feedback), SEEK_SET);
        write(fd, &feedback, sizeof(Feedback));
        write_string(client_socket, "Feedback marked as reviewed.\n");
    }

    set_record_lock(fd, rec_num, sizeof(Feedback), F_UNLCK);
    close(fd);
}

// --- Public Manager Menu ---

void manager_menu(int client_socket, User user) {
    char buffer[MAX_BUFFER];
    char welcome_msg[150]; 
    sprintf(welcome_msg, "    Manager: %s %s (ID: %d)\n", user.firstName, user.lastName, user.userId);

    while(1) {
        write_string(client_socket, "\n\n+---------------------------------------+\n");
        write_string(client_socket, "       📈  Manager Dashboard  📈\n");
        write_string(client_socket, "+---------------------------------------+\n");
        write_string(client_socket, welcome_msg);
        write_string(client_socket, "-----------------------------------------\n");

        write_string(client_socket, "1. Activate/Deactivate Customer & Account\n"); 
        write_string(client_socket, "2. Assign Loan to Employee\n");
        write_string(client_socket, "3. Review Customer Feedback\n");
        write_string(client_socket, "4. View My Personal Details\n");
        write_string(client_socket, "5. Change My Password\n");
        write_string(client_socket, "6. Logout\n");
        write_string(client_socket, "Enter your choice: ");
        
        read_client_input(client_socket, buffer, MAX_BUFFER);
        int choice = atoi(buffer);
        switch(choice) {
            case 1: handle_set_account_status(client_socket, 0); break;
            case 2: handle_assign_loan(client_socket); break;
            case 3: handle_review_feedback(client_socket); break;
            case 4: handle_view_my_details(client_socket, user); break;
            case 5: handle_change_password(client_socket, user.userId); break;
            case 6: write_string(client_socket, "Logging out. Goodbye!\n"); return;
            default: write_string(client_socket, "Invalid choice.\n");
        }
    }
}