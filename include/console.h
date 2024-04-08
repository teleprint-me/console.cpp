/**
 * @file console.h
 *
 * @brief Provides console management functionalities including initialization,
 * display mode setting, and input handling.
 *
 */

#pragma once

#ifndef CONSOLE_H
    #define CONSOLE_H

    #include <stdbool.h>
    #include <stdio.h>
    #include <termios.h>

    // Constants for ANSI escape codes

    // ANSI Color codes
    #define ANSI_COLOR_RED              "\x1b[31m"       // r
    #define ANSI_COLOR_GREEN            "\x1b[32m"       // g
    #define ANSI_COLOR_BLUE             "\x1b[34m"       // b
    #define ANSI_COLOR_YELLOW           "\x1b[33m"       // y
    #define ANSI_COLOR_MAGENTA          "\x1b[35m"       // m
    #define ANSI_COLOR_CYAN             "\x1b[36m"       // c
    #define ANSI_COLOR_BLACK            "\x1b[30m"       // k
    #define ANSI_COLOR_WHITE            "\x1b[37m"       //
    #define ANSI_COLOR_GRAY             "\x1b[90m"       // Bright black, often appears as gray
    #define ANSI_COLOR_DARKGRAY         "\x1b[38;5;242m" // A specific shade of gray
    #define ANSI_COLOR_LIGHTGRAY        "\x1b[37m"       // Bright white, can be used as light gray
    #define ANSI_COLOR_RESET            "\x1b[0m"        //

    // Note: ANSI_COLOR_GRAY and ANSI_COLOR_LIGHTGRAY can sometimes be dependent on the terminal's
    // color scheme. The dark gray color (ANSI_COLOR_DARKGRAY) uses 256-color mode syntax, which
    // might not work on all terminals.

    // ANSI styles
    #define ANSI_ITALIC                 "\x1b[3m"
    #define ANSI_BOLD                   "\x1b[1m"

    // ANSI Cursor codes
    #define ANSI_CURSOR_POS_QUERY       "\033[6n" // Query cursor position

    // Constants for handling special characters
    #define REPLACEMENT_CHARACTER_WIDTH 1 // Assuming U+FFFD's display width is 1

// Enumeration for input modes.
enum StateInput {
    STATE_INPUT_NORMAL,
    STATE_INPUT_INSERT
};

// Enumeration for console display modes.
enum StateDisplay {
    STATE_DISPLAY_RESET,  // Reset display mode
    STATE_DISPLAY_ERROR,  // Error message display mode
    STATE_DISPLAY_NORMAL, // Normal/default display mode
    STATE_DISPLAY_INPUT,  // User input display mode
    STATE_DISPLAY_OUTPUT, // Language Model output display mode
    STATE_DISPLAY_PROMPT  // Prompt display mode
};

enum StreamEvent {
    STREAM_EVENT_ESC,
    STREAM_EVENT_ERROR,
    STREAM_EVENT_POLL,
    STREAM_EVENT_INSERT,
    STREAM_EVENT_BACKSPACE,
    STREAM_EVENT_DEL,
    STREAM_EVENT_UP,
    STREAM_EVENT_DOWN,
    STREAM_EVENT_LEFT,
    STREAM_EVENT_RIGHT
};

enum StreamStatus {
    STREAM_STATUS_INIT,
    STREAM_STATUS_ERROR,
    STREAM_STATUS_OK
};

// Struct to encapsulate console modes.
struct ConsoleState {
    enum StateInput   input;   // Current input mode
    enum StateDisplay display; // Current display mode
};

struct ConsoleIO {
    FILE* output;   // stderr or stdout for general output
    FILE* input;    // stdin, respectively
    FILE* teletype; // Direct terminal control
};

struct ConsoleCursor {
    size_t row; // what row in the "page" are we in?
    size_t col; // what col in the row are we in?
};              // consoles cursor position

struct ConsoleLine { // handle character stream
    char*  buffer;   // current active line
    size_t size;     // buffer allocated 'size' bytes
    size_t length;   // number of characters set to 'buffer'
};

struct ConsolePage {
    struct ConsoleLine* lines;  // all lines are buffers, not all buffers are lines
    size_t              length; // total number of lines
};

struct ConsoleStream {
    int                   last;    // last character read into the buffer
    int                   current; // current character read into the buffer
    enum StreamStatus     status;  // EOF, NULL, and spooky other things...
    enum StreamEvent      event;   // poll, error, esc, backspace, left, right, etc...
    struct ConsoleCursor* cursor;  // cursor position in the line and/or page.
    struct ConsoleLine*   line;    // current active line
    struct ConsolePage*   page;    // track lines as a "page" of text
};

struct Console {
    struct ConsoleState*  state;    // Encapsulates console's modes
    struct ConsoleIO*     io;       // Encapsulates console's input and output streams
    struct ConsoleStream* stream;   //
    struct termios*       terminal; // Terminal settings structure
};

// Console memory management
ConsoleState* console_create_state(void);
void          console_destroy_state(ConsoleState* state);

ConsoleIO* console_create_io(void);
void       console_destroy_io(ConsoleIO* io);

ConsoleCursor* console_create_cursor(void);
void           console_destroy_cursor(ConsoleCursor* cursor);

// Line management
ConsoleLine* console_create_line(void);
void         console_destroy_line(ConsoleLine* line);
bool         console_line_append_char(ConsoleLine* line, char c);
bool         console_line_remove_char(ConsoleLine* line, size_t index);

ConsolePage* console_create_page(void);
void         console_destroy_page(ConsolePage* page);

struct termios* console_create_terminal(void);
void            console_destroy_terminal(struct termios* terminal);

ConsoleStream* console_create_stream(void);
void           console_destroy_stream(ConsoleStream* stream);

Console* console_create(void);
void     console_destroy(Console* console);

// Keep track of current display and only emit ANSI code if it changes
void console_set_display_mode(Console* console, StateDisplay state);

// Handle console modes
void process_normal_mode(Console* console, int ch);
void process_insert_mode(Console* console, int ch);

// Helper function for managing raw input and output buffer streams
void  console_set_char(Console* console, int ch);
int   console_get_char(Console* console);
void  console_set_line(Console* console, char* line);
char* console_get_line(Console* console);

// TODO
bool console_readline(Console* console, int line_number);

#endif // CONSOLE_H
