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
        --text-secondary: #d7dde9;
        --muted: #acb3c4;
        --accent: #93a2be;
        --accent-warm: #f2a15a;
    }
    [data-theme="light"] {
        --bg-a: #f4f5f9;
        --bg-b: #e9ecf2;
        --panel: rgba(255, 255, 255, 0.88);
        --panel-soft: rgba(247, 248, 252, 0.92);
        --line: rgba(80, 95, 120, 0.2);
        --line-strong: rgba(67, 85, 114, 0.44);
        --text: #192233;
        --text-secondary: #3a4556;
        --muted: #5e677a;
        --accent: #7f8da9;
        --accent-warm: #b96d2a;
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
    [data-theme="light"] .topnav {
        background: color-mix(in srgb, var(--panel-soft) 85%, transparent);
    }
    .topnav a {
        color: var(--text);
        text-decoration: none;
        padding: 8px 11px;
        border-radius: 10px;
        border: 1px solid var(--line);
        background: color-mix(in srgb, var(--accent) 10%, rgba(44, 56, 77, 0.96));
        font-size: 13px;
    }
    [data-theme="light"] .topnav a {
        background: color-mix(in srgb, var(--panel-soft) 92%, transparent);
    }
    .topnav a:hover { border-color: var(--line-strong); }
    .wrap { max-width: 1020px; margin: 0 auto; padding: 16px 10px 24px; }
    .hero {
        position: relative;
        overflow: hidden;
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background: color-mix(in srgb, var(--panel) 92%, rgba(37, 47, 66, 0.96));
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
    .hero p { margin: 0; color: var(--text-secondary); }
    .tagRow { margin-top: 12px; display: flex; gap: 8px; flex-wrap: wrap; }
    .tag {
        padding: 5px 9px;
        border-radius: 999px;
        font-size: 12px;
        border: 1px solid var(--line);
        color: var(--text);
        background: color-mix(in srgb, var(--panel) 85%, transparent);
    }
    .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(170px, 1fr)); gap:10px; }
    .card {
        background: var(--panel);
        border: 1px solid var(--line);
        border-radius: 14px;
        padding: 12px;
        box-shadow: 0 10px 24px rgba(0, 0, 0, 0.24);
    }
    [data-theme="light"] .card {
        box-shadow: 0 6px 12px rgba(0, 0, 0, 0.08);
    }
    .card h3 { margin: 2px 0 6px; }
    .card p { margin: 0; color: var(--muted); min-height: 36px; }
    .btn {
        display:inline-block;
        margin-top:10px;
        padding:8px 10px;
        border-radius:10px;
        border:1px solid var(--line);
        color: var(--text);
        text-decoration:none;
        background: color-mix(in srgb, var(--accent) 12%, transparent);
        font-size:13px;
        transition: border-color 0.2s ease;
    }
    .btn:hover { border-color: var(--line-strong); background: color-mix(in srgb, var(--accent) 18%, transparent); }
    .statusPanel {
        margin-top: 14px;
        background: var(--panel);
        border: 1px solid var(--line);
        border-radius: 14px;
        padding: 12px;
        box-shadow: 0 10px 24px rgba(0, 0, 0, 0.24);
    }
    .statusHead {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 10px;
        margin-bottom: 10px;
    }
    .statusHead h3 { margin: 0; }
    .statusPill {
        padding: 6px 10px;
        border-radius: 999px;
        border: 1px solid var(--line);
        background: color-mix(in srgb, var(--panel-soft) 92%, transparent);
        color: var(--muted);
        font-size: 12px;
    }
    .statusGrid {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: 8px;
    }
    .statusItem {
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
        border: 1px solid var(--line);
        border-radius: 10px;
        padding: 8px 10px;
    }
    .statusLabel {
        color: var(--muted);
        font-size: 11px;
        text-transform: uppercase;
        letter-spacing: 0.04em;
    }
    .statusValue {
        color: var(--text);
        font-size: 15px;
        font-weight: 700;
        margin-top: 3px;
    }
    .detailTable {
        margin-top: 10px;
        display: grid;
        grid-template-columns: 1fr auto;
        gap: 6px 10px;
        font-size: 14px;
    }
    .detailKey { color: var(--muted); text-align: left; }
    .detailVal { color: var(--text); text-align: right; }
    .ok { color: #268b46; }
    .warn { color: #b05643; }
    @media (max-width:480px) {
        .wrap { padding: 10px 6px 16px; }
        .topnav { padding: 8px 4px; gap: 4px; }
        .topnav a { padding: 6px 8px; font-size: 11px; }
        .grid { grid-template-columns: repeat(auto-fit, minmax(132px, 1fr)); gap: 8px; }
        .card { padding: 10px; }
        .hero { padding: 10px; }
        .statusGrid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        h2 { margin: 8px 0; font-size: 18px; }
        h3 { margin: 6px 0; font-size: 14px; }
        p { margin: 4px 0; font-size: 12px; }
    }
</style>
</head>
<body>
<div class="topnav">
    <a href="/main">Netzwerk</a>
    <a href="/layout">Layout</a>
    <a href="/live">Live</a>
    <a href="/test">Test</a>
    <a href="#" id="themeToggle">Theme: Dark</a>
</div>
<div class="wrap">
    <div class="hero">
        <h2>WordClock Control Hub</h2>
        <p>Moderne Steuerzentrale für Setup, Live-Regie, OTA und Tests.</p>
        <div class="tagRow">
            <span class="tag">Live Status</span>
            <span class="tag">OTA Ready</span>
            <span class="tag">MQTT + RTC</span>
        </div>
    </div>
    <div class="grid">
        <div class="card"><h3>Netzwerk</h3><p>WiFi, MQTT und OTA zentral auf einer gemeinsamen Seite verwalten.</p><a class="btn" href="/main">Öffnen</a></div>
        <div class="card"><h3>Layout</h3><p>Letter-Grid und Wortpositionen für andere Frontplatten definieren.</p><a class="btn" href="/layout">Öffnen</a></div>
        <div class="card"><h3>Live</h3><p>Direkte Effekt- und Farbregie mit Matrix-Vorschau.</p><a class="btn" href="/live">Öffnen</a></div>
        <div class="card"><h3>Test</h3><p>Quicktests für LEDs, Farben, Clock und Muster.</p><a class="btn" href="/test">Öffnen</a></div>
    </div>
    <section class="statusPanel">
        <div class="statusHead">
            <h3>System status</h3>
            <span class="statusPill">Refresh: 10/s (fest)</span>
        </div>
        <div class="statusGrid">
            <div class="statusItem"><div class="statusLabel">Power</div><div id="hmPower" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">Effekt</div><div id="hmEffect" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">RTC</div><div id="hmRtc" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">WiFi</div><div id="hmWifi" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">MQTT</div><div id="hmMqtt" class="statusValue">-</div></div>
            <div class="statusItem"><div class="statusLabel">RAM</div><div id="hmMem" class="statusValue">-</div></div>
        </div>
        <div id="homeStatusDetails" class="detailTable"></div>
    </section>
</div>
<script>
function setTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('wc_theme', theme);
    const t = document.getElementById('themeToggle');
    if (t) t.textContent = 'Theme: ' + (theme === 'light' ? 'Light' : 'Dark');
}
function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'dark';
    setTheme(current === 'dark' ? 'light' : 'dark');
}
function initTheme() {
    const saved = localStorage.getItem('wc_theme');
    const preferred = (saved === 'light' || saved === 'dark') ? saved : 'dark';
    setTheme(preferred);
    const toggle = document.getElementById('themeToggle');
    if (toggle) {
        toggle.addEventListener('click', function(e) {
            e.preventDefault();
            toggleTheme();
        });
    }
}

function fillSetupFields(status) {
    const ssidManual = document.getElementById('ssid_manual');
    const mqttServer = document.getElementById('mqtt_server');
    const mqttPort = document.getElementById('mqtt_port');
    const mqttUser = document.getElementById('mqtt_user');

    if (ssidManual && typeof status.wifi_ssid === 'string') {
        ssidManual.value = status.wifi_ssid;
    }
    if (mqttServer && typeof status.mqtt_server === 'string') {
        mqttServer.value = status.mqtt_server;
    }
    if (mqttPort && status.mqtt_port !== undefined && status.mqtt_port !== null) {
        mqttPort.value = String(status.mqtt_port);
    }
    if (mqttUser && typeof status.mqtt_user === 'string') {
        mqttUser.value = status.mqtt_user;
    }
}

async function loadSetupFields() {
    try {
        const r = await fetch('/api/status-lite');
        const s = await r.json();
        fillSetupFields(s);
    } catch (_) {}
}

async function refreshHomeStatus() {
    if (document.hidden) return;
    try {
        const r = await fetch('/api/status-lite');
        const s = await r.json();
        const rtcBadge = s.rtc_warning ? '<span class="warn">WARNUNG</span>' : '<span class="ok">OK</span>';
        const mqttBadge = s.mqtt_connected ? '<span class="ok">Verbunden</span>' : '<span class="warn">Getrennt</span>';
        const memLevel = String(s.mem_level || 'OK').toUpperCase();
        const memBadge = (memLevel === 'CRITICAL' || memLevel === 'WARNING') ? '<span class="warn">' + memLevel + '</span>' : '<span class="ok">OK</span>';
        const rtcTempText = (typeof s.rtc_temp_c === 'number' && Number.isFinite(s.rtc_temp_c)) ? (s.rtc_temp_c.toFixed(2) + ' C') : 'n/a';
        const memFree = Number.isFinite(Number(s.mem_free)) ? Number(s.mem_free) : 0;
        const memTotal = Number.isFinite(Number(s.mem_total)) && Number(s.mem_total) > 0 ? Number(s.mem_total) : 1;
        const memUsedPct = ((memTotal - memFree) * 100.0 / memTotal).toFixed(1);

        document.getElementById('hmPower').innerHTML = s.state || '-';
        document.getElementById('hmEffect').innerHTML = s.effect || '-';
        document.getElementById('hmRtc').innerHTML = rtcBadge;
        document.getElementById('hmWifi').innerHTML = (s.rssi + ' dBm');
        document.getElementById('hmMqtt').innerHTML = mqttBadge;
        document.getElementById('hmMem').innerHTML = memBadge;

        document.getElementById('homeStatusDetails').innerHTML =
            '<div class="detailKey">IP</div><div class="detailVal ok">' + (s.ip || '-') + '</div>' +
            '<div class="detailKey">Farbe</div><div class="detailVal">' + (s.color || '-') + '</div>' +
            '<div class="detailKey">Helligkeit</div><div class="detailVal">' + (s.brightness || '-') + '</div>' +
            '<div class="detailKey">Speed / Intensität</div><div class="detailVal">' + (s.speed || '-') + '% / ' + (s.intensity || '-') + '%</div>' +
            '<div class="detailKey">Objektdichte</div><div class="detailVal">' + (s.density || '-') + '%</div>' +
            '<div class="detailKey">Transition</div><div class="detailVal">' + (s.transition_ms || '-') + ' ms</div>' +
            '<div class="detailKey">RTC Temperatur</div><div class="detailVal">' + rtcTempText + '</div>' +
            '<div class="detailKey">RTC OSF/Batterie</div><div class="detailVal">' + (s.rtc_battery_warning ? 'Auffällig' : 'OK') + '</div>' +
            '<div class="detailKey">RAM Frei</div><div class="detailVal">' + memFree + ' B</div>' +
            '<div class="detailKey">RAM Nutzung</div><div class="detailVal">' + memUsedPct + '%</div>';
    } catch (_) {}
}

initTheme();
setInterval(refreshHomeStatus, 500);
refreshHomeStatus();
</script>
</body>
</html>
)rawliteral";

const char setup_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WordClock Setup</title>
<style>
    :root {
        --bg-a: #0f131c;
        --bg-b: #1a2230;
        --panel: rgba(23, 30, 43, 0.92);
        --panel-soft: rgba(31, 40, 56, 0.9);
        --line: rgba(154, 168, 190, 0.22);
        --line-strong: rgba(188, 201, 224, 0.46);
        --text: #f3f5fb;
        --text-secondary: #d7dde9;
        --text-muted: #d6dde9;
        --muted: #acb3c4;
        --ok: #7af0a8;
        --warn: #f6b3a5;
    }
    [data-theme="light"] {
        --bg-a: #f4f5f9;
        --bg-b: #e9ecf2;
        --panel: rgba(255, 255, 255, 0.88);
        --panel-soft: rgba(247, 248, 252, 0.92);
        --line: rgba(80, 95, 120, 0.2);
        --line-strong: rgba(67, 85, 114, 0.44);
        --text: #192233;
        --text-secondary: #3a4556;
        --text-muted: #5e677a;
        --muted: #5e677a;
        --ok: #268b46;
        --warn: #b05643;
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
        background: color-mix(in srgb, var(--panel-soft) 85%, transparent);
        backdrop-filter: blur(14px);
        box-shadow: 0 20px 54px rgba(0, 0, 0, 0.28);
    }
    [data-theme="light"] .topbar {
        box-shadow: 0 8px 16px rgba(0, 0, 0, 0.06);
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
        color: var(--text);
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
        background: color-mix(in srgb, var(--panel-soft) 92%, transparent);
        border-radius: 999px;
        padding: 8px 12px;
        font-size: 13px;
        transition: border-color 0.2s ease, background 0.2s ease;
    }
    .navLinks a.active,
    .navLinks a:hover {
        border-color: var(--line-strong);
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
    }
    .wrap {
        max-width: 1040px;
        margin: 0 auto;
        padding: 0 0 24px;
    }
    .hero {
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background: color-mix(in srgb, var(--panel) 92%, rgba(37, 47, 66, 0.96));
        padding: 14px 16px;
        margin-bottom: 12px;
    }
    .hero h2 { margin: 0 0 6px; font-size: 24px; color: var(--text); }
    .hero p { margin: 0; color: var(--text-secondary); }
    .box {
        background: var(--panel);
        padding: 16px 14px;
        border-radius: 14px;
        border: 1px solid var(--line);
        box-shadow: 0 12px 28px rgba(0, 0, 0, 0.25);
    }
    [data-theme="light"] .box {
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.06);
    }
    select, input {
        width: 100%;
        padding: 11px;
        margin: 8px 0;
        border-radius: 10px;
        border: 1px solid var(--line);
        font-size: 15px;
        box-sizing: border-box;
        color: var(--text);
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
        transition: border-color 0.2s ease;
    }
    select option {
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
        color: var(--text);
    }
    select:focus, input:focus {
        outline: none;
        border-color: var(--line-strong);
    }
    h3 { margin: 8px 0 8px; color: var(--text); }
    label { color: var(--text-muted); font-size: 13px; display: block; text-align: left; }
    button {
        width: 100%;
        padding: 11px;
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
        border: 1px solid var(--line);
        color: var(--text);
        font-size: 14px;
        border-radius: 10px;
        cursor: pointer;
        margin-top: 10px;
        transition: border-color 0.2s ease, background 0.2s ease;
        box-sizing: border-box;
    }
    button:hover { 
        border-color: var(--line-strong);
        background: color-mix(in srgb, var(--panel-soft) 100%, rgba(147, 162, 190, 0.1));
    }
    .layout { display: grid; grid-template-columns: 1.2fr 1fr; gap: 12px; }
    .stack { display: grid; gap: 12px; }
    .tabs { display: flex; gap: 4px; margin-bottom: 12px; border-bottom: 1px solid var(--line); }
    .tab-btn { padding: 10px 14px; border: none; background: transparent; color: var(--text-muted); cursor: pointer; border-bottom: 2px solid transparent; font-size: 13px; transition: 0.2s ease; }
    .tab-btn.active { color: var(--text); border-bottom-color: var(--accent-warm); }
    .tab-btn:hover { color: var(--text); }
    .tab-content { display: none; }
    .tab-content.active { display: block; }
    .statusGrid { display:grid; grid-template-columns:repeat(2, minmax(0, 1fr)); gap:8px; margin-top:10px; }
    .statusItem { 
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
        border: 1px solid var(--line);
        border-radius: 10px;
        padding: 8px 10px;
    }
    .statusLabel { color: var(--muted); font-size:11px; text-transform:uppercase; letter-spacing:0.04em; }
    .statusValue { color: var(--text); font-size:15px; font-weight:700; margin-top:3px; }
    .detailTable { margin-top:10px; display:grid; grid-template-columns:1fr auto; gap:6px 10px; font-size:14px; }
    .detailKey { color: var(--muted); text-align:left; }
    .detailVal { color: var(--text); text-align:right; }
    .ok { color: var(--ok); }
    .warn { color: var(--warn); }
    .muted { color: var(--muted); font-size: 13px; }
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
            <a href="/main">Netzwerk</a>
            <a href="/layout">Layout</a>
            <a href="/live">Studio</a>
            <a href="/test">Test</a>
            <a href="#" id="themeToggle">Theme: Dark</a>
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

            <!-- Tab Navigation -->
            <div class="tabs">
                <button class="tab-btn active" onclick="switchTab('config'); return false;">WiFi/MQTT</button>
                <button class="tab-btn" onclick="switchTab('ota'); return false;">OTA</button>
                <button class="tab-btn" onclick="switchTab('layout'); return false;">Layout</button>
            </div>

            <!-- Configuration Tab (WiFi + MQTT)  -->
            <div id="config-tab" class="tab-content active">
                <h3>WLAN Einstellungen</h3>
                <label>Netzwerk auswählen:</label>
                <select id="ssid_list" name="ssid"></select>
                <input id="ssid_manual" type="text" placeholder="oder SSID manuell eingeben" style="margin-top:4px;">

                <label style="margin-top:12px;">WLAN Passwort:</label>
                <div style="position:relative;">
                    <input name="wifi_pass" id="wifi_pass" type="password" placeholder="optional">
                    <span onclick="toggleWifi()" style="position:absolute; right:10px; top:12px; cursor:pointer;">Show</span>
                </div>

                <button type="button" onclick="saveConfig('wifi')" style="margin-top:10px;">WiFi speichern</button>

                <hr style="margin:14px 0; border:none; border-top:1px solid var(--line);">

                <h3>MQTT Einstellungen</h3>
                <label>MQTT Server:</label>
                <input name="mqtt_server" id="mqtt_server" placeholder="192.168.1.10">

                <label>MQTT Port:</label>
                <input name="mqtt_port" id="mqtt_port" type="number" value="1883">

                <label>MQTT Benutzer:</label>
                <input name="mqtt_user" id="mqtt_user" placeholder="optional">

                <label>MQTT Passwort:</label>
                <div style="position:relative;">
                    <input name="mqtt_pass" id="mqtt_pass" type="password" placeholder="optional">
                    <span onclick="toggleMQTT()" style="position:absolute; right:10px; top:12px; cursor:pointer;">Show</span>
                </div>

                <button type="button" onclick="saveConfig('mqtt')" style="margin-top:10px;">MQTT speichern</button>
                <p id="cfgMsg" style="font-size:14px; min-height:20px; margin-top:8px;"></p>
            </div>

            <!-- OTA Tab -->
            <div id="ota-tab" class="tab-content">
                <h3>OTA Update</h3>
                <p>Aktuelle Firmware: <b id="otaFwVersion">-</b></p>
                <p>Netzwerkstatus: <b id="otaWifi">-</b></p>
                <label for="otaProfile">OTA Prüfprofil:</label>
                <select id="otaProfile" onchange="saveOtaProfile()">
                    <option value="long">Long · 1x pro Woche</option>
                    <option value="norm">Norm · alle 12 Stunden</option>
                    <option value="dev">Dev · alle 2 Minuten</option>
                </select>
                <p>Aktueller Intervall: <b id="otaIntervalLabel">-</b></p>
                <button type="button" onclick="loadOtaInfo()">Status aktualisieren</button>
                <button type="button" onclick="checkOtaNow()">Jetzt auf Update prüfen</button>
                <p id="otaMsg" style="font-size:14px; min-height:20px;"></p>
            </div>

            <!-- Layout Tab -->
            <div id="layout-tab" class="tab-content">
                <h3>Layout Konfiguration</h3>
                <label for="layoutId">Layout auswählen:</label>
                <select id="layoutId" onchange="onLayoutDropdownChange()">
                    <option value="hero">Aumovio</option>
                    <option value="kai">Kai</option>
                    <option value="custom">Custom</option>
                </select>
                <br><br>
                <div id="customLayoutFields" style="display:none;">
                <label for="layoutName">Layout-Name:</label>
                <input type="text" id="layoutName" maxlength="32" placeholder="z.B. Meine Uhr">

                <label for="layoutText" style="margin-top:12px;">Layout-Text (10 Zeilen x 11 Zeichen):</label>
                <textarea id="layoutText" rows="10" placeholder="ESXISTXFUEN&#10;ZEHNZWANZIG&#10;XXXXVIERTEL&#10;VORLOVENACH&#10;HALBXELFUEN&#10;DREIYOUVIER&#10;SECHSSIEBEN&#10;ZEHNEUNZWEI&#10;XACHTZWOLFX&#10;EINSUHR****"></textarea>

                <label for="layoutWords" style="margin-top:12px;">Wortpositionen (JSON):</label>
                <textarea id="layoutWords" rows="10" placeholder='{"ES":[0,0,2],"IST":[3,0,3],"M1":[7,9,1]}'></textarea>

                <button type="button" onclick="saveLayout()" style="margin-top:10px;">Custom Layout speichern</button>
                </div>
                <pre id="layoutPreview" style="margin-top:10px; white-space:pre-wrap; font-size:12px;"></pre>
                <p id="layoutMsg" style="font-size:14px; min-height:20px;"></p>
            </div>

            </form>

            <button type="button" onclick="confirmReboot()" id="rebootBtn" style="margin-top:10px;">Neustart</button>
            <button type="button" onclick="location.href='/live'" style="margin-top:6px;">Live-Regie</button>
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
                <button type="button" onclick="location.href='/live'">Live-Regie öffnen</button>
                <button type="button" onclick="location.href='/test'">Tests öffnen</button>
            </div>
        </div>
    </div>
</div>
</div>


<script>
// ---------------------------------------------------------
// Theme Toggle
// ---------------------------------------------------------
function setTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('wc_theme', theme);
    const t = document.getElementById('themeToggle');
    if (t) t.textContent = 'Theme: ' + (theme === 'light' ? 'Light' : 'Dark');
}

function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'dark';
    setTheme(current === 'dark' ? 'light' : 'dark');
}

function initTheme() {
    const saved = localStorage.getItem('wc_theme');
    const preferred = (saved === 'light' || saved === 'dark') ? saved : 'dark';
    setTheme(preferred);
    const toggle = document.getElementById('themeToggle');
    if (toggle) {
        toggle.addEventListener('click', function(e) {
            e.preventDefault();
            toggleTheme();
        });
    }
}

// ---------------------------------------------------------
// Tab Navigation
// ---------------------------------------------------------
function switchTab(tabName) {
    document.querySelectorAll('.tab-content').forEach(tab => tab.classList.remove('active'));
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    document.getElementById(tabName + '-tab').classList.add('active');
    event.target.classList.add('active');
    
    if (tabName === 'ota') {
        loadOtaInfo();
    } else if (tabName === 'layout') {
        loadLayoutInfo();
    } else if (tabName === 'config') {
        loadSetupFields();
        loadSSIDs();
        scheduleScan();
    }
}

// ---------------------------------------------------------
// Setup Routing (check current route, load all fields initially)
// ---------------------------------------------------------
(function() {
    const path = location.pathname;
    const nav = document.getElementById('setupNavLinks');
    if (nav) {
        Array.from(nav.querySelectorAll('a')).forEach(link => {
            if (link.getAttribute('href') === path) {
                link.classList.add('active');
            }
        });
    }
})();

// ---------------------------------------------------------
// WiFi SSID Scan
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

function scheduleScan() {
    let delay = (lastSSIDs.length === 0) ? 2000 : 5000;
    setTimeout(function() {
        loadSSIDs();
        scheduleScan();
    }, delay);
}

// ---------------------------------------------------------
// Load config fields from device
// ---------------------------------------------------------
async function loadSetupFields() {
    try {
        const r = await fetch('/api/status-lite');
        const s = await r.json();
        const ssidManual = document.getElementById('ssid_manual');
        const mqttServer = document.getElementById('mqtt_server');
        const mqttPort = document.getElementById('mqtt_port');
        const mqttUser = document.getElementById('mqtt_user');

        if (ssidManual && typeof s.wifi_ssid === 'string') ssidManual.value = s.wifi_ssid;
        if (mqttServer && typeof s.mqtt_server === 'string') mqttServer.value = s.mqtt_server;
        if (mqttPort && s.mqtt_port !== undefined) mqttPort.value = String(s.mqtt_port);
        if (mqttUser && typeof s.mqtt_user === 'string') mqttUser.value = s.mqtt_user;
    } catch (_) {}
}

loadSSIDs();
scheduleScan();
loadSetupFields();

// ---------------------------------------------------------
// Password Toggles
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
// Save Config (WiFi or MQTT)
// ---------------------------------------------------------
async function saveConfig(section) {
    const msg = document.getElementById('cfgMsg');
    msg.textContent = 'Speichere...';

    let params = new URLSearchParams();
    
    if (section === 'wifi') {
        let ssidManual = document.getElementById("ssid_manual").value.trim();
        let ssidSelect = document.getElementById("ssid_list").value;
        params.set("ssid", ssidManual || ssidSelect);
        const wifiPass = document.getElementById("wifi_pass").value;
        if (wifiPass && wifiPass.trim().length > 0) {
            params.set("wifi_pass", wifiPass);
        }
    } else if (section === 'mqtt') {
        params.set("mqtt_server", document.getElementById("mqtt_server").value);
        params.set("mqtt_port", document.getElementById("mqtt_port").value);
        params.set("mqtt_user", document.getElementById("mqtt_user").value);
        const mqttPass = document.getElementById("mqtt_pass").value;
        if (mqttPass && mqttPass.trim().length > 0) {
            params.set("mqtt_pass", mqttPass);
        }
    }

    try {
        const r = await fetch("/save", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params
        });
        const j = await r.json();
        if (j.status === "ok") {
            msg.textContent = j.msg || (section === 'wifi' ? 'WiFi gespeichert' : 'MQTT gespeichert');
        } else {
            msg.textContent = j.msg || 'Fehler';
        }
    } catch (_) {
        msg.textContent = 'Speichern fehlgeschlagen';
    }
}

// ---------------------------------------------------------
// Reboot
// ---------------------------------------------------------
function confirmReboot() {
    if (!confirm("WordClock wirklich neu starten?")) return;
    fetch("/reboot");
    alert("WordClock startet neu...");
}

// ---------------------------------------------------------
// OTA Functions
// ---------------------------------------------------------
function formatOtaIntervalLabel(profile) {
    if (profile === 'dev') return 'alle 2 Minuten';
    if (profile === 'norm') return 'alle 12 Stunden';
    return '1x pro Woche';
}

async function loadOtaInfo() {
    const msg = document.getElementById('otaMsg');
    msg.textContent = 'Lade OTA Status...';
    try {
        const r = await fetch('/api/ota/info');
        const j = await r.json();
        document.getElementById('otaFwVersion').textContent = j.fw_version || '-';
        document.getElementById('otaWifi').textContent = j.wifi_connected ? ('Verbunden (' + (j.ip || '-') + ')') : 'Offline';
        document.getElementById('otaProfile').value = j.ota_profile || 'long';
        document.getElementById('otaIntervalLabel').textContent = formatOtaIntervalLabel(j.ota_profile || 'long');
        msg.textContent = '';
    } catch (_) {
        msg.textContent = 'OTA Status konnte nicht geladen werden';
    }
}

async function saveOtaProfile() {
    const msg = document.getElementById('otaMsg');
    const profile = document.getElementById('otaProfile').value || 'long';
    msg.textContent = 'Speichere OTA-Profil...';
    try {
        const p = new URLSearchParams();
        p.set('profile', profile);
        const r = await fetch('/api/ota/profile', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: p
        });
        const j = await r.json();
        document.getElementById('otaProfile').value = j.ota_profile || profile;
        document.getElementById('otaIntervalLabel').textContent = formatOtaIntervalLabel(j.ota_profile || profile);
        msg.textContent = j.message || (r.ok ? 'Profil gespeichert' : 'Fehler');
    } catch (_) {
        msg.textContent = 'OTA-Profil konnte nicht gespeichert werden';
    }
}

async function checkOtaNow() {
    const msg = document.getElementById('otaMsg');
    msg.textContent = 'Prüfe Version...';
    try {
        const r = await fetch('/api/ota/check', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: ''
        });
        const j = await r.json();
        msg.textContent = j.message || (r.ok ? 'Prüfung abgeschlossen' : 'Fehler');
    } catch (_) {
        msg.textContent = 'OTA Prüfung fehlgeschlagen';
    }
}

// ---------------------------------------------------------
// Layout Functions
// ---------------------------------------------------------
function toggleLayoutInputs() {
    const id = document.getElementById('layoutId').value || 'kai';
    const isCustom = (id === 'custom');
    document.getElementById('customLayoutFields').style.display = isCustom ? '' : 'none';
}

async function onLayoutDropdownChange() {
    toggleLayoutInputs();
    const id = document.getElementById('layoutId').value || 'kai';
    if (id !== 'custom') {
        await saveLayout();
    }
}

async function loadLayoutInfo() {
    const msg = document.getElementById('layoutMsg');
    if (!msg) return;
    msg.textContent = 'Lade Layout...';
    try {
        const r = await fetch('/api/layout');
        const j = await r.json();
        document.getElementById('layoutId').value = j.layout_id || 'kai';
        document.getElementById('layoutName').value = j.layout_name || '';
        document.getElementById('layoutText').value = j.layout_text || '';
        document.getElementById('layoutWords').value = j.word_positions || '{}';
        document.getElementById('layoutPreview').textContent = j.layout_text || '';
        toggleLayoutInputs();
        msg.textContent = '';
    } catch (_) {
        msg.textContent = 'Layout konnte nicht geladen werden';
    }
}

async function saveLayout() {
    const msg = document.getElementById('layoutMsg');
    const layoutId = document.getElementById('layoutId').value || 'kai';
    const layoutName = document.getElementById('layoutName').value || '';
    const layoutText = document.getElementById('layoutText').value || '';
    const wordPositions = document.getElementById('layoutWords').value || '{}';
    msg.textContent = 'Speichere Layout...';

    try {
        const p = new URLSearchParams();
        p.set('layout_id', layoutId);
        p.set('layout_name', layoutName);
        p.set('layout_text', layoutText);
        p.set('word_positions', wordPositions);
        const r = await fetch('/api/layout', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: p
        });
        const j = await r.json();
        document.getElementById('layoutId').value = j.layout_id || layoutId;
        if (j.layout_name !== undefined) document.getElementById('layoutName').value = j.layout_name;
        document.getElementById('layoutText').value = j.layout_text || layoutText;
        document.getElementById('layoutPreview').textContent = j.layout_text || layoutText;
        toggleLayoutInputs();
        msg.textContent = j.message || (r.ok ? 'Layout gespeichert' : 'Fehler');
    } catch (_) {
        msg.textContent = 'Layout speichern fehlgeschlagen';
    }
}

// ---------------------------------------------------------
// Status Refresh
// ---------------------------------------------------------
async function refreshSetupStatus() {
    if (document.hidden) return;
    try {
        const r = await fetch('/api/status-lite');
        const s = await r.json();

        const rtcBadge = s.rtc_warning ? '<span class="warn">WARNUNG</span>' : '<span class="ok">OK</span>';
        const mqttBadge = s.mqtt_connected ? '<span class="ok">Verbunden</span>' : '<span class="warn">Getrennt</span>';
        const memLevel = String(s.mem_level || 'OK').toUpperCase();
        const memBadge = (memLevel === 'CRITICAL' || memLevel === 'WARNING') ? '<span class="warn">' + memLevel + '</span>' : '<span class="ok">OK</span>';
        const rtcTempText = (typeof s.rtc_temp_c === 'number' && Number.isFinite(s.rtc_temp_c)) ? (s.rtc_temp_c.toFixed(2) + ' C') : 'n/a';
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
            '<div class="detailKey">OTA-Profil</div><div class="detailVal">' + formatOtaIntervalLabel(s.ota_profile || 'long') + '</div>' +
            '<div class="detailKey">Layout</div><div class="detailVal">' + (s.layout_name || s.layout_id || 'Standard') + '</div>' +
            '<div class="detailKey">RTC Temperatur</div><div class="detailVal">' + rtcTempText + '</div>' +
            '<div class="detailKey">RAM Frei</div><div class="detailVal">' + memFree + ' B</div>' +
            '<div class="detailKey">RAM Nutzung</div><div class="detailVal">' + memUsedPct + '%</div>';
    } catch (_) {}
}

// ---------------------------------------------------------
// Init
// ---------------------------------------------------------
initTheme();
loadSetupFields();
loadSSIDs();
scheduleScan();
setInterval(refreshSetupStatus, 500);
refreshSetupStatus();
</script>

</body>
</html>
)rawliteral";

const char live_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de" data-theme="dark">
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
            background: linear-gradient(180deg, color-mix(in srgb, var(--panel-soft) 94%, transparent), color-mix(in srgb, var(--panel) 94%, transparent));
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
            border: 1px solid color-mix(in srgb, var(--line-strong) 34%, transparent);
            background: color-mix(in srgb, var(--panel-soft) 96%, transparent);
            color: var(--text);
            font: inherit;
        }
        select option {
            background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
            color: var(--text);
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
            <a href="/main">Netzwerk</a>
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
                Direkte Regie für Effekt, Farbe, Helligkeit, Geschwindigkeit, Intensität,
                Objektdichte und Übergang. Dazu Live-Telemetrie, Matrix-Vorschau und Schnelltests
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
                            <option value="love">Spezial</option>
                            <option value="colorloop">colorloop</option>
                            <option value="colorwipe">colorwipe</option>
                            <option value="fire2d">fire2d</option>
                            <option value="matrix">matrix</option>
                            <option value="plasma">plasma</option>
                            <option value="waterdrop_r">waterdrop (reverse)</option>
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
                        <div class="fieldRow"><label for="intensity">Intensität</label><span class="metricBadge" id="intensityValue">50%</span></div>
                        <input id="intensity" type="range" min="1" max="100" value="50">
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="density">Objektdichte</label><span class="metricBadge" id="densityValue">50%</span></div>
                        <input id="density" type="range" min="1" max="100" value="50">
                    </div>

                    <div class="field">
                        <div class="fieldRow"><label for="transitionMs">Übergang</label><span class="metricBadge" id="transitionValue">1000 ms</span></div>
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
    const el = document.getElementById(id);
    if (el) el.textContent = value;
}

function setHtml(id, value) {
    const el = document.getElementById(id);
    if (el) el.innerHTML = value;
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

const defaultLayoutRows = [
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

let currentLayoutRows = defaultLayoutRows.slice();

function setLayoutRowsFromText(layoutText) {
    if (typeof layoutText !== 'string' || !layoutText.trim()) {
        currentLayoutRows = defaultLayoutRows.slice();
        return;
    }

    const rows = layoutText
        .split('\n')
        .map(function(r) { return r.replace(/\r/g, ''); });

    const normalized = [];
    for (let y = 0; y < 10; y++) {
        let row = (rows[y] || '').toUpperCase();
        if (row.length < 11) row = row + '.'.repeat(11 - row.length);
        if (row.length > 11) row = row.substring(0, 11);
        normalized.push(row);
    }

    currentLayoutRows = normalized;
}

function layoutGlyphAt(x, y) {
    if (y < 0 || y >= currentLayoutRows.length) return '';
    const row = currentLayoutRows[y];
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
                // Minuten-LED: einfache Strich-Anzeige statt Pfoten-Symbol
                const minuteColor = mqttConnected ? ('#' + uiHex) : '#ff4444';
                const minuteGlow = mqttConnected
                    ? (isOn ? ('0 0 8px #' + uiHex + ', 0 0 16px #' + uiHex) : 'none')
                    : '0 0 8px #ff4444, 0 0 16px #ff4444';
                html += '<div class="cell" title="x=' + x + ' y=' + y + ' [MINUTE]">'
                    + '<div class="glyph" style="color:' + minuteColor + ';text-shadow:' + minuteGlow + ';">-</div>'
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
    const container = document.getElementById('statusDetails');
    if (!container) return;
    const details = [
        ['IP', s.ip || '-'],
        ['Farbe', s.color || '-'],
        ['Helligkeit', String(s.brightness)],
        ['Geschwindigkeit', String(s.speed) + '%'],
        ['Intensität', String(s.intensity) + '%'],
        ['Objektdichte', String(s.density) + '%'],
        ['Übergang', String(s.transition_ms) + ' ms'],
        ['RTC Temperatur', rtcTempText],
        ['RTC OSF/Batterie', s.rtc_battery_warning ? 'Auffällig' : 'OK'],
        ['RAM Frei', String(memFree) + ' B'],
        ['RAM Nutzung', memUsedPct + '%'],
        ['Max Block', String(s.mem_max_alloc || 0) + ' B']
    ];

    container.innerHTML = details.map(function(item) {
        return '<div class="detailCard"><div class="detailKey">' + item[0] + '</div><div class="detailVal">' + item[1] + '</div></div>';
    }).join('');
}

const WC_STATUS_CACHE_KEY = 'wc_status_cache';

function applyStatus(s, fromCache) {
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

    setHtml('stPower', s.state);
    setHtml('stEffect', s.effect);
    setHtml('stRtc', rtcBadge);
    setHtml('stWifi', (s.rssi + ' dBm'));
    setHtml('stMqtt', mqttBadge);
    setHtml('stMem', memBadge);
    setText('heroEffect', s.effect || '-');
    setText('heroColor', s.color || '-');
    setText('heroTuning', String(s.brightness) + ' / ' + String(s.speed) + '%');

    setLayoutRowsFromText(s.layout_text);

    const memAlert = document.getElementById('memAlert');
    if (memAlert) {
        memAlert.className = 'memAlert';
        memAlert.textContent = '';
        if (memLevel === 'CRITICAL') {
            memAlert.className = 'memAlert critical';
            memAlert.textContent = 'Low Memory: Kritischer RAM-Zustand. Schwere Effekte werden automatisch verlassen.';
        } else if (memLevel === 'WARNING') {
            memAlert.className = 'memAlert warn';
            memAlert.textContent = 'Low Memory: RAM wird knapp. Bitte eher leichte Effekte verwenden.';
        }
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

    // Matrix is heavy to cache (110 pixels); only render when we have live data
    if (!fromCache && Array.isArray(s.matrix)) {
        renderMatrix(s.matrix, mqttConnected);
    }
}

function preloadFromCache() {
    try {
        const raw = localStorage.getItem(WC_STATUS_CACHE_KEY);
        if (!raw) return;
        const s = JSON.parse(raw);
        if (s && typeof s === 'object') {
            applyStatus(s, true);
        }
    } catch (_) {}
}

async function refreshStatus() {
    if (document.hidden) return;
    if (refreshInFlight) return;
    refreshInFlight = true;
    try {
        const r = await fetch('/api/status');
        const s = await r.json();
        applyStatus(s, false);
        // Cache status without matrix (saves ~3KB in localStorage)
        try {
            const cacheable = Object.assign({}, s);
            delete cacheable.matrix;
            localStorage.setItem(WC_STATUS_CACHE_KEY, JSON.stringify(cacheable));
        } catch (_) {}
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
preloadFromCache();
refreshStatus();
startRefreshTimer();
</script>
</body>
</html>
)rawliteral";

const char test_html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de" data-theme="dark">
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
        --text-secondary: #d7dde9;
        --muted: #acb3c4;
        --accent: #bfc6d6;
    }
    [data-theme="light"] {
        --bg-a: #f4f5f9;
        --bg-b: #e9ecf2;
        --panel: rgba(255, 255, 255, 0.88);
        --panel-soft: rgba(247, 248, 252, 0.92);
        --line: rgba(80, 95, 120, 0.2);
        --line-strong: rgba(67, 85, 114, 0.44);
        --text: #192233;
        --text-secondary: #3a4556;
        --muted: #5e677a;
        --accent: #7a8599;
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
        background: color-mix(in srgb, var(--panel-soft) 85%, transparent);
        backdrop-filter: blur(14px);
        box-shadow: 0 20px 54px rgba(0, 0, 0, 0.28);
    }
    [data-theme="light"] .topbar {
        box-shadow: 0 8px 16px rgba(0, 0, 0, 0.06);
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
        color: var(--text);
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
        border: 1px solid var(--line);
        border-radius: 999px;
        background: color-mix(in srgb, var(--panel-soft) 92%, transparent);
        font-size: 13px;
        transition: border-color 0.2s ease, background 0.2s ease;
    }
    .navLinks a.active,
    .navLinks a:hover {
        border-color: var(--line-strong);
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
    }
    .wrap { max-width:980px; margin:0 auto; padding:0 0 20px; }
    .hero {
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background: color-mix(in srgb, var(--panel) 92%, rgba(37, 47, 66, 0.96));
        padding: 14px 16px;
        margin-bottom: 12px;
    }
    .hero h2 { margin: 0 0 6px; font-size: 24px; color: var(--text); }
    .hero p { margin: 0; color: var(--text-secondary); }
    .card {
        background: var(--panel);
        border: 1px solid var(--line);
        border-radius: 16px;
        padding: 16px;
        box-shadow: 0 14px 28px rgba(0, 0, 0, 0.24);
    }
    [data-theme="light"] .card {
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.06);
    }
    .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(140px, 1fr)); gap:8px; margin-top:10px; }
    button {
        width: 100%;
        padding: 10px 8px;
        border-radius: 10px;
        border: 1px solid var(--line);
        background: color-mix(in srgb, var(--panel-soft) 100%, transparent);
        color: var(--text);
        cursor: pointer;
        font-size: 13px;
        transition: border-color 0.2s ease, background 0.2s ease;
    }
    button:hover {
        border-color: var(--line-strong);
        background: color-mix(in srgb, var(--panel-soft) 100%, rgba(147, 162, 190, 0.1));
    }
    button.warn { 
        border-color: rgba(245, 179, 165, 0.42);
        background: color-mix(in srgb, #b05643 18%, transparent);
        color: #b05643;
    }
    [data-theme="light"] button.warn {
        color: #a04430;
    }
    button.info { 
        background: color-mix(in srgb, var(--accent) 12%, transparent);
        color: var(--accent);
    }
    button.special { 
        border-color: rgba(195, 220, 178, 0.36);
        background: color-mix(in srgb, #268b46 12%, transparent);
        color: #268b46;
    }
    [data-theme="light"] button.special {
        color: #1d6a35;
    }
    .msg { color: var(--text); margin-top:10px; min-height:20px; font-size:13px; }
    .section-title { margin-top:12px; font-weight:bold; color: var(--accent); font-size:12px; text-transform: uppercase; letter-spacing: 0.04em; }
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
            <a href="/main">Netzwerk</a>
            <a href="/live">Studio</a>
            <a href="/test" class="active">Test</a>
            <a href="#" id="themeToggle">Theme: Dark</a>
        </div>
    </div>
<div class="wrap">
    <div class="hero">
        <h2>WordClock Test Deck</h2>
        <p>Diagnose für Farben, Helligkeit, Muster und Segment-Checks in Studio-Optik.</p>
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
            <button style="background:#27ae60" onclick="quick('color_green')">Grün</button>
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
function setTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('wc_theme', theme);
    const t = document.getElementById('themeToggle');
    if (t) t.textContent = 'Theme: ' + (theme === 'light' ? 'Light' : 'Dark');
}

function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'dark';
    setTheme(current === 'dark' ? 'light' : 'dark');
}

function initTheme() {
    const saved = localStorage.getItem('wc_theme');
    const preferred = (saved === 'light' || saved === 'dark') ? saved : 'dark';
    setTheme(preferred);
    const toggle = document.getElementById('themeToggle');
    if (toggle) {
        toggle.addEventListener('click', function(e) {
            e.preventDefault();
            toggleTheme();
        });
    }
}

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
    msg.textContent = 'Führe Test aus (3s Haltedauer)...';
    const p = new URLSearchParams();
    p.set('action', action);
    p.set('hold_ms', '3000');

    try {
        const r = await fetch('/api/quicktest', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: p
        });
        msg.textContent = r.ok ? 'Test ausgeführt' : 'Fehler beim Test';
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

initTheme();
</script>
</body>
</html>
)rawliteral";

