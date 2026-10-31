// The dashboard web page: HTML (layout), CSS (style), JavaScript (live updates).
// Stored on the ESP32 and sent to the browser when you open the board's address.
#pragma once

const char PAGE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Signal Mapper</title>
<style>
  :root { --bg:#0f1720; --card:#18222d; --txt:#e8eef4; --mut:#8a9bab;
          --good:#22c55e; --fair:#84cc16; --ok:#eab308; --bad:#ef4444; }
  * { box-sizing:border-box; }
  body { margin:0 auto; max-width:520px; padding:16px; background:var(--bg);
         color:var(--txt); font-family:-apple-system, system-ui, sans-serif; }
  h1 { font-size:20px; margin:4px 0 14px; }
  .card { background:var(--card); border-radius:16px; padding:16px; margin-bottom:14px; }
  .gauge { display:block; width:100%; max-width:320px; margin:0 auto; }
  #needle { transform-box:view-box; transform-origin:100px 100px; transform:rotate(-90deg);
            transition:transform .8s cubic-bezier(.3,1.4,.5,1); }
  .end { font-size:10px; font-weight:700; }
  #val { font-size:46px; font-weight:700; text-align:center; margin-top:10px; transition:color .5s; }
  .unit { font-size:16px; color:var(--mut); }
  #lbl { text-align:center; font-weight:600; font-size:18px; transition:color .5s; }
  #dot { display:inline-block; width:8px; height:8px; border-radius:50%; background:var(--good);
         margin-right:6px; animation:blink 1s infinite; }
  @keyframes blink { 50% { opacity:.2; } }
  .mut { color:var(--mut); font-size:13px; }
  .rooms { display:flex; flex-wrap:wrap; gap:8px; margin-top:10px; }
  button { border:0; border-radius:10px; padding:10px 12px; font-size:15px;
           background:#2a3846; color:var(--txt); }
  button:active { transform:scale(.95); }
  button.saved { background:var(--good); color:#08130c; }
  .row { display:flex; gap:8px; margin-top:10px; }
  input { flex:1; padding:10px; border-radius:10px; border:0; background:var(--bg);
          color:var(--txt); font-size:15px; }
  .bar { margin:10px 0; }
  .top { display:flex; justify-content:space-between; font-size:14px; }
  .track { height:12px; background:var(--bg); border-radius:6px; overflow:hidden; margin-top:4px; }
  .fill { height:100%; border-radius:6px; transition:width .6s; }
</style>
</head>
<body>
  <h1>&#128246; WiFi Signal Mapper</h1>

  <div class="card">
    <svg class="gauge" viewBox="0 0 200 132">
      <defs>
        <linearGradient id="g" x1="0" x2="1">
          <stop offset="0" stop-color="#ef4444"/>
          <stop offset=".5" stop-color="#eab308"/>
          <stop offset="1" stop-color="#22c55e"/>
        </linearGradient>
      </defs>
      <path d="M20 100 A80 80 0 0 1 180 100" fill="none" stroke="url(#g)"
            stroke-width="16" stroke-linecap="round"/>
      <line id="needle" x1="100" y1="100" x2="100" y2="30" stroke="#e8eef4"
            stroke-width="5" stroke-linecap="round"/>
      <text class="end" x="20" y="128" text-anchor="middle" fill="#ef4444">BAD</text>
      <text class="end" x="180" y="128" text-anchor="middle" fill="#22c55e">GOOD</text>
      <circle cx="100" cy="100" r="7" fill="#e8eef4"/>
    </svg>
    <div id="val">--<span class="unit"> dBm</span></div>
    <div id="lbl">Connecting...</div>
    <div class="mut" style="text-align:center;margin-top:6px"><span id="dot"></span>Live, updates every second</div>
  </div>

  <div class="card">
    <b>Save a reading</b>
    <div class="mut">Stand in a room for 5 seconds, then tap its name.</div>
    <div class="rooms" id="rooms"></div>
    <div class="row">
      <input id="custom" placeholder="Other room...">
      <button onclick="save(document.getElementById('custom').value, this)">Save</button>
    </div>
  </div>

  <div class="card">
    <b>Room ranking</b> <span class="mut">(strongest first)</span>
    <div id="list"></div>
    <div class="row"><button onclick="clearAll()">Clear all</button></div>
  </div>

<script>
  // If anything in this script breaks, show the error on the page instead of failing silently.
  window.onerror = function (msg, src, line) {
    var l = document.getElementById("lbl");
    if (l) { l.textContent = "Page error: " + msg + " (line " + line + ")"; l.style.color = "#ef4444"; }
  };
  const ROOMS = ["Living Room","Kitchen","My Bedroom","Basement","Bathroom","Office","Hallway","Backyard"];
  let recent = [];   // last 5 readings, averaged when you save a room
  let data = {};     // saved rooms -> signal
  try { data = JSON.parse(localStorage.getItem("wifimap") || "{}"); } catch (e) {}

  const $ = id => document.getElementById(id);

  // Turn a dBm number into a label + colour.
  function rate(r) {
    if (r >= -55) return ["Excellent", "var(--good)"];
    if (r >= -67) return ["Good", "var(--fair)"];
    if (r >= -75) return ["Weak", "var(--ok)"];
    return ["Dead zone", "var(--bad)"];
  }
  // -90 dBm = 0% (needle far left), -30 dBm = 100% (needle far right).
  const pct = r => Math.max(0, Math.min(1, (r + 90) / 60));

  // Every second: ask the ESP32 for the latest signal and update the dial.
  async function tick() {
    try {
      const res = await fetch("/data", { cache: "no-store" });
      const r = (await res.json()).rssi;
      recent.push(r); if (recent.length > 5) recent.shift();
      $("needle").style.transform = "rotate(" + (-90 + pct(r) * 180) + "deg)";
      const [label, colour] = rate(r);
      $("val").innerHTML = r + '<span class="unit"> dBm</span>';
      $("val").style.color = colour;
      $("lbl").textContent = label;
      $("lbl").style.color = colour;
      $("dot").style.background = "var(--good)";
    } catch (e) {
      $("lbl").textContent = "Board not reachable (likely a dead zone)";
      $("lbl").style.color = "var(--bad)";
      $("dot").style.background = "var(--bad)";
    }
  }

  function save(name, btn) {
    name = (name || "").trim();
    if (!name || recent.length === 0) return;
    data[name] = Math.round(recent.reduce((a, b) => a + b, 0) / recent.length);
    try { localStorage.setItem("wifimap", JSON.stringify(data)); } catch (e) {}
    $("custom").value = "";
    if (btn) { btn.classList.add("saved"); setTimeout(() => btn.classList.remove("saved"), 800); }
    render();
  }

  function clearAll() {
    if (!confirm("Clear all saved rooms?")) return;
    data = {};
    try { localStorage.removeItem("wifimap"); } catch (e) {}
    render();
  }

  const esc = s => s.replace(/[&<>"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));

  // Draw the ranking bars, strongest room first.
  function render() {
    const names = Object.keys(data).sort((a, b) => data[b] - data[a]);
    $("list").innerHTML = names.length ? names.map(n => {
      const r = data[n], [label, colour] = rate(r);
      return '<div class="bar"><div class="top"><span>' + esc(n) + '</span><span style="color:' + colour + '">' +
             r + ' dBm, ' + label + '</span></div><div class="track"><div class="fill" style="width:' +
             Math.max(4, pct(r) * 100) + '%;background:' + colour + '"></div></div></div>';
    }).join("") : '<div class="mut" style="margin-top:8px">No rooms saved yet.</div>';
  }

  $("rooms").innerHTML = ROOMS.map(n => '<button onclick="save(\'' + n + '\', this)">' + n + '</button>').join("");
  render();
  tick();
  setInterval(tick, 1000);
</script>
</body>
</html>
)rawliteral";
