@echo off
echo Compiling UI shaders...

glslc ui.vert -o ui_vert.spv
if %errorlevel% neq 0 (
    echo Failed to compile vertex shader
    exit /b 1
)

glslc ui.frag -o ui_frag.spv
if %errorlevel% neq 0 (
    echo Failed to compile fragment shader
    exit /b 1
)

echo UI shaders compiled successfully