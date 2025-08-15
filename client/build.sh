#!/bin/bash

echo "Building hawkBit DDI Client..."

# Clean previous build
rm -rf build

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make

cd ..

if [ -f "build/client" ]; then
    echo "Client build completed successfully!"
    echo "Executable is at: build/client"
    echo ""
    echo "Usage:"
    echo "  ./build/client [server_url] [controller_id]"
    echo ""
    echo "Example:"
    echo "  ./build/client http://localhost:8000 device001"
else
    echo "Build failed!"
    exit 1
fi