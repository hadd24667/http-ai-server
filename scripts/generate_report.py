import sys
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Image, PageBreak
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import getSampleStyleSheet
import os

# ==========================================
#  UTILS: Save plot
# ==========================================
def save_plot(path, title):
    plt.title(title)
    plt.tight_layout()
    plt.savefig(path)
    plt.close()


# ==========================================
#  MAIN FUNCTION
# ==========================================
def generate_report(csv_path):

    # Load CSV
    df = pd.read_csv(csv_path)

    # Ensure required columns exist
    required_cols = [
        "cpu","queue_len","latency_avg","response_ms",
        "algo_at_run","request_path_length"
    ]
    for col in required_cols:
        if col not in df.columns:
            raise ValueError(f"Missing column: {col} in CSV file")

    # Summary
    summary = {
        "Total requests": len(df),
        "Average CPU (%)": round(df["cpu"].mean(), 2),
        "Average queue length": round(df["queue_len"].mean(), 2),
        "Average latency (ms)": round(df["latency_avg"].mean(), 3),
        "Average response time (ms)": round(df["response_ms"].mean(), 3),
    }

    # ==========================================
    #  PLOTS
    # ==========================================

    out_charts = {}

    # 1) Algorithm Distribution
    algo_counts = df["algo_at_run"].value_counts()
    plt.figure(figsize=(6,4))
    algo_counts.plot(kind="bar")
    save_plot("chart_alg_usage.png", "Algorithm Usage Distribution")
    out_charts["alg_usage"] = "chart_alg_usage.png"

    # 2) CPU Over Time
    plt.figure(figsize=(7,4))
    df["cpu"].plot()
    plt.ylabel("CPU (%)")
    plt.xlabel("Request Index")
    save_plot("chart_cpu.png", "CPU Load Over Time")
    out_charts["cpu"] = "chart_cpu.png"

    # 3) Queue Length Over Time
    plt.figure(figsize=(7,4))
    df["queue_len"].plot()
    plt.ylabel("Queue Length")
    plt.xlabel("Request Index")
    save_plot("chart_queue.png", "Queue Length Over Time")
    out_charts["queue"] = "chart_queue.png"

    # 4) Latency Scatter
    plt.figure(figsize=(7,4))
    plt.scatter(range(len(df)), df["response_ms"], s=3)
    plt.ylabel("Response Time (ms)")
    plt.xlabel("Request Index")
    save_plot("chart_latency.png", "Response Time Scatter Plot")
    out_charts["latency"] = "chart_latency.png"


    # ==========================================
    #  PDF GENERATION
    # ==========================================

    pdf_path = "adaptive_scheduler_report.pdf"
    styles = getSampleStyleSheet()
    doc = SimpleDocTemplate(pdf_path, pagesize=letter)
    story = []

    story.append(Paragraph("<b>Adaptive Scheduler – Detailed Performance Report</b>",
                           styles["Title"]))
    story.append(Spacer(1, 12))

    # Summary Section
    story.append(Paragraph("<b>1. Summary</b>", styles["Heading2"]))
    for k, v in summary.items():
        story.append(Paragraph(f"{k}: {v}", styles["Normal"]))
    story.append(Spacer(1, 12))

    # Add Charts
    story.append(PageBreak())

    story.append(Paragraph("<b>2. Algorithm Usage</b>", styles["Heading2"]))
    story.append(Image(out_charts["alg_usage"], width=400, height=300))
    story.append(Spacer(1, 24))

    story.append(Paragraph("<b>3. CPU Over Time</b>", styles["Heading2"]))
    story.append(Image(out_charts["cpu"], width=450, height=300))
    story.append(Spacer(1, 24))

    story.append(Paragraph("<b>4. Queue Length Over Time</b>", styles["Heading2"]))
    story.append(Image(out_charts["queue"], width=450, height=300))
    story.append(Spacer(1, 24))

    story.append(Paragraph("<b>5. Response Time Scatter</b>", styles["Heading2"]))
    story.append(Image(out_charts["latency"], width=450, height=300))

    doc.build(story)

    print(f"🎉 PDF report generated: {pdf_path}")


# ==========================================
#  ENTRYPOINT
# ==========================================
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python generate_report.py http_server_log.csv")
        sys.exit(1)

    generate_report(sys.argv[1])
