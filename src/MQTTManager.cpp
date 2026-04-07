#include "MQTTManager.h"
#include <WiFi.h>
#include "ControlConfig.h"
#include "DebugManager.h"
#include "StateManager.h"
#include "RTCManager.h"
#include "SystemControl.h"
#include "ota_https_update.h"

// Effect tuning parameters defined in main.cpp
extern uint8_t  effectSpeed;
extern uint8_t  effectIntensity;
extern uint16_t transitionMs;
extern uint8_t  effectPalette;

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

MQTTManager::MQTTManager(WiFiClient& wifi_client, const String& device_id)
    : mqtt(wifi_client), wifi_client(wifi_client), device_id(device_id),
      server(""), port(1883), user(""), password(""),
      command_topic("wordclock/set"), state_topic("wordclock/state"),
      discover_topic("homeassistant/light/wordclock/config"),
            availability_topic("wordclock/availability"),
            uptime_topic("wordclock/uptime"),
            rssi_topic("wordclock/rssi"),
            ip_topic("wordclock/ip"),
            mqtt_state_topic("wordclock/mqtt_state"),
            version_topic("wordclock/version"),
            reboot_command_topic("wordclock/reboot/set"),
            speed_command_topic("wordclock/speed/set"),
            speed_state_topic("wordclock/speed/state"),
            intensity_command_topic("wordclock/intensity/set"),
            intensity_state_topic("wordclock/intensity/state"),
            transition_command_topic("wordclock/transition/set"),
            transition_state_topic("wordclock/transition/state"),
            tuning_reset_command_topic("wordclock/tuning_reset/set"),
            rtc_temp_topic("wordclock/rtc/temperature"),
            rtc_battery_warning_topic("wordclock/rtc/battery_warning"),
            ota_check_command_topic("wordclock/ota_check/set"),
            last_reconnect_attempt(0),
            last_telemetry_publish(0) {
    mqtt.setBufferSize(2048);  // Discovery-Payload kann ~1100 Bytes gross sein, Default wäre nur 256
    mqtt.setCallback([this](char* topic, byte* payload, unsigned int length) {
        String msg;
        for (unsigned int i = 0; i < length; i++) {
            msg += (char)payload[i];
        }
        
        String topic_str(topic);
        this->internalCallback(topic_str, msg);
    });
}

void MQTTManager::setConfig(const String& new_server, int new_port, const String& new_user, const String& new_password) {
    server = new_server;
    port = new_port;
    user = new_user;
    password = new_password;
    
    mqtt.setServer(server.c_str(), port);
}

void MQTTManager::setTopics(const String& new_command, const String& new_state, const String& new_discover) {
    command_topic = new_command;
    state_topic = new_state;
    discover_topic = new_discover;
}

void MQTTManager::connect() {
    if (server.isEmpty()) {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Server nicht konfiguriert");
        return;
    }
    
    if (isConnected()) {
        return;
    }
    
    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Verbinde mit ");
    DebugManager::print(DebugCategory::MQTT, server);
    DebugManager::print(DebugCategory::MQTT, ":");
    DebugManager::println(DebugCategory::MQTT, port);
    
    if (mqtt.connect(device_id.c_str(), user.c_str(), password.c_str(),
                     availability_topic.c_str(), 0, true, "offline")) {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Verbunden!");
        mqtt.subscribe(command_topic.c_str());
        mqtt.subscribe(reboot_command_topic.c_str());
        mqtt.subscribe(speed_command_topic.c_str());
        mqtt.subscribe(intensity_command_topic.c_str());
        mqtt.subscribe(transition_command_topic.c_str());
        mqtt.subscribe(tuning_reset_command_topic.c_str());
        mqtt.subscribe(ota_check_command_topic.c_str());
        publishDiscovery();
        publishTelemetry();
    } else {
        DebugManager::print(DebugCategory::MQTT, "MQTTManager: Verbindung fehlgeschlagen, Fehler: ");
        DebugManager::println(DebugCategory::MQTT, mqtt.state());
    }
}

void MQTTManager::disconnect() {
    if (isConnected()) {
        mqtt.disconnect();
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Getrennt");
    }
}

bool MQTTManager::isConnected() {
    return mqtt.connected();
}

void MQTTManager::publish(const String& topic, const String& payload) {
    if (!isConnected()) {
        return;
    }

    const uint32_t startUs = micros();
    bool ok = mqtt.publish(topic.c_str(), payload.c_str());
    const uint32_t durationUs = micros() - startUs;

    if (ok) {
        DebugManager::print(DebugCategory::MQTT, "MQTTManager: Published to ");
        DebugManager::print(DebugCategory::MQTT, topic);
        DebugManager::print(DebugCategory::MQTT, ": ");
        DebugManager::println(DebugCategory::MQTT, payload);
    } else {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Publish failed");
    }

    logSlowPublish("publish", topic, durationUs, ok);
}

void MQTTManager::publishState(bool power, const String& effect, uint32_t color, uint8_t brightness) {
    if (!isConnected()) {
        return;
    }
    
    DynamicJsonDocument doc(256);
    doc["state"] = power ? "ON" : "OFF";
    doc["brightness"] = brightness;
    doc["effect"] = effect;
    doc["color_mode"] = "rgb";
    
    JsonObject color_obj = doc.createNestedObject("color");
    color_obj["r"] = (color >> 16) & 0xFF;
    color_obj["g"] = (color >> 8) & 0xFF;
    color_obj["b"] = color & 0xFF;
    
    String payload;
    serializeJson(doc, payload);

    // retain=true: HA kennt den State auch nach Reconnect sofort
    const uint32_t startUs = micros();
    bool ok = mqtt.publish(state_topic.c_str(), payload.c_str(), true);
    const uint32_t durationUs = micros() - startUs;
    
    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Published to ");
    DebugManager::print(DebugCategory::MQTT, state_topic);
    DebugManager::print(DebugCategory::MQTT, ": ");
    DebugManager::println(DebugCategory::MQTT, payload);

    logSlowPublish("state", state_topic, durationUs, ok);
    if (!ok) {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: State publish failed");
    }
}

void MQTTManager::publishDiscovery() {
    if (!isConnected()) {
        return;
    }

    DynamicJsonDocument doc(2048);
    doc["name"]                  = "WordClock";
    doc["unique_id"]             = device_id;
    doc["schema"]                = "json";
    doc["command_topic"]         = command_topic;
    doc["state_topic"]           = state_topic;
    doc["availability_topic"]    = availability_topic;
    doc["payload_available"]     = "online";
    doc["payload_not_available"] = "offline";
    doc["brightness"]            = true;
    doc["brightness_scale"]      = 255;
    doc["optimistic"]            = false;
    doc["effect"]                = true;

    JsonArray color_modes = doc.createNestedArray("supported_color_modes");
    color_modes.add("rgb");

    JsonArray effect_list = doc.createNestedArray("effect_list");
    effect_list.add("clock");
    effect_list.add("wifi");
    effect_list.add("waterdrop");
    effect_list.add("love");
    effect_list.add("colorloop");
    effect_list.add("colorwipe");
    effect_list.add("fire2d");
    effect_list.add("matrix");
    effect_list.add("plasma");
    effect_list.add("inward");
    effect_list.add("twinkle");
    effect_list.add("balls");
    effect_list.add("aurora");
    effect_list.add("enchantment");
    effect_list.add("snake");

    JsonObject device = doc.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(device_id);
    device["name"]         = "WordClock";
    device["model"]        = "Seeed XIAO ESP32-C3";
    device["manufacturer"] = "Custom";

    String payload;
    serializeJson(doc, payload);

    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Discovery payload size: ");
    DebugManager::println(DebugCategory::MQTT, payload.length());

    // Discovery mit retain=true damit HA es nach Reconnect sofort kennt
    bool ok = mqtt.publish(discover_topic.c_str(), payload.c_str(), true);
    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Discovery publish ");
    DebugManager::println(DebugCategory::MQTT, ok ? "OK" : "FEHLGESCHLAGEN (Payload zu gross?)");

    // Availability online melden
    mqtt.publish(availability_topic.c_str(), "online", true);
    publishDiagnosticsDiscovery();
    publishTuningDiscovery();

    DebugManager::println(DebugCategory::MQTT, "MQTTManager: Discovery + Availability published");
}

void MQTTManager::publishDiagnosticsDiscovery() {
    if (!isConnected()) {
        return;
    }

    auto attachDevice = [this](JsonDocument& doc) {
        JsonObject dev = doc.createNestedObject("device");
        JsonArray ids = dev.createNestedArray("identifiers");
        ids.add(device_id);
        dev["name"] = "WordClock";
        dev["model"] = "Seeed XIAO ESP32-C3";
        dev["manufacturer"] = "Custom";
    };

    DynamicJsonDocument uptimeDoc(512);
    uptimeDoc["name"] = "WordClock Uptime";
    uptimeDoc["object_id"] = "wordclock_uptime";
    uptimeDoc["unique_id"] = device_id + "_uptime";
    uptimeDoc["state_topic"] = uptime_topic;
    uptimeDoc["availability_topic"] = availability_topic;
    uptimeDoc["payload_available"] = "online";
    uptimeDoc["payload_not_available"] = "offline";
    uptimeDoc["unit_of_measurement"] = "s";
    uptimeDoc["device_class"] = "duration";
    uptimeDoc["state_class"] = "measurement";
    uptimeDoc["entity_category"] = "diagnostic";
    uptimeDoc["icon"] = "mdi:timer-outline";
    attachDevice(uptimeDoc);
    String uptimeCfg;
    serializeJson(uptimeDoc, uptimeCfg);
    mqtt.publish("homeassistant/sensor/wordclock_uptime/config", uptimeCfg.c_str(), true);

    DynamicJsonDocument rssiDoc(512);
    rssiDoc["name"] = "WordClock RSSI";
    rssiDoc["object_id"] = "wordclock_rssi";
    rssiDoc["unique_id"] = device_id + "_rssi";
    rssiDoc["state_topic"] = rssi_topic;
    rssiDoc["availability_topic"] = availability_topic;
    rssiDoc["payload_available"] = "online";
    rssiDoc["payload_not_available"] = "offline";
    rssiDoc["unit_of_measurement"] = "dBm";
    rssiDoc["device_class"] = "signal_strength";
    rssiDoc["state_class"] = "measurement";
    rssiDoc["entity_category"] = "diagnostic";
    rssiDoc["icon"] = "mdi:wifi";
    attachDevice(rssiDoc);
    String rssiCfg;
    serializeJson(rssiDoc, rssiCfg);
    mqtt.publish("homeassistant/sensor/wordclock_rssi/config", rssiCfg.c_str(), true);

    DynamicJsonDocument ipDoc(512);
    ipDoc["name"] = "WordClock IP";
    ipDoc["object_id"] = "wordclock_ip";
    ipDoc["unique_id"] = device_id + "_ip";
    ipDoc["state_topic"] = ip_topic;
    ipDoc["availability_topic"] = availability_topic;
    ipDoc["payload_available"] = "online";
    ipDoc["payload_not_available"] = "offline";
    ipDoc["entity_category"] = "diagnostic";
    ipDoc["icon"] = "mdi:ip-network";
    attachDevice(ipDoc);
    String ipCfg;
    serializeJson(ipDoc, ipCfg);
    mqtt.publish("homeassistant/sensor/wordclock_ip/config", ipCfg.c_str(), true);

    DynamicJsonDocument versionDoc(512);
    versionDoc["name"] = "WordClock Version";
    versionDoc["object_id"] = "wordclock_version";
    versionDoc["unique_id"] = device_id + "_version";
    versionDoc["state_topic"] = version_topic;
    versionDoc["availability_topic"] = availability_topic;
    versionDoc["payload_available"] = "online";
    versionDoc["payload_not_available"] = "offline";
    versionDoc["entity_category"] = "diagnostic";
    versionDoc["icon"] = "mdi:tag-text-outline";
    attachDevice(versionDoc);
    String versionCfg;
    serializeJson(versionDoc, versionCfg);
    mqtt.publish("homeassistant/sensor/wordclock_version/config", versionCfg.c_str(), true);

    // --- Update: Check-Button ---
    DynamicJsonDocument otaCheckDoc(512);
    otaCheckDoc["name"] = "WordClock Update pr\u00fcfen";
    otaCheckDoc["object_id"] = "wordclock_update_check";
    otaCheckDoc["unique_id"] = device_id + "_ota_check";
    otaCheckDoc["command_topic"] = ota_check_command_topic;
    otaCheckDoc["payload_press"] = "CHECK";
    otaCheckDoc["entity_category"] = "config";
    otaCheckDoc["icon"] = "mdi:update";
    otaCheckDoc["availability_topic"] = availability_topic;
    attachDevice(otaCheckDoc);
    String otaCheckCfg;
    serializeJson(otaCheckDoc, otaCheckCfg);
    mqtt.publish("homeassistant/button/wordclock_ota_check/config", otaCheckCfg.c_str(), true);

    DynamicJsonDocument mqttStateDoc(512);
    mqttStateDoc["name"] = "WordClock MQTT State";
    mqttStateDoc["object_id"] = "wordclock_mqtt_state";
    mqttStateDoc["unique_id"] = device_id + "_mqtt_state";
    mqttStateDoc["state_topic"] = mqtt_state_topic;
    mqttStateDoc["availability_topic"] = availability_topic;
    mqttStateDoc["payload_available"] = "online";
    mqttStateDoc["payload_not_available"] = "offline";
    mqttStateDoc["entity_category"] = "diagnostic";
    mqttStateDoc["icon"] = "mdi:lan-connect";
    attachDevice(mqttStateDoc);
    String mqttStateCfg;
    serializeJson(mqttStateDoc, mqttStateCfg);
    mqtt.publish("homeassistant/sensor/wordclock_mqtt_state/config", mqttStateCfg.c_str(), true);

    DynamicJsonDocument rebootDoc(512);
    rebootDoc["name"] = "WordClock Reboot";
    rebootDoc["object_id"] = "wordclock_reboot";
    rebootDoc["unique_id"] = device_id + "_reboot";
    rebootDoc["command_topic"] = reboot_command_topic;
    rebootDoc["payload_press"] = "REBOOT";
    rebootDoc["entity_category"] = "config";
    rebootDoc["icon"] = "mdi:restart";
    attachDevice(rebootDoc);
    String rebootCfg;
    serializeJson(rebootDoc, rebootCfg);
    mqtt.publish("homeassistant/button/wordclock_reboot/config", rebootCfg.c_str(), true);

    // --- RTC Temperatur ---
    DynamicJsonDocument rtcTempDoc(512);
    rtcTempDoc["name"] = "WordClock RTC Temperatur";
    rtcTempDoc["object_id"] = "wordclock_rtc_temp";
    rtcTempDoc["unique_id"] = device_id + "_rtc_temp";
    rtcTempDoc["state_topic"] = rtc_temp_topic;
    rtcTempDoc["availability_topic"] = availability_topic;
    rtcTempDoc["payload_available"] = "online";
    rtcTempDoc["payload_not_available"] = "offline";
    rtcTempDoc["unit_of_measurement"] = "\u00b0C";
    rtcTempDoc["device_class"] = "temperature";
    rtcTempDoc["state_class"] = "measurement";
    rtcTempDoc["entity_category"] = "diagnostic";
    rtcTempDoc["icon"] = "mdi:thermometer";
    attachDevice(rtcTempDoc);
    String rtcTempCfg;
    serializeJson(rtcTempDoc, rtcTempCfg);
    mqtt.publish("homeassistant/sensor/wordclock_rtc_temp/config", rtcTempCfg.c_str(), true);

    // --- RTC Batterie-Warnung ---
    DynamicJsonDocument rtcBatDoc(512);
    rtcBatDoc["name"] = "WordClock RTC Batterie";
    rtcBatDoc["object_id"] = "wordclock_rtc_battery";
    rtcBatDoc["unique_id"] = device_id + "_rtc_battery";
    rtcBatDoc["state_topic"] = rtc_battery_warning_topic;
    rtcBatDoc["availability_topic"] = availability_topic;
    rtcBatDoc["payload_available"] = "online";
    rtcBatDoc["payload_not_available"] = "offline";
    rtcBatDoc["device_class"] = "problem";
    rtcBatDoc["payload_on"] = "1";
    rtcBatDoc["payload_off"] = "0";
    rtcBatDoc["entity_category"] = "diagnostic";
    rtcBatDoc["icon"] = "mdi:battery-alert";
    attachDevice(rtcBatDoc);
    String rtcBatCfg;
    serializeJson(rtcBatDoc, rtcBatCfg);
    mqtt.publish("homeassistant/binary_sensor/wordclock_rtc_battery/config", rtcBatCfg.c_str(), true);

    DebugManager::println(DebugCategory::MQTT, "MQTTManager: Informationen discovery published");
}

void MQTTManager::publishTuningDiscovery() {
    if (!isConnected()) return;

    auto attachDevice = [this](JsonDocument& doc) {
        JsonObject dev = doc.createNestedObject("device");
        JsonArray ids = dev.createNestedArray("identifiers");
        ids.add(device_id);
        dev["name"] = "WordClock";
        dev["model"] = "Seeed XIAO ESP32-C3";
        dev["manufacturer"] = "Custom";
    };

    // --- Speed ---
    DynamicJsonDocument speedDoc(512);
    speedDoc["name"] = "WordClock Geschwindigkeit";
    speedDoc["object_id"] = "wordclock_geschwindigkeit";
    speedDoc["unique_id"] = device_id + "_speed";
    speedDoc["command_topic"] = speed_command_topic;
    speedDoc["state_topic"] = speed_state_topic;
    speedDoc["availability_topic"] = availability_topic;
    speedDoc["min"] = ControlConfig::SPEED_MIN;
    speedDoc["max"] = ControlConfig::SPEED_MAX;
    speedDoc["step"] = 1;
    speedDoc["unit_of_measurement"] = "%";
    speedDoc["icon"] = "mdi:speedometer";
    speedDoc["retain"] = true;
    attachDevice(speedDoc);
    String speedCfg;
    serializeJson(speedDoc, speedCfg);
    mqtt.publish("homeassistant/number/wordclock_speed/config", speedCfg.c_str(), true);

    // --- Intensity ---
    DynamicJsonDocument intDoc(512);
    intDoc["name"] = "WordClock Intensit\u00e4t";
    intDoc["object_id"] = "wordclock_intensitaet";
    intDoc["unique_id"] = device_id + "_intensity";
    intDoc["command_topic"] = intensity_command_topic;
    intDoc["state_topic"] = intensity_state_topic;
    intDoc["availability_topic"] = availability_topic;
    intDoc["min"] = ControlConfig::INTENSITY_MIN;
    intDoc["max"] = ControlConfig::INTENSITY_MAX;
    intDoc["step"] = 1;
    intDoc["unit_of_measurement"] = "%";
    intDoc["icon"] = "mdi:brightness-6";
    intDoc["retain"] = true;
    attachDevice(intDoc);
    String intCfg;
    serializeJson(intDoc, intCfg);
    mqtt.publish("homeassistant/number/wordclock_intensity/config", intCfg.c_str(), true);

    // --- Transition ---
    DynamicJsonDocument transDoc(512);
    transDoc["name"] = "WordClock \u00dcbergang";
    transDoc["object_id"] = "wordclock_uebergang";
    transDoc["unique_id"] = device_id + "_transition";
    transDoc["command_topic"] = transition_command_topic;
    transDoc["state_topic"] = transition_state_topic;
    transDoc["availability_topic"] = availability_topic;
    transDoc["min"] = ControlConfig::TRANSITION_MIN_MS;
    transDoc["max"] = ControlConfig::TRANSITION_MAX_MS;
    transDoc["step"] = 100;
    transDoc["unit_of_measurement"] = "ms";
    transDoc["icon"] = "mdi:transition";
    transDoc["retain"] = true;
    attachDevice(transDoc);
    String transCfg;
    serializeJson(transDoc, transCfg);
    mqtt.publish("homeassistant/number/wordclock_transition/config", transCfg.c_str(), true);

    // --- Tuning Reset Button ---
    DynamicJsonDocument resetDoc(512);
    resetDoc["name"] = "WordClock Default";
    resetDoc["object_id"] = "wordclock_default";
    resetDoc["unique_id"] = device_id + "_tuning_reset";
    resetDoc["command_topic"] = tuning_reset_command_topic;
    resetDoc["payload_press"] = "RESET";
    resetDoc["entity_category"] = "config";
    resetDoc["icon"] = "mdi:restore";
    resetDoc["availability_topic"] = availability_topic;
    attachDevice(resetDoc);
    String resetCfg;
    serializeJson(resetDoc, resetCfg);
    mqtt.publish("homeassistant/button/wordclock_tuning_reset/config", resetCfg.c_str(), true);

    // Entfernte HA-Entities aktiv aus Discovery loeschen (retained empty payload).
    mqtt.publish("homeassistant/text/wordclock_service/config", "", true);
    mqtt.publish("homeassistant/select/wordclock_palette/config", "", true);
    mqtt.publish("homeassistant/number/wordclock_hueshift/config", "", true);

    DebugManager::println(DebugCategory::MQTT, "MQTTManager: Tuning discovery published");
}

void MQTTManager::publishTelemetry() {
    if (!isConnected()) {
        return;
    }

    String uptime = String(millis() / 1000UL);
    String rssi = String(WiFi.RSSI());
    String ip = WiFi.isConnected() ? WiFi.localIP().toString() : "offline";
    const char* fwVersion = getFirmwareVersion();

    mqtt.publish(availability_topic.c_str(), "online", true);
    mqtt.publish(mqtt_state_topic.c_str(), "connected", true);
    mqtt.publish(uptime_topic.c_str(), uptime.c_str(), true);
    mqtt.publish(rssi_topic.c_str(), rssi.c_str(), true);
    mqtt.publish(ip_topic.c_str(), ip.c_str(), true);
    mqtt.publish(version_topic.c_str(), fwVersion, true);
    // Publish current tuning values so HA sliders reflect device state
    mqtt.publish(speed_state_topic.c_str(), String(effectSpeed).c_str(), true);
    mqtt.publish(intensity_state_topic.c_str(), String(effectIntensity).c_str(), true);
    mqtt.publish(transition_state_topic.c_str(), String(transitionMs).c_str(), true);

    // RTC health telemetry
    extern RTCManager rtcManager;
    float rtcTemp = rtcManager.getTemperatureC();
    if (!isnan(rtcTemp)) {
        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%.2f", rtcTemp);
        mqtt.publish(rtc_temp_topic.c_str(), tempBuf, true);
    }
    mqtt.publish(rtc_battery_warning_topic.c_str(),
                 rtcManager.hasBatteryWarning() ? "1" : "0", true);
}

void MQTTManager::loop() {
    if (!mqtt.connected()) {
        unsigned long now = millis();
        if (now - last_reconnect_attempt > RECONNECT_INTERVAL) {
            last_reconnect_attempt = now;
            connect();
        }
    } else {
        const uint32_t loopStartUs = micros();
        mqtt.loop();
        const uint32_t loopDurationUs = micros() - loopStartUs;
        if (loopDurationUs >= LOOP_WARN_US) {
            DebugManager::printf(DebugCategory::MQTT, "[MQTT][WARN] mqtt.loop took %luus\n", (unsigned long)loopDurationUs);
        }

        unsigned long now = millis();
        if (now - last_telemetry_publish > TELEMETRY_INTERVAL) {
            last_telemetry_publish = now;
            publishTelemetry();
        }
    }
}

void MQTTManager::internalCallback(const String& topic, const String& payload) {
    const uint32_t callbackStartUs = micros();
    auto logCallbackDuration = [&](const char* action) {
        const uint32_t durationUs = micros() - callbackStartUs;
        if (durationUs >= CALLBACK_WARN_US) {
            DebugManager::printf(DebugCategory::MQTT,
                                 "[MQTT][WARN] callback action=%s topic=%s took %luus\n",
                                 action,
                                 topic.c_str(),
                                 (unsigned long)durationUs);
        }
    };

    StateManager& stateManager = StateManager::getInstance();
    auto resetTuningToDefaults = [&]() {
        effectSpeed     = ControlConfig::DEFAULT_SPEED;
        effectIntensity = ControlConfig::DEFAULT_INTENSITY;
        transitionMs    = ControlConfig::DEFAULT_TRANSITION_MS;
        effectPalette   = ControlConfig::DEFAULT_PALETTE;
        stateManager.setSpeed(effectSpeed);
        stateManager.setIntensity(effectIntensity);
        stateManager.setTransitionMs(transitionMs);
        stateManager.setPalette(effectPalette);
        stateManager.scheduleSave();
        mqtt.publish(speed_state_topic.c_str(),      String(effectSpeed).c_str(),     true);
        mqtt.publish(intensity_state_topic.c_str(),  String(effectIntensity).c_str(), true);
        mqtt.publish(transition_state_topic.c_str(), String(transitionMs).c_str(),   true);
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Tuning reset to defaults");
    };

    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Message on ");
    DebugManager::print(DebugCategory::MQTT, topic);
    DebugManager::print(DebugCategory::MQTT, ": ");
    DebugManager::println(DebugCategory::MQTT, payload);

    if (topic == reboot_command_topic && payload == "REBOOT") {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Reboot command received");
        mqtt.publish(mqtt_state_topic.c_str(), "rebooting", true);
        mqtt.publish(availability_topic.c_str(), "offline", true);
        logCallbackDuration("reboot");
        rebootDevice("MQTT reboot command", 150);
        return;
    }

    if (topic == speed_command_topic) {
        int val = payload.toInt();
        if (val >= ControlConfig::SPEED_MIN && val <= ControlConfig::SPEED_MAX) {
            effectSpeed = (uint8_t)val;
            stateManager.setSpeed(effectSpeed);
            stateManager.scheduleSave();
            mqtt.publish(speed_state_topic.c_str(), String(effectSpeed).c_str(), true);
            DebugManager::printf(DebugCategory::MQTT, "MQTTManager: Speed set to %d\n", effectSpeed);
        }
        logCallbackDuration("speed");
        return;
    }

    if (topic == intensity_command_topic) {
        int val = payload.toInt();
        if (val >= ControlConfig::INTENSITY_MIN && val <= ControlConfig::INTENSITY_MAX) {
            effectIntensity = (uint8_t)val;
            stateManager.setIntensity(effectIntensity);
            stateManager.scheduleSave();
            mqtt.publish(intensity_state_topic.c_str(), String(effectIntensity).c_str(), true);
            DebugManager::printf(DebugCategory::MQTT, "MQTTManager: Intensity set to %d\n", effectIntensity);
        }
        logCallbackDuration("intensity");
        return;
    }

    if (topic == transition_command_topic) {
        int val = payload.toInt();
        if (val >= ControlConfig::TRANSITION_MIN_MS && val <= ControlConfig::TRANSITION_MAX_MS) {
            transitionMs = (uint16_t)val;
            stateManager.setTransitionMs(transitionMs);
            stateManager.scheduleSave();
            mqtt.publish(transition_state_topic.c_str(), String(transitionMs).c_str(), true);
            DebugManager::printf(DebugCategory::MQTT, "MQTTManager: TransitionMs set to %d\n", transitionMs);
        }
        logCallbackDuration("transition");
        return;
    }

    if (topic == tuning_reset_command_topic && payload == "RESET") {
        resetTuningToDefaults();
        logCallbackDuration("tuning_reset");
        return;
    }

    if (topic == ota_check_command_topic && payload == "CHECK") {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: OTA check via MQTT ausgeloest");
        checkForUpdateAndInstall(true);
        logCallbackDuration("ota_check");
        return;
    }

    if (on_message) {
        on_message(topic, payload);
    }
    logCallbackDuration("passthrough");
}

void MQTTManager::logSlowPublish(const char* label, const String& topic, uint32_t durationUs, bool ok) {
    if (durationUs < PUBLISH_WARN_US) {
        return;
    }

    DebugManager::printf(DebugCategory::MQTT, "[MQTT][WARN] %s topic=%s took %luus (%s)\n",
                         label,
                         topic.c_str(),
                         (unsigned long)durationUs,
                         ok ? "ok" : "failed");
}
