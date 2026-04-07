import math
import random
import socket
import threading
from collections import deque
from datetime import datetime

import streamlit as st

WIDTH = 11
HEIGHT = 10
SERIAL_BUFFER_LINES = 500

EFFECTS = [
    "clock",
    "aurora",
    "twinkle",
    "matrix",
    "fire2d",
    "wifi",
    "plasma",
    "snake",
]


class WifiLogTail:
    def __init__(self) -> None:
        self.lines = deque(maxlen=SERIAL_BUFFER_LINES)
        self._lock = threading.Lock()
        self._stop_event = threading.Event()
        self._thread = None
        self._sock = None
        self.host = ""
        self.port = 23
        self.connected = False
        self.last_error = ""
        self._partial = ""

    def start(self, host: str, port: int) -> None:
        self.stop()
        self.last_error = ""
        try:
            self._sock = socket.create_connection((host, port), timeout=3.0)
            self._sock.settimeout(0.3)
            self.host = host
            self.port = port
            self.connected = True
            self._stop_event.clear()
            self._thread = threading.Thread(target=self._reader_loop, daemon=True)
            self._thread.start()
            self._append_line(f"[SYSTEM] Verbunden mit {host}:{port}")
        except Exception as exc:
            self.last_error = str(exc)
            self.connected = False
            self._sock = None

    def stop(self) -> None:
        self._stop_event.set()
        if self._thread is not None and self._thread.is_alive():
            self._thread.join(timeout=1.0)
        self._thread = None

        if self._sock is not None:
            try:
                self._sock.close()
            except Exception:
                pass

        if self.connected:
            self._append_line("[SYSTEM] WiFi-Logstream getrennt")

        self._sock = None
        self.connected = False

    def _append_line(self, line: str) -> None:
        timestamp = datetime.now().strftime("%H:%M:%S")
        with self._lock:
            self.lines.append(f"{timestamp}  {line}")

    def _reader_loop(self) -> None:
        while not self._stop_event.is_set() and self._sock is not None:
            try:
                raw = self._sock.recv(1024)
                if not raw:
                    break

                chunk = raw.decode("utf-8", errors="replace")
                self._partial += chunk
                lines = self._partial.splitlines(keepends=False)
                if self._partial and not self._partial.endswith("\n") and not self._partial.endswith("\r"):
                    self._partial = lines.pop() if lines else self._partial
                else:
                    self._partial = ""

                for line in lines:
                    line = line.strip()
                    if line:
                        self._append_line(line)
            except Exception as exc:
                self.last_error = str(exc)
                self._append_line(f"[ERROR] {exc}")
                break

        self.connected = False

    def send_line(self, line: str) -> None:
        if not self.connected or self._sock is None:
            return
        try:
            self._sock.sendall((line + "\n").encode("utf-8"))
        except Exception as exc:
            self.last_error = str(exc)
            self._append_line(f"[ERROR] send failed: {exc}")

    def get_lines(self) -> list[str]:
        with self._lock:
            return list(self.lines)


def clamp(value: float, min_v: float, max_v: float) -> float:
    return max(min_v, min(max_v, value))


def blend(c0: tuple[int, int, int], c1: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    t = clamp(t, 0.0, 1.0)
    return (
        int(c0[0] + (c1[0] - c0[0]) * t),
        int(c0[1] + (c1[1] - c0[1]) * t),
        int(c0[2] + (c1[2] - c0[2]) * t),
    )


def to_hex(rgb: tuple[int, int, int]) -> str:
    return f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"


def scaled(rgb: tuple[int, int, int], brightness: int, factor: float = 1.0) -> tuple[int, int, int]:
    scale = (brightness / 100.0) * factor
    return (
        int(clamp(rgb[0] * scale, 0, 255)),
        int(clamp(rgb[1] * scale, 0, 255)),
        int(clamp(rgb[2] * scale, 0, 255)),
    )


def matrix_preview(cells: list[list[tuple[int, int, int]]], title: str) -> None:
    html = [
        "<div style='padding:16px;border-radius:14px;background:linear-gradient(150deg,#0b1220,#111827 45%,#1f2937);border:1px solid #334155;'>",
        f"<div style='color:#e2e8f0;font-weight:600;margin-bottom:10px'>{title}</div>",
        "<div style='display:grid;grid-template-columns:repeat(11,22px);gap:6px;justify-content:center'>",
    ]

    for y in range(HEIGHT):
        for x in range(WIDTH):
            html.append(
                f"<div style='width:22px;height:22px;border-radius:6px;background:{to_hex(cells[y][x])};"
                "box-shadow:0 0 7px rgba(255,255,255,.09) inset,0 0 10px rgba(0,0,0,.3);'></div>"
            )

    html.append("</div></div>")
    st.markdown("".join(html), unsafe_allow_html=True)


def empty_matrix() -> list[list[tuple[int, int, int]]]:
    return [[(8, 10, 16) for _ in range(WIDTH)] for _ in range(HEIGHT)]


def make_frame(effect: str, color: tuple[int, int, int], brightness: int, speed: int, intensity: int, density: int) -> list[list[tuple[int, int, int]]]:
    frame = empty_matrix()
    now = datetime.now()
    t = now.second + now.microsecond / 1_000_000.0
    speed_f = 0.3 + speed / 100.0 * 2.2
    inten_f = 0.2 + intensity / 100.0 * 1.2
    dens_f = density / 100.0

    if effect == "clock":
        phrase = f"{now.hour:02d}:{now.minute:02d}"
        start_x = max(0, (WIDTH - len(phrase)) // 2)
        y = HEIGHT // 2
        for i, ch in enumerate(phrase[:WIDTH]):
            if ch != " ":
                frame[y][start_x + i] = scaled(color, brightness, inten_f)
        return frame

    if effect == "aurora":
        c0 = blend((20, 90, 70), color, 0.55)
        c1 = blend((80, 30, 120), color, 0.35)
        for y in range(HEIGHT):
            for x in range(WIDTH):
                wave = (
                    math.sin((x * 0.8 + t * speed_f) * (0.6 + dens_f))
                    + math.cos((y * 0.9 - t * speed_f * 0.7) * (0.7 + dens_f * 0.8))
                ) * 0.5
                mix = (wave + 1.0) * 0.5
                px = blend(c0, c1, mix)
                frame[y][x] = scaled(px, brightness, inten_f)
        return frame

    if effect == "twinkle":
        stars = int(3 + dens_f * 26)
        for _ in range(stars):
            x = random.randint(0, WIDTH - 1)
            y = random.randint(0, HEIGHT - 1)
            sparkle = 0.4 + 0.6 * abs(math.sin(t * speed_f * 2.3 + x + y))
            frame[y][x] = scaled(color, brightness, inten_f * sparkle)
        return frame

    if effect == "matrix":
        heads = max(1, int(1 + dens_f * WIDTH))
        for col in random.sample(range(WIDTH), k=min(heads, WIDTH)):
            head_y = int((t * speed_f * 4 + col * 1.7) % HEIGHT)
            for i in range(HEIGHT):
                d = (head_y - i) % HEIGHT
                if d < int(2 + inten_f * 3):
                    fade = 1.0 - d / max(1, int(2 + inten_f * 3))
                    frame[i][col] = scaled(color, brightness, 0.25 + fade * 0.9)
        return frame

    if effect == "fire2d":
        base = blend((255, 80, 8), color, 0.25)
        for y in range(HEIGHT):
            for x in range(WIDTH):
                noise = random.random() * (0.35 + dens_f * 0.65)
                lift = (HEIGHT - 1 - y) / max(1, HEIGHT - 1)
                flame = clamp(noise + lift * inten_f, 0.0, 1.0)
                frame[y][x] = scaled(base, brightness, flame)
        return frame

    if effect == "wifi":
        ring = int(dens_f * 4)
        top = ring
        left = ring
        right = WIDTH - 1 - ring
        bottom = HEIGHT - 1 - ring
        trail = int(2 + inten_f * 4)
        perimeter = []
        for x in range(left, right + 1):
            perimeter.append((top, x))
        for y in range(top + 1, bottom + 1):
            perimeter.append((y, right))
        for x in range(right - 1, left - 1, -1):
            perimeter.append((bottom, x))
        for y in range(bottom - 1, top, -1):
            perimeter.append((y, left))

        if perimeter:
            head = int((t * speed_f * 5) % len(perimeter))
            for i in range(trail):
                idx = (head - i) % len(perimeter)
                y, x = perimeter[idx]
                frame[y][x] = scaled(color, brightness, 1.0 - i / max(1, trail))
        return frame

    if effect == "plasma":
        for y in range(HEIGHT):
            for x in range(WIDTH):
                value = (
                    math.sin((x + t * speed_f) * (0.6 + dens_f))
                    + math.sin((y - t * speed_f * 0.9) * (0.7 + dens_f))
                ) * 0.5
                m = (value + 1.0) * 0.5
                px = blend((20, 40, 110), color, m)
                frame[y][x] = scaled(px, brightness, inten_f)
        return frame

    if effect == "snake":
        length = int(4 + dens_f * 20)
        head_x = int((t * speed_f * 3) % WIDTH)
        head_y = int((t * speed_f * 2) % HEIGHT)
        for i in range(length):
            x = (head_x - i) % WIDTH
            y = (head_y + (i // 3)) % HEIGHT
            fade = 1.0 - i / max(1, length)
            frame[y][x] = scaled(color, brightness, (0.4 + fade * 0.8) * inten_f)
        return frame

    return frame


def init_state() -> None:
    defaults = {
        "power": True,
        "brightness": 47,
        "effect": "clock",
        "color": "#ff9900",
        "speed": 50,
        "intensity": 50,
        "density": 50,
        "transition_ms": 1000,
        "version": "0.2.4",
        "wifi_rssi": -58,
        "mqtt_connected": True,
        "rtc_ok": True,
        "rtc_temp_c": 31.8,
        "uptime_s": 0,
        "refresh_ms": 1000,
        "log_host": "192.168.178.73",
        "log_port": 23,
        "event_log": deque(maxlen=120),
    }
    for k, v in defaults.items():
        if k not in st.session_state:
            st.session_state[k] = v

    if "log_tail" not in st.session_state:
        st.session_state.log_tail = WifiLogTail()


def log_event(message: str) -> None:
    timestamp = datetime.now().strftime("%H:%M:%S")
    st.session_state.event_log.appendleft(f"{timestamp}  {message}")


def update_telemetry() -> None:
    st.session_state.uptime_s += max(1, int(st.session_state.refresh_ms / 1000))


def tcp_probe(host: str, port: int, timeout_s: float = 1.2) -> tuple[bool, str]:
    try:
        with socket.create_connection((host, port), timeout=timeout_s):
            return True, "reachable"
    except Exception as exc:
        return False, str(exc)


def render_header_cards() -> None:
    c1, c2, c3, c4 = st.columns(4)
    with c1:
        st.metric("Power", "ON" if st.session_state.power else "OFF")
    with c2:
        st.metric("Effect", st.session_state.effect)
    with c3:
        st.metric("Brightness", st.session_state.brightness)
    with c4:
        st.metric("Firmware", st.session_state.version)


def render_sidebar() -> None:
    with st.sidebar:
        st.subheader("Live Controls")

        old_power = st.session_state.power
        st.session_state.power = st.toggle("Power", value=st.session_state.power)
        if old_power != st.session_state.power:
            log_event(f"Power -> {'ON' if st.session_state.power else 'OFF'}")

        st.session_state.brightness = st.slider("Brightness", 0, 100, st.session_state.brightness)
        st.session_state.effect = st.selectbox("Effect", EFFECTS, index=EFFECTS.index(st.session_state.effect))
        st.session_state.color = st.color_picker("Color", st.session_state.color)

        st.divider()
        st.subheader("Effect Tuning")
        st.session_state.speed = st.slider("Speed", 1, 100, st.session_state.speed)
        st.session_state.intensity = st.slider("Intensity", 1, 100, st.session_state.intensity)
        st.session_state.density = st.slider("Density", 1, 100, st.session_state.density)
        st.session_state.transition_ms = st.slider("Transition ms", 100, 4000, st.session_state.transition_ms, step=100)

        st.divider()
        st.subheader("Panel Refresh")
        st.session_state.refresh_ms = st.select_slider("Refresh interval", options=[500, 1000, 1500, 2000, 3000], value=st.session_state.refresh_ms)
        if st.button("Refresh now"):
            st.rerun()


def main() -> None:
    st.set_page_config(page_title="WordClock UI Test", page_icon="⌚", layout="wide")
    init_state()
    update_telemetry()

    st.title("WordClock Control Center")
    st.caption("Erweiterter Python-Prototyp mit Live Matrix, Diagnose und WiFi-Logstream")

    render_sidebar()
    render_header_cards()

    rgb = tuple(int(st.session_state.color[i : i + 2], 16) for i in (1, 3, 5))
    tabs = st.tabs(["Live Stage", "Effect Studio", "Device Ops", "Test", "Serial Live"])

    with tabs[0]:
        c1, c2 = st.columns([2, 1])
        with c1:
            if st.session_state.power:
                frame = make_frame(
                    st.session_state.effect,
                    rgb,
                    st.session_state.brightness,
                    st.session_state.speed,
                    st.session_state.intensity,
                    st.session_state.density,
                )
            else:
                frame = empty_matrix()
            matrix_preview(frame, "11x10 Live Preview")

        with c2:
            st.subheader("Now Playing")
            st.write(f"Color: {st.session_state.color}")
            st.write(f"Transition: {st.session_state.transition_ms} ms")
            st.write(f"Uptime: {st.session_state.uptime_s}s")
            st.write(f"WiFi RSSI: {st.session_state.wifi_rssi} dBm")
            st.write(f"MQTT: {'connected' if st.session_state.mqtt_connected else 'disconnected'}")
            st.write(f"RTC: {'ok' if st.session_state.rtc_ok else 'warning'}")
            st.write(f"RTC Temp: {st.session_state.rtc_temp_c:.1f} °C")

            st.subheader("Recent Events")
            if st.session_state.event_log:
                st.code("\n".join(list(st.session_state.event_log)[:10]), language="text")
            else:
                st.info("Noch keine Events")

    with tabs[1]:
        st.subheader("Preset Lab")
        p1, p2, p3, p4 = st.columns(4)
        if p1.button("Calm"):
            st.session_state.speed = 25
            st.session_state.intensity = 35
            st.session_state.density = 30
            log_event("Preset geladen: Calm")
        if p2.button("Showroom"):
            st.session_state.speed = 55
            st.session_state.intensity = 70
            st.session_state.density = 65
            log_event("Preset geladen: Showroom")
        if p3.button("Party"):
            st.session_state.speed = 90
            st.session_state.intensity = 95
            st.session_state.density = 90
            log_event("Preset geladen: Party")
        if p4.button("Night"):
            st.session_state.speed = 20
            st.session_state.intensity = 22
            st.session_state.density = 25
            log_event("Preset geladen: Night")

        st.divider()
        st.info("History wurde bewusst entfernt. Fokus: direkte Steuerung + Diagnose + Live-Logs.")

    with tabs[2]:
        st.subheader("Device Operations")
        d1, d2, d3 = st.columns(3)
        with d1:
            st.metric("WiFi RSSI", f"{st.session_state.wifi_rssi} dBm")
            if st.button("WiFi reconnect test"):
                st.session_state.wifi_rssi = random.randint(-75, -45)
                log_event("WiFi reconnect simulated")
        with d2:
            st.metric("MQTT", "Connected" if st.session_state.mqtt_connected else "Disconnected")
            if st.button("Toggle MQTT"):
                st.session_state.mqtt_connected = not st.session_state.mqtt_connected
                log_event("MQTT state toggled")
        with d3:
            st.metric("RTC", "OK" if st.session_state.rtc_ok else "Warning")
            st.caption(f"RTC Temperatur: {st.session_state.rtc_temp_c:.1f} °C")

        st.divider()
        o1, o2, o3 = st.columns(3)
        with o1:
            if st.button("OTA check now"):
                log_event("OTA check simulated")
                st.success("OTA Check simuliert")
        with o2:
            if st.button("Reboot simulation"):
                log_event("Reboot simulated (persisted state kept)")
                st.warning("Reboot simuliert")
        with o3:
            if st.button("Clear event log"):
                st.session_state.event_log.clear()

        st.divider()
        st.subheader("Ready for WordClock")
        r1, r2 = st.columns([2, 1])
        with r1:
            st.write("**Device Profile**")
            st.write(f"- Ziel-IP: {st.session_state.log_host}")
            st.write(f"- Logstream TCP: {int(st.session_state.log_port)}")
            st.write("- MQTT Basis-Topic: wordclock")
            st.write("- Unterstützte Regler: brightness(0-100 UI), speed, intensity, density")
        with r2:
            if st.button("Apply WordClock Defaults", use_container_width=True):
                st.session_state.log_host = "192.168.178.73"
                st.session_state.log_port = 23
                st.session_state.effect = "clock"
                st.session_state.brightness = 47
                st.session_state.speed = 50
                st.session_state.intensity = 50
                st.session_state.density = 50
                st.session_state.color = "#ff9900"
                log_event("WordClock defaults applied")

    with tabs[3]:
        st.subheader("Test Reiter")
        st.caption("Schnelle Smoke-Tests und Readiness-Checks fur die reale Uhr")

        t1, t2, t3 = st.columns(3)
        with t1:
            if st.button("Smoke: Effect Cycle"):
                for fx in ["clock", "aurora", "twinkle", "matrix", "fire2d", "wifi"]:
                    log_event(f"TEST effect -> {fx}")
                st.success("Effect-Cycle Test ausgelost")
        with t2:
            if st.button("Smoke: State Persist"):
                log_event("TEST persist: power/effect/color should survive reboot")
                st.success("Persistenz-Test markiert")
        with t3:
            if st.button("Smoke: OTA Check"):
                log_event("TEST ota_check trigger")
                st.success("OTA-Test markiert")

        st.divider()
        c1, c2 = st.columns([1, 2])
        with c1:
            if st.button("Run Connectivity Check", use_container_width=True):
                ok, detail = tcp_probe(st.session_state.log_host, int(st.session_state.log_port))
                st.session_state["last_probe_ok"] = ok
                st.session_state["last_probe_detail"] = detail
                log_event(f"Probe {st.session_state.log_host}:{int(st.session_state.log_port)} -> {'OK' if ok else 'FAIL'}")

            probe_ok = st.session_state.get("last_probe_ok", None)
            probe_detail = st.session_state.get("last_probe_detail", "-")
            if probe_ok is True:
                st.success(f"TCP erreichbar ({probe_detail})")
            elif probe_ok is False:
                st.error(f"TCP nicht erreichbar: {probe_detail}")
            else:
                st.info("Noch kein Connectivity-Test")

        with c2:
            checklist = [
                ("Power Steuerung vorhanden", True),
                ("Effektwahl vorhanden", True),
                ("Brightness 0-100", 0 <= st.session_state.brightness <= 100),
                ("RTC nur read-only", True),
                ("WiFi Logstream konfiguriert", bool(st.session_state.log_host) and int(st.session_state.log_port) > 0),
            ]
            passed = 0
            for title, ok in checklist:
                st.write(f"{'PASS' if ok else 'FAIL'} - {title}")
                if ok:
                    passed += 1
            st.progress(passed / len(checklist), text=f"Readiness Score: {passed}/{len(checklist)}")

    with tabs[4]:
        st.subheader("WiFi Logstream (TCP)")

        c1, c2, c3, c4 = st.columns([2, 1, 1, 1])
        with c1:
            st.session_state.log_host = st.text_input("Host", value=st.session_state.log_host)
        with c2:
            st.session_state.log_port = st.number_input("Port", min_value=1, max_value=65535, value=int(st.session_state.log_port), step=1)
        with c3:
            if st.button("Connect", use_container_width=True):
                st.session_state.log_tail.start(st.session_state.log_host, int(st.session_state.log_port))
                if st.session_state.log_tail.connected:
                    log_event(f"WiFi log connected: {st.session_state.log_host}:{int(st.session_state.log_port)}")
        with c4:
            if st.button("Disconnect", use_container_width=True):
                st.session_state.log_tail.stop()
                log_event("WiFi log disconnected")

        status = st.session_state.log_tail
        if status.connected:
            st.success(f"Verbunden: {status.host}:{status.port}")
        elif status.last_error:
            st.error(status.last_error)
        else:
            st.info("Nicht verbunden")

        lines = st.session_state.log_tail.get_lines()
        st.text_area("Live Log Output", value="\n".join(lines[-250:]), height=420)

        cmd = st.text_input("Send command (optional)", value="")
        if st.button("Send") and cmd.strip():
            st.session_state.log_tail.send_line(cmd.strip())
            log_event(f"Befehl gesendet: {cmd.strip()}")

        if st.button("Clear log buffer"):
            st.session_state.log_tail.lines.clear()

    st.caption("Tipp: Setze als Host die IP deiner Uhr (aktuell oft 192.168.178.73) und den TCP-Logport (z. B. 23/Telnet).")


if __name__ == "__main__":
    main()
