#!/bin/bash
echo "Compiling UI shaders..."

glslc ui.vert -o ui_vert.spv
if [ $? -ne 0 ]; then
    echo "Failed to compile vertex shader"
    exit 1
fi

glslc ui.frag -o ui_frag.spv
if [ $? -ne 0 ]; then
    echo "Failed to compile fragment shader"
    exit 1
fi

echo "UI shaders compiled successfully"