#!/bin/bash
# ANSI Color Codes
G='\033[0;32m' # Green
B='\033[0;34m' # Blue
Y='\033[1;33m' # Yellow
RED='\033[0;31m'
NC='\033[0m'   # No Color

echo -e "${B}         SCHEDULER SYSTEM: AUTOMATED VALIDATION        ${NC}"

# Step 1: Compilation
echo -e "${Y}[STEP 1] Building System...${NC}"
START_TIME=$SECONDS
make clean > /dev/null
if make > /dev/null 2>&1; then
    echo -e "  ${G}✔${NC} Binary 'scheduler_app' created successfully. ($(($(($SECONDS - $START_TIME))))s)"
else
    echo -e "  ${RED}✘${NC} Build Failed. Check Makefile."
    exit 1
fi

# Step 2: Stress Test
echo -e "\n${Y}[STEP 2] Stress Testing (Throughput & Jitter)${NC}"
TARGET=$(grep "#define NUM_TASKS" src/arithmetic_test.c | awk '{print $3}')
echo "  Target: $TARGET Concurrent Tasks"

# stdout goes to log, stderr (Progress) stays on screen
./scheduler_app arithmetic > stress.log

# Give the OS a millisecond to finish writing the file
sleep 0.2

if grep -qi "STRESS TEST COMPLETE" stress.log; then
    echo -e "  ${G}✔${NC} Scheduler stabilized under heavy load."
    echo -e "  ${B}--- Performance Metrics ---${NC}"
   
    grep "CPU Time Consumed" stress.log | sed "s/^/    /"
    grep "Average Latency" stress.log | sed "s/^/    /"
    grep "Worst-Case Jitter" stress.log | sed "s/^/    /"
else
    echo -e "  ${RED}✘${NC} Stress test results not found in stress.log"

    tail -n 3 stress.log
fi

# Step 3: Cancellation
echo -e "\n${Y}[STEP 3] Integrity Testing (Thread Safety)${NC}"
echo "  Target: Concurrent Mass Cancellation"
./scheduler_app cancel > tee cancel.log

if grep -q "CANCELLATION TEST COMPLETE" cancel.log; then
    echo -e "\n  ${G}✔${NC} Hash Map & Min-Heap remained synchronized."
    grep "Successfully cancelled" cancel.log | sed 's/^/    /'
else
    echo -e "\n  ${RED}✘${NC} Data corruption detected during cancellation."
fi

echo -e "${B} VALIDATION COMPLETE ${NC}"
