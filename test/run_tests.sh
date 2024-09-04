#!/bin/bash

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
WHITE='\033[1;37m'
NC='\033[0m' # No color

# Rectangle characters
TOP_LEFT="╔"
TOP_RIGHT="╗"
BOTTOM_LEFT="╚"
BOTTOM_RIGHT="╝"
HORIZONTAL="═════════════════════════════════"
VERTICAL="║"

# Read hostfile.txt to get client and server hostnames
CLIENT_HOST=$(grep "client:" hostfile.txt | awk '{print $2}')
SERVER_HOST=$(grep "server:" hostfile.txt | awk '{print $2}')

# Directory where the binaries are stored
BUILD_DIR="~/mnt/RUPT/test/build"

# Test directories
TEST_DIRS=$(ls -d test*)

# Control output visibility based on the argument
SHOW_OUTPUT=false
if [ "$1" == "--show-output" ]; then
    SHOW_OUTPUT=true
fi

# Function to print a rectangle around text
print_rectangle() {
    local color=$1
    local text=$2
    local extra_text=$3

    # Calculate the length of the longest line
    local text_length=${#text}
    local extra_length=${#extra_text}
    local max_length=$(( text_length > extra_length ? text_length : extra_length ))

    # Create horizontal line
    local horizontal_line=$(printf "%-${max_length}s" "${HORIZONTAL}" | tr ' ' '═')

    # Print top line
    echo -e "${color}${TOP_LEFT}${horizontal_line}${TOP_RIGHT}${NC}"
    # Print main text
    echo -e "${color} ${text} ${NC}"

    # Print extra text if available
    if [ ! -z "$extra_text" ]; then
        echo -e "${color} ${extra_text} ${NC}"
    fi

    # Print bottom line
    echo -e "${color}${BOTTOM_LEFT}${horizontal_line}${BOTTOM_RIGHT}${NC}"
}

# Function to run a test
run_test() {
    local test_dir=$1
    local client_binary="${BUILD_DIR}/${test_dir}_client"
    local server_binary="${BUILD_DIR}/${test_dir}_server"

    # Print running test message
    print_rectangle "${WHITE}" "Running $test_dir"

    # Start the server in the background on the server host
    if $SHOW_OUTPUT; then
        ssh $SERVER_HOST "$server_binary" &

    else
        ssh $SERVER_HOST "$server_binary" > /dev/null 2>&1 &
    fi
    SERVER_PID=$!


    # Give the server a moment to start
    sleep 2

    # Run the client on the client host
    if $SHOW_OUTPUT; then
        ssh $CLIENT_HOST "$client_binary"
    else
        ssh $CLIENT_HOST "$client_binary" > /dev/null 2>&1
    fi
    CLIENT_STATUS=$?

    # Wait for the server to finish
    wait $SERVER_PID
    SERVER_STATUS=$?

    # Check results and print the outcome in respective colors
    if [ $CLIENT_STATUS -eq 0 ] && [ $SERVER_STATUS -eq 0 ]; then
        print_rectangle "${GREEN}" "Test $test_dir: SUCCESS"
    else
        print_rectangle "${RED}" "Test $test_dir: FAILED" "(Client: $CLIENT_STATUS, Server: $SERVER_STATUS)"
    fi
}

# Run each test
for test_dir in $TEST_DIRS; do
    run_test $test_dir
done
