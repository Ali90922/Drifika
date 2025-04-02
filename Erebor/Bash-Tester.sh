#!/bin/bash

echo " Clean previous builds......."
make clean

echo"# Build all targets"
make

echo " Run the executables"
./nqp_printer

#./nqp_refiner

