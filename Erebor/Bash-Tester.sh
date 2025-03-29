#!/bin/bash

echo " Clean previous builds......."
make clean

echo"# Build all targets"
make

# Run the executables
./nqp_printer
./nqp_refiner

