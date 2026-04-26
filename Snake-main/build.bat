@echo off
echo Compiling Premium Snake Game...
gcc main.c -o snake.exe -O2 -Wall -std=c99 -m32 -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm
if %errorlevel% neq 0 (
    echo Compilation failed. Ensure you have GCC installed and added to your PATH.
    pause
) else (
    echo Compilation successful!
    echo Running snake.exe...
    snake.exe
)
