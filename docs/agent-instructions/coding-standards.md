# Coding Standards

Conventions for the skinflatplus C++ codebase. Code formatting (braces, indentation, line width, pointer alignment) is handled by `.clang-format`, so it is not repeated here. This file covers naming, comments, and error handling only.

## Naming Conventions

- Use PascalCase for class, struct, and type names.
- Prefix private class members with an underscore (`_`).
- Use ALL_CAPS for constants and macros.

## Comments

- Write comments in English (this also applies to git commit messages).
- Start a comment with an uppercase letter.

## Error Handling

- Use `try`/`catch` around operations that can throw (e.g. the ImageMagick/GraphicsMagick wrapper, file I/O).
- Validate inputs and handle error paths in display and menu rendering rather than letting failures propagate silently.
- Always log errors with contextual information (what failed and for which item).
