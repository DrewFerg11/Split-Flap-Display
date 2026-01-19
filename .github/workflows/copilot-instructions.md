# Custom instructions for Copilot in C++ Project

## Project Context
This project is a C++ application deployed to an esp32 device used to control one or more 3D printed split-flap displays with a custom pcb controller running a stepper motor. More details on the display hardware can be found the /info/sfd-instructions.html file. The sfd-instructions.html contains information about assembling the Stepper Motor Driver Board, Expander Module, etc but I've resorted to using a custom make pcb that combines the same functionality into a single board. The project structure follows a typical pattern with `src/` for source files.

## PRIME DIRECTIVE
	Avoid working on more than one file at a time.
	Multiple simultaneous edits to a file will cause corruption.
	Be chatting and teach about what you are doing while coding.

## LARGE FILE & COMPLEX CHANGE PROTOCOL

### MANDATORY PLANNING PHASE
	When working with large files (>300 lines) or complex changes:
		1. ALWAYS start by creating a detailed plan BEFORE making any edits
            2. Your plan MUST include:
                   - All functions/sections that need modification
                   - The order in which changes should be applied
                   - Dependencies between changes
                   - Estimated number of separate edits required
                
            3. Format your plan as:
## PROPOSED EDIT PLAN
	Working with: [filename]
	Total planned edits: [number]

### MAKING EDITS
	- Focus on one conceptual change at a time
	- Show clear "before" and "after" snippets when proposing changes
	- Include concise explanations of what changed and why
	- Always check if the edit maintains the project's coding style

### Edit sequence:
	1. [First specific change] - Purpose: [why]
	2. [Second specific change] - Purpose: [why]
	3. Do you approve this plan? I'll proceed with Edit [number] after your confirmation.
	4. WAIT for explicit user confirmation before making ANY edits when user ok edit [number]
            
### EXECUTION PHASE
	- After each individual edit, clearly indicate progress:
		"✅ Completed edit [#] of [total]. Ready for next edit?"
	- If you discover additional needed changes during editing:
	- STOP and update the plan
	- Get approval before continuing
                
### REFACTORING GUIDANCE
	When refactoring large files:
	- Break work into logical, independently functional chunks
	- Ensure each intermediate state maintains functionality
	- Consider temporary duplication as a valid interim step
	- Always indicate the refactoring pattern being applied
                
### RATE LIMIT AVOIDANCE
	- For very large files, suggest splitting changes across multiple sessions
	- Prioritize changes that are logically complete units
	- Always provide clear stopping points

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