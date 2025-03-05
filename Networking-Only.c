/* CSEE 4840 Lab 2 - Modified to Support Simulated Input Without FPGA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Chat Server Configuration */
#define SERVER_HOST "128.59.19.114"  // Change to the correct IP if needed
#define SERVER_PORT 42000
#define BUFFER_SIZE 128
#define MAX_INPUT_LENGTH 128

int sockfd; /* Socket file descriptor */
struct libusb_device_handle *keyboard;
uint8_t endpoint_address;
pthread_t network_thread;
void *network_thread_f(void *);

/* Input Buffer for Typed Characters */
char input_buffer[MAX_INPUT_LENGTH];
int buffer_pos = 0;

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

int main() {
    int err;
    struct sockaddr_in serv_addr;
    struct usb_keyboard_packet packet;
    int transferred;
    
    /* Try to Open the Keyboard */
    if ((keyboard = openkeyboard(&endpoint_address)) == NULL) {
        printf("No USB keyboard found. Simulating input...\n");
        keyboard = NULL; // No real keyboard, set to NULL
    }
    
    /* Create a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Could not create socket\n");
        exit(1);
    }

    /* Set up server address */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: Invalid server IP address\n");
        exit(1);
    }

    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Error: Could not connect to chat server\n");
        exit(1);
    }

    /* Start the network thread */
    pthread_create(&network_thread, NULL, network_thread_f, NULL);

    /* Simulate Keyboard Input If No USB Keyboard Is Found */
    if (!keyboard) {
        char simulated_keys[] = "Hello from simulated keyboard!\n";
        for (int i = 0; simulated_keys[i] != '\0'; i++) {
            printf("Simulated Key Pressed: %c\n", simulated_keys[i]);
            input_buffer[buffer_pos++] = simulated_keys[i];
            input_buffer[buffer_pos] = '\0';
            usleep(100000); // Simulate typing delay
        }

        /* Simulate pressing Enter */
        printf("\nSimulated Enter Key Pressed.\n");
        write(sockfd, input_buffer, buffer_pos);
        printf("Message Sent: %s\n", input_buffer);
        buffer_pos = 0;
    } else {
        /* Real Keyboard Input Handling */
        printf("Chat client started. Type messages and press Enter to send.\n");
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
                        printf("\b \b"); // Erase last character in terminal
                    } else if (key == '\n') {  // Enter key sends message
                        input_buffer[buffer_pos] = '\0';
                        write(sockfd, input_buffer, buffer_pos);
                        printf("\nMessage Sent: %s\n", input_buffer);
                        buffer_pos = 0;  // Reset buffer
                    } else if (buffer_pos < MAX_INPUT_LENGTH - 1) {  
                        input_buffer[buffer_pos++] = key;
                        input_buffer[buffer_pos] = '\0';
                        printf("%c", key);  // Print typed character
                    }
                }

                /* Exit on ESC key (0x29) */
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

/* Network thread to receive messages */
void *network_thread_f(void *ignored) {
    char recvBuf[BUFFER_SIZE];
    int n;
    
    while ((n = read(sockfd, recvBuf, BUFFER_SIZE - 1)) > 0) {
        recvBuf[n] = '\0';
        printf("\nMessage Received: %s\n", recvBuf);
    }

    return NULL;
}


