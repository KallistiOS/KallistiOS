
# KOS C Coding Style Guide

This document outlines the C coding style guidelines for the KallistiOS (KOS) codebase. These guidelines are not strict rules but should be followed for consistency and readability.

## 1. General Guidelines
- Write readable and maintainable code.
- Avoid overly complex functions or structures.
- Prioritize code clarity over optimization unless critical.
- **Consistency**: Follow the same patterns throughout the project.

## 2. Indentation
- Use **4-character tabs** (or equivalent **4 spaces**) for indentation.
- Limit code to **3 levels** of indentation to maintain readability.
- Example:
    ```c
    if(condition) {
        // level 1
        if(other_condition) {
            // level 2
            while(loop_condition) {
                // level 3
            }
        }
    }
    ```

## 3. Brace Placement
- Place the opening brace `{` on the **same line** as the control statement or function header.
- Place the closing brace `}` on its own line.
- Example:
    ```c
    if(x == y) {
        do_something();
    } 
    else {
        do_something_else();
    }
    ```
    
## 4. Naming Conventions
- **Global Variables/Functions**: Use descriptive names, typically prefixed with the subsystem (e.g., `pvr_`, `net_`).
    - Example: `pvr_init()`, `net_send_packet()`.
- **Local Variables**: Use short, clear names (e.g., `i`, `tmp`, `ptr`), especially in small scopes.
- **Constants**: Use all uppercase letters and underscores (e.g., `MAX_BUFFER_SIZE`).
- Example:
    ```c
    static const int MAX_USERS = 100;
    int user_count = 0;
    ```

## 5. Functions
- Keep functions **short** and **focused**—ideally fitting within 1-2 screenfuls (80x24 characters).
- Avoid more than 5-10 local variables in a function. Use helper functions if needed.
- Use **descriptive names** for functions, reflecting their purpose.
- Example:
    ```c
    void reset_device(int device_id) {
        // Clear settings and restart the device
    }
    ```
- Functions that take **no arguments** should be explicitly marked with `void` in the argument list.
    - Example:
    ```c
    void my_function(void) {
        // Function logic
    }
    ```
    
## 6. Commenting
- Prefer **block comments** over inline comments for better clarity.
    - Example of block comments:
    ```c
    /* 
     * Align the given address to the specified alignment.
     * Return the aligned address.
     */
    size_t align_to(size_t address, size_t alignment) { ... }
    ```
- Add **header-style comments** before functions or complex sections:
    - Describe the function’s purpose, parameters, and return value.
- Avoid redundant comments like:
    ```c
    // Open the file
    f = fopen(filename, "r");
    ```

## 7. Header Files

Each header file should follow this structure for consistency and clarity:

- Start with a file header comment, which includes basic information about the file:
    ```c
    /* KallistiOS ##version##
       directory/foobar.h
       Copyright (C) 2024 Your Name
    */
    ```

- Follow this with a **Doxygen comment block** that describes the purpose of the file, its group, and any relevant authors:
    ```c
    /** \file    directory/foobar.h
        \brief   Foo support.
        \ingroup foobar

        This file contains the interface to the foo system of KOS. Foobars are used 
        to ...

        \author Your Name
    */
    ```

- Next, include a **header guard** to prevent multiple inclusions:
    ```c
    #ifndef __FOOBAR_H
    #define __FOOBAR_H
    ```

- Inside the header guard, add a **C++ guard** for compatibility and include any necessary headers:
    ```c
    #include <sys/cdefs.h>
    __BEGIN_DECLS
    
    // Insert other headers here
    ```

- If applicable, define your **Doxygen group** and provide a brief overview of the API:
    ```c
    /** \defgroup foobar  Foobar Management
        \brief            Foobar System API
        \ingroup          Utility

        This module provides functionality for managing foobar objects, including
        operations to toggle between active and inactive states.

        @{ 
    */
    ```

- Next, add **type definitions** and **function declarations**. Each of those should be accompanied by a **Doxygen comment** that explains its purpose, parameters, etc:
    ```c
    /** \brief   Initializes the foobar system.
    
        Comment here if you want to get into more detail.
    
        \return  0 on success, -1 on failure.
    */
    int foobar_init(void);
    ```

- Finally, close the **Doxygen group** and both the C++ and header guards:
    ```c
    /** @} */
    
    __END_DECLS
    
    #endif /* __FOOBAR_H */
    ```
    
## 8. Source Files
- Each source file should start with a file header comment:
    ```c
    /* KallistiOS ##version##
       directory/foobar.c
       Copyright (C) 2024 Your Name
    */
    ```
- Followed by headers. Include only necessary headers and avoid redundant includes.
    - Include order:
        1. Standard C headers (`<stdio.h>`, `<string.h>`, etc.)
        2. KOS headers (`<kos/thread.h>`, etc.)
        3. Architecture-specific headers (`<dc/g2bus.h>`, etc.)
-  After that, feel free to put a block comment explaining what's in the file
and describing any notes about the module, followed by the functions/variables themselves:
    - Example:
    ```c
    /*
        This module supports foobars in KOS. A foobar is used to ...
    */
    
    // Insert function definitions here
    ```


## 9. Sized Integer Types
- Use sized integer types from `<stdint.h>` rather than built-in types like `uint8`, `uint16`, etc.
    - Example:
    ```c
    #include <stdint.h>
    
    int32_t my_var;
    ```

## 10. Type Names
- Type names should have a `_t` suffix if typedef'd.
- Prefix types related to a specific subsystem with an identifier.
    - Example: `typedef uint32 pvr_token_t;`
    
## 11. Error Handling
- Always check return values of functions that can fail (e.g., memory allocation, file operations).
- Use error codes or `errno` for error reporting.
- Example:
    ```c
    if((ptr = malloc(size)) == NULL) {
        dbglog(DBG_ERROR, "malloc failed");
        return -1;
    }
    ```

---

Follow these guidelines to ensure that your code remains clean, readable, and maintainable. While certain guidelines may be flexible, consistency and clarity are paramount.
