#!/bin/bash
set -e

bash ./scripts/run_fifo.sh
bash ./scripts/run_sjf.sh
bash ./scripts/run_rr.sh
bash ./scripts/run_wfq.sh

echo ""
echo "✅ DONE: all 4 algorithms finished. Raw logs in data/raw/"
