// src/admin_util.c
#include "common.h"
#include "utils.h"  // Include utils for write_string
#include "model.h"  // Include model for file paths

int main() {
    int fd_user, fd_account;

    // --- Create Files (Truncate them to empty) ---
    fd_user = open(USER_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_user == -1) { perror("Error opening user file"); return 1; }
    
    fd_account = open(ACCOUNT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_account == -1) { perror("Error opening account file"); return 1; }
    
    open(LOAN_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    open(FEEDBACK_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    
    // --- User 1: Administrator 1 ---
    User admin1;
    admin1.userId = 1;
    admin1.role = ADMINISTRATOR;
    admin1.isActive = 1;
    strcpy(admin1.password, "admin123"); 
    strcpy(admin1.firstName, "Admin");
    strcpy(admin1.lastName, "One");
    strcpy(admin1.phone, "1111111111");
    strcpy(admin1.email, "admin1@bank.com");
    strcpy(admin1.address, "1 Admin Road");
    write(fd_user, &admin1, sizeof(User));
    write_string(STDOUT_FILENO, "Admin user 1 created (ID: 1, Pass: admin123)\n");

    // --- User 2: Customer 1 ---
    User customer1;
    customer1.userId = 2;
    customer1.role = CUSTOMER;
    customer1.isActive = 1;
    strcpy(customer1.password, "cust123");
    strcpy(customer1.firstName, "Customer");
    strcpy(customer1.lastName, "One");
    strcpy(customer1.phone, "2222222222");
    strcpy(customer1.email, "cust1@gmail.com");
    strcpy(customer1.address, "1 Customer Road");
    write(fd_user, &customer1, sizeof(User));
    write_string(STDOUT_FILENO, "Customer user 1 created (ID: 2, Pass: cust123)\n");
    
    // --- User 3: Employee 1 ---
    User employee1;
    employee1.userId = 3;
    employee1.role = EMPLOYEE;
    employee1.isActive = 1;
    strcpy(employee1.password, "emp123");
    strcpy(employee1.firstName, "Employee");
    strcpy(employee1.lastName, "One");
    strcpy(employee1.phone, "3333333333");
    strcpy(employee1.email, "emp1@bank.com");
    strcpy(employee1.address, "1 Employee Road");
    write(fd_user, &employee1, sizeof(User));
    write_string(STDOUT_FILENO, "Employee user 1 created (ID: 3, Pass: emp123)\n");

    // --- User 4: Manager 1 ---
    User manager1;
    manager1.userId = 4;
    manager1.role = MANAGER;
    manager1.isActive = 1;
    strcpy(manager1.password, "man123");
    strcpy(manager1.firstName, "Manager");
    strcpy(manager1.lastName, "One");
    strcpy(manager1.phone, "4444444444");
    strcpy(manager1.email, "man1@bank.com");
    strcpy(manager1.address, "1 Manager Road");
    write(fd_user, &manager1, sizeof(User));
    write_string(STDOUT_FILENO, "Manager user 1 created (ID: 4, Pass: man123)\n");

    // --- User 5: Administrator 2 ---
    User admin2;
    admin2.userId = 5;
    admin2.role = ADMINISTRATOR;
    admin2.isActive = 1;
    strcpy(admin2.password, "admin456"); 
    strcpy(admin2.firstName, "Admin");
    strcpy(admin2.lastName, "Two");
    strcpy(admin2.phone, "5555555555");
    strcpy(admin2.email, "admin2@bank.com");
    strcpy(admin2.address, "2 Admin Road");
    write(fd_user, &admin2, sizeof(User));
    write_string(STDOUT_FILENO, "Admin user 2 created (ID: 5, Pass: admin456)\n");

    // --- User 6: Customer 2 ---
    User customer2;
    customer2.userId = 6;
    customer2.role = CUSTOMER;
    customer2.isActive = 1;
    strcpy(customer2.password, "cust456");
    strcpy(customer2.firstName, "Customer");
    strcpy(customer2.lastName, "Two");
    strcpy(customer2.phone, "6666666666");
    strcpy(customer2.email, "cust2@gmail.com");
    strcpy(customer2.address, "2 Customer Road");
    write(fd_user, &customer2, sizeof(User));
    write_string(STDOUT_FILENO, "Customer user 2 created (ID: 6, Pass: cust456)\n");
    
    // --- User 7: Employee 2 ---
    User employee2;
    employee2.userId = 7;
    employee2.role = EMPLOYEE;
    employee2.isActive = 1;
    strcpy(employee2.password, "emp456");
    strcpy(employee2.firstName, "Employee");
    strcpy(employee2.lastName, "Two");
    strcpy(employee2.phone, "7777777777");
    strcpy(employee2.email, "emp2@bank.com");
    strcpy(employee2.address, "2 Employee Road");
    write(fd_user, &employee2, sizeof(User));
    write_string(STDOUT_FILENO, "Employee user 2 created (ID: 7, Pass: emp456)\n");

    // --- User 8: Manager 2 ---
    User manager2;
    manager2.userId = 8;
    manager2.role = MANAGER;
    manager2.isActive = 1;
    strcpy(manager2.password, "man456");
    strcpy(manager2.firstName, "Manager");
    strcpy(manager2.lastName, "Two");
    strcpy(manager2.phone, "8888888888");
    strcpy(manager2.email, "man2@bank.com");
    strcpy(manager2.address, "2 Manager Road");
    write(fd_user, &manager2, sizeof(User));
    write_string(STDOUT_FILENO, "Manager user 2 created (ID: 8, Pass: man456)\n");
    
    close(fd_user);

    // --- Account for Customer 1 (ID 2) ---
    Account cust_account1;
    cust_account1.accountId = 2; // Matches Customer 1's ID
    cust_account1.ownerUserId = 2; 
    strcpy(cust_account1.accountNumber, "SB-2"); 
    cust_account1.balance = 5000.00; 
    cust_account1.isActive = 1;
    write(fd_account, &cust_account1, sizeof(Account));
    
    // --- Account for Customer 2 (ID 6) ---
    Account cust_account2;
    cust_account2.accountId = 6; // Matches Customer 2's ID
    cust_account2.ownerUserId = 6; 
    strcpy(cust_account2.accountNumber, "SB-6");
    cust_account2.balance = 10000.00; 
    cust_account2.isActive = 1;
    write(fd_account, &cust_account2, sizeof(Account));

    // UPDATED: Print message
    write_string(STDOUT_FILENO, "Customer accounts created (SB-2, SB-6)\n");
    
    close(fd_account);
    
    write_string(STDOUT_FILENO, "All data files initialized successfully.\n");

    return 0;
}