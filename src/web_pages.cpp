#include "web_pages.h"

const char home_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WordClock Main</title>
<style>
    body { font-family: Arial, sans-serif; background:#111824; color:#eef4ff; margin:0; }
    .topnav { position:sticky; top:0; z-index:10; background:#1b2738; border-bottom:1px solid #324760; padding:10px 8px; display:flex; gap:8px; flex-wrap:wrap; }
    .topnav a { color:#e7f0ff; text-decoration:none; padding:8px 10px; border:1px solid #45607f; border-radius:8px; background:#24344a; font-size:13px; }
    .topnav a:hover { background:#305073; }
    .wrap { max-width:900px; margin:0 auto; padding:14px 8px; box-sizing:border-box; }
    .hero { background:#202f44; border:1px solid #3d5572; border-radius:12px; padding:14px; margin-bottom:12px; }
    .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(140px, 1fr)); gap:10px; }
    .card { background:#223249; border:1px solid #405a77; border-radius:10px; padding:12px; }
    .btn { display:inline-block; margin-top:8px; padding:7px 10px; border-radius:8px; border:1px solid #6d8eb8; color:#fff; text-decoration:none; background:#3a69b0; font-size:13px; }
    @media (max-width:480px) {
        .wrap { padding: 8px 4px; }
        .topnav { padding: 8px 4px; gap: 4px; }
        .topnav a { padding: 6px 8px; font-size: 11px; }
        .grid { grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 8px; }
        .card { padding: 10px; }
        .hero { padding: 10px; }
        h2 { margin: 8px 0; font-size: 18px; }
        h3 { margin: 6px 0; font-size: 14px; }
        p { margin: 4px 0; font-size: 12px; }
    }
</style>
</head>
<body>
<div class="topnav">
    <a href="/main">Main</a>
    <a href="/wifi">Wifi</a>
    <a href="/mqtt">MQTT</a>
    <a href="/live">Live</a>
    <a href="/test">Test</a>
</div>
<div class="wrap">
    <div class="hero">
        <h2>WordClock Hauptseite</h2>
        <p>Wähle eine Unterseite über die Top-Leiste oder die Kacheln unten.</p>
    </div>
    <div class="grid">
        <div class="card"><h3>Main</h3><p>Konfiguration und Neustart.</p><a class="btn" href="/main">Öffnen</a></div>
        <div class="card"><h3>Wifi</h3><p>WLAN-Einstellungen und Netzwerkscan.</p><a class="btn" href="/wifi">Öffnen</a></div>
        <div class="card"><h3>MQTT</h3><p>Broker, User und Port konfigurieren.</p><a class="btn" href="/mqtt">Öffnen</a></div>
        <div class="card"><h3>Live</h3><p>Live-Vorschau und Steuerung.</p><a class="btn" href="/live">Öffnen</a></div>
        <div class="card"><h3>Test</h3><p>Schnelltests für LEDs und Clock.</p><a class="btn" href="/test">Öffnen</a></div>
    </div>
</div>
</body>
</html>
)rawliteral";

const char setup_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WordClock Setup</title>

<style>
    body {
        font-family: Arial, sans-serif;
        background: #111;
        color: #eee;
        text-align: center;
        margin: 0;
    }
    .topnav {
        position: sticky;
        top: 0;
        z-index: 10;
        background: #1a2432;
        border-bottom: 1px solid #304359;
        padding: 10px 12px;
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
        justify-content: center;
    }
    .topnav a {
        color: #eef6ff;
        text-decoration: none;
        border: 1px solid #4e6785;
        background: #26384f;
        border-radius: 8px;
        padding: 7px 11px;
        font-size: 14px;
    }
    .topnav a:hover { background: #2f4e70; }
    .box {
        background: #222;
        padding: 16px 12px;
        border-radius: 10px;
        max-width: 420px;
        margin: 16px auto;
        box-shadow: 0 0 10px #000;
    }
    select, input {
        width: 100%;
        padding: 10px;
        margin: 8px 0;
        border-radius: 6px;
        border: none;
        font-size: 16px;
        box-sizing: border-box;
    }
    button {
        width: 100%;
        padding: 12px;
        background: #4CAF50;
        border: none;
        color: white;
        font-size: 16px;
        border-radius: 6px;
        cursor: pointer;
        margin-top: 10px;
        transition: background 0.3s ease;
        box-sizing: border-box;
    }
    button:hover {
        background: #45a049;
    }
    @media (max-width:480px) {
        .box { padding: 12px 8px; margin: 12px 8px; }
        select, input { font-size: 16px; padding: 8px; }
        button { padding: 10px; font-size: 14px; }
        .topnav { padding: 8px 6px; gap: 4px; }
        .topnav a { padding: 6px 8px; font-size: 12px; }
    }
</style>
</head>

<body>

<div class="topnav">
    <a href="/main">Main</a>
    <a href="/wifi">Wifi</a>
    <a href="/mqtt">MQTT</a>
    <a href="/live">Live</a>
    <a href="/test">Test</a>
</div>

<h2>WordClock Setup</h2>

<div class="box">
    <form id="setupForm">

        <div id="wifiSection" style="display:none;">
            <h3>WLAN Einstellungen</h3>

            <label>Netzwerk auswählen:</label><br>
            <select id="ssid_list" name="ssid"></select>
            <input id="ssid_manual" type="text" placeholder="oder SSID manuell eingeben" style="margin-top:4px;">

            <label>WLAN Passwort:</label><br>
            <div style="position:relative;">
                <input name="wifi_pass" id="wifi_pass" type="password" placeholder="optional">
                <span onclick="toggleWifi()" style="position:absolute; right:10px; top:12px; cursor:pointer;">👁</span>
            </div>

            <button id="saveBtn" type="button" onclick="saveSettings()">Speichern</button>
        </div>

        <div id="mqttSection" style="display:none;">
            <h3>MQTT Einstellungen</h3>

            <label>MQTT Server:</label><br>
            <input name="mqtt_server" id="mqtt_server" placeholder="192.168.1.10"><br>

            <label>MQTT Port:</label><br>
            <input name="mqtt_port" id="mqtt_port" type="number" value="1883"><br>

            <label>MQTT Benutzer:</label><br>
            <input name="mqtt_user" id="mqtt_user" placeholder="optional"><br>

            <label>MQTT Passwort:</label><br>
            <div style="position:relative;">
                <input name="mqtt_pass" id="mqtt_pass" type="password" placeholder="optional">
                <span onclick="toggleMQTT()" style="position:absolute; right:10px; top:12px; cursor:pointer;">👁</span>
            </div>

            <button id="saveBtn" type="button" onclick="saveSettings()">Speichern</button>
        </div>

        <div id="mainSection" style="display:none;">
            <h3>Allgemein</h3>
            <p>Wähle eine Option unten aus.</p>
        </div>

    </form>

    <button type="button" onclick="confirmReboot()" id="rebootBtn" style="display:none;">Neustart</button>
    <button type="button" onclick="location.href='/live'" id="liveBtn" style="display:none;">Live-Vorschau & Schnelltest</button>
</div>

<script>
// ---------------------------------------------------------
// Route-basiertes Anzeigen
// ---------------------------------------------------------
(function() {
    const path = location.pathname;
    const wifiSection = document.getElementById('wifiSection');
    const mqttSection = document.getElementById('mqttSection');
    const mainSection = document.getElementById('mainSection');
    const rebootBtn = document.getElementById('rebootBtn');
    const liveBtn = document.getElementById('liveBtn');
    
    if (path === '/wifi') {
        wifiSection.style.display = 'block';
        rebootBtn.style.display = 'block';
        liveBtn.style.display = 'block';
        document.querySelector('h2').textContent = 'WLAN Konfiguration';
    } else if (path === '/mqtt') {
        mqttSection.style.display = 'block';
        rebootBtn.style.display = 'block';
        liveBtn.style.display = 'block';
        document.querySelector('h2').textContent = 'MQTT Konfiguration';
    } else {
        mainSection.style.display = 'block';
        rebootBtn.style.display = 'block';
        liveBtn.style.display = 'block';
        document.querySelector('h2').textContent = 'WordClock Setup';
    }
})();

// ---------------------------------------------------------
// Stabiler SSID-Scan mit Cache
// ---------------------------------------------------------
let lastSSIDs = [];
let scanInProgress = false;

function loadSSIDs() {
    if (scanInProgress) return;
    scanInProgress = true;

    fetch("/scan")
      .then(r => r.json())
      .then(list => {
          scanInProgress = false;

          if (!list || list.length === 0) return;

          if (JSON.stringify(list) === JSON.stringify(lastSSIDs)) return;

          lastSSIDs = list;

          let sel = document.getElementById("ssid_list");
          let current = sel.value;

          sel.innerHTML = "";

          list.forEach(ssid => {
              let opt = document.createElement("option");
              opt.value = ssid;
              opt.textContent = ssid;
              sel.appendChild(opt);
          });

          if (list.includes(current)) {
              sel.value = current;
          }
      })
      .catch(() => {
          scanInProgress = false;
      });
}

// Schnelles Polling: alle 2 s wenn noch keine Netze sichtbar, sonst alle 5 s
function scheduleScan() {
    let delay = (lastSSIDs.length === 0) ? 2000 : 5000;
    setTimeout(function() {
        loadSSIDs();
        scheduleScan();
    }, delay);
}

// Nur auf /wifi Seite scannen
if (location.pathname === '/wifi') {
    loadSSIDs();
    scheduleScan();
}

// ---------------------------------------------------------
// Passwort-Toggles
// ---------------------------------------------------------
function toggleWifi() {
    let f = document.getElementById("wifi_pass");
    f.type = (f.type === "password") ? "text" : "password";
}
function toggleMQTT() {
    let f = document.getElementById("mqtt_pass");
    f.type = (f.type === "password") ? "text" : "password";
}

// ---------------------------------------------------------
// Speichern nur relevante Parameter je nach Route
// ---------------------------------------------------------
function saveSettings() {
    let btn = document.getElementById("saveBtn");
    btn.style.background = "#777";
    btn.innerText = "Speichere…";

    const path = location.pathname;
    let params = new URLSearchParams();
    
    if (path === '/wifi') {
        // Nur WLAN Parameter
        let ssidManual = document.getElementById("ssid_manual").value.trim();
        let ssidSelect = document.getElementById("ssid_list").value;
        params.set("ssid", ssidManual || ssidSelect);
        params.set("wifi_pass", document.getElementById("wifi_pass").value);
    } else if (path === '/mqtt') {
        // Nur MQTT Parameter
        params.set("mqtt_server", document.getElementById("mqtt_server").value);
        params.set("mqtt_port", document.getElementById("mqtt_port").value);
        params.set("mqtt_user", document.getElementById("mqtt_user").value);
        params.set("mqtt_pass", document.getElementById("mqtt_pass").value);
    }

    fetch("/save", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params
    })
        .then(r => r.json())
        .then(j => {
            if (j.status === "ok") {
                btn.style.background = "#4CAF50";
                btn.innerText = "Gespeichert!";
                setTimeout(() => {
                    btn.style.background = "";
                    btn.innerText = "Speichern";
                }, 1200);
            } else {
                btn.style.background = "#a00";
                btn.innerText = j.msg || "Fehler";
                setTimeout(() => { btn.disabled = false; btn.style.background = ""; btn.innerText = "Speichern"; }, 2000);
            }
        })
        .catch(() => {
            btn.style.background = "#a00";
            btn.innerText = "Fehler";
            setTimeout(() => { btn.disabled = false; btn.style.background = ""; btn.innerText = "Speichern"; }, 2000);
        });
}

// ---------------------------------------------------------
// Neustart mit Bestätigung
// ---------------------------------------------------------
function confirmReboot() {
    if (!confirm("WordClock wirklich neu starten?")) return;
    fetch("/reboot");
    alert("WordClock startet neu…");
}
</script>

</body>
</html>
)rawliteral";

const char live_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WordClock Live</title>
<style>
        body { font-family: Arial, sans-serif; background:#1a2330; color:#f4f8ff; margin:0; }
        .topnav { position:sticky; top:0; z-index:10; background:#1b2738; border-bottom:1px solid #324760; padding:10px 14px; display:flex; gap:10px; flex-wrap:wrap; }
        .topnav a { color:#e7f0ff; text-decoration:none; padding:8px 12px; border:1px solid #45607f; border-radius:8px; background:#24344a; }
        .topnav a:hover { background:#305073; }
        .wrap { max-width: 640px; margin: 0 auto; padding:16px; }
        .card { background:#253144; border:1px solid #3d4f68; border-radius:12px; padding:16px; margin-bottom:14px; box-shadow:0 8px 22px rgba(0,0,0,0.18); }
        h2, h3 { margin: 0 0 10px; }
        label { display:block; margin:10px 0 4px; color:#d6e0f2; }
        input, select, button { width:100%; padding:10px; border-radius:8px; border:1px solid #587095; background:#1f2a3c; color:#f2f7ff; }
        input[type='range'] { padding:0; }
        .grid2 { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
        .row { display:flex; align-items:center; gap:8px; }
        .muted { color:#c7d3ea; font-size:14px; }
        .ok { color:#8ff7a5; }
        .warn { color:#ff9a9a; font-weight:700; }
        .statusGrid { display:grid; grid-template-columns:repeat(2, minmax(0, 1fr)); gap:8px; margin-top:10px; }
        .statusItem { background:#1b2637; border:1px solid #3e5069; border-radius:8px; padding:8px 10px; }
        .statusLabel { color:#a9bbd8; font-size:12px; text-transform:uppercase; letter-spacing:0.04em; }
        .statusValue { color:#f4f8ff; font-size:15px; font-weight:700; margin-top:3px; }
        .detailTable { margin-top:10px; display:grid; grid-template-columns:1fr auto; gap:6px 12px; font-size:14px; }
        .detailKey { color:#a9bbd8; }
        .detailVal { color:#f4f8ff; text-align:right; }
        .btn { cursor:pointer; background:#3a69b0; border-color:#5b8ad5; }
        .btn:hover { background:#4a7ac5; }
        .btn.warn { background:#8f4545; border-color:#b76464; }
        .memAlert { display:none; margin-top:10px; padding:10px 12px; border-radius:8px; border:1px solid #b76464; background:#45272a; color:#ffd7d7; font-weight:700; }
        .memAlert.warn { display:block; border-color:#c28a35; background:#48371f; color:#ffe0ab; }
        .memAlert.critical { display:block; border-color:#d15d5d; background:#4b2525; color:#ffd1d1; }
        .matrixWrap {
            position:relative;
            width:min(95vw, 560px);
            margin-top:10px;
            padding:14px 12px;
            border-radius:12px;
            background:#121a27;
            border:1px solid #41526b;
            aspect-ratio:1 / 1;
        }
        .matrix {
            display:grid;
            grid-template-columns:repeat(11, minmax(0, 1fr));
            gap:4px;
            width:100%;
            height:100%;
        }
        .cell {
            border:none;
            border-radius:0;
            padding:0;
            text-align:center;
            font-size:12px;
            background:transparent;
            aspect-ratio:1 / 1;
            display:flex;
            justify-content:center;
            align-items:center;
            overflow:hidden;
        }
        .glyph {
            font-family: Bahnschrift, "DIN 1451 Mittelschrift", "Arial Narrow", Arial, sans-serif;
            font-size:clamp(12px, 2.05vw, 20px);
            font-weight:700;
            color:#435672;
            line-height:1;
            letter-spacing:-0.03em;
            text-transform:uppercase;
            transform:scaleX(0.88);
            transform-origin:center;
            -webkit-font-smoothing:antialiased;
            text-rendering:geometricPrecision;
            transition:color 50ms linear, text-shadow 50ms linear;
        }
        .minuteSlot {
            width:100%;
            height:100%;
            display:flex;
            align-items:center;
            justify-content:center;
        }
        .minutePaw {
            width:88%;
            height:88%;
            background-color:#435672;
            -webkit-mask-image:url('/Pfote.png');
            mask-image:url('/Pfote.png');
            -webkit-mask-repeat:no-repeat;
            mask-repeat:no-repeat;
            -webkit-mask-position:center;
            mask-position:center;
            -webkit-mask-size:contain;
            mask-size:contain;
            transition:background-color 50ms linear, filter 50ms linear;
        }
</style>
</head>
<body>
<div class="topnav">
    <a href="/main">Main</a>
    <a href="/wifi">Wifi</a>
    <a href="/mqtt">MQTT</a>
    <a href="/live">Live</a>
    <a href="/test">Test</a>
</div>
<div class="wrap">
    <div class="card">
        <h2>WordClock Live-Vorschau</h2>
        <div class="muted">Direktes Anwenden von Farbe, Effekt und Helligkeit.</div>
    </div>

    <div class="card">
        <h3>Status</h3>
        <div id="memAlert" class="memAlert"></div>
        <div id="statusGrid" class="statusGrid">
            <div class="statusItem"><div class="statusLabel">Power</div><div id="stPower" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">Effekt</div><div id="stEffect" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">RTC</div><div id="stRtc" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">WiFi</div><div id="stWifi" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">MQTT</div><div id="stMqtt" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">RAM</div><div id="stMem" class="statusValue">-</div></div>
        </div>
        <div id="statusDetails" class="detailTable"></div>
        <div id="applyMsg" class="muted" style="margin-top:6px;"></div>
        <div class="muted" style="margin-top:8px;">Refresh: 10/s (fest)</div>
    </div>

    <div class="card">
        <h3>WordClock Layout Live</h3>
        <div class="matrixWrap">
            <div id="matrix" class="matrix"></div>
        </div>
    </div>

    <div class="card">
        <h3>Live-Steuerung</h3>
        <label>Power</label>
        <select id="power">
            <option value="ON">ON</option>
            <option value="OFF">OFF</option>
        </select>

        <label>Effekt</label>
        <select id="effect">
            <option value="clock">clock (light)</option><option value="wifi">wifi (light)</option><option value="waterdrop">waterdrop (light)</option><option value="love">love (light)</option>
            <option value="colorloop">colorloop (medium)</option><option value="colorwipe">colorwipe (light)</option><option value="fire2d">fire2d (heavy)</option><option value="matrix">matrix (heavy)</option><option value="plasma">plasma (heavy)</option><option value="inward">inward (medium)</option><option value="twinkle">twinkle (medium)</option><option value="balls">balls (medium)</option><option value="aurora">aurora (medium)</option><option value="enchantment">enchantment (medium)</option><option value="snake">snake (medium)</option>
        </select>

        <label>Farbe</label>
        <div style="display:flex; gap:10px; align-items:center;">
            <input id="color" type="color" value="#ff9900" oninput="document.getElementById('colorPreview').style.backgroundColor=this.value; document.getElementById('colorHex').textContent=this.value.toUpperCase();">
            <div id="colorPreview" style="width:60px; height:40px; border-radius:8px; background:#ff9900; border:2px solid #587095; box-shadow:0 0 10px rgba(255,153,0,0.6);"></div>
            <span id="colorHex" style="font-size:12px; color:#afd6ff;font-family:monospace;">#FF9900</span>
        </div>

        <label>Helligkeit: <span id="brv">120</span></label>
        <input id="brightness" type="range" min="0" max="255" value="120" oninput="brv.textContent=this.value">

        <div class="grid2" style="margin-top:10px;">
            <button class="btn" onclick="applyLive()">Anwenden</button>
            <button class="btn" onclick="location.href='/'">Zur Setup-Seite</button>
        </div>
    </div>

    <div class="card">
        <h3>Schnelltest</h3>
        <div class="grid2">
            <button class="btn" onclick="quick('all_on')">Alle AN</button>
            <button class="btn warn" onclick="quick('all_off')">Alle AUS</button>
            <button class="btn" onclick="quick('clock_test')">Clock Test</button>
            <button class="btn" onclick="quick('gradient')">Gradient Test</button>
        </div>
    </div>
</div>

<script>
let isDirty = false;
let refreshTimer = null;
let refreshInFlight = false;

function markDirty() { isDirty = true; }

const layoutRows = [
    'ESSISTTFUENF',
    'ZEHNZWANZIG',
    'DREIVIERTEL',
    'VORLOVENACH',
    'HALBEELFUENF',
    'DREIYOUVIER',
    'SECHSSIEBEN',
    'ZEHNEUNZWEI',
    'AACHTZWÖLFF',
    'EINSUHR....'
];

function layoutGlyphAt(x, y) {
    if (y < 0 || y >= layoutRows.length) return '';
    const row = layoutRows[y];
    if (x < 0 || x >= row.length) return '';
    const ch = row.charAt(x);
    return ch === '.' ? '' : ch;
}

function brightenHex(hex, factor) {
    const n = parseInt(hex, 16);
    if (!Number.isFinite(n)) return hex;
    const r = (n >> 16) & 0xFF;
    const g = (n >> 8) & 0xFF;
    const b = n & 0xFF;
    const br = Math.min(255, Math.round(r * factor));
    const bg = Math.min(255, Math.round(g * factor));
    const bb = Math.min(255, Math.round(b * factor));
    return ((br << 16) | (bg << 8) | bb).toString(16).toUpperCase().padStart(6, '0');
}

function renderMatrix(matrix, mqttConnected) {
    const box = document.getElementById('matrix');
    if (!Array.isArray(matrix)) {
        box.innerHTML = '';
        return;
    }
    let html = '';
    for (let y = 0; y < matrix.length; y++) {
        for (let x = 0; x < matrix[y].length; x++) {
            const hex = String(matrix[y][x] || '000000').toUpperCase();
            const isOn = hex !== '000000';
            const uiHex = isOn ? brightenHex(hex, 1.35) : '435672';
            const isMinuteCell = (y === 9 && x >= 7 && x <= 10);
            if (isMinuteCell) {
                // Minuten-LED: Rot wenn MQTT nicht verbunden, sonst normal
                let pawColor = mqttConnected ? ('#' + uiHex) : '#ff4444';
                let pawFilter = mqttConnected 
                    ? (isOn ? ('drop-shadow(0 0 5px #' + uiHex + ') drop-shadow(0 0 11px #' + uiHex + ')') : 'none')
                    : 'drop-shadow(0 0 5px #ff4444) drop-shadow(0 0 11px #ff4444)';
                html += '<div class="cell" title="x=' + x + ' y=' + y + ' [MINUTE]">'
                    + '<div class="minuteSlot"><div class="minutePaw" style="background-color:' + pawColor + ';filter:' + pawFilter + ';"></div></div>'
                    + '</div>';
            } else {
                const glyph = layoutGlyphAt(x, y);
                const glyphColor = '#' + uiHex;
                const glow = isOn ? ('0 0 8px #' + uiHex + ', 0 0 16px #' + uiHex) : 'none';
                html += '<div class="cell" title="x=' + x + ' y=' + y + '">'
                    + '<div class="glyph" style="color:' + glyphColor + ';text-shadow:' + glow + ';">' + glyph + '</div>'
                    + '</div>';
            }
        }
    }
    box.innerHTML = html;
}

async function refreshStatus() {
    if (refreshInFlight) return;
    refreshInFlight = true;
    try {
        const r = await fetch('/api/status');
        const s = await r.json();
        const rtcWarn = !!s.rtc_warning;
        const rtcBadge = rtcWarn ? '<span class="warn">WARNUNG</span>' : '<span class="ok">OK</span>';
        const rtcTempText = (typeof s.rtc_temp_c === 'number' && Number.isFinite(s.rtc_temp_c))
            ? (s.rtc_temp_c.toFixed(2) + ' C')
            : 'n/a';
        const mqttConnected = !!s.mqtt_connected;
        const mqttBadge = mqttConnected ? '<span class="ok">Verbunden</span>' : '<span class="warn">Getrennt</span>';
        const memLevel = String(s.mem_level || 'OK').toUpperCase();
        const memFree = Number.isFinite(Number(s.mem_free)) ? Number(s.mem_free) : 0;
        const memTotal = Number.isFinite(Number(s.mem_total)) && Number(s.mem_total) > 0 ? Number(s.mem_total) : 1;
        const memUsedPct = ((memTotal - memFree) * 100.0 / memTotal).toFixed(1);
        const memBadge = (memLevel === 'CRITICAL')
            ? '<span class="warn">KRITISCH</span>'
            : (memLevel === 'WARNING' ? '<span class="warn">WARNUNG</span>' : '<span class="ok">OK</span>');
        
        document.getElementById('stPower').innerHTML = s.state;
        document.getElementById('stEffect').innerHTML = s.effect;
        document.getElementById('stRtc').innerHTML = rtcBadge;
        document.getElementById('stWifi').innerHTML = (s.rssi + ' dBm');
        document.getElementById('stMqtt').innerHTML = mqttBadge;
        document.getElementById('stMem').innerHTML = memBadge;

        const memAlert = document.getElementById('memAlert');
        memAlert.className = 'memAlert';
        memAlert.textContent = '';
        if (memLevel === 'CRITICAL') {
            memAlert.className = 'memAlert critical';
            memAlert.textContent = 'Low Memory: Kritischer RAM-Zustand. Schwere Effekte werden automatisch verlassen.';
        } else if (memLevel === 'WARNING') {
            memAlert.className = 'memAlert warn';
            memAlert.textContent = 'Low Memory: RAM wird knapp. Bitte eher leichte Effekte verwenden.';
        }

        document.getElementById('statusDetails').innerHTML =
            '<div class="detailKey">IP</div><div class="detailVal ok">' + s.ip + '</div>' +
            '<div class="detailKey">Farbe</div><div class="detailVal">' + s.color + '</div>' +
            '<div class="detailKey">Helligkeit</div><div class="detailVal">' + s.brightness + '</div>' +
            '<div class="detailKey">RTC Temperatur</div><div class="detailVal">' + rtcTempText + '</div>' +
            '<div class="detailKey">RTC OSF/Batterie</div><div class="detailVal">' + (s.rtc_battery_warning ? 'Auffaellig' : 'OK') + '</div>' +
            '<div class="detailKey">RAM Frei</div><div class="detailVal">' + memFree + ' B</div>' +
            '<div class="detailKey">RAM Nutzung</div><div class="detailVal">' + memUsedPct + '%</div>' +
            '<div class="detailKey">Max Block</div><div class="detailVal">' + (s.mem_max_alloc || 0) + ' B</div>';

        if (!isDirty) {
            document.getElementById('power').value = s.state;
            document.getElementById('effect').value = s.effect;
            document.getElementById('brightness').value = s.brightness;
            document.getElementById('brv').textContent = s.brightness;
            if (/^#[0-9a-fA-F]{6}$/.test(s.color)) {
                document.getElementById('color').value = s.color;
            }
        }

        renderMatrix(s.matrix, mqttConnected);
    } catch (_) {}
    refreshInFlight = false;
}

async function applyLive() {
    const msg = document.getElementById('applyMsg');
    msg.textContent = 'Sende...';

    const p = new URLSearchParams();
    p.set('state', document.getElementById('power').value);
    p.set('effect', document.getElementById('effect').value);
    p.set('color', document.getElementById('color').value);
    p.set('brightness', document.getElementById('brightness').value);
    const r = await fetch('/api/preview', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: p
    });
    msg.textContent = r.ok ? 'Angewendet' : 'Fehler beim Anwenden';
    setTimeout(() => { msg.textContent = ''; }, 1500);
    isDirty = false;
    refreshStatus();
}

async function quick(action) {
    const p = new URLSearchParams();
    p.set('action', action);
    await fetch('/api/quicktest', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: p
    });
    refreshStatus();
}

document.getElementById('power').addEventListener('change', markDirty);
document.getElementById('effect').addEventListener('change', markDirty);
document.getElementById('color').addEventListener('input', markDirty);
document.getElementById('brightness').addEventListener('input', markDirty);

function startRefreshTimer() {
    const periodMs = Math.floor(1000 / 10);
    if (refreshTimer) clearInterval(refreshTimer);
    refreshTimer = setInterval(refreshStatus, periodMs);
}

refreshStatus();
startRefreshTimer();
</script>
</body>
</html>
)rawliteral";

const char test_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WordClock Test</title>
<style>
    body { font-family: Arial, sans-serif; background:#172131; color:#f1f6ff; margin:0; }
    .topnav { position:sticky; top:0; z-index:10; background:#1b2738; border-bottom:1px solid #324760; padding:10px 14px; display:flex; gap:10px; flex-wrap:wrap; }
    .topnav a { color:#e7f0ff; text-decoration:none; padding:8px 12px; border:1px solid #45607f; border-radius:8px; background:#24344a; }
    .topnav a:hover { background:#305073; }
    .wrap { max-width:760px; margin:0 auto; padding:14px 8px; }
    .card { background:#233249; border:1px solid #415a78; border-radius:12px; padding:14px; }
    .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(120px, 1fr)); gap:8px; margin-top:10px; }
    button { width:100%; padding:10px 6px; border-radius:8px; border:1px solid #5f82b0; background:#3a69b0; color:#fff; cursor:pointer; font-size:14px; }
    button.warn { background:#8f4545; border-color:#b76464; }
    button.info { background:#456a8f; }
    .msg { color:#cfdcf2; margin-top:10px; min-height:20px; }
    .section-title { margin-top:12px; font-weight:bold; color:#afc8e8; font-size:13px; }
    @media (max-width:480px) {
        .wrap { padding:8px 4px; }
        .card { padding:10px; }
        .grid { grid-template-columns:repeat(auto-fit, minmax(100px, 1fr)); gap:6px; }
        button { padding:9px 4px; font-size:12px; }
        .topnav { padding:8px 6px; gap:4px; }
        .topnav a { padding:6px 8px; font-size:12px; }
    }
</style>
</head>
<body>
<div class="topnav">
    <a href="/main">Main</a>
    <a href="/wifi">Wifi</a>
    <a href="/mqtt">MQTT</a>
    <a href="/live">Live</a>
    <a href="/test">Test</a>
</div>
<div class="wrap">
    <div class="card">
        <h2>WordClock Schnelltests</h2>
        <p>Teste Farben, LEDs, Helligkeit und Muster.</p>
        
        <div class="section-title">Basis-Tests</div>
        <div class="grid">
            <button onclick="quick('all_on')">Alle AN</button>
            <button class="warn" onclick="quick('all_off')">Alle AUS</button>
            <button onclick="quick('clock_test')">Clock Test</button>
            <button onclick="quick('gradient')">Gradient Test</button>
        </div>

        <div class="section-title">Farb-Tests</div>
        <div class="grid">
            <button style="background:#e74c3c" onclick="quick('color_red')">Rot</button>
            <button style="background:#27ae60" onclick="quick('color_green')">Grün</button>
            <button style="background:#3498db" onclick="quick('color_blue')">Blau</button>
            <button style="background:#f39c12" onclick="quick('color_yellow')">Gelb</button>
            <button style="background:#1abc9c" onclick="quick('color_cyan')">Cyan</button>
            <button style="background:#e91e63" onclick="quick('color_magenta')">Magenta</button>
        </div>

        <div class="section-title">Helligkeit (50% Weiß)</div>
        <div class="grid">
            <button class="info" onclick="quick('brightness_0')">0%</button>
            <button class="info" onclick="quick('brightness_25')">25%</button>
            <button class="info" onclick="quick('brightness_50')">50%</button>
            <button class="info" onclick="quick('brightness_75')">75%</button>
            <button class="info" onclick="quick('brightness_100')">100%</button>
        </div>

        <div class="section-title">Spezial-Tests</div>
        <div class="grid">
            <button onclick="quick('rainbow')">Regenbogen</button>
            <button onclick="quick('blink')">Blinken</button>
            <button onclick="quick('pulse')">Puls</button>
            <button onclick="quick('spiral')">Spirale</button>
        </div>

        <div id="msg" class="msg"></div>
    </div>
</div>
<script>
async function quick(action) {
    const msg = document.getElementById('msg');
    msg.textContent = 'Sende Testbefehl...';
    const p = new URLSearchParams();
    p.set('action', action);
    const r = await fetch('/api/quicktest', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: p
    });
    msg.textContent = r.ok ? 'Test ausgefuehrt' : 'Fehler beim Test';
    setTimeout(() => { msg.textContent = ''; }, 1500);
}
</script>
</body>
</html>
)rawliteral";
