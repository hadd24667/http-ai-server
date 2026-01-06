const WS_URL = "ws://localhost:9000/ws/metrics";

const el = (id) => document.getElementById(id);

const wsState = el("wsState");
const lastSeen = el("lastSeen");
const algoNow = el("algoNow");

const cpuVal = el("cpuVal");
const qVal = el("qVal");
const schedVal = el("schedVal");

// AI panel
const ai_cpu = el("ai_cpu");
const ai_q = el("ai_q");
const ai_est = el("ai_est");
const ai_size = el("ai_size");
const ai_method = el("ai_method");
const ai_pathlen = el("ai_pathlen");

const ai_pred = el("ai_pred");
const ai_fallback = el("ai_fallback");
const ai_run = el("ai_run");
const ai_enq = el("ai_enq");
const ai_rt = el("ai_rt");
const ai_latavg = el("ai_latavg");

const ALGOS = ["FIFO","SJF","RR","WFQ","ADAPTIVE","UNKNOWN"];
const algoIndex = (a) => {
  const i = ALGOS.indexOf(a);
  return i === -1 ? ALGOS.length - 1 : i;
};

function fmtTime(ts) {
  if (!ts) return "-";
  const d = new Date(ts);
  if (isNaN(d.getTime())) return ts;
  return d.toLocaleTimeString();
}

function safeNum(v, digits=1) {
  if (v === null || v === undefined || Number.isNaN(v)) return "-";
  return Number(v).toFixed(digits);
}

// store events
let events = [];
const MAX_EVENTS = 2000;

// ===== Charts (Chart.js) =====
let timelineChart, latencyChart, throughputChart;

function initCharts() {
  // Timeline: y = algo index
  timelineChart = new Chart(document.getElementById("timelineChart"), {
    type: "line",
    data: {
      labels: [],
      datasets: [
        {
          label: "Algorithm",
          data: [],
          tension: 0.2,
          pointRadius: 0,
        },
        {
          label: "Switch",
          data: [],
          showLine: false,
          pointRadius: 5,
        }
      ],
    },
    options: {
      responsive: true,
      animation: false,
      scales: {
        y: {
          ticks: {
            callback: (v) => ALGOS[v] ?? "?",
          },
          suggestedMin: 0,
          suggestedMax: ALGOS.length - 1,
        }
      },
      plugins: {
        tooltip: {
          callbacks: {
            label: (ctx) => {
              if (ctx.datasetIndex === 0) return `Algo: ${ALGOS[ctx.parsed.y] ?? "?"}`;
              return `Switch: ${ALGOS[ctx.parsed.y] ?? "?"}`;
            }
          }
        }
      }
    }
  });

  // Avg latency by algo (bar)
  latencyChart = new Chart(document.getElementById("latencyChart"), {
    type: "bar",
    data: { labels: [], datasets: [{ label: "Avg latency (ms)", data: [] }] },
    options: { responsive: true, animation: false }
  });

  // Throughput by algo (bar)
  throughputChart = new Chart(document.getElementById("throughputChart"), {
    type: "bar",
    data: { labels: [], datasets: [{ label: "Throughput (req/s)", data: [] }] },
    options: { responsive: true, animation: false }
  });
}

function updateOverview(last) {
  cpuVal.textContent = last?.cpu != null ? `${safeNum(last.cpu, 1)}%` : "-";
  qVal.textContent = last?.queue_len != null ? `${last.queue_len}` : "-";
  schedVal.textContent = last?.algo_at_run ?? "-";

  lastSeen.textContent = `Last: ${fmtTime(last?.timestamp)}`;
  algoNow.textContent = `Algo: ${last?.algo_at_run ?? "-"}`;
}

function updateAIPanel(last) {
  ai_cpu.textContent = last?.cpu ?? "-";
  ai_q.textContent = last?.queue_len ?? "-";
  ai_est.textContent = last?.estimated_workload ?? "-";
  ai_size.textContent = last?.req_size ?? "-";
  ai_method.textContent = last?.request_method ?? "-";
  ai_pathlen.textContent = last?.request_path_length ?? "-";

  ai_run.textContent = last?.algo_at_run ?? "-";
  ai_enq.textContent = last?.algo_at_enqueue ?? "-";
  ai_rt.textContent = last?.response_time_ms != null ? safeNum(last.response_time_ms, 2) : "-";
  ai_latavg.textContent = last?.prev_latency_avg != null ? safeNum(last.prev_latency_avg, 2) : "-";

  // nếu log có ai_predicted/ai_fallback thì show, nếu không thì báo
  ai_pred.textContent = last?.ai_predicted ?? "(chưa log ai_predicted)";
  ai_fallback.textContent = (last?.ai_fallback !== undefined) ? String(last.ai_fallback) : "(chưa log ai_fallback)";
}

function updateTimeline() {
  // lấy subset cuối để mượt
  const slice = events.slice(-300);
  const labels = slice.map(e => fmtTime(e.timestamp));
  const y = slice.map(e => algoIndex(e.algo_at_run || "UNKNOWN"));

  // marker switch
  const switchPoints = [];
  for (let i = 1; i < slice.length; i++) {
    const prev = slice[i-1].algo_at_run;
    const cur = slice[i].algo_at_run;
    if (prev && cur && prev !== cur) {
      switchPoints.push({ x: labels[i], y: algoIndex(cur) });
    }
  }

  timelineChart.data.labels = labels;
  timelineChart.data.datasets[0].data = y;

  // Switch dataset uses {x,y}
  timelineChart.data.datasets[1].data = switchPoints;
  timelineChart.update();
}

function updatePerfCharts() {
  // group by algo
  const map = new Map();
  for (const e of events) {
    const algo = e.algo_at_run || "UNKNOWN";
    if (!map.has(algo)) map.set(algo, { algo, count: 0, sumLatency: 0 });
    const g = map.get(algo);
    g.count += 1;
    g.sumLatency += Number(e.response_time_ms ?? 0);
  }

  // time window for throughput (overall duration)
  let dt = 1;
  if (events.length >= 2) {
    const t0 = new Date(events[0].timestamp).getTime();
    const t1 = new Date(events[events.length - 1].timestamp).getTime();
    dt = Math.max(1, (t1 - t0) / 1000);
  }

  const arr = Array.from(map.values())
    .map(g => ({
      algo: g.algo,
      avgLatency: g.count ? g.sumLatency / g.count : 0,
      throughput: g.count / dt,
    }))
    .sort((a,b) => algoIndex(a.algo) - algoIndex(b.algo));

  latencyChart.data.labels = arr.map(x => x.algo);
  latencyChart.data.datasets[0].data = arr.map(x => Number(x.avgLatency.toFixed(2)));
  latencyChart.update();

  throughputChart.data.labels = arr.map(x => x.algo);
  throughputChart.data.datasets[0].data = arr.map(x => Number(x.throughput.toFixed(2)));
  throughputChart.update();
}

function pushEvent(e) {
  events.push(e);
  if (events.length > MAX_EVENTS) events = events.slice(events.length - MAX_EVENTS);

  const last = events[events.length - 1];
  updateOverview(last);
  updateAIPanel(last);

  updateTimeline();
  updatePerfCharts();
}

// ===== WebSocket connect =====
function connectWS() {
  const ws = new WebSocket(WS_URL);

  ws.onopen = () => {
    wsState.textContent = "WS: connected";
    wsState.className = "pill green";

    // ping keepalive
    ws._ping = setInterval(() => {
      if (ws.readyState === 1) ws.send("ping");
    }, 5000);
  };

  ws.onclose = () => {
    wsState.textContent = "WS: disconnected";
    wsState.className = "pill red";
    if (ws._ping) clearInterval(ws._ping);

    // reconnect
    setTimeout(connectWS, 1000);
  };

  ws.onerror = () => { /* ignore */ };

  ws.onmessage = (evt) => {
    const msg = JSON.parse(evt.data);
    if (msg.type === "snapshot" && Array.isArray(msg.data)) {
      events = msg.data;
      // render full
      if (events.length) {
        updateOverview(events[events.length - 1]);
        updateAIPanel(events[events.length - 1]);
      }
      updateTimeline();
      updatePerfCharts();
      return;
    }
    if (msg.type === "metric") {
      pushEvent(msg.data);
    }
  };
}

initCharts();
connectWS();
