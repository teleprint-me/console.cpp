# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.20)

# Define your project name and the languages used
project(console VERSION 1.0 LANGUAGES C CXX)

# Check for the header file, ensuring it's available
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("./include/console.h" HAVE_CONSOLE_H)

# Specify the include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define the source files for the target
set(SOURCE_FILES "./src/console.cpp")

# Add a library target to be built from the source files.
# The library is named "console" and will be a shared library.
add_library(console SHARED ${SOURCE_FILES})

# Set the output name of the library to "console.so"
set_target_properties(console PROPERTIES OUTPUT_NAME console)

# Optional: Set the C++ standard to use, ensuring compatibility with your codebase
set_target_properties(console PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# If there are specific compiler options or definitions required, they can be added like this:
# target_compile_options(console PRIVATE -Wall -Wextra)
# target_compile_definitions(console PRIVATE SOME_DEFINITION)

# If your console library depends on other libraries, link them here. For example:
# target_link_libraries(console other_library)
