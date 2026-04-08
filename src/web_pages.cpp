#include "web_pages.h"

const char home_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WordClock Control Hub</title>
<style>
    :root {
        --bg-a: #0f131c;
        --bg-b: #1a2230;
        --panel: rgba(23, 30, 43, 0.92);
        --panel-soft: rgba(31, 40, 56, 0.9);
        --line: rgba(154, 168, 190, 0.22);
        --line-strong: rgba(188, 201, 224, 0.46);
        --text: #f3f5fb;
        --muted: #acb3c4;
        --accent: #93a2be;
        --accent-warm: #f2a15a;
    }
    * { box-sizing: border-box; }
    body {
        margin: 0;
        color: var(--text);
        font-family: "Sora", "Manrope", "Segoe UI", sans-serif;
        background:
            radial-gradient(circle at 12% 0%, rgba(147, 162, 190, 0.22), transparent 28%),
            radial-gradient(circle at 88% 10%, rgba(242, 161, 90, 0.12), transparent 24%),
            linear-gradient(180deg, var(--bg-b) 0%, var(--bg-a) 100%);
        min-height: 100vh;
    }
    .topnav {
        position: sticky;
        top: 0;
        z-index: 20;
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
        align-items: center;
        padding: 10px 10px;
        background: rgba(19, 26, 38, 0.82);
        border-bottom: 1px solid var(--line);
        backdrop-filter: blur(8px);
    }
    .topnav a {
        color: var(--text);
        text-decoration: none;
        padding: 8px 11px;
        border-radius: 10px;
        border: 1px solid var(--line);
        background: linear-gradient(145deg, rgba(44, 56, 77, 0.96), rgba(28, 36, 50, 0.96));
        font-size: 13px;
    }
    .topnav a:hover { border-color: var(--line-strong); }
    .wrap { max-width: 1020px; margin: 0 auto; padding: 16px 10px 24px; }
    .hero {
        position: relative;
        overflow: hidden;
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background: linear-gradient(145deg, rgba(37, 47, 66, 0.96), rgba(22, 29, 42, 0.98));
        padding: 18px;
        margin-bottom: 14px;
    }
    .hero:before {
        content: "";
        position: absolute;
        width: 260px;
        height: 260px;
        border-radius: 50%;
        right: -90px;
        top: -120px;
        background: radial-gradient(circle, rgba(242, 161, 90, 0.24), transparent 70%);
    }
    .hero h2 { margin: 0 0 8px; font-size: 24px; letter-spacing: 0.01em; }
    .hero p { margin: 0; color: #d7dde9; }
    .tagRow { margin-top: 12px; display: flex; gap: 8px; flex-wrap: wrap; }
    .tag {
        padding: 5px 9px;
        border-radius: 999px;
        font-size: 12px;
        border: 1px solid rgba(188, 201, 224, 0.32);
        color: #edf2fb;
        background: rgba(37, 48, 66, 0.92);
    }
    .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(170px, 1fr)); gap:10px; }
    .card {
        background: var(--panel);
        border: 1px solid var(--line);
        border-radius: 14px;
        padding: 12px;
        box-shadow: 0 10px 24px rgba(0, 0, 0, 0.24);
    }
    .card h3 { margin: 2px 0 6px; }
    .card p { margin: 0; color: var(--muted); min-height: 36px; }
    .btn {
        display:inline-block;
        margin-top:10px;
        padding:8px 10px;
        border-radius:10px;
        border:1px solid rgba(188, 201, 224, 0.32);
        color:#fff;
        text-decoration:none;
        background: linear-gradient(145deg, rgba(73, 90, 120, 0.98), rgba(44, 56, 77, 0.98));
        font-size:13px;
    }
    .btn:hover { border-color: rgba(242, 161, 90, 0.44); }
    @media (max-width:480px) {
        .wrap { padding: 10px 6px 16px; }
        .topnav { padding: 8px 4px; gap: 4px; }
        .topnav a { padding: 6px 8px; font-size: 11px; }
        .grid { grid-template-columns: repeat(auto-fit, minmax(132px, 1fr)); gap: 8px; }
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
    <a href="/ota">OTA</a>
    <a href="/live">Live</a>
    <a href="/test">Test</a>
</div>
<div class="wrap">
    <div class="hero">
        <h2>WordClock Control Hub</h2>
        <p>Moderne Steuerzentrale fuer Setup, Live-Regie, OTA und Tests.</p>
        <div class="tagRow">
            <span class="tag">Live Status</span>
            <span class="tag">OTA Ready</span>
            <span class="tag">MQTT + RTC</span>
        </div>
    </div>
    <div class="grid">
        <div class="card"><h3>Main</h3><p>Systemueberblick und Zugriff auf alle Einstellungen.</p><a class="btn" href="/main">Oeffnen</a></div>
        <div class="card"><h3>Wifi</h3><p>WLAN setzen, Netze scannen und Neustart ausloesen.</p><a class="btn" href="/wifi">Oeffnen</a></div>
        <div class="card"><h3>MQTT</h3><p>Broker, User, Port und Verbindung sauber einrichten.</p><a class="btn" href="/mqtt">Oeffnen</a></div>
        <div class="card"><h3>OTA</h3><p>Firmware-Status ansehen und Updates manuell starten.</p><a class="btn" href="/ota">Oeffnen</a></div>
        <div class="card"><h3>Live</h3><p>Direkte Effekt- und Farbregie mit Matrix-Vorschau.</p><a class="btn" href="/live">Oeffnen</a></div>
        <div class="card"><h3>Test</h3><p>Quicktests fuer LEDs, Farben, Clock und Muster.</p><a class="btn" href="/test">Oeffnen</a></div>
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
<title>WordClock Setup Studio</title>

<style>
    :root {
        --bg-a: #0f131c;
        --bg-b: #1a2230;
        --panel: rgba(23, 30, 43, 0.92);
        --panel-soft: rgba(31, 40, 56, 0.9);
        --line: rgba(154, 168, 190, 0.22);
        --line-strong: rgba(188, 201, 224, 0.46);
        --text: #f3f5fb;
        --muted: #acb3c4;
        --ok: #7af0a8;
        --warn: #f6b3a5;
    }
    * { box-sizing: border-box; }
    body {
        margin: 0;
        color: var(--text);
        font-family: "Sora", "Manrope", "Segoe UI", sans-serif;
        background:
            radial-gradient(circle at 6% 0%, rgba(147, 162, 190, 0.22), transparent 26%),
            radial-gradient(circle at 94% 12%, rgba(242, 161, 90, 0.12), transparent 24%),
            linear-gradient(180deg, var(--bg-b) 0%, var(--bg-a) 100%);
        min-height: 100vh;
    }
    .shell {
        max-width: 1240px;
        margin: 0 auto;
        padding: 14px 10px 24px;
    }
    .topbar {
        position: sticky;
        top: 0;
        z-index: 20;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 10px;
        flex-wrap: wrap;
        margin-bottom: 12px;
        padding: 12px 14px;
        border: 1px solid var(--line);
        border-radius: 20px;
        background: rgba(23, 30, 43, 0.84);
        backdrop-filter: blur(14px);
        box-shadow: 0 20px 54px rgba(0, 0, 0, 0.28);
    }
    .brand {
        display: flex;
        align-items: center;
        gap: 12px;
    }
    .brandMark {
        width: 40px;
        height: 40px;
        border-radius: 13px;
        background: linear-gradient(145deg, #94a4bf, #f2a15a);
    }
    .brandText strong {
        display: block;
        font-size: 14px;
        letter-spacing: 0.04em;
        text-transform: uppercase;
    }
    .brandText span {
        color: var(--muted);
        font-size: 12px;
    }
    .navLinks {
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
    }
    .navLinks a {
        color: var(--text);
        text-decoration: none;
        border: 1px solid var(--line);
        background: linear-gradient(145deg, rgba(44, 56, 77, 0.96), rgba(28, 36, 50, 0.96));
        border-radius: 999px;
        padding: 8px 12px;
        font-size: 13px;
    }
    .navLinks a.active,
    .navLinks a:hover {
        border-color: var(--line-strong);
        background: linear-gradient(145deg, rgba(73, 90, 120, 0.98), rgba(44, 56, 77, 0.98));
    }
    .wrap {
        max-width: 1040px;
        margin: 0 auto;
        padding: 0 0 24px;
    }
    .hero {
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background: linear-gradient(145deg, rgba(37, 47, 66, 0.96), rgba(22, 29, 42, 0.98));
        padding: 14px 16px;
        margin-bottom: 12px;
    }
    .hero h2 { margin: 0 0 6px; font-size: 24px; }
    .hero p { margin: 0; color: #d7dde9; }
    .box {
        background: var(--panel);
        padding: 16px 14px;
        border-radius: 14px;
        border: 1px solid var(--line);
        box-shadow: 0 12px 28px rgba(0, 0, 0, 0.25);
    }
    select, input {
        width: 100%;
        padding: 11px;
        margin: 8px 0;
        border-radius: 10px;
        border: 1px solid rgba(188, 201, 224, 0.28);
        font-size: 15px;
        box-sizing: border-box;
        color: var(--text);
        background: rgba(26, 34, 49, 0.92);
    }
    h3 { margin: 8px 0 8px; }
    label { color: #d6dde9; font-size: 13px; display: block; text-align: left; }
    button {
        width: 100%;
        padding: 11px;
        background: linear-gradient(145deg, rgba(73, 90, 120, 0.98), rgba(44, 56, 77, 0.98));
        border: 1px solid rgba(188, 201, 224, 0.32);
        color: white;
        font-size: 14px;
        border-radius: 10px;
        cursor: pointer;
        margin-top: 10px;
        transition: border-color 0.2s ease;
        box-sizing: border-box;
    }
    button:hover { border-color: rgba(242, 161, 90, 0.44); }
    .layout { display: grid; grid-template-columns: 1.2fr 1fr; gap: 12px; }
    .stack { display: grid; gap: 12px; }
    .statusGrid { display:grid; grid-template-columns:repeat(2, minmax(0, 1fr)); gap:8px; margin-top:10px; }
    .statusItem { background:rgba(29, 38, 53, 0.92); border:1px solid rgba(188, 201, 224, 0.2); border-radius:10px; padding:8px 10px; }
    .statusLabel { color:#acb3c4; font-size:11px; text-transform:uppercase; letter-spacing:0.04em; }
    .statusValue { color:#f4f8ff; font-size:15px; font-weight:700; margin-top:3px; }
    .detailTable { margin-top:10px; display:grid; grid-template-columns:1fr auto; gap:6px 10px; font-size:14px; }
    .detailKey { color:#acb3c4; text-align:left; }
    .detailVal { color:#f4f8ff; text-align:right; }
    .muted { color: var(--muted); font-size: 13px; }
    .ok { color: var(--ok); }
    .warn { color: var(--warn); }
    @media (max-width:480px) {
        .shell { padding: 10px 6px 16px; }
        .wrap { padding: 0 0 16px; }
        .hero h2 { font-size: 19px; }
        .layout { grid-template-columns: 1fr; }
        .box { padding: 12px 8px; }
        select, input { font-size: 16px; padding: 8px; }
        button { padding: 10px; font-size: 14px; }
        .topbar { padding: 9px 8px; border-radius: 14px; }
        .brandMark { width: 34px; height: 34px; }
        .brandText strong { font-size: 12px; }
        .brandText span { font-size: 11px; }
        .navLinks a { padding: 6px 9px; font-size: 12px; }
    }
</style>
</head>

<body>

<div class="shell">
    <div class="topbar">
        <div class="brand">
            <div class="brandMark"></div>
            <div class="brandText">
                <strong>WordClock Studio</strong>
                <span>Onboard control website</span>
            </div>
        </div>
        <div class="navLinks" id="setupNavLinks">
            <a href="/main">Setup</a>
            <a href="/wifi">Wifi</a>
            <a href="/mqtt">MQTT</a>
            <a href="/ota">OTA</a>
            <a href="/live">Studio</a>
            <a href="/test">Test</a>
        </div>
    </div>

<div class="wrap">
    <div class="hero">
        <h2>WordClock Setup Studio</h2>
        <p>Alle Konfigurationen plus Live-Systemstatus direkt auf einer Seite.</p>
    </div>

    <div class="layout">
        <div class="box">
            <form id="setupForm">

        <div id="wifiSection" style="display:none;">
            <h3>WLAN Einstellungen</h3>

            <label>Netzwerk auswaehlen:</label><br>
            <select id="ssid_list" name="ssid"></select>
            <input id="ssid_manual" type="text" placeholder="oder SSID manuell eingeben" style="margin-top:4px;">

            <label>WLAN Passwort:</label><br>
            <div style="position:relative;">
                <input name="wifi_pass" id="wifi_pass" type="password" placeholder="optional">
                <span onclick="toggleWifi()" style="position:absolute; right:10px; top:12px; cursor:pointer;">Show</span>
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
                <span onclick="toggleMQTT()" style="position:absolute; right:10px; top:12px; cursor:pointer;">Show</span>
            </div>

            <button id="saveBtn" type="button" onclick="saveSettings()">Speichern</button>
        </div>

        <div id="otaSection" style="display:none; text-align:left;">
            <h3>OTA Update</h3>
            <p>Aktuelle Firmware: <b id="otaFwVersion">-</b></p>
            <p>Netzwerkstatus: <b id="otaWifi">-</b></p>
            <button type="button" onclick="loadOtaInfo()">Status aktualisieren</button>
            <button type="button" onclick="checkOtaNow()">Jetzt auf Update pruefen</button>
            <p id="otaMsg" style="font-size:14px; min-height:20px;"></p>
        </div>

        <div id="mainSection" style="display:none;">
            <h3>Allgemein</h3>
            <p>Waehle eine Option unten aus.</p>
        </div>

            </form>

            <button type="button" onclick="confirmReboot()" id="rebootBtn" style="display:none;">Neustart</button>
            <button type="button" onclick="location.href='/live'" id="liveBtn" style="display:none;">Live-Vorschau & Schnelltest</button>
        </div>

        <div class="stack">
            <div class="box">
                <h3>Status</h3>
                <div id="setupStatusGrid" class="statusGrid">
                    <div class="statusItem"><div class="statusLabel">Power</div><div id="spPower" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">Effekt</div><div id="spEffect" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">RTC</div><div id="spRtc" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">WiFi</div><div id="spWifi" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">MQTT</div><div id="spMqtt" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">RAM</div><div id="spMem" class="statusValue">-</div></div>
                </div>
                <div id="setupStatusDetails" class="detailTable"></div>
                <div class="muted" style="margin-top:8px;">Refresh: 10/s (fest)</div>
            </div>

            <div class="box">
                <h3>Quick Access</h3>
                <p class="muted">Direkter Wechsel in Regie und Tests.</p>
                <button type="button" onclick="location.href='/live'">Live-Regie oeffnen</button>
                <button type="button" onclick="location.href='/test'">Tests oeffnen</button>
            </div>
        </div>
    </div>
</div>
</div>


<script>
// ---------------------------------------------------------
// Route-basiertes Anzeigen
// ---------------------------------------------------------
(function() {
    const path = location.pathname;
    const wifiSection = document.getElementById('wifiSection');
    const mqttSection = document.getElementById('mqttSection');
    const otaSection = document.getElementById('otaSection');
    const mainSection = document.getElementById('mainSection');
    const rebootBtn = document.getElementById('rebootBtn');
    const liveBtn = document.getElementById('liveBtn');
    const nav = document.getElementById('setupNavLinks');
    Array.from(nav.querySelectorAll('a')).forEach(link => {
        if (link.getAttribute('href') === path) {
            link.classList.add('active');
        }
    });
    
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
    } else if (path === '/ota') {
        otaSection.style.display = 'block';
        rebootBtn.style.display = 'block';
        liveBtn.style.display = 'block';
        document.querySelector('h2').textContent = 'OTA Firmware Update';
        loadOtaInfo();
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
    btn.innerText = "Speichere...";

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
// Neustart mit Best+�tigung
// ---------------------------------------------------------
function confirmReboot() {
    if (!confirm("WordClock wirklich neu starten?")) return;
    fetch("/reboot");
    alert("WordClock startet neu...");
}

async function loadOtaInfo() {
    const msg = document.getElementById('otaMsg');
    msg.textContent = 'Lade OTA Status...';
    try {
        const r = await fetch('/api/ota/info');
        const j = await r.json();
        document.getElementById('otaFwVersion').textContent = j.fw_version || '-';
        document.getElementById('otaWifi').textContent = j.wifi_connected ? ('Verbunden (' + (j.ip || '-') + ')') : 'Offline';
        msg.textContent = '';
    } catch (_) {
        msg.textContent = 'OTA Status konnte nicht geladen werden';
    }
}

async function checkOtaNow() {
    const msg = document.getElementById('otaMsg');
    msg.textContent = 'Pruefe Version...';
    try {
        const r = await fetch('/api/ota/check', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: ''
        });
        const j = await r.json();
        msg.textContent = j.message || (r.ok ? 'Pruefung abgeschlossen' : 'Fehler');
    } catch (_) {
        msg.textContent = 'OTA Pruefung fehlgeschlagen';
    }
}

async function refreshSetupStatus() {
    try {
        const r = await fetch('/api/status');
        const s = await r.json();

        const rtcBadge = s.rtc_warning ? '<span class="warn">WARNUNG</span>' : '<span class="ok">OK</span>';
        const mqttBadge = s.mqtt_connected ? '<span class="ok">Verbunden</span>' : '<span class="warn">Getrennt</span>';
        const memLevel = String(s.mem_level || 'OK').toUpperCase();
        const memBadge = (memLevel === 'CRITICAL' || memLevel === 'WARNING') ? '<span class="warn">' + memLevel + '</span>' : '<span class="ok">OK</span>';
        const rtcTempText = (typeof s.rtc_temp_c === 'number' && Number.isFinite(s.rtc_temp_c))
            ? (s.rtc_temp_c.toFixed(2) + ' C')
            : 'n/a';
        const memFree = Number.isFinite(Number(s.mem_free)) ? Number(s.mem_free) : 0;
        const memTotal = Number.isFinite(Number(s.mem_total)) && Number(s.mem_total) > 0 ? Number(s.mem_total) : 1;
        const memUsedPct = ((memTotal - memFree) * 100.0 / memTotal).toFixed(1);

        document.getElementById('spPower').innerHTML = s.state || '-';
        document.getElementById('spEffect').innerHTML = s.effect || '-';
        document.getElementById('spRtc').innerHTML = rtcBadge;
        document.getElementById('spWifi').innerHTML = (s.rssi + ' dBm');
        document.getElementById('spMqtt').innerHTML = mqttBadge;
        document.getElementById('spMem').innerHTML = memBadge;

        document.getElementById('setupStatusDetails').innerHTML =
            '<div class="detailKey">IP</div><div class="detailVal ok">' + (s.ip || '-') + '</div>' +
            '<div class="detailKey">Farbe</div><div class="detailVal">' + (s.color || '-') + '</div>' +
            '<div class="detailKey">Helligkeit</div><div class="detailVal">' + (s.brightness || '-') + '</div>' +
            '<div class="detailKey">RTC Temperatur</div><div class="detailVal">' + rtcTempText + '</div>' +
            '<div class="detailKey">RTC OSF/Batterie</div><div class="detailVal">' + (s.rtc_battery_warning ? 'Auffaellig' : 'OK') + '</div>' +
            '<div class="detailKey">RAM Frei</div><div class="detailVal">' + memFree + ' B</div>' +
            '<div class="detailKey">RAM Nutzung</div><div class="detailVal">' + memUsedPct + '%</div>' +
            '<div class="detailKey">Max Block</div><div class="detailVal">' + (s.mem_max_alloc || 0) + ' B</div>';
    } catch (_) {}
}

setInterval(refreshSetupStatus, 100);
refreshSetupStatus();
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
<title>WordClock Studio</title>
<style>
        :root {
            --bg: #0f131c;
            --bg-soft: #171d28;
            --panel: rgba(23, 30, 43, 0.9);
            --panel-soft: rgba(30, 38, 53, 0.9);
            --line: rgba(154, 168, 190, 0.24);
            --line-strong: rgba(188, 201, 224, 0.52);
            --text: #f3f5fb;
            --muted: #acb3c4;
            --accent: #9aa9c4;
            --accent-strong: #6e7f9e;
            --accent-warm: #f2a15a;
            --ok: #8de8ac;
            --warn: #f6b3a5;
            --shadow: 0 24px 70px rgba(0, 0, 0, 0.34);
            --radius-xl: 24px;
            --radius-lg: 18px;
            --radius-md: 14px;
        }
        [data-theme="light"] {
            --bg: #f4f5f9;
            --bg-soft: #e9ecf2;
            --panel: rgba(255, 255, 255, 0.88);
            --panel-soft: rgba(247, 248, 252, 0.92);
            --line: rgba(80, 95, 120, 0.2);
            --line-strong: rgba(67, 85, 114, 0.44);
            --text: #192233;
            --muted: #5e677a;
            --accent: #7f8da9;
            --accent-strong: #5d6880;
            --accent-warm: #b96d2a;
            --ok: #268b46;
            --warn: #b05643;
            --shadow: 0 18px 50px rgba(22, 40, 36, 0.14);
        }
        * { box-sizing: border-box; }
        html { scroll-behavior: smooth; }
        body {
            margin: 0;
            color: var(--text);
            font-family: "Manrope", "Segoe UI", sans-serif;
            background:
                radial-gradient(circle at top left, rgba(147, 162, 190, 0.24), transparent 30%),
                radial-gradient(circle at 85% 12%, rgba(242, 161, 90, 0.14), transparent 25%),
                linear-gradient(180deg, var(--bg-soft) 0%, var(--bg) 100%);
            min-height: 100vh;
        }
        .shell {
            max-width: 1280px;
            margin: 0 auto;
            padding: 14px 14px 28px;
        }
        .topnav {
            position: sticky;
            top: 0;
            z-index: 20;
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 10px;
            flex-wrap: wrap;
            margin-bottom: 16px;
            padding: 12px 14px;
            border: 1px solid var(--line);
            border-radius: 20px;
            background: color-mix(in srgb, var(--panel-soft) 82%, transparent);
            backdrop-filter: blur(14px);
            box-shadow: var(--shadow);
        }
        .brand {
            display: flex;
            align-items: center;
            gap: 12px;
        }
        .brandMark {
            width: 42px;
            height: 42px;
            border-radius: 14px;
            background: linear-gradient(145deg, #94a4bf, var(--accent-warm));
            box-shadow: 0 10px 30px color-mix(in srgb, var(--accent) 45%, transparent);
        }
        .brandText strong {
            display: block;
            font-size: 15px;
            letter-spacing: 0.04em;
            text-transform: uppercase;
        }
        .brandText span {
            color: var(--muted);
            font-size: 12px;
        }
        .navLinks {
            display: flex;
            flex-wrap: wrap;
            gap: 8px;
        }
        .navLinks a {
            color: var(--text);
            text-decoration: none;
            padding: 9px 12px;
            border-radius: 999px;
            border: 1px solid var(--line);
            background: color-mix(in srgb, var(--panel-soft) 92%, transparent);
            font-size: 13px;
        }
        .navLinks a.active,
        .navLinks a:hover {
            border-color: var(--line-strong);
            background: color-mix(in srgb, var(--accent) 18%, transparent);
        }
        .hero {
            display: grid;
            grid-template-columns: 1.3fr 0.9fr;
            gap: 16px;
            margin-bottom: 16px;
        }
        .heroCard,
        .heroAside,
        .panel {
            position: relative;
            overflow: hidden;
            border-radius: var(--radius-xl);
            border: 1px solid var(--line);
            background: linear-gradient(180deg, rgba(13, 24, 39, 0.95), rgba(10, 19, 30, 0.9));
            box-shadow: var(--shadow);
        }
        .heroCard {
            padding: 28px;
            min-height: 230px;
            border-color: var(--line-strong);
            background:
                radial-gradient(circle at top right, color-mix(in srgb, #9aa9c4 28%, transparent), transparent 36%),
                linear-gradient(145deg, color-mix(in srgb, var(--panel-soft) 94%, transparent), color-mix(in srgb, var(--panel) 94%, transparent));
        }
        .heroCard:after {
            content: "";
            position: absolute;
            inset: auto -80px -80px auto;
            width: 220px;
            height: 220px;
            border-radius: 50%;
            background: radial-gradient(circle, rgba(255, 160, 91, 0.24), transparent 70%);
        }
        .eyebrow {
            display: inline-flex;
            padding: 6px 10px;
            border-radius: 999px;
            border: 1px solid color-mix(in srgb, var(--accent) 34%, transparent);
            background: color-mix(in srgb, var(--accent) 20%, transparent);
            color: color-mix(in srgb, var(--text) 90%, white 10%);
            font-size: 11px;
            letter-spacing: 0.08em;
            text-transform: uppercase;
        }
        .heroCard h1 {
            margin: 14px 0 10px;
            font-size: clamp(30px, 5vw, 52px);
            line-height: 0.98;
            letter-spacing: -0.04em;
        }
        .heroCard p {
            max-width: 620px;
            margin: 0;
            color: color-mix(in srgb, var(--text) 78%, var(--muted) 22%);
            font-size: 15px;
            line-height: 1.6;
        }
        .heroMeta {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            margin-top: 18px;
        }
        .heroMeta span {
            padding: 8px 11px;
            border-radius: 999px;
            border: 1px solid color-mix(in srgb, var(--line-strong) 55%, transparent);
            background: color-mix(in srgb, var(--panel-soft) 88%, transparent);
            color: color-mix(in srgb, var(--text) 85%, white 15%);
            font-size: 12px;
        }
        .navLinks a#themeToggle {
            border-color: color-mix(in srgb, var(--accent-warm) 55%, transparent);
            background: color-mix(in srgb, var(--accent-warm) 18%, transparent);
        }
        .heroAside {
            padding: 18px;
            display: grid;
            gap: 12px;
            align-content: start;
        }
        .miniPanel {
            padding: 14px;
            border-radius: var(--radius-lg);
            border: 1px solid color-mix(in srgb, var(--line-strong) 34%, transparent);
            background: color-mix(in srgb, var(--panel-soft) 88%, transparent);
        }
        .miniLabel {
            color: var(--muted);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.08em;
        }
        .miniValue {
            margin-top: 8px;
            font-size: 24px;
            font-weight: 700;
        }
        .layout {
            display: grid;
            grid-template-columns: minmax(0, 1.55fr) minmax(320px, 0.95fr);
            gap: 16px;
        }
        .column {
            display: grid;
            gap: 16px;
            align-content: start;
        }
        .panel {
            padding: 18px;
        }
        .panelHead {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 10px;
            margin-bottom: 14px;
        }
        .panelHead h2,
        .panelHead h3 {
            margin: 0;
            letter-spacing: -0.02em;
        }
        .pill {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 7px 10px;
            border-radius: 999px;
            border: 1px solid color-mix(in srgb, var(--line-strong) 38%, transparent);
            background: color-mix(in srgb, var(--panel-soft) 86%, transparent);
            color: color-mix(in srgb, var(--text) 88%, white 12%);
            font-size: 12px;
        }
        .statusGrid {
            display: grid;
            grid-template-columns: repeat(3, minmax(0, 1fr));
            gap: 10px;
        }
        .statusItem {
            border-radius: var(--radius-md);
            border: 1px solid color-mix(in srgb, var(--line-strong) 40%, transparent);
            background: linear-gradient(180deg, color-mix(in srgb, var(--panel-soft) 94%, transparent), color-mix(in srgb, var(--panel) 94%, transparent));
            padding: 12px;
            min-height: 92px;
        }
        .statusLabel {
            color: var(--muted);
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 0.08em;
        }
        .statusValue {
            margin-top: 8px;
            font-size: 18px;
            font-weight: 700;
            line-height: 1.2;
        }
        .ok { color: var(--ok); }
        .warn { color: var(--warn); }
        .muted { color: var(--muted); }
        .detailGrid {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            gap: 10px;
            margin-top: 14px;
        }
        .detailCard {
            padding: 12px;
            border-radius: var(--radius-md);
            border: 1px solid color-mix(in srgb, var(--line) 80%, transparent);
            background: color-mix(in srgb, var(--panel-soft) 92%, transparent);
        }
        .detailKey {
            color: var(--muted);
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 0.08em;
        }
        .detailVal {
            margin-top: 8px;
            font-size: 15px;
            font-weight: 700;
        }
        .memAlert {
            display: none;
            margin-bottom: 14px;
            padding: 12px 14px;
            border-radius: var(--radius-md);
            border: 1px solid rgba(255, 173, 109, 0.38);
            background: rgba(73, 44, 18, 0.6);
            color: #ffd7ab;
        }
        .memAlert.warn { display: block; }
        .memAlert.critical {
            display: block;
            border-color: rgba(255, 131, 118, 0.44);
            background: rgba(79, 25, 25, 0.62);
            color: #ffd0cc;
        }
        .matrixStage {
            position: relative;
            border-radius: calc(var(--radius-xl) - 4px);
            padding: 16px;
            border: 1px solid color-mix(in srgb, var(--line-strong) 40%, transparent);
            background:
                radial-gradient(circle at top, color-mix(in srgb, var(--accent) 16%, transparent), transparent 34%),
                linear-gradient(180deg, color-mix(in srgb, var(--panel-soft) 94%, transparent), color-mix(in srgb, var(--panel) 96%, transparent));
        }
        .matrixWrap {
            width: min(100%, 620px);
            margin: 0 auto;
            padding: 14px;
            border-radius: 22px;
            border: 1px solid color-mix(in srgb, var(--line-strong) 34%, transparent);
            background: linear-gradient(180deg, color-mix(in srgb, var(--panel-soft) 94%, transparent), color-mix(in srgb, var(--panel) 96%, transparent));
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.04);
            aspect-ratio: 1 / 1;
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
            padding:0;
            text-align:center;
            background:transparent;
            aspect-ratio:1 / 1;
            display:flex;
            justify-content:center;
            align-items:center;
            overflow:hidden;
        }
        .glyph {
            font-family: "Rajdhani", "Bahnschrift", "Arial Narrow", Arial, sans-serif;
            font-size: clamp(12px, 2.1vw, 21px);
            font-weight: 700;
            color: #435672;
            line-height: 1;
            letter-spacing: -0.03em;
            text-transform: uppercase;
            transform: scaleX(0.9);
            transform-origin: center;
            -webkit-font-smoothing: antialiased;
            text-rendering: geometricPrecision;
            transition: color 50ms linear, text-shadow 50ms linear;
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
        .controlGrid {
            display: grid;
            gap: 14px;
        }
        .field {
            display: grid;
            gap: 7px;
        }
        .fieldRow {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 10px;
        }
        .fieldRow label {
            color: color-mix(in srgb, var(--text) 88%, var(--muted) 12%);
            font-size: 13px;
            font-weight: 600;
        }
        .metricBadge {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            min-width: 68px;
            padding: 6px 10px;
            border-radius: 999px;
            border: 1px solid color-mix(in srgb, var(--line-strong) 38%, transparent);
            background: color-mix(in srgb, var(--panel-soft) 88%, transparent);
            color: color-mix(in srgb, var(--text) 92%, white 8%);
            font-size: 12px;
            font-weight: 700;
        }
        input,
        select,
        button {
            width: 100%;
            padding: 12px 13px;
            border-radius: 14px;
            border: 1px solid rgba(115, 157, 206, 0.24);
            background: rgba(14, 28, 43, 0.82);
            color: var(--text);
            font: inherit;
        }
        select,
        input[type='color'] { min-height: 48px; }
        input[type='range'] {
            padding: 0;
            border: none;
            background: transparent;
            accent-color: var(--accent);
        }
        button {
            cursor: pointer;
            background: linear-gradient(145deg, color-mix(in srgb, var(--accent) 26%, transparent), color-mix(in srgb, var(--accent-strong) 76%, transparent));
        }
        button:hover {
            border-color: color-mix(in srgb, var(--accent) 55%, transparent);
        }
        button.secondary {
            background: color-mix(in srgb, var(--panel-soft) 90%, transparent);
        }
        button.warn {
            background: linear-gradient(145deg, color-mix(in srgb, #ff7f6b 26%, transparent), color-mix(in srgb, #7c2921 86%, transparent));
        }
        .colorRow {
            display: grid;
            grid-template-columns: 1fr 70px;
            gap: 10px;
            align-items: center;
        }
        .colorSwatch {
            height: 48px;
            border-radius: 14px;
            border: 1px solid color-mix(in srgb, var(--line-strong) 45%, transparent);
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.05), 0 10px 22px rgba(0,0,0,0.18);
        }
        .controlActions,
        .quickGrid {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            gap: 10px;
        }
        .quickGrid {
            grid-template-columns: repeat(4, minmax(0, 1fr));
        }
        .inlineMsg {
            color: var(--muted);
            font-size: 12px;
        }
        .sectionText {
            margin: 0 0 14px;
            color: color-mix(in srgb, var(--text) 76%, var(--muted) 24%);
            font-size: 14px;
            line-height: 1.6;
        }
        @media (max-width: 1040px) {
            .hero,
            .layout {
                grid-template-columns: 1fr;
            }
        }
        @media (max-width: 720px) {
            .statusGrid,
            .detailGrid,
            .quickGrid,
            .controlActions {
                grid-template-columns: 1fr 1fr;
            }
            .topnav {
                border-radius: 16px;
            }
            .heroCard,
            .heroAside,
            .panel {
                border-radius: 18px;
            }
        }
        @media (max-width: 560px) {
            .statusGrid,
            .detailGrid,
            .quickGrid,
            .controlActions,
            .colorRow {
                grid-template-columns: 1fr;
            }
            .heroCard {
                padding: 20px;
            }
        }
</style>
</head>
<body>
<div class="shell">
    <div class="topnav">
        <div class="brand">
            <div class="brandMark"></div>
            <div class="brandText">
                <strong>WordClock Studio</strong>
                <span>Onboard control website</span>
            </div>
        </div>
        <div class="navLinks">
            <a href="/">Home</a>
            <a href="/main">Setup</a>
            <a href="/wifi">Wifi</a>
            <a href="/mqtt">MQTT</a>
            <a href="/ota">OTA</a>
            <a class="active" href="/live">Studio</a>
            <a href="/test">Test</a>
            <a href="#" id="themeToggle">Theme: Dark</a>
        </div>
    </div>

    <div class="hero">
        <section class="heroCard">
            <span class="eyebrow">Live control website</span>
            <h1>One control surface for light, motion and timing.</h1>
            <p>
                Direkte Regie fuer Effekt, Farbe, Helligkeit, Geschwindigkeit, Intensitaet,
                Objektdichte und Uebergang. Dazu Live-Telemetrie, Matrix-Vorschau und Schnelltests
                in einer durchgehenden Website.
            </p>
            <div class="heroMeta">
                <span>Live preview</span>
                <span>Status 10/s</span>
                <span>MQTT + RTC aware</span>
                <span>OTA release</span>
            </div>
        </section>
        <aside class="heroAside">
            <div class="miniPanel">
                <div class="miniLabel">Current effect</div>
                <div id="heroEffect" class="miniValue">-</div>
            </div>
            <div class="miniPanel">
                <div class="miniLabel">Current color</div>
                <div id="heroColor" class="miniValue">-</div>
            </div>
            <div class="miniPanel">
                <div class="miniLabel">Brightness / speed</div>
                <div id="heroTuning" class="miniValue">-</div>
            </div>
        </aside>
    </div>

    <div class="layout">
        <div class="column">
            <section class="panel">
                <div class="panelHead">
                    <h2>System status</h2>
                    <span class="pill">Refresh: 10/s (fest)</span>
                </div>
                <div id="memAlert" class="memAlert"></div>
                <div class="statusGrid">
                    <div class="statusItem"><div class="statusLabel">Power</div><div id="stPower" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">Effekt</div><div id="stEffect" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">RTC</div><div id="stRtc" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">WiFi</div><div id="stWifi" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">MQTT</div><div id="stMqtt" class="statusValue">-</div></div>
                    <div class="statusItem"><div class="statusLabel">RAM</div><div id="stMem" class="statusValue">-</div></div>
                </div>
                <div id="statusDetails" class="detailGrid"></div>
            </section>

            <section class="panel">
                <div class="panelHead">
                    <h3>WordClock matrix</h3>
                    <span class="pill">Exact device preview</span>
                </div>
                <p class="sectionText">Die Matrix zeigt live den aktuell gerenderten Zustand der Uhr, inklusive Minutenpfoten und MQTT-Statusindikator.</p>
                <div class="matrixStage">
                    <div class="matrixWrap">
                        <div id="matrix" class="matrix"></div>
                    </div>
                </div>
            </section>

            <section class="panel">
                <div class="panelHead">
                    <h3>Quick tests</h3>
                    <span class="pill">Fast operator actions</span>
                </div>
                <div class="quickGrid">
                    <button onclick="quick('all_on')">Alle AN</button>
                    <button class="warn" onclick="quick('all_off')">Alle AUS</button>
                    <button onclick="quick('clock_test')">Clock Test</button>
                    <button onclick="quick('gradient')">Gradient</button>
                </div>
            </section>
        </div>

        <aside class="column">
            <section class="panel">
                <div class="panelHead">
                    <h2>Control studio</h2>
                    <span id="applyMsg" class="inlineMsg">Auto-Apply aktiv</span>
                </div>
                <p class="sectionText">Steuere die Uhr wie eine normale Website: links Telemetrie und Vorschau, rechts alle Controls in einem sauberen Operator-Panel.</p>
                <div class="controlGrid">
                    <div class="field">
                        <div class="fieldRow"><label for="power">Power</label><span class="metricBadge" id="powerBadge">ON</span></div>
                        <select id="power">
                            <option value="ON">ON</option>
                            <option value="OFF">OFF</option>
                        </select>
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="effect">Effekt</label><span class="metricBadge" id="effectBadge">clock</span></div>
                        <select id="effect">
                            <option value="clock">clock</option>
                            <option value="wifi">wifi</option>
                            <option value="waterdrop">waterdrop</option>
                            <option value="love">love</option>
                            <option value="colorloop">colorloop</option>
                            <option value="colorwipe">colorwipe</option>
                            <option value="fire2d">fire2d</option>
                            <option value="matrix">matrix</option>
                            <option value="plasma">plasma</option>
                            <option value="inward">inward</option>
                            <option value="twinkle">twinkle</option>
                            <option value="balls">balls</option>
                            <option value="aurora">aurora</option>
                            <option value="enchantment">enchantment</option>
                            <option value="snake">snake</option>
                        </select>
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="color">Farbe</label><span class="metricBadge" id="colorHex">#FF9900</span></div>
                        <div class="colorRow">
                            <input id="color" type="color" value="#ff9900">
                            <div id="colorPreview" class="colorSwatch" style="background:#ff9900;"></div>
                        </div>
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="brightness">Helligkeit</label><span class="metricBadge" id="brightnessValue">120</span></div>
                        <input id="brightness" type="range" min="0" max="255" value="120">
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="speed">Geschwindigkeit</label><span class="metricBadge" id="speedValue">50%</span></div>
                        <input id="speed" type="range" min="1" max="100" value="50">
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="intensity">Intensitaet</label><span class="metricBadge" id="intensityValue">50%</span></div>
                        <input id="intensity" type="range" min="1" max="100" value="50">
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="density">Objektdichte</label><span class="metricBadge" id="densityValue">50%</span></div>
                        <input id="density" type="range" min="1" max="100" value="50">
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="transitionMs">Uebergang</label><span class="metricBadge" id="transitionValue">1000 ms</span></div>
                        <input id="transitionMs" type="range" min="200" max="10000" step="50" value="1000">
                    </div>

                    <div class="controlActions">
                        <button class="secondary" onclick="applyPreset('balanced')">Balanced</button>
                        <button class="secondary" onclick="applyPreset('dramatic')">Dramatic</button>
                        <button class="secondary" onclick="applyPreset('calm')">Calm</button>
                    </div>
                </div>
            </section>
        </aside>
    </div>
</div>

<script>
let isDirty = false;
let refreshTimer = null;
let refreshInFlight = false;
let applyDebounceTimer = null;

function setTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('wc_theme', theme);
    setText('themeToggle', 'Theme: ' + (theme === 'light' ? 'Light' : 'Dark'));
}

function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'dark';
    setTheme(current === 'dark' ? 'light' : 'dark');
}

function initTheme() {
    const saved = localStorage.getItem('wc_theme');
    const preferred = (saved === 'light' || saved === 'dark') ? saved : 'dark';
    setTheme(preferred);
    document.getElementById('themeToggle').addEventListener('click', function(e) {
        e.preventDefault();
        toggleTheme();
    });
}

function markDirty() { isDirty = true; }

function queueAutoApply(delayMs) {
    markDirty();
    const msg = document.getElementById('applyMsg');
    msg.textContent = 'Auto-Apply...';
    if (applyDebounceTimer) {
        clearTimeout(applyDebounceTimer);
    }
    applyDebounceTimer = setTimeout(function() {
        applyLive();
    }, delayMs || 260);
}

function setText(id, value) {
    document.getElementById(id).textContent = value;
}

function updateRangeBadges() {
    setText('brightnessValue', document.getElementById('brightness').value);
    setText('speedValue', document.getElementById('speed').value + '%');
    setText('intensityValue', document.getElementById('intensity').value + '%');
    setText('densityValue', document.getElementById('density').value + '%');
    setText('transitionValue', document.getElementById('transitionMs').value + ' ms');
    setText('powerBadge', document.getElementById('power').value);
    setText('effectBadge', document.getElementById('effect').value);
}

function updateColorUi(value) {
    document.getElementById('colorPreview').style.backgroundColor = value;
    setText('colorHex', value.toUpperCase());
}

function applyPreset(name) {
    if (name === 'balanced') {
        document.getElementById('speed').value = 50;
        document.getElementById('intensity').value = 50;
        document.getElementById('density').value = 50;
        document.getElementById('transitionMs').value = 1000;
    } else if (name === 'dramatic') {
        document.getElementById('speed').value = 78;
        document.getElementById('intensity').value = 82;
        document.getElementById('density').value = 68;
        document.getElementById('transitionMs').value = 2200;
    } else if (name === 'calm') {
        document.getElementById('speed').value = 30;
        document.getElementById('intensity').value = 38;
        document.getElementById('density').value = 28;
        document.getElementById('transitionMs').value = 2800;
    }
    updateRangeBadges();
    queueAutoApply(140);
}

const layoutRows = [
    'ESSISTTFUENF',
    'ZEHNZWANZIG',
    'DREIVIERTEL',
    'VORLOVENACH',
    'HALBEELFUENF',
    'DREIYOUVIER',
    'SECHSSIEBEN',
    'ZEHNEUNZWEI',
    'AACHTZWOELF',
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

function renderDetails(s, rtcTempText, memFree, memUsedPct) {
    const details = [
        ['IP', s.ip || '-'],
        ['Farbe', s.color || '-'],
        ['Helligkeit', String(s.brightness)],
        ['Geschwindigkeit', String(s.speed) + '%'],
        ['Intensitaet', String(s.intensity) + '%'],
        ['Objektdichte', String(s.density) + '%'],
        ['Uebergang', String(s.transition_ms) + ' ms'],
        ['RTC Temperatur', rtcTempText],
        ['RTC OSF/Batterie', s.rtc_battery_warning ? 'Auffaellig' : 'OK'],
        ['RAM Frei', String(memFree) + ' B'],
        ['RAM Nutzung', memUsedPct + '%'],
        ['Max Block', String(s.mem_max_alloc || 0) + ' B']
    ];

    document.getElementById('statusDetails').innerHTML = details.map(function(item) {
        return '<div class="detailCard"><div class="detailKey">' + item[0] + '</div><div class="detailVal">' + item[1] + '</div></div>';
    }).join('');
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
        setText('heroEffect', s.effect || '-');
        setText('heroColor', s.color || '-');
        setText('heroTuning', String(s.brightness) + ' / ' + String(s.speed) + '%');

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

        renderDetails(s, rtcTempText, memFree, memUsedPct);

        if (!isDirty) {
            document.getElementById('power').value = s.state;
            document.getElementById('effect').value = s.effect;
            document.getElementById('brightness').value = s.brightness;
            document.getElementById('speed').value = s.speed;
            document.getElementById('intensity').value = s.intensity;
            document.getElementById('density').value = s.density;
            document.getElementById('transitionMs').value = s.transition_ms;
            if (/^#[0-9a-fA-F]{6}$/.test(s.color)) {
                document.getElementById('color').value = s.color;
                updateColorUi(s.color);
            }
            updateRangeBadges();
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
    p.set('speed', document.getElementById('speed').value);
    p.set('intensity', document.getElementById('intensity').value);
    p.set('density', document.getElementById('density').value);
    p.set('transition_ms', document.getElementById('transitionMs').value);
    const r = await fetch('/api/preview', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: p
    });
    msg.textContent = r.ok ? 'Auto-Apply aktiv' : 'Fehler beim Anwenden';
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

document.getElementById('power').addEventListener('change', function() { updateRangeBadges(); queueAutoApply(180); });
document.getElementById('effect').addEventListener('change', function() { updateRangeBadges(); queueAutoApply(180); });
document.getElementById('color').addEventListener('input', function() { updateColorUi(this.value); queueAutoApply(220); });
document.getElementById('brightness').addEventListener('input', function() { updateRangeBadges(); queueAutoApply(240); });
document.getElementById('speed').addEventListener('input', function() { updateRangeBadges(); queueAutoApply(240); });
document.getElementById('intensity').addEventListener('input', function() { updateRangeBadges(); queueAutoApply(240); });
document.getElementById('density').addEventListener('input', function() { updateRangeBadges(); queueAutoApply(240); });
document.getElementById('transitionMs').addEventListener('input', function() { updateRangeBadges(); queueAutoApply(300); });

function startRefreshTimer() {
    const periodMs = Math.floor(1000 / 10);
    if (refreshTimer) clearInterval(refreshTimer);
    refreshTimer = setInterval(refreshStatus, periodMs);
}

updateColorUi(document.getElementById('color').value);
updateRangeBadges();
initTheme();
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
    :root {
        --bg-a: #0f131c;
        --bg-b: #1a2230;
        --panel: rgba(23, 30, 43, 0.92);
        --panel-soft: rgba(31, 40, 56, 0.9);
        --line: rgba(154, 168, 190, 0.22);
        --line-strong: rgba(188, 201, 224, 0.46);
        --text: #f3f5fb;
        --muted: #acb3c4;
    }
    * { box-sizing: border-box; }
    body {
        margin: 0;
        font-family: "Sora", "Manrope", "Segoe UI", sans-serif;
        background:
            radial-gradient(circle at 0% 0%, rgba(147, 162, 190, 0.22), transparent 26%),
            radial-gradient(circle at 100% 8%, rgba(242, 161, 90, 0.12), transparent 22%),
            linear-gradient(180deg, var(--bg-b) 0%, var(--bg-a) 100%);
        color: var(--text);
        min-height: 100vh;
    }
    .shell {
        max-width: 1240px;
        margin: 0 auto;
        padding: 14px 10px 24px;
    }
    .topbar {
        position: sticky;
        top: 0;
        z-index: 20;
        display:flex;
        align-items: center;
        justify-content: space-between;
        gap: 10px;
        flex-wrap:wrap;
        margin-bottom: 12px;
        padding: 12px 14px;
        border: 1px solid var(--line);
        border-radius: 20px;
        background: rgba(23, 30, 43, 0.84);
        backdrop-filter: blur(14px);
        box-shadow: 0 20px 54px rgba(0, 0, 0, 0.28);
    }
    .brand {
        display: flex;
        align-items: center;
        gap: 12px;
    }
    .brandMark {
        width: 40px;
        height: 40px;
        border-radius: 13px;
        background: linear-gradient(145deg, #94a4bf, #f2a15a);
    }
    .brandText strong {
        display: block;
        font-size: 14px;
        letter-spacing: 0.04em;
        text-transform: uppercase;
    }
    .brandText span {
        color: var(--muted);
        font-size: 12px;
    }
    .navLinks {
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
    }
    .navLinks a {
        color: var(--text);
        text-decoration: none;
        padding: 8px 12px;
        border:1px solid var(--line);
        border-radius:999px;
        background:linear-gradient(145deg, rgba(44, 56, 77, 0.96), rgba(28, 36, 50, 0.96));
        font-size:13px;
    }
    .navLinks a.active,
    .navLinks a:hover {
        border-color: var(--line-strong);
        background: linear-gradient(145deg, rgba(73, 90, 120, 0.98), rgba(44, 56, 77, 0.98));
    }
    .wrap { max-width:980px; margin:0 auto; padding:0 0 20px; }
    .hero {
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background: linear-gradient(145deg, rgba(37, 47, 66, 0.96), rgba(22, 29, 42, 0.98));
        padding: 14px 16px;
        margin-bottom: 12px;
    }
    .hero h2 { margin: 0 0 6px; font-size: 24px; }
    .hero p { margin: 0; color: #d7dde9; }
    .card {
        background: var(--panel);
        border:1px solid var(--line);
        border-radius:16px;
        padding:16px;
        box-shadow: 0 14px 28px rgba(0, 0, 0, 0.24);
    }
    .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(140px, 1fr)); gap:8px; margin-top:10px; }
    button {
        width:100%;
        padding:10px 8px;
        border-radius:10px;
        border:1px solid rgba(188, 201, 224, 0.32);
        background:linear-gradient(145deg, rgba(73, 90, 120, 0.98), rgba(44, 56, 77, 0.98));
        color:#fff;
        cursor:pointer;
        font-size:13px;
    }
    button.warn { background:linear-gradient(145deg, rgba(170, 92, 92, 0.98), rgba(120, 48, 48, 0.98)); border-color:rgba(245, 179, 165, 0.42); }
    button.info { background:linear-gradient(145deg, rgba(92, 103, 122, 0.98), rgba(58, 67, 82, 0.98)); }
    button.special { background:linear-gradient(145deg, rgba(98, 120, 92, 0.98), rgba(62, 82, 56, 0.98)); border-color:rgba(195, 220, 178, 0.36); }
    .msg { color:#cfdcf2; margin-top:10px; min-height:20px; font-size:13px; }
    .section-title { margin-top:12px; font-weight:bold; color:#bfc6d6; font-size:12px; text-transform: uppercase; letter-spacing: 0.04em; }
    .sub { color: var(--muted); margin: 0; }
    @media (max-width:480px) {
        .shell { padding:8px 4px; }
        .wrap { padding:0 0 14px; }
        .hero h2 { font-size: 19px; }
        .topbar { padding: 9px 8px; border-radius: 14px; }
        .brandMark { width: 34px; height: 34px; }
        .brandText strong { font-size: 12px; }
        .brandText span { font-size: 11px; }
        .navLinks a { padding: 6px 9px; font-size: 12px; }
        .card { padding:10px; }
        .grid { grid-template-columns:repeat(auto-fit, minmax(100px, 1fr)); gap:6px; }
        button { padding:9px 4px; font-size:12px; }
    }
</style>
</head>
<body>
<div class="shell">
    <div class="topbar">
        <div class="brand">
            <div class="brandMark"></div>
            <div class="brandText">
                <strong>WordClock Studio</strong>
                <span>Onboard control website</span>
            </div>
        </div>
        <div class="navLinks" id="testNavLinks">
            <a href="/main">Setup</a>
            <a href="/wifi">Wifi</a>
            <a href="/mqtt">MQTT</a>
            <a href="/ota">OTA</a>
            <a href="/live">Studio</a>
            <a href="/test" class="active">Test</a>
        </div>
    </div>
<div class="wrap">
    <div class="hero">
        <h2>WordClock Test Deck</h2>
        <p>Diagnose fuer Farben, Helligkeit, Muster und Segment-Checks in Studio-Optik.</p>
    </div>
    <div class="card">
        <p class="sub">Tests werden ca. 3 Sekunden gehalten, damit du das Ergebnis stabil siehst.</p>
        
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
            <button style="background:#27ae60" onclick="quick('color_green')">Gruen</button>
            <button style="background:#3498db" onclick="quick('color_blue')">Blau</button>
            <button style="background:#f39c12" onclick="quick('color_yellow')">Gelb</button>
            <button style="background:#1abc9c" onclick="quick('color_cyan')">Cyan</button>
            <button style="background:#e91e63" onclick="quick('color_magenta')">Magenta</button>
        </div>

        <div class="section-title">Helligkeit (50% Weiss)</div>
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

        <div class="section-title">Muster-Tests</div>
        <div class="grid">
            <button class="special" onclick="quick('checker')">Schachbrett</button>
            <button class="special" onclick="quick('rows')">Zeilen</button>
            <button class="special" onclick="quick('columns')">Spalten</button>
            <button class="special" onclick="quick('sparkle')">Sparkle</button>
            <button class="info" onclick="quick('warm_white')">Warmweiss</button>
            <button class="info" onclick="quick('cool_white')">Kaltweiss</button>
        </div>

        <div id="msg" class="msg"></div>
    </div>
</div>
</div>
<script>
let testBusy = false;

function waitMs(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function setButtonsDisabled(disabled) {
    Array.from(document.querySelectorAll('button')).forEach(btn => {
        btn.disabled = disabled;
        btn.style.opacity = disabled ? '0.6' : '1';
    });
}

async function quick(action) {
    if (testBusy) return;
    testBusy = true;
    setButtonsDisabled(true);

    const msg = document.getElementById('msg');
    msg.textContent = 'Fuehre Test aus (3s Haltedauer)...';
    const p = new URLSearchParams();
    p.set('action', action);
    p.set('hold_ms', '3000');

    try {
        const r = await fetch('/api/quicktest', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: p
        });
        msg.textContent = r.ok ? 'Test ausgefuehrt' : 'Fehler beim Test';
    } catch (_) {
        msg.textContent = 'Fehler beim Test';
    }

    await waitMs(400);
    setButtonsDisabled(false);
    testBusy = false;
    setTimeout(() => { msg.textContent = ''; }, 1600);
}

(function() {
    const path = location.pathname;
    const nav = document.getElementById('testNavLinks');
    Array.from(nav.querySelectorAll('a')).forEach(link => {
        if (link.getAttribute('href') === path) {
            link.classList.add('active');
        }
    });
})();
</script>
</body>
</html>
)rawliteral";
