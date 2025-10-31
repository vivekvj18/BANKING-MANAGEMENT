# 🏦 Banking Management System (C)

![Language](https://img.shields.io/badge/Language-C-blue.svg) 
![Build](https://img.shields.io/badge/Build-gcc-lightgrey.svg) 
![Concurrency](https://img.shields.io/badge/Concurrency-pthreads-blue.svg)

A **multi-threaded, socket-based Banking Management System** built in C.  
This project simulates a real-world banking environment using a **professional 3-tier modular architecture** ensuring clean separation of concerns and maintainability.

---

## 🧩 Core Highlights

- **🧱 Modular 3-Tier Design:** Network Layer (`server.c`), Logic Layer (`controller.c`, `admin.c`, etc.), and Data Layer (`model.c`).
- **🌐 Socket Programming:** Uses TCP/IP sockets to handle multiple clients concurrently.
- **🧵 Multithreading:** Uses `pthreads` to handle each client in a separate thread.
- **💾 Full ACID Compliance:** Ensures data integrity using file locks and a Write-Ahead Log (WAL).
- **🔒 Concurrency Control:** Uses `fcntl` record locking and `pthread_mutex_t` for session synchronization.
- **🧠 Robust Error Handling:** Validates inputs, formats, and checks return values of all system calls.
- **🧾 Crash Recovery:** Built-in recovery mechanism for unfinished transactions using WAL.

---

## 🏛️ System Architecture

This project follows a **3-tier separation of concerns** model:  
Network → Controller → Data.

*(You can add your architecture diagram here, e.g., `![Architecture](blueprint.png)`)*
  
### Structure Overview
- **`server.c` (Network Layer):**
  - Handles socket creation, binding, listening, and accepting clients.
  - Spawns a new `pthread` per client and passes control to controllers.

- **`controller.c` (Routing & Session Layer):**
  - Handles login, validates user roles, and manages session list using mutex locks.
  - Routes users to their respective role menus (`admin_menu`, `customer_menu`, etc.).

- **Role Controllers (`admin.c`, `customer.c`, `employee.c`, `manager.c`):**
  - Each role file handles its specific features and menu loops.

- **`shared.c` (Shared Logic):**
  - Contains cross-role features like `handle_add_user`, `handle_change_password`, and validation helpers.

- **`model.c` (Data Access Layer):**
  - Directly interacts with `.dat` files.
  - Implements data access functions, WAL, and crash recovery routines.

- **`utils.c` (Utility Layer):**
  - Contains common I/O helpers and record-locking utilities.

---

## 🛡️ Data Integrity & ACID Properties

### **A - Atomicity**
- Implemented in `handle_transfer_funds` using **Write-Ahead Logging (WAL)**.
- Transaction flow:
  1. Log `LOG_START`
  2. Debit sender account
  3. Credit receiver account
  4. Log `LOG_COMMIT`
- On crash recovery, unfinished transactions are rolled back automatically.

### **C - Consistency**
- Enforced before writing:
  - Validates numeric inputs
  - Prevents overdrafts (`amount > balance`)
  - Ensures receiver account is active
  - Maintains unique emails for new users

### **I - Isolation**
- Record-level file locking using `fcntl`.
- Only specific account records are locked during operations.
- Dual locking in transfers ensures isolation across concurrent threads.

### **D - Durability**
- All data persists using `write()` system calls.
- Committed transactions and logs survive restarts.

---

## 🚀 Features by Role

| Role | Features |
|------|-----------|
| **Administrator** | Add/modify users, activate/deactivate accounts |
| **Manager** | Assign loans, manage employees, handle customer feedback |
| **Employee** | Add/edit customer accounts, process loans |
| **Customer** | Deposit, withdraw, transfer funds atomically, view history, apply for loans |
| **Shared** | Change password, view personal details |

---

## 📁 Project Structure

banking-system/
├── include/ # Header files
│ ├── admin.h
│ ├── controller.h
│ ├── customer.h
│ ├── employee.h
│ ├── manager.h
│ ├── model.h
│ ├── shared.h
│ ├── utils.h
│ └── constants.h
│
├── src/ # Source files
│ ├── admin.c
│ ├── admin_util.c
│ ├── client.c
│ ├── controller.c
│ ├── customer.c
│ ├── employee.c
│ ├── manager.c
│ ├── model.c
│ ├── server.c
│ ├── shared.c
│ └── utils.c
│
├── data/ # Runtime data files
│ ├── accounts.dat
│ ├── users.dat
│ ├── transfer_log.dat
│ └── ...
│
├── obj/ # Compiled object files
│ ├── admin.o
│ ├── model.o
│ └── ...
│
├── Makefile # Compilation script
├── init_data # Admin utility executable
├── server # Server executable
└── client # Client executable


---

## 🛠️ Compilation & Execution

You can compile and run the entire project using the provided **Makefile**.

### **Makefile**
```makefile
# Makefile for Modular Banking System

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lpthread

SRC_DIR = src
OBJ_DIR = obj
DATA_DIR = data

SERVER_SRCS = $(SRC_DIR)/utils.c \
              $(SRC_DIR)/model.c \
              $(SRC_DIR)/shared.c \
              $(SRC_DIR)/customer.c \
              $(SRC_DIR)/employee.c \
              $(SRC_DIR)/manager.c \
              $(SRC_DIR)/admin.c \
              $(SRC_DIR)/controller.c \
              $(SRC_DIR)/server.c

CLIENT_SRCS = $(SRC_DIR)/client.c $(SRC_DIR)/utils.c
INIT_SRCS = $(SRC_DIR)/admin_util.c $(SRC_DIR)/model.c $(SRC_DIR)/utils.c

SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CLIENT_SRCS))
INIT_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(INIT_SRCS))

SERVER_EXE = server
CLIENT_EXE = client
INIT_EXE = init_data

all: setup $(SERVER_EXE) $(CLIENT_EXE) $(INIT_EXE)

$(SERVER_EXE): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENT_EXE): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(INIT_EXE): $(INIT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

setup:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DATA_DIR)

.PHONY: clean all setup
clean:
	rm -f $(OBJ_DIR)/*.o $(SERVER_EXE) $(CLIENT_EXE) $(INIT_EXE)
	rm -rf $(DATA_DIR)
