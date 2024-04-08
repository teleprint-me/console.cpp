/**
 * @file console.cpp
 *
 * @brief Provides console management functionalities including initialization,
 * display mode setting, and input handling.
 *
 */

#include <console.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <locale.h>
#include <optional>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

// state
ConsoleState* console_create_state(void) {
    // Initialize console states
    ConsoleState* state = (ConsoleState*) malloc(sizeof(ConsoleState));
    if (NULL == state) {
        return NULL;
    }

    state->input   = STATE_INPUT_NORMAL;
    state->display = STATE_DISPLAY_INPUT;
    return state;
}

void console_destroy_state(ConsoleState* state) {
    if (NULL != state) {
        free(state);
    }
}

// io
ConsoleIO* console_create_io(void) {
    // initialize console i/o
    ConsoleIO* io = (ConsoleIO*) malloc(sizeof(ConsoleIO));
    if (NULL == io) {
        return NULL;
    }

    // set standard input and output
    io->input  = stdin;
    io->output = stdout;

    // set teletype device
    io->teletype = fopen("/dev/tty", "w+");
    if (NULL == io->teletype) {
        io->teletype = io->output; // fallback to stdout
    }

    return io; // assuming all went well :)
}

void console_destroy_io(ConsoleIO* io) {
    if (NULL != io) {
        // Only close teletype if it's not using the fallback stdout
        if (io->teletype != stdout && io->teletype != stderr) {
            fclose(io->teletype);
        }
        free(io);
    }
}

// cursor
ConsoleCursor* console_create_cursor(void) {
    ConsoleCursor* cursor = (ConsoleCursor*) malloc(sizeof(ConsoleCursor));
    if (NULL == cursor) {
        return NULL;
    }

    cursor->row = 0; // Initialize cursor at the start of the page
    cursor->col = 0; // Initialize cursor at the start of the line
    return cursor;
}

void console_destroy_cursor(ConsoleCursor* cursor) {
    if (NULL != cursor) {
        free(cursor);
    }
}

// line
ConsoleLine* console_create_line(size_t size) {
    ConsoleLine* line = (ConsoleLine*) malloc(sizeof(ConsoleLine));
    if (NULL == line) {
        return NULL;
    }

    // Allocate an initial buffer size (e.g., 64 characters)
    if (0 == size) {
        size = 64; // default if set to 0
    }
    line->buffer = (char*) malloc(size * sizeof(char));
    if (NULL == line->buffer) {
        // Handle buffer allocation failure
        free(line);
        return NULL;
    }

    line->size      = size; // the number allocated bytes
    line->buffer[0] = '\0'; // Initialize buffer to empty string
    line->length    = 0;    // the characters in the buffer
    return line;
}

void console_destroy_line(ConsoleLine* line) {
    if (NULL != line) {
        if (NULL != line->buffer) {
            free(line->buffer);
        }
        free(line);
    }
}

bool console_line_append_char(ConsoleLine* line, char c) {
    if (line->length + 1 >= line->size) {   // +1 for the null terminator
        size_t new_size   = line->size * 2; // Double the buffer size
        char*  new_buffer = (char*) realloc(line->buffer, new_size);
        if (new_buffer == NULL) {
            return false; // Reallocation failed
        }
        line->buffer = new_buffer;
        line->size   = new_size;
    }
    line->buffer[line->length] = c;
    line->length++;
    line->buffer[line->length] = '\0'; // Maintain null-terminator
    return true;
}

bool console_line_remove_char(ConsoleLine* line, size_t index) {
    if (NULL == line || index >= line->length) {
        return false; // Index out of bounds or line is NULL
    }

    // Shift characters to the left to overwrite the character at `index`
    for (size_t i = index; i < line->length - 1; i++) {
        line->buffer[i] = line->buffer[i + 1];
    }

    line->length--;                    // Update the length of the line
    line->buffer[line->length] = '\0'; // Maintain null-terminator

    // Optional: Consider shrinking the buffer if the current usage is significantly below the
    // allocated size This is a trade-off decision. Frequent allocations and deallocations can
    // impact performance.

    return true;
}

// TODO: page
ConsolePage* console_create_page(void) {}

void console_destroy_page(ConsolePage* page) {}

// stream
ConsoleStream* console_create_stream(void) {
    ConsoleStream* stream = (ConsoleStream*) malloc(sizeof(ConsoleStream));
    if (NULL == stream) {
        return NULL;
    }

    stream->last    = -1;                      // int
    stream->current = -1;                      // int
    stream->status  = STREAM_STATUS_INIT;      // enum StreamStatus
    stream->event   = STREAM_EVENT_POLL;       // enum StreamEvent
    stream->cursor  = console_create_cursor(); // struct ConsoleCursor
    stream->line    = console_create_line(0);  // struct ConsoleLine
    stream->page    = NULL;                    // TODO: struct ConsolePage

    return stream;
}

void console_destroy_stream(ConsoleStream* stream) {}

// terminal
struct termios* console_create_terminal(void) {
    // POSIX-specific console initialization
    struct termios* terminal;

    tcgetattr(STDIN_FILENO, terminal);
    terminal->c_lflag     &= ~(ICANON | ECHO); // Disable canonical mode and echo
    terminal->c_cc[VMIN]   = 1; // Minimum number of characters for noncanonical read.
    terminal->c_cc[VTIME]  = 0; // Timeout in deciseconds for noncanonical read.
    tcsetattr(STDIN_FILENO, TCSANOW, terminal);

    setlocale(LC_ALL, "");
    return terminal;
}

void console_destroy_terminal(struct termios* terminal) {
    tcsetattr(STDIN_FILENO, TCSANOW, terminal);
}

Console* console_create(void) {
    // Initialize console
    Console* console = (Console*) malloc(sizeof(Console));
    if (NULL == console) {
        return NULL;
    }

    // Initialize console states
    console->state    = console_create_state();
    // Initialize console i/o
    console->io       = console_create_io();
    // initialize console stream
    console->stream   = console_create_stream(); // TODO: struct ConsoleStream
    // POSIX-specific console initialization
    console->terminal = console_create_terminal();
    return console;
}

// Don't forget to restore the original terminal settings upon exit
void console_destroy(Console* console) {
    console_set_display_mode(console, STATE_DISPLAY_RESET);

    if (console->io->teletype) {
        fclose(console->io->teletype);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &console->terminal);

    if (console->state) {
        free(console->state);
    }
    free(console);
}

// Keep track of current display and only emit ANSI code if it changes
void console_set_display_mode(Console* console, StateDisplay state) {
    if (console->state->display != state) {
        fflush(stdout);

        switch (state) {
            case STATE_DISPLAY_RESET:
                fprintf(console->io->teletype, ANSI_COLOR_RESET);
                break;
            case STATE_DISPLAY_PROMPT:
                fprintf(console->io->teletype, ANSI_COLOR_YELLOW);
                break;
            case STATE_DISPLAY_INPUT:
                fprintf(console->io->teletype, ANSI_BOLD ANSI_COLOR_GREEN);
                break;
            case STATE_DISPLAY_ERROR:
                fprintf(console->io->teletype, ANSI_BOLD ANSI_COLOR_RED);
        }

        console->state->display = state;
        fflush(console->io->teletype);
    }
}

// manipulate a pre-existing character in the display, e.g. deletion, insertion, etc.
// mostly focused on cursor movement, implementation details TBD.
void console_set_char(Console* console, int character) {}

// this is simple enough. get a character input from the user.
// characters may be treated as a buffered stream when used in a loop.
// this should allow for some neat things to happen.
int console_get_char(Console* console) {
    char control_ch;
    if (!(std::cin >> control_ch)) {
        console_set_display_mode(console, STATE_DISPLAY_ERROR);
        if (std::cin.eof()) {
            fprintf(stderr, "debug: console_get_char: reached end of file.\n");
        } else {
            fprintf(stderr, "debug: console_get_char: error reading input.\n");
        }
        std::cin.clear();                                                   // Clear error flags
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip to the next line
        return EOF; // Return EOF if there's an error or end of file
    }
    return static_cast<unsigned char>(control_ch); // Return the character
}

// modify the line, similar to console_set_char, but for a line instead
void console_set_line(Console* console, std::string line) {}

// Modify the signature if using std::optional
char* console_get_line(Console* console) {
    char* line = fgets_unlocked(
        console->stream->line->buffer, console->stream->line->size, console->io->input
    );
    if (NULL == line) {
        console_set_display_mode(console, STATE_DISPLAY_ERROR);
        fprintf(stderr, "debug: console_get_line: input stream is bad or EOF reached\n");
        return NULL;
    }
    return line;
}

void process_normal_mode(Console* console, char ch) {
    if (ch == 'i') { // Example: Enter insert state
        console->state->input = STATE_INPUT_INSERT;
    }
    // Add other commands for normal state here
}

void process_insert_mode(Console* console, char ch) {
    if (ch == 27) { // ESC
        console->state->input = STATE_INPUT_NORMAL;
    } else if (ch == '\b' || ch == 127) { // Backspace
        // Code to remove the last character from the buffer and update display
    } else {
        // Append character to buffer and echo to display
        fprintf(stdout, "%c", ch);
        fflush(stdout); // Display character
    }
}

bool console_readline(std::string &line) {
    return false; // TODO
}

int main() {
    Console* console = console_create();

    while (1) {
        char ch;
        read(STDIN_FILENO, &ch, 1);

        switch (console->state->input) {
            case STATE_INPUT_NORMAL:
                process_normal_mode(console, ch);
                break;
            case STATE_INPUT_INSERT:
                process_insert_mode(console, ch);
                break;
        }
    }

    console_destroy(console);
    return 0;
}
