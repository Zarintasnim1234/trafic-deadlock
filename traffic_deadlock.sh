#!/bin/bash
# traffic_deadlock.sh
# Shell script to compile and run the Traffic Deadlock Simulation C program

echo "🔧 Compiling the Traffic Deadlock Simulation..."
gcc -o traffic_deadlock test911.c -lpthread

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful. Running the program..."
    ./traffic_deadlock
else
    echo "❌ Compilation failed. Please fix the errors in test911.c"
fi
