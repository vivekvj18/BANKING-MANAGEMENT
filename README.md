# 🏦 Banking Management System (C)

![Language](https://img.shields.io/badge/Language-C-blue.svg) ![Build](https://img.shields.io/badge/Build-gcc-lightgrey.svg) ![Concurrency](https://img.shields.io/badge/Concurrency-pthreads-blue.svg)

This is a multi-threaded, socket-based banking application built in C. It simulates a real-world banking environment using a **professional 3-tier modular architecture** to ensure a clean separation of concerns.

This project is a comprehensive demonstration of core system software concepts:
* **Modular 3-Tier Design:** Code is separated into a Network Layer (`server.c`), Logic Layer (`controller.c`, `admin.c`, etc.), and Data Layer (`model.c`).
* **Socket Programming:** Uses TCP/IP sockets to handle multiple clients concurrently.
* **Multithreading:** Leverages `pthreads` to assign a separate thread for every client.
* **Full ACID Compliance:** Guarantees data integrity through file locking and a write-ahead log.
* **Concurrency Control:** Implements `fcntl` record locking to prevent race conditions and a `pthread_mutex_t` to protect the active session list.
* **Write-Ahead Logging (WAL):** Ensures transaction **Atomicity** (even in a server crash) by logging all transfers to `transfer_log.dat` before committing them.
* **Robust Error Handling:** Validates all user input (for length, format, and uniqueness) and checks the return values of all critical system calls (`read`, `write`).

---

## 🏛️ System Architecture

This project follows a 3-tier "separation of concerns" model. The logic is separated into a Network layer, a routing/session layer, distinct role-based controllers, and a data-access layer.

*(This is the perfect place to insert the `blueprint.png` diagram you generated)*

The code is organized as follows:
* **`server.c` (Network Layer):**
    * Its **single responsibility** is to `socket`, `bind`, `listen`, and `accept` new client connections.
    * Spawns a new `pthread` for each client and passes control to the controller.
* **`controller.c` (Routing & Session Layer):**
    * Handles the initial login, validates the user's role, and manages active sessions using a mutex-locked array.
    * Acts as a "router," sending the client to the correct menu (`admin_menu`, `customer_menu`, etc.).
* **Role Controllers (`admin.c`, `customer.c`, etc.):**
    * Each file is responsible for *one* user role.
    * Contains the menu loop and all "handler" functions for that role (e.g., `customer.c` contains `handle_deposit`).
* **`shared.c` (Shared Business Logic):**
    * Contains handler functions used by *multiple* roles, such as `handle_add_user`, `handle_change_password`, and all input validation helpers (`get_valid_string`, `get_valid_email`).
* **`model.c` (Data Access Layer):**
    * The **only** layer that directly reads from or writes to the `.dat` files.
    * Contains all data-access logic (`find_user_record`, `log_transaction`) and the **Atomicity/WAL functions** (`perform_recovery_check`, `write_transfer_log`).
* **`utils.c` (Utility Layer):**
    * Contains generic, reusable helper functions like `write_string`, `read_client_input`, and `set_record_lock`.

---

## 🛡️ Data Integrity & ACID Properties

This project was built to meet strict requirements for data integrity.
* **A - Atomicity (All or Nothing):**
    * Implemented for `handle_transfer_funds` using a **Write-Ahead Log (WAL)**.
    * **Flow:**
        1. A `LOG_START` record is written to `transfer_log.dat`.
        2. The debit is written to `accounts.dat`.
        3. The credit is written to `accounts.dat`.
        4. A `LOG_COMMIT` record is written to `transfer_log.dat`.
    * **Recovery:** On startup, `perform_recovery_check()` reads the log. If it finds any `LOG_START` without a `LOG_COMMIT`, it **rolls back the transaction** by refunding the sender. This makes the transfer crash-proof.
* **C - Consistency:**
    * Enforced by application-level logic *before* any database write.
    * `is_valid_amount()` prevents non-numeric input.
    * `handle_withdraw` checks `if (amount > balance)`.
    * `handle_transfer_funds` checks `if (!receiver_account.isActive)`.
    * `handle_add_user` checks for unique email addresses.
* **I - Isolation:**
    * Implemented using `fcntl` **record-level locking**.
    * When `handle_deposit` runs, it locks only the specific byte-range for that one account.
    * `handle_transfer_funds` atomically locks *both* the sender's and receiver's records, forcing other threads to wait and preventing any interference.
* **D - Durability:**
    * All data is written to disk using the `write()` system call. All committed transactions (and the log itself) are persistent and will survive a server restart.

---

## 🚀 Features by Role
* **Administrator (`admin.c`):**
    * Add new users (Employee, Manager, Customer).
    * Modify any user's details and role.
    * Activate/Deactivate any user account.
* **Manager (`manager.c`):**
    * Assign pending loan applications to Employees.
    * Review and resolve customer feedback.
    * Activate/Deactivate Customer accounts.
* **Employee (`employee.c`):**
    * Add new Customer accounts.
    * Modify Customer details (KYC).
    * View assigned loans and Process them (Approve/Reject).
* **Customer (`customer.c`):**
    * View Balance, Deposit, and Withdraw funds.
    * Transfer funds to other customers (Atomically).
    * View detailed transaction history.
    * Apply for loans and view their status.
    * Submit feedback.
* **Shared (`shared.c`):**
    * All roles can view their personal details and change their password.


## 🛠️ How to Compile and Run

### 1. Compilation
Compile all modules and link them into three final executables.

```bash
# 1. Create directories
mkdir -p obj data

# 2. Compile all .c files into .o files
gcc -Iinclude -Wall -c src/utils.c       -o obj/utils.o
gcc -Iinclude -Wall -c src/model.c       -o obj/model.o
gcc -Iinclude -Wall -c src/shared.c      -o obj/shared.o
gcc -Iinclude -Wall -c src/customer.c    -o obj/customer.o
gcc -Iinclude -Wall -c src/employee.c    -o obj/employee.o
gcc -Iinclude -Wall -c src/manager.c     -o obj/manager.o
gcc -Iinclude -Wall -c src/admin.c       -o obj/admin.o
gcc -Iinclude -Wall -c src/controller.c  -o obj/controller.o
gcc -Iinclude -Wall -c src/server.c      -o obj/server.o
gcc -Iinclude -Wall -c src/client.c      -o obj/client.o
gcc -Iinclude -Wall -c src/admin_util.c  -o obj/admin_util.o

# 3. Link the executables
gcc obj/admin_util.o obj/model.o obj/utils.o -o init_data
gcc obj/server.o obj/controller.o obj/admin.o obj/manager.o obj/employee.o obj/customer.o obj/shared.o obj/model.o obj/utils.o -o server -lpthread
gcc obj/client.o obj/utils.o -o client
