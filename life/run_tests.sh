#!/bin/bash

# Compile the project
echo "Compiling..."
make
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo ""
echo "=========================================="
echo "Game of Life Speed Test Suite"
echo "=========================================="
echo ""

# Test configurations
declare -a thread_counts=(1 64 128)
declare -a grid_configs=(
    "100 100 100"
    "1000 1000 100"
    "5000 1000 100"
    "1000 5000 100"
    # "10000 10000 10" // Disabled due to long runtime
)

# Run benchmarks
for config in "${grid_configs[@]}"; do
    read rows cols gens <<< "$config"
    echo "Grid: ${rows}x${cols}, Generations: ${gens}"
    echo "----------------------------------------"
    
    for threads in "${thread_counts[@]}"; do
        echo "Running with $threads threads..."
        ./build/speed_test $rows $cols $gens $threads 
        echo ""
    done
    
    echo "=========================================="
    echo ""
done

echo "All benchmarks completed!"
