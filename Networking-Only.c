#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_HOST "128.59.19.114"  
#define SERVER_PORT 42000
#define BUFFER_SIZE 128
#define MAX_INPUT_LENGTH 128

int sockfd; 
pthread_t network_thread;
void *network_thread_f(void *);


void connect_to_server() {
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error: Could not create socket");
        exit(1);
    }

    printf("Socket created successfully.\n");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: Invalid IP address format or unreachable host \"%s\"\n", SERVER_HOST);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error: connect() failed. Is the server running?");
        exit(1);
    }

    printf("Successfully connected to chat server at %s:%d\n", SERVER_HOST, SERVER_PORT);
}


void send_message_to_server(char *message) {
    int len = strlen(message);
    if (write(sockfd, message, len) != len) {
        perror("Error: Failed to send message to server");
    } else {
        printf("[Sent]: %s\n", message);
    }
}


void *network_thread_f(void *ignored) {
    char recvBuf[BUFFER_SIZE];
    int n;

    printf("Network thread started, waiting for messages...\n");

    while (1) {
        n = read(sockfd, recvBuf, BUFFER_SIZE - 1);
        if (n <= 0) {
            perror("Error: Connection lost or server closed connection");
            close(sockfd);
            pthread_exit(NULL);
        }
        recvBuf[n] = '\0';
        printf("\n[Received]: %s\n", recvBuf);
    }

    return NULL;
}

int main() {
    char input_buffer[MAX_INPUT_LENGTH];

    
    connect_to_server();

    
    pthread_create(&network_thread, NULL, network_thread_f, NULL);

    
    printf("Chat client started. Type messages and press Enter to send.\n");
    printf("Type '/exit' to quit.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input_buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
            printf("Error reading input.\n");
            break;
        }

        
        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        
        if (strcmp(input_buffer, "/exit") == 0) {
            printf("Exiting chat client.\n");
            break;
        }

        send_message_to_server(input_buffer);
    }

    
    pthread_cancel(network_thread);
    pthread_join(network_thread, NULL);
    close(sockfd);

    return 0;
}
