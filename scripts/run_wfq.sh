#!/bin/bash
set -e

source ./scripts/workloads.sh

RAW_DIR="data/raw"
SERVER_LOG="data/logs/http_server_log.csv"
mkdir -p "${RAW_DIR}"

for i in "${!WORKLOADS[@]}"; do
  IDX=$((i+1))
  OUT_FILE="${RAW_DIR}/W${IDX}_WFQ.csv"

  echo ""
  echo "======================================="
  echo "[WFQ] Workload W${IDX}"
  echo "======================================="

  ./build/server/http_server --algo=WFQ --threads=2 &
  SERVER_PID=$!
  sleep 3

  : > "${SERVER_LOG}"

  echo "[WFQ] CMD: ${WORKLOADS[$i]}"
  bash -c "${WORKLOADS[$i]}"

  sleep 1

  if [ -s "${SERVER_LOG}" ]; then
    cp "${SERVER_LOG}" "${OUT_FILE}"
    echo "[WFQ] Saved -> ${OUT_FILE}"
  else
    echo "[WFQ] ⚠ NO LOG for W${IDX}"
  fi

  kill -9 "${SERVER_PID}" >/dev/null 2>&1 || true
  sleep 1
done
