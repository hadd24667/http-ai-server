import requests
import csv
import time
import random
from datetime import datetime

AI_URL = "http://127.0.0.1:5000/predict"
OUT_CSV = "../data/logs/ai_predictions.csv"

HEADERS = {
    "Content-Type": "application/json"
}

# ---- sinh workload giả lập để ép AI chọn đủ 4 thuật toán ----
def generate_sample():
    queue_len = random.choice([
        random.randint(0, 10),      # FIFO
        random.randint(15, 40),     # SJF
        random.randint(50, 100),    # WFQ
        random.randint(100, 200),   # RR / overload
    ])

    cpu = random.choice([
        random.uniform(5, 20),
        random.uniform(20, 50),
        random.uniform(50, 85)
    ])

    estimated_workload = random.choice([
        random.uniform(1, 5),     # short
        random.uniform(10, 30),   # medium
        random.uniform(50, 120)   # long
    ])

    return {
        "cpu": round(cpu, 2),
        "queue_len": queue_len,
        "queue_bin": None,
        "request_method": random.choice(["GET", "POST"]),
        "request_path_length": random.randint(1, 6),
        "estimated_workload": round(estimated_workload, 2),
        "req_size": random.randint(100, 5000)
    }

def main():
    print("▶ AI Predict Logger started...")
    print(f"▶ Output CSV: {OUT_CSV}")

    with open(OUT_CSV, mode="w", newline="") as f:
        writer = csv.writer(f)

        # CSV header (chuẩn để train lại)
        writer.writerow([
            "timestamp",
            "cpu",
            "queue_len",
            "request_method",
            "request_path_length",
            "estimated_workload",
            "req_size",
            "predicted_algorithm"
        ])

        for i in range(300):  # số sample
            payload = generate_sample()

            try:
                r = requests.post(AI_URL, json=payload, headers=HEADERS, timeout=1.0)
                r.raise_for_status()
                algo = r.json().get("algorithm", "UNKNOWN")

            except Exception as e:
                algo = "ERROR"
                print(f"[WARN] request failed: {e}")

            writer.writerow([
                datetime.now().isoformat(),
                payload["cpu"],
                payload["queue_len"],
                payload["request_method"],
                payload["request_path_length"],
                payload["estimated_workload"],
                payload["req_size"],
                algo
            ])

            print(f"[{i:03}] q={payload['queue_len']:3} "
                  f"cpu={payload['cpu']:5} "
                  f"work={payload['estimated_workload']:6} "
                  f"→ {algo}")

            time.sleep(0.05)  # tránh spam server

    print("✅ Done. CSV saved.")

if __name__ == "__main__":
    main()
