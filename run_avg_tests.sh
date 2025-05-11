#!/bin/bash

NUM_RUNS=10
MATRIX_SIZE=1000
MODE=1
OUTPUT_FILE="results_1000_open_mp_static_5.csv"

# Create/clear the CSV file with header
echo "num_threads,run,real_time" > "$OUTPUT_FILE"

# For each thread count (1 to 10)
for threads in {1..10}; do
    echo "Testing with $threads threads..."

    # Run the test NUM_RUNS times
    for run in $(seq 1 $NUM_RUNS); do
        echo " Run $run/$NUM_RUNS"
        
        # Capture only real time using /usr/bin/time
        real_time=$(/usr/bin/time -f "%e" ./main "$MATRIX_SIZE" "$threads" "$MODE" 2>&1)
        
        # Append to CSV
        echo "$threads,$run,$real_time" >> "$OUTPUT_FILE"
    done
done

echo "Results written to $OUTPUT_FILE"
