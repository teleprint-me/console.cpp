/**
 * @file console.h
 * @brief Provides console management functionalities including initialization, display mode
 * setting, and input handling.
 */

#pragma once

#ifndef CONSOLE_H
    #define CONSOLE_H

    #include <iostream>    // Standard I/O stream
    #include <stdio.h>     // For FILE*
    #include <string>      //
    #include <sys/ioctl.h> // Terminal I/O control
    #include <termios.h>   // Terminal I/O settings
    #include <wchar.h>     // Extended character handling

    // Constants for ANSI escape codes

    // ANSI Color codes
    #define ANSI_COLOR_RED              "\x1b[31m"
    #define ANSI_COLOR_GREEN            "\x1b[32m"
    #define ANSI_COLOR_YELLOW           "\x1b[33m"
    #define ANSI_COLOR_BLUE             "\x1b[34m"
    #define ANSI_COLOR_MAGENTA          "\x1b[35m"
    #define ANSI_COLOR_CYAN             "\x1b[36m"
    #define ANSI_COLOR_RESET            "\x1b[0m"
    #define ANSI_BOLD                   "\x1b[1m"

    // ANSI Cursor codes
    #define ANSI_CURSOR_POS_QUERY       "\033[6n" // Query cursor position

    // Constants for handling special characters
    #define REPLACEMENT_CHARACTER_WIDTH 1 // Assuming U+FFFD's display width is 1

// Enumeration for console display modes.
typedef enum ConsoleMode {
    CONSOLE_RESET = 0, // Reset display mode
    CONSOLE_PROMPT,    // Prompt display mode
    CONSOLE_INPUT,     // User input display mode
    CONSOLE_ERROR      // Error message display mode
} console_mode_t;

// Console Input/Output configuration.
typedef struct ConsoleIO {
    bool  simple    = true;    // Flag for simple I/O mode.
    bool  multiline = false;   // Multiline I/O mode.
    FILE* output    = stdout;  // Output stream, defaults to standard output.
    FILE* teletype  = nullptr; // Teletype for direct terminal I/O, not used by default.
} ConsoleIO;

// Console Display configuration.
typedef struct ConsoleDisplay {
    bool           advanced = false;         // Flag for advanced display features.
    console_mode_t mode     = CONSOLE_RESET; // Current display mode.
} ConsoleDisplay;

// Console state.
typedef struct ConsoleState {
    ConsoleIO      io;       // I/O configuration
    ConsoleDisplay display;  // Display configuration
    termios        terminal; // Terminal settings
} ConsoleState;

//
// Console state
//
extern ConsoleState console_state;

/**
 * Initializes the console with specified I/O and display configurations.
 * @param use_simple_io Use simple I/O mode.
 * @param use_advanced_display Use advanced display features.
 */
void console_create(bool use_simple_io, bool use_advanced_display);

/**
 * Resets the console, cleaning up resources.
 */
void console_reset(void);

/**
 * Sets the console's display mode.
 * @param console_display New display mode to set.
 */
void console_set_display_mode(console_mode_t mode);

/**
 * Reads a line of input from the console.
 * @param line Reference to store the input line.
 * @param multiline_input Allow multiline input.
 * @return true if input was successfully read, false otherwise.
 */
bool console_readline(std::string &line);

#endif // CONSOLE_H
