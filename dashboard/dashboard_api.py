# dashboard_api.py
import os, asyncio, csv
from collections import deque
from typing import Dict, Any, List
from datetime import datetime, timezone 

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware

LOG_PATH = os.getenv("SERVER_LOG_PATH", "./data/logs/http_server_log.csv")
HISTORY_LIMIT = int(os.getenv("HISTORY_LIMIT", "2000"))

app = FastAPI(title="HTTP-AI Dashboard API")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

history = deque(maxlen=HISTORY_LIMIT)
clients: List[WebSocket] = []


def _to_int(v):
    try:
        return int(float(v))
    except Exception:
        return None


def _to_float(v):
    try:
        return float(v)
    except Exception:
        return None


def parse_csv_line(header, line):
    try:
        row = next(csv.DictReader([line], fieldnames=header))
        row = {k.strip(): v.strip() for k, v in row.items()}

        # ===== ALIAS MAP (QUAN TRỌNG NHẤT) =====
        alias_map = {
            "response_ms": "response_time_ms",
            "latency_avg": "prev_latency_avg"
        }

        for src, dst in alias_map.items():
            if src in row and dst not in row:
                row[dst] = row[src]

        # ép kiểu cho frontend
        row["cpu"] = float(row.get("cpu", 0))
        row["queue_len"] = int(row.get("queue_len", 0))
        row["response_time_ms"] = float(row.get("response_time_ms", 0))
        row["prev_latency_avg"] = float(row.get("prev_latency_avg", 0))

        return row
    except Exception as e:
        return None


async def broadcast(obj: Dict[str, Any]):
    dead = []
    for ws in clients:
        try:
            await ws.send_json(obj)
        except Exception:
            dead.append(ws)
    for ws in dead:
        try:
            clients.remove(ws)
        except ValueError:
            pass


async def tail_log_forever():
    while not os.path.exists(LOG_PATH):
        print(f"[DASH] Waiting log file: {LOG_PATH}")
        await asyncio.sleep(1)

    with open(LOG_PATH, "r", encoding="utf-8", errors="ignore") as f:
        header_line = f.readline().strip()
        header = [h.strip() for h in header_line.split(",")]

        # load nhanh cuối file để UI có dữ liệu ngay
        rest = f.read().splitlines()
        for line in rest[-500:]:
            row = parse_csv_line(header, line)
            if row:
                history.append(row)

        f.seek(0, os.SEEK_END)
        print("[DASH] Tail started.")

        while True:
            line = f.readline()
            if not line:
                await asyncio.sleep(0.2)
                continue
            line = line.strip()
            if not line:
                continue
            row = parse_csv_line(header, line)
            if not row:
                continue

            history.append(row)
            await broadcast({"type": "metric", "data": row})


@app.on_event("startup")
async def startup_event():
    asyncio.create_task(tail_log_forever())


@app.get("/api/history")
def get_history(limit: int = 500):
    limit = max(1, min(limit, HISTORY_LIMIT))
    return list(history)[-limit:]


@app.websocket("/ws/metrics")
async def ws_metrics(ws: WebSocket):
    await ws.accept()
    clients.append(ws)
    try:
        if history:
            await ws.send_json({"type": "snapshot", "data": list(history)[-200:]})
        while True:
            await ws.receive_text()  # ping/pong
    except WebSocketDisconnect:
        pass
    finally:
        if ws in clients:
            clients.remove(ws)


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("dashboard_api:app", host="0.0.0.0", port=9000, reload=True)
