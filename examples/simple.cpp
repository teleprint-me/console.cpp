/**
 * @file console.example.cpp
 * @brief Example providing console management functionalities including initialization, display
 * mode setting, and input handling.
 */

#include "console.h"
#include <iostream>

int main() {
    // Initialize the console with advanced display features enabled and simple I/O mode
    console_create(true, true);

    // Set the console's display mode to prompt
    console_set_display_mode(CONSOLE_PROMPT);
    std::cout << "Prompt mode: Enter a command" << std::endl;

    // Switch to input mode
    console_set_display_mode(CONSOLE_INPUT);
    std::string inputLine;

    while (true) {
        std::cout << "> ";
        if (!console_readline(inputLine)) {
            console_set_display_mode(CONSOLE_ERROR);
            std::cout << "Failed to read input or EOF reached." << std::endl;
            console_set_display_mode(CONSOLE_INPUT);
            break;
        }

        if (!console_state.io.multiline) {
            std::cout << "Exiting multiline mode." << std::endl;
            break;
        }

        std::cout << inputLine << std::endl;
    }

    // Set the console's display mode to error for demonstration
    console_set_display_mode(CONSOLE_ERROR);
    std::cout << "Error mode: This is an error message" << std::endl;

    // Reset the console before exiting
    console_reset();

    return 0;
}
