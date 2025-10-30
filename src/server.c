// src/server.c
#include "common.h"
#include "controller.h" // For handle_client
#include "utils.h"      // For write_string

// --- Main Server Setup (Threaded) ---
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    write_string(STDOUT_FILENO, "Server listening on port 8080 (Threaded Mode)...\n");

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept"); continue;
        }

        int* client_sock_ptr = (int*)malloc(sizeof(int));
        *client_sock_ptr = new_socket;

        // We now pass the handle_client function from our controller
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_sock_ptr) < 0) {
            perror("pthread_create failed");
            close(new_socket);
            free(client_sock_ptr);
        } else {
            pthread_detach(thread_id);
            write_string(STDOUT_FILENO, "New client connected, thread created.\n");
        }
    }
    
    close(server_fd);
    return 0;
}