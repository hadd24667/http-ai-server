import sys
import os
import math
import pandas as pd
import matplotlib.pyplot as plt
from collections import defaultdict

# ============================
#  Helper: ensure output dir
# ============================
def ensure_dir(path: str):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)


# ============================
#  Helper: percentiles
# ============================
def percentile(series, q):
    return float(series.quantile(q / 100.0))


# ============================
#  Main analysis function
# ============================
def analyze_advanced(csv_path: str):
    out_dir = "analysis_outputs"
    ensure_dir(out_dir)

    print(f"Loading log from: {csv_path}")
    df = pd.read_csv(csv_path)

    # Check required columns
    required_cols = [
        "cpu",
        "queue_len",
        "request_method",
        "request_path_length",
        "estimated_workload",
        "algo_at_enqueue",
        "algo_at_run",
        "req_size",
        "response_ms",
        "latency_avg",
    ]
    missing = [c for c in required_cols if c not in df.columns]
    if missing:
        print(f"❌ Missing columns in CSV: {missing}")
        sys.exit(1)

    # ============================
    #  Convert types safely
    # ============================
    num_cols = [
        "cpu",
        "queue_len",
        "request_path_length",
        "estimated_workload",
        "req_size",
        "response_ms",
        "latency_avg",
    ]
    for c in num_cols:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    df = df.dropna(subset=["response_ms"])  # response_ms là bắt buộc

    total = len(df)
    if total == 0:
        print("❌ No valid rows after parsing numeric columns.")
        sys.exit(1)

    print("\n=============================")
    print("  📊 GLOBAL SUMMARY")
    print("=============================")
    avg_cpu = df["cpu"].mean()
    avg_q   = df["queue_len"].mean()
    avg_lat = df["latency_avg"].mean()
    avg_rsp = df["response_ms"].mean()

    print(f"Total requests          : {total}")
    print(f"Average CPU (%)         : {avg_cpu:.2f}")
    print(f"Average queue length    : {avg_q:.2f}")
    print(f"Average latency (ms)    : {avg_lat:.3f}")
    print(f"Average response (ms)   : {avg_rsp:.3f}")

    # Percentiles for response_ms
    p50 = percentile(df["response_ms"], 50)
    p90 = percentile(df["response_ms"], 90)
    p95 = percentile(df["response_ms"], 95)
    p99 = percentile(df["response_ms"], 99)

    print("\nLatency percentiles (response_ms):")
    print(f"  P50 : {p50:.3f} ms")
    print(f"  P90 : {p90:.3f} ms")
    print(f"  P95 : {p95:.3f} ms")
    print(f"  P99 : {p99:.3f} ms")

    # ============================
    #  Per-algorithm stats
    # ============================
    print("\n=============================")
    print("  ⚙️  PER-ALGORITHM STATS")
    print("=============================")

    algos = sorted(df["algo_at_run"].unique())
    per_algo_stats = {}

    for algo in algos:
        sub = df[df["algo_at_run"] == algo]
        n   = len(sub)
        if n == 0:
            continue

        ratio = n / total * 100.0

        a_cpu = sub["cpu"].mean()
        a_q   = sub["queue_len"].mean()
        a_lat = sub["latency_avg"].mean()
        a_rsp = sub["response_ms"].mean()

        a_p50 = percentile(sub["response_ms"], 50)
        a_p90 = percentile(sub["response_ms"], 90)
        a_p95 = percentile(sub["response_ms"], 95)
        a_p99 = percentile(sub["response_ms"], 99)

        per_algo_stats[algo] = {
            "count": n,
            "ratio": ratio,
            "avg_cpu": a_cpu,
            "avg_q": a_q,
            "avg_lat": a_lat,
            "avg_rsp": a_rsp,
            "p50": a_p50,
            "p90": a_p90,
            "p95": a_p95,
            "p99": a_p99,
        }

    for algo in per_algo_stats:
        st = per_algo_stats[algo]
        print(f"\nAlgorithm: {algo}")
        print(f"  Count            : {st['count']} ({st['ratio']:.2f}%)")
        print(f"  Avg CPU (%)      : {st['avg_cpu']:.2f}")
        print(f"  Avg queue length : {st['avg_q']:.2f}")
        print(f"  Avg latency (ms) : {st['avg_lat']:.3f}")
        print(f"  Avg response (ms): {st['avg_rsp']:.3f}")
        print(f"  P50 latency (ms) : {st['p50']:.3f}")
        print(f"  P90 latency (ms) : {st['p90']:.3f}")
        print(f"  P95 latency (ms) : {st['p95']:.3f}")
        print(f"  P99 latency (ms) : {st['p99']:.3f}")

    # ============================
    #  Plots
    # ============================

    # 1) Algorithm usage
    algo_counts = df["algo_at_run"].value_counts()
    plt.figure(figsize=(6,4))
    algo_counts.plot(kind="bar")
    plt.title("Algorithm Usage Distribution")
    plt.xlabel("Algorithm")
    plt.ylabel("Count")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "alg_usage.png"))
    plt.close()

    # 2) Histogram of response_ms (overall)
    plt.figure(figsize=(6,4))
    plt.hist(df["response_ms"], bins=50)
    plt.title("Response Time Histogram (Overall)")
    plt.xlabel("Response Time (ms)")
    plt.ylabel("Frequency")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "response_hist_overall.png"))
    plt.close()

    # 3) Boxplot of response_ms per algo
    plt.figure(figsize=(6,4))
    data_box = [df[df["algo_at_run"] == algo]["response_ms"] for algo in algos]
    plt.boxplot(data_box, labels=algos, showfliers=False)
    plt.title("Response Time per Algorithm (Boxplot, no outliers)")
    plt.xlabel("Algorithm")
    plt.ylabel("Response Time (ms)")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "response_box_per_algo.png"))
    plt.close()

    # Color map cho algo
    colors_map = {}
    base_colors = ["tab:blue", "tab:orange", "tab:green", "tab:red", "tab:purple", "tab:brown"]
    for i, algo in enumerate(algos):
        colors_map[algo] = base_colors[i % len(base_colors)]

    # 4) Scatter: queue_len vs response_ms, colored by algo
    plt.figure(figsize=(7,5))
    for algo in algos:
        sub = df[df["algo_at_run"] == algo]
        plt.scatter(sub["queue_len"], sub["response_ms"],
                    s=5, alpha=0.5, label=algo, color=colors_map[algo])
    plt.xlabel("Queue Length")
    plt.ylabel("Response Time (ms)")
    plt.title("Queue Length vs Response Time (colored by Algorithm)")
    plt.legend(markerscale=3)
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "scatter_queue_vs_response.png"))
    plt.close()

    # 5) Scatter: cpu vs response_ms, colored by algo
    plt.figure(figsize=(7,5))
    for algo in algos:
        sub = df[df["algo_at_run"] == algo]
        plt.scatter(sub["cpu"], sub["response_ms"],
                    s=5, alpha=0.5, label=algo, color=colors_map[algo])
    plt.xlabel("CPU (%)")
    plt.ylabel("Response Time (ms)")
    plt.title("CPU vs Response Time (colored by Algorithm)")
    plt.legend(markerscale=3)
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "scatter_cpu_vs_response.png"))
    plt.close()

    # 6) Scatter: workload vs response_ms
    plt.figure(figsize=(7,5))
    plt.scatter(df["estimated_workload"], df["response_ms"], s=5, alpha=0.5)
    plt.xlabel("Estimated Workload")
    plt.ylabel("Response Time (ms)")
    plt.title("Estimated Workload vs Response Time")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "scatter_workload_vs_response.png"))
    plt.close()

    # 7) Correlation heatmap
    corr_cols = [
        "cpu",
        "queue_len",
        "request_path_length",
        "estimated_workload",
        "req_size",
        "response_ms",
        "latency_avg",
    ]
    corr = df[corr_cols].corr()

    plt.figure(figsize=(7,6))
    plt.imshow(corr, cmap="viridis")
    plt.colorbar(label="Correlation")
    plt.xticks(range(len(corr_cols)), corr_cols, rotation=45, ha="right")
    plt.yticks(range(len(corr_cols)), corr_cols)
    plt.title("Correlation Heatmap")
    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, "corr_heatmap.png"))
    plt.close()

    print(f"\n✅ Advanced analysis done. Plots saved in: {out_dir}/")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python analyze_advanced.py http_server_log.csv")
        sys.exit(1)

    analyze_advanced(sys.argv[1])
