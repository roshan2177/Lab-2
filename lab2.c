#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "usbkeyboard.h"

#define SERVER_HOST "128.59.19.114"  // Change if needed
#define SERVER_PORT 42000
#define BUFFER_SIZE 128
#define MAX_INPUT_LENGTH 128

int sockfd; /* Socket file descriptor */
struct libusb_device_handle *keyboard;
uint8_t endpoint_address;
pthread_t network_thread;
void *network_thread_f(void *);

/* Input Buffer */
char input_buffer[MAX_INPUT_LENGTH];
int buffer_pos = 0;
int use_terminal_input = 0;  // Fallback if USB keyboard isn't found

/* Function to Convert USB Keycodes to ASCII */
char usb_to_ascii(uint8_t keycode, uint8_t modifiers) {
    static char ascii_map[256] = {0};
    ascii_map[0x04] = 'a'; ascii_map[0x05] = 'b'; ascii_map[0x06] = 'c';
    ascii_map[0x07] = 'd'; ascii_map[0x08] = 'e'; ascii_map[0x09] = 'f';
    ascii_map[0x0A] = 'g'; ascii_map[0x0B] = 'h'; ascii_map[0x0C] = 'i';
    ascii_map[0x0D] = 'j'; ascii_map[0x0E] = 'k'; ascii_map[0x0F] = 'l';
    ascii_map[0x10] = 'm'; ascii_map[0x11] = 'n'; ascii_map[0x12] = 'o';
    ascii_map[0x13] = 'p'; ascii_map[0x14] = 'q'; ascii_map[0x15] = 'r';
    ascii_map[0x16] = 's'; ascii_map[0x17] = 't'; ascii_map[0x18] = 'u';
    ascii_map[0x19] = 'v'; ascii_map[0x1A] = 'w'; ascii_map[0x1B] = 'x';
    ascii_map[0x1C] = 'y'; ascii_map[0x1D] = 'z';
    ascii_map[0x2C] = ' ';  // Spacebar
    ascii_map[0x28] = '\n'; // Enter key
    ascii_map[0x2A] = '\b'; // Backspace

    if (modifiers & USB_LSHIFT || modifiers & USB_RSHIFT) {
        return ascii_map[keycode] - 32;  // Convert lowercase to uppercase
    }
    return ascii_map[keycode];
}

/* Connect to the chat server */
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

/* This is to send the messages  to the Server */
void send_message_to_server(char *message) {
    int len = strlen(message);
    if (write(sockfd, message, len) != len) {
        perror("Error: Failed to send message to server");
    } else {
        printf("[Sent]: %s\n", message);
    }
}

/* Network Thread: This is used to receives Messages from the Server */
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
    struct usb_keyboard_packet packet;
    int transferred;

    /* This is to  to Open the USB Keyboard */
    if ((keyboard = openkeyboard(&endpoint_address)) == NULL) {
        printf("No USB keyboard found. Falling back to terminal input.\n");
        use_terminal_input = 1;  // Enable standard input
    }

    /* Used to connect to the chat server */
    connect_to_server();

    /* Used to start network thread */
    pthread_create(&network_thread, NULL, network_thread_f, NULL);

    if (use_terminal_input) {
        /* Here is the Terminal-based Input Handling */
        printf("Chat client started. Type messages and press Enter to send.\n");
        printf("Type '/exit' to quit.\n");

        while (1) {
            printf("> ");
            fflush(stdout);

            if (fgets(input_buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
                printf("Error reading input.\n");
                break;
            }

            /* Remove newline character */
            input_buffer[strcspn(input_buffer, "\n")] = '\0';

            /* Listed is the exit condition */
            if (strcmp(input_buffer, "/exit") == 0) {
                printf("Exiting chat client.\n");
                break;
            }

            /* Used to Send Message */
            send_message_to_server(input_buffer);
        }
    } else {
        /* This section is the USB Keyboard Input Handling */
        printf("Chat client started. Use the USB keyboard to type messages.\n");

        while (1) {
            libusb_interrupt_transfer(keyboard, endpoint_address,
                                      (unsigned char *) &packet, sizeof(packet),
                                      &transferred, 0);

            if (transferred == sizeof(packet)) {
                char key = usb_to_ascii(packet.keycode[0], packet.modifiers);
                if (key) {
                    if (key == '\b' && buffer_pos > 0) {  // Backspace handling
                        buffer_pos--;
                        input_buffer[buffer_pos] = '\0';
                        printf("\b \b"); // Erases the last charecter in the input buffer
                    } else if (key == '\n') {  // Uses enter to sent message
                        input_buffer[buffer_pos] = '\0';
                        send_message_to_server(input_buffer);
                        buffer_pos = 0;  // Used to rest the buffer
                    } else if (buffer_pos < MAX_INPUT_LENGTH - 1) {  
                        input_buffer[buffer_pos++] = key;
                        input_buffer[buffer_pos] = '\0';
                        printf("%c", key);  // Used to pring the typed charecter
                    }
                }

                /* Use to escape*/
                if (packet.keycode[0] == 0x29) {
                    break;
                }
            }
        }
    }

    /* Clean up */
    pthread_cancel(network_thread);
    pthread_join(network_thread, NULL);
    close(sockfd);

    return 0;
}
