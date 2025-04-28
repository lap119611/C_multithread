#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 10000
#define BUFFER_SIZE 1024

volatile int running = 1;

void *thread1_func(void *arg) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Thread 1: Listening on port %d...\n", PORT);

    while (running) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd == -1) {
            if (running) perror("Accept failed");
            continue;
        }

        printf("Thread 1: Connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        while (1) {
            ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    printf("Thread 1: Client disconnected\n");
                } else {
                    perror("Receive failed");
                }
                close(client_fd);
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Thread 1: Received: %s\n", buffer);
        }
    }

    close(server_fd);
    return NULL;
}

void *thread2_func(void *arg) {
    while (running) {
        int sock;
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        snprintf(buffer, BUFFER_SIZE, "Current time: %02d:%02d:%02d",
                 t->tm_hour, t->tm_min, t->tm_sec);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("Socket creation failed");
            sleep(1);
            continue;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Connect failed");
            close(sock);
            sleep(1);
            continue;
        }

        send(sock, buffer, strlen(buffer), 0);
        printf("Thread 2: Sent: %s\n", buffer);
        close(sock);
        sleep(1);
    }
    return NULL;
}

void *thread3_func(void *arg) {
    char buffer[BUFFER_SIZE];

    while (running) {
        printf("Thread 3: Enter a message (type 'quit' to exit): ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("Input failed");
            continue;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        if (strcmp(buffer, "quit") == 0) {
            running = 0;
            break;
        }

        int sock;
        struct sockaddr_in server_addr;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("Socket creation failed");
            continue;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Connect failed");
            close(sock);
            continue;
        }

        send(sock, buffer, strlen(buffer), 0);
        printf("Thread 3: Sent: %s\n", buffer);
        close(sock);
    }
    return NULL;
}

int main() {
    pthread_t thread1, thread2, thread3;

    pthread_create(&thread1, NULL, thread1_func, NULL);
    pthread_create(&thread2, NULL, thread2_func, NULL);
    pthread_create(&thread3, NULL, thread3_func, NULL);

    pthread_join(thread3, NULL);

    running = 0;
    pthread_cancel(thread1);
    pthread_cancel(thread2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Application exiting...\n");
    return 0;
}