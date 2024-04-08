/**
 * @file console.cpp
 * @brief Console functions source file.
 */

#include "simple.h" // Include the header file for console functions

#include <iostream> // Standard input/output stream
#include <vector>   // Dynamic array container

#include <climits> // Constants for integral types
#include <limits>
#include <signal.h> // Signal handling
#include <sstream>
#include <stdio.h>  // For printf(), FILE*, fwrite(), etc.
#include <stdlib.h> // Standard library definitions
#include <string>
#include <sys/ioctl.h> // For ioctl() and struct winsize
#include <termios.h>   // Terminal I/O settings
#include <unistd.h>    // For STDOUT_FILENO
#include <wchar.h>     // Extended character handling

// Console state
// NOTE: Ensure that console_state is defined in exactly one .cpp file to prevent linker errors
// related to multiple definitions.
struct ConsoleState console_state;

//
// Init and cleanup
//
void console_create(bool use_simple_io, bool use_advanced_display) {
    console_state.display.advanced = use_advanced_display;
    console_state.io.simple        = use_simple_io;

    // POSIX-specific console initialization
    if (!console_state.io.simple) {
        struct termios new_termios;
        tcgetattr(STDIN_FILENO, &console_state.terminal);
        new_termios              = console_state.terminal;
        new_termios.c_lflag     &= ~(ICANON | ECHO);
        new_termios.c_cc[VMIN]   = 1;
        new_termios.c_cc[VTIME]  = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        console_state.io.teletype = fopen("/dev/tty", "w+");
        if (console_state.io.teletype != nullptr) {
            console_state.io.output = console_state.io.teletype;
        }
    }

    setlocale(LC_ALL, "");
}

void console_reset(void) {
    // Reset console display
    console_set_display_mode(CONSOLE_RESET);

    // Restore settings on POSIX systems
    if (!console_state.io.simple) {
        if (console_state.io.teletype != nullptr) {
            fclose(console_state.io.teletype);
            console_state.io.teletype = nullptr;
            console_state.io.output   = stdout;
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &console_state.terminal);
    }
}

//
// Display and IO
//

// Keep track of current display and only emit ANSI code if it changes
void console_set_display_mode(console_mode_t mode) {
    if (console_state.display.advanced && console_state.display.mode != mode) {
        fflush(stdout);
        switch (mode) {
            case CONSOLE_RESET:
                fprintf(console_state.io.output, ANSI_COLOR_RESET);
                break;
            case CONSOLE_PROMPT:
                fprintf(console_state.io.output, ANSI_COLOR_YELLOW);
                break;
            case CONSOLE_INPUT:
                fprintf(console_state.io.output, ANSI_BOLD ANSI_COLOR_GREEN);
                break;
            case CONSOLE_ERROR:
                fprintf(console_state.io.output, ANSI_BOLD ANSI_COLOR_RED);
        }
        console_state.display.mode = mode;
        fflush(console_state.io.output);
    }
}

/**
 * @brief Reads a UTF-32 character from the standard input stream.
 *
 * This function reads a UTF-32 character from the standard input stream.
 * It handles surrogate pairs in UTF-16 encoding, converting them to UTF-32.
 * If an invalid surrogate pair is encountered, it returns the Unicode replacement
 * character U+FFFD.
 *
 * @return The next UTF-32 character from the standard input stream, or WEOF (-1) if the end of file
 * is reached.
 */
static char32_t getchar32() {
    wchar_t wc = getwchar(); // Read a wide character from the standard input stream
    if (static_cast<wint_t>(wc) == WEOF) {
        return WEOF; // If end-of-file or error indicator is set, return WEOF
    }

#if WCHAR_MAX == 0xFFFF
    if ((wc >= 0xD800) && (wc <= 0xDBFF)) { // Check if wc is a high surrogate
        wchar_t low_surrogate = getwchar(); // Read the next wide character
        if ((low_surrogate >= 0xDC00)
            && (low_surrogate <= 0xDFFF)) { // Check if the next wchar is a low surrogate
            // Combine high and low surrogates to form a UTF-32 character
            return (static_cast<char32_t>(wc & 0x03FF) << 10) + (low_surrogate & 0x03FF) + 0x10000;
        }
    }
    if ((wc >= 0xD800) && (wc <= 0xDFFF)) { // Invalid surrogate pair
        return 0xFFFD;                      // Return the replacement character U+FFFD
    }
#endif

    return static_cast<char32_t>(wc); // Return the Unicode character
}

static void pop_cursor() {
    putc('\b', console_state.io.output);
}

static int estimate_width(char32_t codepoint) {
    return wcwidth(codepoint);
}

static void replace_last(char ch) {
    fprintf(console_state.io.output, "\b%c", ch);
}

static int put_codepoint(const char* utf8_codepoint, size_t length, int expected_width) {
    // Directly use expected width if valid or output is not a teletype
    if (expected_width >= 0 || console_state.io.teletype == nullptr) {
        fwrite(utf8_codepoint, length, 1, console_state.io.output);
        return expected_width;
    }

    // Query initial cursor position
    fputs(ANSI_CURSOR_POS_QUERY, console_state.io.teletype);
    int start_x, start_y, end_x, end_y;
    // ANSI cursor pos current is "\033[{row};{column}R"
    int result_count = fscanf(console_state.io.teletype, "\033[%d;%dR", &start_y, &start_x);

    // Output the UTF-8 codepoint
    fwrite(utf8_codepoint, length, 1, console_state.io.teletype);

    // Query final cursor position
    fputs(ANSI_CURSOR_POS_QUERY, console_state.io.teletype);
    result_count += fscanf(console_state.io.teletype, "\033[%d;%dR", &end_y, &end_x);

    // Ensure both cursor position queries succeeded
    if (4 != result_count) {
        return expected_width; // Fall back to the expected width if querying failed
    }

    // Calculate width based on cursor position change
    int width = end_x - start_x;
    if (0 > width) {
        // Handle potential line wrap
        struct winsize window_size;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
        width += window_size.ws_col; // Adjust width for wrapping
    }

    return width;
}

/**
 * @brief Appends a UTF-8 representation of a Unicode character to a string.
 *
 * This function appends the UTF-8 representation of the given Unicode character
 * to the provided string. It follows the UTF-8 encoding rules to encode the
 * character into one to four bytes, depending on its Unicode code point.
 *
 * @param ch The Unicode character to append to the string.
 * @param out The string to which the UTF-8 representation of the character will be appended.
 *
 * @note If the given Unicode character is outside the valid Unicode code point range (U+0000 to
 * U+10FFFF), no action is taken and the function returns without appending anything to the string.
 */
static void append_utf8(char32_t ch, std::string &out) {
    if (ch <= 0x7F) { // Single-byte UTF-8 encoding
        out.push_back(static_cast<unsigned char>(ch));
    } else if (ch <= 0x7FF) { // Two-byte UTF-8 encoding
        out.push_back(static_cast<unsigned char>(0xC0 | ((ch >> 6) & 0x1F)));
        out.push_back(static_cast<unsigned char>(0x80 | (ch & 0x3F)));
    } else if (ch <= 0xFFFF) { // Three-byte UTF-8 encoding
        out.push_back(static_cast<unsigned char>(0xE0 | ((ch >> 12) & 0x0F)));
        out.push_back(static_cast<unsigned char>(0x80 | ((ch >> 6) & 0x3F)));
        out.push_back(static_cast<unsigned char>(0x80 | (ch & 0x3F)));
    } else if (ch <= 0x10FFFF) { // Four-byte UTF-8 encoding
        out.push_back(static_cast<unsigned char>(0xF0 | ((ch >> 18) & 0x07)));
        out.push_back(static_cast<unsigned char>(0x80 | ((ch >> 12) & 0x3F)));
        out.push_back(static_cast<unsigned char>(0x80 | ((ch >> 6) & 0x3F)));
        out.push_back(static_cast<unsigned char>(0x80 | (ch & 0x3F)));
    } else {
        // Invalid Unicode code point, no action taken
    }
}

// Helper function to remove the last UTF-8 character from a string
static void pop_back_utf8_char(std::string &line) {
    if (line.empty()) {
        return;
    }

    size_t pos = line.length() - 1;

    // Find the start of the last UTF-8 character (checking up to 4 bytes back)
    for (size_t i = 0; i < 3 && pos > 0; ++i, --pos) {
        if ((line[pos] & 0xC0) != 0x80) {
            break; // Found the start of the character
        }
    }
    line.erase(pos);
}

static bool console_readline_advanced(std::string &line) {
    if (console_state.io.output != stdout) {
        fflush(stdout);
    }

    line.clear();
    std::vector<int> widths;
    bool             is_special_char = false;
    bool             end_of_stream   = false;

    char32_t input_char;
    while (true) {
        // Ensure all output is displayed before waiting for input
        fflush(console_state.io.output);
        input_char = getchar32();

        if (input_char == '\r' || input_char == '\n') {
            break;
        }

        if (input_char == (char32_t) WEOF || input_char == 0x04 /* Ctrl+D*/) {
            end_of_stream = true;
            break;
        }

        if (is_special_char) {
            console_set_display_mode(CONSOLE_INPUT);
            replace_last(line.back());
            is_special_char = false;
        }

        if (input_char == '\033') { // Escape sequence
            char32_t code = getchar32();
            if (code == '[' || code == 0x1B) {
                // Discard the rest of the escape sequence
                while ((code = getchar32()) != (char32_t) WEOF) {
                    if ((code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z')
                        || code == '~') {
                        break;
                    }
                }
            }
        } else if (input_char == 0x08 || input_char == 0x7F) { // Backspace
            if (!widths.empty()) {
                int count;
                do {
                    count = widths.back();
                    widths.pop_back();
                    // Move cursor back, print space, and move cursor back again
                    for (int i = 0; i < count; i++) {
                        replace_last(' ');
                        pop_cursor();
                    }
                    pop_back_utf8_char(line);
                } while (count == 0 && !widths.empty());
            }
        } else {
            int offset = line.length();
            append_utf8(input_char, line);
            int width = put_codepoint(
                line.c_str() + offset, line.length() - offset, estimate_width(input_char)
            );
            if (width < 0) {
                width = 0;
            }
            widths.push_back(width);
        }

        if (!line.empty() && (line.back() == '\\' || line.back() == '/')) {
            console_set_display_mode(CONSOLE_PROMPT);
            replace_last(line.back());
            is_special_char = true;
        }
    }

    bool has_more = console_state.io.multiline;
    if (is_special_char) {
        replace_last(' ');
        pop_cursor();

        char last = line.back();
        line.pop_back();
        if (last == '\\') {
            line += '\n';
            fputc('\n', console_state.io.output);
            has_more = !has_more;
        } else {
            // llama will just eat the single space, it won't act as a space
            if (line.length() == 1 && line.back() == ' ') {
                line.clear();
                pop_cursor();
            }
            has_more = false;
        }
    } else {
        if (end_of_stream) {
            has_more = false;
        } else {
            line += '\n';
            fputc('\n', console_state.io.output);
        }
    }

    fprintf(stderr, "debug:console_readline_advanced:special character triggered: %zu\n", has_more);

    fflush(console_state.io.output);
    return has_more;
}

int console_input_character(void) {
    char control_ch;
    if (!(std::cin >> control_ch)) {
        console_set_display_mode(CONSOLE_ERROR);
        if (std::cin.eof()) {
            fprintf(stderr, "debug: console_input_character: reached end of file.\n");
        } else {
            fprintf(stderr, "debug: console_input_character: error reading input.\n");
        }
        std::cin.clear();                                                   // Clear error flags
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip to the next line
        return EOF; // Return EOF if there's an error or end of file
    }
    return static_cast<unsigned char>(control_ch); // Return the character
}

static bool console_input_string(std::string &line) {
    if (!std::getline(std::cin, line)) {
        console_set_display_mode(CONSOLE_ERROR);
        fprintf(stderr, "debug: console_input_string: input stream is bad or EOF reached\n");
        line.clear();
        return false; // Indicates failure to read line
    }
    return true; // Indicates success to read line
}

int console_input_control(std::string &line) {
    // Get the next character after ESC to determine the control action
    int control_ch = console_input_character();
    if (control_ch == EOF) { // Check for EOF or error
        return EOF;
    }

    // Process control characters
    switch (control_ch) {
        case 'm': // 'm' control character for toggling multiline
            fprintf(stderr, "debug: console_input_control: toggle multiline mode\n");
            console_state.io.multiline = !console_state.io.multiline; // Toggle multiline mode
            break;
        default:
            fprintf(
                stderr, "debug: console_input_control: received unexpected control character\n"
            );
            break;
    }
    // Return the received control character for further handling if needed
    return control_ch;
}

// Revised console_readline_simple function definition
static bool console_readline_simple(std::string &line) {
    // If input stream is bad or EOF is reached, log and clear the line
    if (!console_input_string(line)) {
        return false; // console_input_string outputs error to stderr
    }

    if (line.empty()) { // block on empty line
        console_set_display_mode(CONSOLE_ERROR);
        fprintf(stderr, "debug: console_readline_simple: input stream is empty or EOF reached\n");
        return true; // the line is valid even though it is empty.
    }

    fprintf(stderr, "debug: console_readline_simple: line.back(%i)\n", line.back());

    // Process control characters and manage multiline state directly
    char last_ch = line.back(); // "last_ch" is more descriptive than just using "last"
    // Get the last control character (ESC in this case)
    if (last_ch == 27) { // If the last character is not ESC, return EOF
        console_input_control(line);
    }

    // Append a newline character for consistency in single-line mode
    if (!console_state.io.multiline) {
        line += '\n';
    }

    fprintf(stderr, "debug: console_readline_simple: multiline: %zu\n", console_state.io.multiline);

    // By default, return true to indicate the line was successfully read,
    // and continue input if multiline mode is enabled
    return true;
}

bool console_readline(std::string &line) {
    console_set_display_mode(CONSOLE_INPUT);

    if (console_state.io.simple) {
        fprintf(stderr, "debug: console_readline: using console_readline_simple\n");
        return console_readline_simple(line);
    }

    fprintf(stderr, "debug: console_readline: using console_readline_advanced\n");
    return console_readline_advanced(line);
}
