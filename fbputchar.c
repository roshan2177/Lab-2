/*
 * fbputchar: Framebuffer character generator (Modified for macOS/Linux without framebuffer)
 *
 * Since macOS does not have a framebuffer (/dev/fb0), this version replaces framebuffer
 * operations with terminal-based text output using printf().
 */

#include "fbputchar.h"
#include <stdio.h>

/* 
 * Open the framebuffer to prepare it to be written to.
 * Since macOS does not have a framebuffer, we return 0 to indicate success.
 */
int fbopen() {
    return 0;  // Stub for macOS, does nothing
}

/*
 * Simulated function to draw a character on the screen.
 * Since there's no framebuffer, this prints the character to the terminal.
 */
void fbputchar(char c, int row, int col) {
    printf("%c", c);  // Print character to terminal instead of framebuffer
}

/*
 * Simulated function to write a string on the screen.
 * Since there's no framebuffer, this prints the string to the terminal.
 */
void fbputs(const char *s, int row, int col) {
    printf("%s\n", s);  // Print string to terminal instead of framebuffer
}
