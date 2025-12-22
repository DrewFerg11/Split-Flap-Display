# Custom instructions for Copilot in C++ Project

## Project Context
This project is a C++ application deployed to an esp32 device used to control one or more 3D printed split-flap displays with a custom pcb controller running a stepper motor. More details on the display hardware can be found the /info/sfd-instructions.html file. The sfd-instructions.html contains information about assembling the Stepper Motor Driver Board, Expander Module, etc but I've resorted to using a custom make pcb that combines the same functionality into a single board. The project structure follows a typical pattern with `src/` for source files.

## Coding Style
- **Indentation:** Use 4 spaces for indentation, no tabs.
- **Naming Conventions:**
    - Classes and Structs: `PascalCase` (e.g., `MyClass`).
    - Functions: `camelCase` (e.g., `myFunction`).
    - Variables: `camelCase` (e.g., `myVariable`).
    - Constants: `SCREAMING_SNAKE_CASE` (e.g., `MY_CONSTANT`).
- **Header Guards:** Use `#pragma once` for header guards.
- **Includes:** Prefer angle brackets `<>` for standard library headers and double quotes `""` for project-specific headers.
- **Error Handling:** Use exceptions for error handling, avoiding raw pointers and favoring smart pointers where appropriate.

## Refactoring and Modernization
- When suggesting refactoring, prioritize using C++ standard library algorithms over raw loops where appropriate.
- Aim for modern C++ idioms and best practices.

## Specific File Type Instructions
---
applyTo: "**/*.cpp"
---
# Instructions for C++ Source Files
- Ensure `main` function follows standard structure.
- When generating code, consider performance implications and suggest optimizations where relevant.

---
applyTo: "**/*.h"
---
# Instructions for C++ Header Files
- Ensure all necessary header guards are present.
- Avoid including unnecessary headers to minimize compilation times.