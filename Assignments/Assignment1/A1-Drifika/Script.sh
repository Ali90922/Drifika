#!/bin/bash
# Exit immediately if any command exits with a non-zero status.
set -e

# Clean previous builds
echo "Running: make clean"
make clean

# Build the project
echo "Running: make"
make

# Run the custom 'cat' command on Drifika.img and README.md
echo "Running: ./cat Drifika.img README.md"
./cat Drifika.img README.md

# Run the custom 'ls' command on Drifika.img and Juventus
echo "Running: ./ls Drifika.img Juventus"
./ls Drifika.img Juventus

# Run the custom 'paste' command on Drifika.img, README.md, and Uefa.txt
echo "Running: ./paste Drifika.img README.md Uefa.txt"
./paste Drifika.img README.md Uefa.txt

