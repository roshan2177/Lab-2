/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);


void test_display(){
    
}
# define CHAT_ROWS 20
# define CHAT_COLS 64
void fbclear() {
  for (int row = 0; row < 24; row++) {
      for (int col = 0; col < 64; col++) {
          fbputchar(' ', row, col, 0);  // 用空格覆盖整个屏幕
      }
  }
}
void fbclear_chat() {
  for (int row = 0; row < CHAT_ROWS; row++) {
      for (int col = 0; col < CHAT_COLS; col++) {
          fbputchar(' ', row, col, 0);  // 用空格覆盖整个屏幕
      }
  }
}
void fbclear_input(){
  for (int row = CHAT_ROWS + 1; row < 24; row++) {
      for (int col = 0; col < 64; col++) {
          fbputchar(' ', row, col, 0);  // 用空格覆盖整个屏幕
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
        // chat_buffer[line][len] = '\0';

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

#define INPUT_BUFFER_SIZE  256
// #define CHAT_COLS 64
#define INPUT_ROWS 2
char input_buffer[INPUT_BUFFER_SIZE];
char display_buffer[INPUT_ROWS][CHAT_COLS];
char previous_display_buffer[INPUT_ROWS][CHAT_COLS];
int cursor_pos = 0;
int display_start = 0;
// int input_length = 0;

void update_display_buffer(){
  for (int i = 0; i < CHAT_ROWS; i++)
  {
    memset(display_buffer[i], ' ', CHAT_COLS);
    display_buffer[i][CHAT_COLS - 1] = '\0';
  }
  int start_idx = (cursor_pos < CHAT_COLS * INPUT_ROWS) ? 0 : ((cursor_pos ) / CHAT_COLS - 1 ) * CHAT_COLS;
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
// void print_display_buffer(){
//   for(int row = 0; row < INPUT_ROWS; row++) {
//     for(int col = 0; col < CHAT_COLS; col++) {
//       if (display_buffer[row][col] != previous_display_buffer[row][col]) {
//         fbputchar(display_buffer[row][col], row + CHAT_ROWS + 1, col);
//         previous_display_buffer[row][col] = display_buffer[row][col];
//       }
//     }
//   }
// }
#include <pthread.h>

pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
#include<stdbool.h>
void print_display_buffer() {
  pthread_mutex_lock(&display_mutex); // 加锁，确保线程安全
  // fbopen();
  int cursor_row = cursor_pos / CHAT_COLS + CHAT_ROWS + 1;
  int cursor_col = cursor_pos % CHAT_COLS;
  fbclear_input();
  for(int row = 0; row < INPUT_ROWS; row++) {
    // fbputs_with_cursor(display_buffer[row], row + CHAT_ROWS + 1, 0, cursor_row, cursor_col);
    // for(int col = 0; col < CHAT_COLS; col++) {
      
      // if (display_buffer[row][col] != previous_display_buffer[row][col]) {
      //   // bool is_cursor = (row * CHAT_COLS + col) == cursor_pos;
      //   bool is_cursor = 0;
      //   // printf("row: %d, col: %d, is_cursor: %d\n", row, col, is_cursor);
      //   // printf("Char: %d\n", display_buffer[row][col]);
      //   fbputchar(display_buffer[row][col], row + CHAT_ROWS + 1, col, is_cursor);
      //   // fbputchar('=', CHAT_ROWS, col, 0);
      //   // fbputchar(display_buffer[row][col], row + CHAT_ROWS + 1, col);
      //   previous_display_buffer[row][col] = display_buffer[row][col];
      // }
    // }
  }

  pthread_mutex_unlock(&display_mutex); // 解锁
}

void handle_input(char c){
  if (c == '\n') {
    // send message
    printf("send message: %s\n", input_buffer);
    // TODO: send message to server
    cursor_pos = 0;
    handle_chat_message(input_buffer);
    memset(input_buffer, 0, sizeof(input_buffer));
    update_display_buffer();
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
     else if (c == 27){
      // left arrow
      if (cursor_pos > 0) {
        cursor_pos--;
        update_display_buffer();
        print_display_buffer();
      } }
      else if(c == 91){
        // right arrow
        if (cursor_pos < INPUT_BUFFER_SIZE - 1 && input_buffer[cursor_pos] != '\0') {
          cursor_pos++;
          update_display_buffer();
          print_display_buffer();
        }
      } else {
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
int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  /* Draw rows of asterisks across the top and bottom of the screen */
  // for (col = 0 ; col < 64 ; col++) {
  //   fbputchar('*', 0, col, 0);
  //   fbputchar('*', 23, col, 0);
  // }

  // fbputs("Hello CSEE 4840 World!", 4, 10);
  fbclear();
  for(int col = 0; col < 64; col++) {
    fbputchar('=', CHAT_ROWS, col, 0);
  }
  handle_chat_message("Welcome to the chat room!");
  handle_chat_message("Type your message and press Enter to send.");
  handle_input('a');
  // for(int i = 0; i < 3; i++) {
  //     handle_input('a');
  //     handle_input('b');
  // }
  // for(int i = 0; i < 32; i++) {
  //     handle_input('a');
  //     handle_input('b');
  // }
  // handle_input(27);
  // handle_input('\b');
  // handle_input(27);
  // handle_input('\n');
  // for(int i = 0; i < 32; i++) {
  //   handle_input('a');
  //   handle_input('b');
  // }  
  // handle_input(27);
  // /* Open the keyboard */
  // if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
  //   fprintf(stderr, "Did not find a keyboard\n");
  //   exit(1);
  // }
    
  // /* Create a TCP communications socket */
  // if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
  //   fprintf(stderr, "Error: Could not create socket\n");
  //   exit(1);
  // }

  // /* Get the server address */
  // memset(&serv_addr, 0, sizeof(serv_addr));
  // serv_addr.sin_family = AF_INET;
  // serv_addr.sin_port = htons(SERVER_PORT);
  // if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
  //   fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
  //   exit(1);
  // }

  // /* Connect the socket to the server */
  // if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
  //   fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
  //   exit(1);
  // }

  // /* Start the network thread */
  // pthread_create(&network_thread, NULL, network_thread_f, NULL);

  // /* Look for and handle keypresses */
  // for (;;) {
  //   libusb_interrupt_transfer(keyboard, endpoint_address,
	// 		      (unsigned char *) &packet, sizeof(packet),
	// 		      &transferred, 0);
  //   if (transferred == sizeof(packet)) {
  //     sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
	//       packet.keycode[1]);
  //     printf("%s\n", keystate);
  //     fbputs(keystate, 6, 0);
  //     if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	// break;
  //     }
  //   }
  // }

  // /* Terminate the network thread */
  // pthread_cancel(network_thread);

  // /* Wait for the network thread to finish */
  // pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    fbputs(recvBuf, 8, 0);
  }

  return NULL;
}

