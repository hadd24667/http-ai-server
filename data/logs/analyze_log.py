import sys
import csv
from collections import defaultdict
from statistics import mean

def analyze_log(path):
    data = []

    with open(path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            # Convert numeric fields
            row["cpu"] = float(row["cpu"])
            row["queue_len"] = int(row["queue_len"])
            row["request_path_length"] = int(row["request_path_length"])
            row["estimated_workload"] = int(row["estimated_workload"])
            row["req_size"] = int(row["req_size"])
            row["response_ms"] = float(row["response_ms"])      # ← CHỈNH Ở ĐÂY
            row["latency_avg"] = float(row["latency_avg"])      # ← CHỈNH Ở ĐÂY
            data.append(row)

    if not data:
        print("❌ File log rỗng hoặc không đúng format.")
        return

    total = len(data)
    print(f"\n=============================")
    print(f"  📊 LOG SUMMARY")
    print(f"=============================")
    print(f"Total requests: {total}")

    # Mean metrics
    avg_cpu = mean([x["cpu"] for x in data])
    avg_latency = mean([x["latency_avg"] for x in data])      # ← CHỈNH
    avg_resp = mean([x["response_ms"] for x in data])         # ← CHỈNH
    avg_qlen = mean([x["queue_len"] for x in data])

    print(f"Average CPU: {avg_cpu:.2f}%")
    print(f"Average latency: {avg_latency:.3f} ms")
    print(f"Average response time: {avg_resp:.3f} ms")
    print(f"Average queue length: {avg_qlen:.2f}")

    # Count per algorithm
    algo_count = defaultdict(int)
    algo_resp = defaultdict(list)
    algo_latency = defaultdict(list)

    for row in data:
        algo = row["algo_at_run"]
        algo_count[algo] += 1
        algo_resp[algo].append(row["response_ms"])         # ← CHỈNH
        algo_latency[algo].append(row["latency_avg"])      # ← CHỈNH

    print("\n=============================")
    print("  ⚙️  ALGORITHM USAGE")
    print("=============================")

    for algo in ["FIFO", "SJF", "RR", "WFQ"]:
        count = algo_count.get(algo, 0)
        percent = (count / total) * 100
        print(f"{algo:<5}: {count:6d}  ({percent:5.1f}%)")

    print("\n=============================")
    print("  ⏱ AVERAGE RESPONSE / ALGO")
    print("=============================")

    for algo in algo_resp:
        print(f"{algo:<5}: {mean(algo_resp[algo]):.3f} ms")

    print("\n=============================")
    print("  📉 AVERAGE LATENCY / ALGO")
    print("=============================")

    for algo in algo_latency:
        print(f"{algo:<5}: {mean(algo_latency[algo]):.3f} ms")

    print("\nDone.\n")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python analyze_log.py logfile.csv")
        sys.exit(1)

    analyze_log(sys.argv[1])
