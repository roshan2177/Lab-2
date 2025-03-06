/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Haoming Ma (hm3070), Roshan Prakash (rp3187)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "usbkeyboard.h"

#define SERVER_HOST "128.59.19.114" // IP address given to connect
#define SERVER_PORT 42000
#define BUFFER_SIZE 128
#define MAX_INPUT_LENGTH 128

int sockfd;
struct libusb_device_handle *keyboard;
uint8_t endpoint_address;
pthread_t network_thread;
void *network_thread_f(void *);



void send_message_to_server(char *message);

# define CHAT_ROWS 20
# define CHAT_COLS 64
void fbclear() {
  for (int row = 0; row < 24; row++) {
      for (int col = 0; col < 64; col++) {
          fbputchar(' ', row, col, 0); 
      }
  }
}
void fbclear_chat() {
  for (int row = 0; row < CHAT_ROWS; row++) {
      for (int col = 0; col < CHAT_COLS; col++) {
          fbputchar(' ', row, col, 0);  
      }
  }
}
void fbclear_input(){
  for (int row = CHAT_ROWS + 1; row < 24; row++) {
      for (int col = 0; col < 64; col++) {
          fbputchar(' ', row, col, 0);  
      }
  }


}
char char_buffer[CHAT_ROWS][CHAT_COLS];
int chat_start = 0;
int chat_count = 0;
void add_chat_message(const char *msg){

    int msg_len = strlen(msg);
    int start = 0;
    while(start < msg_len) {
        int len = (msg_len - start < CHAT_COLS) ? (msg_len - start) : CHAT_COLS;
        int line  = (chat_start + chat_count) % CHAT_ROWS;
        strncpy(char_buffer[line], msg + start, len);
        for(int i = len; i < CHAT_COLS; i++) {
            char_buffer[line][i] = '\0';
        }
        if (chat_count < CHAT_ROWS) {
            chat_count++;
        } else{
            chat_start = (chat_start + 1) % CHAT_ROWS;
        }
        start += len;

    }
}
void redraw_chat(){
    fbclear_chat();
    for(int i = 0; i < chat_count; i++) {
        int line = (chat_start + i) % CHAT_ROWS;
        fbputs(char_buffer[line], i, 0);
    }
}
#include <pthread.h>

pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;
void handle_chat_message(const char *msg){
pthread_mutex_lock(&chat_mutex);
add_chat_message(msg);
redraw_chat();
pthread_mutex_unlock(&chat_mutex);
}

#define INPUT_BUFFER_SIZE 256
// #define CHAT_COLS 64
#define INPUT_ROWS 2
char input_buffer[INPUT_BUFFER_SIZE];
char display_buffer[INPUT_ROWS][CHAT_COLS];
char previous_display_buffer[INPUT_ROWS][CHAT_COLS];
int cursor_pos = 0;
int display_start = 0;
int start_idx = 0;
void update_display_buffer(){

  for (int i = 0; i < INPUT_ROWS; i++)
  {
    memset(display_buffer[i], ' ', CHAT_COLS);
  }
  start_idx = (cursor_pos < CHAT_COLS * INPUT_ROWS) ? 0 : ((cursor_pos ) / CHAT_COLS - 1 ) * CHAT_COLS;
  int len_to_display = INPUT_ROWS * CHAT_COLS > INPUT_BUFFER_SIZE - start_idx ? INPUT_BUFFER_SIZE - start_idx : INPUT_ROWS * CHAT_COLS;
  if(len_to_display > 0){
    int line_idx = 0;
    for (int i = 0; i < len_to_display; i++)
    {
      display_buffer[line_idx][i % CHAT_COLS] = input_buffer[start_idx + i];
      if ((i + 1) % CHAT_COLS == 0)
      {
        line_idx++;
      }
    }
    
  }
}

#include <pthread.h>

pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
#include<stdbool.h>
char tmp[CHAT_COLS + 1];
void print_display_buffer() {

  pthread_mutex_lock(&display_mutex); 
  int cursor_row = cursor_pos / CHAT_COLS + CHAT_ROWS + 1 - start_idx / CHAT_COLS;
  int cursor_col = cursor_pos % CHAT_COLS + 1;
  fbclear_input();
  for(int row = 0; row < INPUT_ROWS; row++) {
    memcpy(tmp, display_buffer[row], CHAT_COLS);
    tmp[CHAT_COLS] = '\0';
    fbputs_with_cursor(tmp, row + CHAT_ROWS + 1, 0, cursor_row, cursor_col);

  }

  pthread_mutex_unlock(&display_mutex); 
}

void handle_input(char c){

  if (c == '\n') {
    // send message
    printf("send message: %s\n", input_buffer);
    // TODO: send message to server
    send_message_to_server(input_buffer);
    cursor_pos = 0;
    // handle_chat_message(input_buffer);
    memset(input_buffer, 0, sizeof(input_buffer));
    // fbclear_input();
    // printf("input_buffer: %s\n", input_buffer);
    update_display_buffer();
    // printf("display_buffer: %s\n", display_buffer[0]);
    print_display_buffer();
    
    return;
  }else if (c == '\b') {
    // backspace
        if (cursor_pos > 0) {
        cursor_pos--;
        for (int i = cursor_pos; i < INPUT_BUFFER_SIZE - 1; i++) {
            input_buffer[i] = input_buffer[i + 1];
        }
        input_buffer[INPUT_BUFFER_SIZE - 1] = '\0';
        // input_buffer[cursor_pos] = '\0';
        update_display_buffer();
        print_display_buffer();
        return;
        }
    }
     else if (c == 2){
      // left arrow
      if (cursor_pos > 0) {
        cursor_pos--;
        update_display_buffer();
        print_display_buffer();
      } }
      else if(c == 3){
        // right arrow
        if (cursor_pos < INPUT_BUFFER_SIZE - 1 && input_buffer[cursor_pos] != '\0') {
          cursor_pos++;
          update_display_buffer();
          print_display_buffer();
        }
      } 
      else {
        // char input
        if (cursor_pos < INPUT_BUFFER_SIZE - 1) {
          int idx = cursor_pos;
          for (int i = INPUT_BUFFER_SIZE - 1; i > idx; i--) {
            input_buffer[i] = input_buffer[i - 1];
          }
          input_buffer[cursor_pos] = c;
          

          cursor_pos++;
          update_display_buffer();

          print_display_buffer();
        }
      }
    
  
  return;

}


int use_terminal_input = 0;  // Fallback if USB keyboard isn't found


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

    ascii_map[0x1E] = '1'; ascii_map[0x1F] = '2'; ascii_map[0x20] = '3';
    ascii_map[0x21] = '4'; ascii_map[0x22] = '5'; ascii_map[0x23] = '6';
    ascii_map[0x24] = '7'; ascii_map[0x25] = '8'; ascii_map[0x26] = '9';
    ascii_map[0x27] = '0';

    ascii_map[0x2D] = '-'; ascii_map[0x2E] = '='; ascii_map[0x2F] = '[';
    ascii_map[0x30] = ']'; ascii_map[0x31] = '\\'; ascii_map[0x33] = ';';
    ascii_map[0x34] = '\''; ascii_map[0x35] = '`'; ascii_map[0x36] = ',';
    ascii_map[0x37] = '.'; ascii_map[0x38] = '/';
    ascii_map[0x2C] = ' ';  
    ascii_map[0x28] = '\n'; 
    ascii_map[0x2A] = '\b'; 
    ascii_map[0x50] = 2;
    ascii_map[0x4F] = 3;

    if (modifiers & USB_LSHIFT || modifiers & USB_RSHIFT) {
        if (keycode >= 0x04 && keycode <= 0x1D) {
            return ascii_map[keycode] - 32;  // Uppercase
        } else {
            switch (keycode) {
                case 0x1E: return '!';  // 1 -> !
                case 0x1F: return '@';  // 2 -> @
                case 0x20: return '#';  // 3 -> #
                case 0x21: return '$';  // 4 -> $
                case 0x22: return '%';  // 5 -> %
                case 0x23: return '^';  // 6 -> ^
                case 0x24: return '&';  // 7 -> &
                case 0x25: return '*';  // 8 -> *
                case 0x26: return '(';  // 9 -> (
                case 0x27: return ')';  // 0 -> )
                case 0x2D: return '_';  // - -> _
                case 0x2E: return '+';  // = -> +
                case 0x2F: return '{';  // [ -> {
                case 0x30: return '}';  // ] -> }
                case 0x31: return '|';  // \ -> |
                case 0x33: return ':';  // ; -> :
                case 0x34: return '"';  // ' -> "
                case 0x35: return '~';  // ` -> ~
                case 0x36: return '<';  // , -> <
                case 0x37: return '>';  // . -> >
                case 0x38: return '?';  // / -> ?
            }
        }
    }
    return ascii_map[keycode];
}

/* Used in order to connect to the chat server */

void connect_to_server() {
struct sockaddr_in serv_addr;

sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd < 0) {
perror("Error: Problem in creating socket");
exit(1);
}

printf("Socket created successfully.\n");

memset(&serv_addr, 0, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(SERVER_PORT);

if (inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
fprintf(stderr, "Error: Possibly may be unreachable host or Invalid IP address format \"%s\"\n", SERVER_HOST);
exit(1);
}

if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
perror("Error: connect() failed. Please check whether the server running?");
exit(1);
}

printf("Connected to chat server Successfully at %s:%d\n", SERVER_HOST, SERVER_PORT);
}

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
        handle_chat_message(recvBuf);
    }

    return NULL;
}
# define MAX_KEYS 6
int main() {
    struct usb_keyboard_packet packet;
    int transferred;
    int err, col;

    struct sockaddr_in serv_addr;

    char keystate[12];

    if ((err = fbopen()) != 0) {
      fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
      exit(1);
    }
    fbclear();
    for(int col = 0; col < 64; col++) {
      fbputchar('=', CHAT_ROWS, col, 0);
    }
    handle_chat_message("Welcome to the chat room!");
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

            
            input_buffer[strcspn(input_buffer, "\n")] = '\0';

            /* Listed is the exit condition */
            if (strcmp(input_buffer, "/exit") == 0) {
                printf("Exiting chat client.\n");
                break;
            }

            /* This is used to  to Send Message */
            // send_message_to_server(input_buffer);
        }
    } else {
        /* This section is the USB Keyboard Input Handling */
        printf("Chat client started. Use the USB keyboard to type messages.\n");
        uint8_t prev_keys[MAX_KEYS] = {0};
        while (1) {

            int r = libusb_interrupt_transfer(keyboard, endpoint_address,
                (unsigned char *) &packet, sizeof(packet),
                &transferred, 0);
            if (r != 0) {
                fprintf(stderr, "Error: libusb_interrupt_transfer failed: %s\n", libusb_error_name(r));
                continue;
            }
            if (transferred < sizeof(packet)) {
                fprintf(stderr, "Warning: Incomplete packet received: %d bytes\n", transferred);
                continue;
            }
            if (transferred == sizeof(packet)) {
                // handle_input(usb_to_ascii(packet.keycode[0], packet.modifiers));
                for (int i = 0; i < MAX_KEYS; i++) {
                    uint8_t key = packet.keycode[i];
                    if (key != 0){
                        int already_pressed = 0;
                        for (int j = 0; j < MAX_KEYS; j++) {
                            if (prev_keys[j] == key) {
                                already_pressed = 1;
                                break;
                            }
                        }
                        if (!already_pressed) {
                            handle_input(usb_to_ascii(key, packet.modifiers));
                        }
                    }

                }
                memcpy(prev_keys, packet.keycode, MAX_KEYS);


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

if ((keyboard = openkeyboard(&endpoint_address)) == NULL) {
printf("Cannot find USB keyboard. Openning terminal input.\n");
use_terminal_input = 1; 
}

connect_to_server();
pthread_create(&network_thread, NULL, network_thread_f, NULL);
if (use_terminal_input) {
printf("Startin Chat client. Type in messages and press Enter to send.\n");
printf("To Quit Type '/exit'.\n");

while (1) {
printf("> ");
fflush(stdout);

if (fgets(input_buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
printf("Error: Cannot read input.\n");
break;
}

input_buffer[strcspn(input_buffer, "\n")] = '\0';

if (strcmp(input_buffer, "/exit") == 0) {
printf("Exiting chat client.\n");
break;
}

// send_message_to_server(input_buffer);
}
} else {

printf("Started Chat client. To type messages use the USB keyboard.\n");
uint8_t prev_keys[MAX_KEYS] = {0};
while (1) {

int r = libusb_interrupt_transfer(keyboard, endpoint_address,
(unsigned char *) &packet, sizeof(packet),
&transferred, 0);
if (r != 0) {
fprintf(stderr, "Error: libusb_interrupt_transfer failed: %s\n", libusb_error_name(r));
continue;
}
if (transferred < sizeof(packet)) {
fprintf(stderr, "Warning: Incomplete packet received: %d bytes\n", transferred);
continue;
}
if (transferred == sizeof(packet)) {
// handle_input(usb_to_ascii(packet.keycode[0], packet.modifiers));
for (int i = 0; i < MAX_KEYS; i++) {
uint8_t key = packet.keycode[i];
if (key != 0){
int already_pressed = 0;
for (int j = 0; j < MAX_KEYS; j++) {
if (prev_keys[j] == key) {
already_pressed = 1;
break;
}
}
if (!already_pressed) {
handle_input(usb_to_ascii(key, packet.modifiers));
}
}

}
memcpy(prev_keys, packet.keycode, MAX_KEYS);


/* Use to escape*/
if (packet.keycode[0] == 0x29) {
break;
}
}
}
}

/* clean up for networking portion*/
pthread_cancel(network_thread);

pthread_join(network_thread, NULL);
close(sockfd);

return 0;
}