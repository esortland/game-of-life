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

# Correctness configurations


declare -a test_file_inputs=(
    "tests/test_case_1.txt"
    "tests/test_case_2.txt"
    "tests/test_case_3.txt"
    "tests/test_case_4.txt"
    "tests/test_case_5.txt"
)

declare -a test_case_1_outputs=(
    "tests/test_case_1_001_it.txt"
    "tests/test_case_1_100_it.txt"
    "tests/test_case_1_101_it.txt"
    "tests/test_case_1_300_it.txt"
    "tests/test_case_1_301_it.txt"
)

declare -a test_case_2_outputs=(
    "tests/test_case_2_100_it.txt"
    "tests/test_case_2_200_it.txt"
    "tests/test_case_2_500_it.txt"
    "tests/test_case_2_1000_it.txt"
)

declare -a test_case_3_outputs=(
    "tests/test_case_3_100_it.txt"
    "tests/test_case_3_200_it.txt"
    "tests/test_case_3_500_it.txt"
    "tests/test_case_3_1000_it.txt"
)

declare -a test_case_4_outputs=(
    "tests/test_case_4_100_it.txt"
    "tests/test_case_4_200_it.txt"
    "tests/test_case_4_500_it.txt"
    "tests/test_case_4_1000_it.txt"
)
declare -a test_case_1_num_itrs=(1 100 101 300 301)
declare -a test_case_2_num_itrs=(100 200 500 1000)
declare -a test_case_3_num_itrs=(100 200 500 1000)
declare -a test_case_4_num_itrs=(100 200 500 1000)

declare -a thread_counts=(16 64 128) # add 1 to this list if needed
echo "Running correctness tests..."

for i in "${!test_file_inputs[@]}"; do
    input_file=${test_file_inputs[$i]}
    
    case $i in
        0)
            outputs=("${test_case_1_outputs[@]}")
            iterations=("${test_case_1_num_itrs[@]}")
            ;;
        1)
            outputs=("${test_case_2_outputs[@]}")
            iterations=("${test_case_2_num_itrs[@]}")
            ;;
        2)
            outputs=("${test_case_3_outputs[@]}")
            iterations=("${test_case_3_num_itrs[@]}")
            ;;
        3)
            outputs=("${test_case_4_outputs[@]}")
            iterations=("${test_case_4_num_itrs[@]}")
            ;;
        4)
            outputs=() # No additional outputs for test case 5
            iterations=()
            ;;
    esac

    for j in "${!outputs[@]}"; do
        output_file=${outputs[$j]}
        num_iters=${iterations[$j]}
        
        for threads in "${thread_counts[@]}"; do
            echo "Testing input: $input_file, Expected output: $output_file, Iterations: $num_iters, Threads: $threads"
            ./build/run_tests_correctness "$input_file" "$output_file" "$num_iters" "$threads"
            echo ""
        done
    done
done




# Speed test configurations
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
