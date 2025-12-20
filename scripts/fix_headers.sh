#!/bin/bash

RAW_DIR="data/raw"
HEADER="timestamp,cpu,queue_len,request_method,request_path_length,estimated_workload,algo_at_enqueue,algo_at_run,req_size,response_ms,latency_avg"

echo "===== CHECKING RAW LOG FILES ====="
echo "Directory: $RAW_DIR"
echo ""

for f in "$RAW_DIR"/*.csv; do
    # Bỏ qua nếu không phải file
    [ -f "$f" ] || continue

    # Đọc dòng đầu tiên
    first_line=$(head -n 1 "$f")

    if [[ "$first_line" == timestamp,* ]]; then
        echo "[OK] Header exists → $f"
    else
        echo "[FIX] Adding header → $f"

        # Tạo file tạm có header
        tmp_file="${f}.tmp"
        echo "$HEADER" > "$tmp_file"
        cat "$f" >> "$tmp_file"

        # Ghi đè file cũ
        mv "$tmp_file" "$f"
    fi
done

echo ""
echo "===== DONE. ALL HEADERS FIXED ====="
