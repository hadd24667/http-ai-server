#!/bin/bash

SERVER="http://127.0.0.1:8080"
SCRIPT="adaptive_full.lua"
LOG="http_server_log.csv"

echo "======================================="
echo " Adaptive Scheduler – Full 4-Phase Test"
echo "======================================="
echo "Output log file: $LOG"
echo

###########################################
# Mỗi phase chạy wrk để kích hoạt 1 vùng
# của AdaptiveScheduler: FIFO → SJF → RR → WFQ
###########################################

run_phase() {
    PHASE_NAME=$1
    THREADS=$2
    CONN=$3
    DURATION=$4

    echo "---------------------------------------"
    echo ">>> $PHASE_NAME (t=$THREADS, c=$CONN, d=$DURATION)"
    echo "---------------------------------------"

    echo "#PHASE:$PHASE_NAME" >> $LOG

    wrk -t$THREADS -c$CONN -d$DURATION -s $SCRIPT $SERVER
    echo
}

###########################################
# 4 PHA
###########################################

# 1️⃣ FIFO — load nhẹ
run_phase "FIFO_PHASE" 4 4 10s

# 2️⃣ SJF — queue_len trung bình
run_phase "SJF_PHASE" 8 50 15s

# 3️⃣ RR — CPU cao nhưng queue_len chưa quá lớn
run_phase "RR_PHASE" 16 150 20s

# 4️⃣ WFQ — CPU cực cao, queue_len lớn
run_phase "WFQ_PHASE" 32 400 20s

echo "======================================="
echo " DONE – Check $LOG to see algorithm distribution."
echo "======================================="
