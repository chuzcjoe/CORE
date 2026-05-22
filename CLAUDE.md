# Claude Code Guidelines

Claude must follow these rules when working on this project:

## 1. C++ Code Style
- All C++ code must comply with the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
- Naming, indentation, header include order, comment style, etc. all follow that guide.

## 2. Simplicity
- Complete the task with the minimum amount of code changes.
- Do not add features, refactors, or abstractions beyond the scope of the task.
- Do not design for hypothetical future requirements.
- Avoid unnecessary helper functions or wrappers.

## 3. Minimal Blast Radius
- Touch only what you must. Clean up only your own mess.
- Only modify the files and code strictly necessary to complete the current task.
- Do not opportunistically refactor, reformat, or "optimize" unrelated code.
- Only clean up temporary code or debug leftovers that you introduced; do not clean up pre-existing code unrelated to this task.
