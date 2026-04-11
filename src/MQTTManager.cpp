#include "MQTTManager.h"
#include <WiFi.h>
#include "ControlConfig.h"
#include "DebugManager.h"
#include "StateManager.h"
#include "RTCManager.h"
#include "SystemControl.h"
#include "ota_https_update.h"
#include <Preferences.h>

// Effect tuning parameters defined in main.cpp
extern uint8_t  effectSpeed;
extern uint8_t  effectIntensity;
extern uint8_t  effectDensity;
extern uint16_t transitionMs;
extern uint8_t  effectPalette;

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

static String buildConnectClientId(const String& baseId) {
    // baseId is already made unique via MAC suffix during initialization.
    return baseId;
}

static String getMacToken() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

    if (mac.length() == 12) {
        return mac;
    }
    return "000000000000";
}

static String getMacSuffix6() {
    String mac = getMacToken();
    return mac.substring(mac.length() - 6);
}

static const char* kNameRequestTopic = "wordclock/name/request";

static bool equalsIgnoreCaseTrimmed(String left, String right) {
    left.trim();
    right.trim();
    left.toLowerCase();
    right.toLowerCase();
    return left == right;
}

static String buildDeviceDisplayName(const String& alias) {
    String trimmed = alias;
    trimmed.trim();
    if (trimmed.isEmpty()) {
        return "Wordclock";
    }
    return String("Wordclock ") + trimmed;
}

static String buildNameReplyTopic(const String& deviceId) {
    return String("wordclock/name/reply/") + deviceId;
}

static bool loadStoredDeviceAlias(String& alias) {
    static const char* kNames[] = {
        "Knut", "Rika", "Bernd", "Maike", "Günter", "Leyla", "Harald", "Maja",
        "Edmund", "Mira", "Rüdiger", "Ria", "Horst", "Tilda", "Konrad", "Hanne",
        "Malte", "Frieda", "Jannis", "Irmgard", "Sören", "Jassi", "Armin", "Nele",
        "Torben", "Thea", "Heiko", "Celina", "Henning", "Zoe", "Lennart", "Mara",
        "Jasper", "Gina", "Frank", "Hilda", "Werner", "Lea", "Taron", "Lotte",
        "Fredi", "Tia", "Georg", "Linda", "Kai", "Claudia", "Sebi", "Lena",
        "Peter", "Viola", "Lukas", "Anna", "Basti", "Sabrina", "Nick", "Maren",
        "Dieter", "Sabine", "Ulrich", "Beate"
    };
    static const size_t kNameCount = sizeof(kNames) / sizeof(kNames[0]);

    Preferences prefs;
    if (!prefs.begin("device", true)) {
        alias = "";
        return false;
    }

    alias = prefs.getString("clock_name", "");
    prefs.end();
    alias.trim();
    (void)kNameCount;
    return !alias.isEmpty();
}

static bool saveStoredDeviceAlias(const String& alias) {
    Preferences prefs;
    if (!prefs.begin("device", false)) {
        return false;
    }
    prefs.putString("clock_name", alias);
    prefs.end();
    return true;
}

static String pickRandomDeviceAlias(const String& excludedAlias = "") {
    static const char* kNames[] = {
        "Knut", "Rika", "Bernd", "Maike", "Günter", "Leyla", "Harald", "Maja",
        "Edmund", "Mira", "Rüdiger", "Ria", "Horst", "Tilda", "Konrad", "Hanne",
        "Malte", "Frieda", "Jannis", "Irmgard", "Sören", "Jassi", "Armin", "Nele",
        "Torben", "Thea", "Heiko", "Celina", "Henning", "Zoe", "Lennart", "Mara",
        "Jasper", "Gina", "Frank", "Hilda", "Werner", "Lea", "Taron", "Lotte",
        "Fredi", "Tia", "Georg", "Linda", "Kai", "Claudia", "Sebi", "Lena",
        "Peter", "Viola", "Lukas", "Anna", "Basti", "Sabrina", "Nick", "Maren",
        "Dieter", "Sabine", "Ulrich", "Beate"
    };
    static const size_t kNameCount = sizeof(kNames) / sizeof(kNames[0]);

    const size_t start = (size_t)(esp_random() % kNameCount);
    for (size_t offset = 0; offset < kNameCount; offset++) {
        const String candidate = String(kNames[(start + offset) % kNameCount]);
        if (!excludedAlias.isEmpty() && equalsIgnoreCaseTrimmed(candidate, excludedAlias)) {
            continue;
        }
        return candidate;
    }
    return String(kNames[start]);
}

static String buildDiscoveryConfigTopic(const char* component, const String& objectId) {
    return String("homeassistant/") + component + "/" + objectId + "/config";
}

static String buildDiscoveryConfigTopic(const char* component, const String& nodeId, const String& objectId) {
    return String("homeassistant/") + component + "/" + nodeId + "/" + objectId + "/config";
}

static const char* mqttStateText(int state) {
    switch (state) {
        case MQTT_CONNECTION_TIMEOUT: return "MQTT_CONNECTION_TIMEOUT";
        case MQTT_CONNECTION_LOST: return "MQTT_CONNECTION_LOST";
        case MQTT_CONNECT_FAILED: return "MQTT_CONNECT_FAILED";
        case MQTT_DISCONNECTED: return "MQTT_DISCONNECTED";
        case MQTT_CONNECTED: return "MQTT_CONNECTED";
        case MQTT_CONNECT_BAD_PROTOCOL: return "MQTT_CONNECT_BAD_PROTOCOL";
        case MQTT_CONNECT_BAD_CLIENT_ID: return "MQTT_CONNECT_BAD_CLIENT_ID";
        case MQTT_CONNECT_UNAVAILABLE: return "MQTT_CONNECT_UNAVAILABLE";
        case MQTT_CONNECT_BAD_CREDENTIALS: return "MQTT_CONNECT_BAD_CREDENTIALS";
        case MQTT_CONNECT_UNAUTHORIZED: return "MQTT_CONNECT_UNAUTHORIZED";
        default: return "MQTT_STATE_UNKNOWN";
    }
}

MQTTManager::MQTTManager(WiFiClient& wifi_client, const String& device_id)
    : mqtt(wifi_client), wifi_client(wifi_client), device_id(device_id),
    device_name(""), device_alias(""), server(""), port(1883), user(""), password(""),
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
            density_command_topic("wordclock/density/set"),
            density_state_topic("wordclock/density/state"),
            transition_command_topic("wordclock/transition/set"),
            transition_state_topic("wordclock/transition/state"),
            tuning_reset_command_topic("wordclock/tuning_reset/set"),
            rtc_temp_topic("wordclock/rtc/temperature"),
            rtc_battery_warning_topic("wordclock/rtc/battery_warning"),
            ota_check_command_topic("wordclock/ota_check/set"),
            name_request_topic(kNameRequestTopic),
            name_reply_topic(""),
            last_reconnect_attempt(0),
            reconnect_interval_ms(RECONNECT_INTERVAL),
            last_telemetry_publish(0),
            last_connect_ms(0),
            reconnect_failures(0),
            has_persisted_device_name(false),
            collecting_name_replies(false),
            name_reply_count(0) {
            const String macSuffix = getMacSuffix6();
            const String stableId = String("wordclock_") + macSuffix;
            String storedAlias;
            const bool hasStoredAlias = loadStoredDeviceAlias(storedAlias);

            this->device_id = stableId;
            this->name_reply_topic = buildNameReplyTopic(stableId);
            this->has_persisted_device_name = hasStoredAlias;
            setDeviceAlias(hasStoredAlias ? storedAlias : pickRandomDeviceAlias());

            const String baseTopic = stableId;
            command_topic = baseTopic + "/set";
            state_topic = baseTopic + "/state";
            discover_topic = buildDiscoveryConfigTopic("light", stableId);
            availability_topic = baseTopic + "/availability";
            uptime_topic = baseTopic + "/uptime";
            rssi_topic = baseTopic + "/rssi";
            ip_topic = baseTopic + "/ip";
            mqtt_state_topic = baseTopic + "/mqtt_state";
            version_topic = baseTopic + "/version";
            reboot_command_topic = baseTopic + "/reboot/set";
            speed_command_topic = baseTopic + "/speed/set";
            speed_state_topic = baseTopic + "/speed/state";
            intensity_command_topic = baseTopic + "/intensity/set";
            intensity_state_topic = baseTopic + "/intensity/state";
            density_command_topic = baseTopic + "/density/set";
            density_state_topic = baseTopic + "/density/state";
            transition_command_topic = baseTopic + "/transition/set";
            transition_state_topic = baseTopic + "/transition/state";
            tuning_reset_command_topic = baseTopic + "/tuning_reset/set";
            rtc_temp_topic = baseTopic + "/rtc/temperature";
            rtc_battery_warning_topic = baseTopic + "/rtc/battery_warning";
            ota_check_command_topic = baseTopic + "/ota_check/set";

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

    DebugManager::printf(DebugCategory::MQTT,
                         "MQTTManager: WiFi=%s ip=%s rssi=%d\n",
                         WiFi.isConnected() ? "connected" : "offline",
                         WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "offline",
                         WiFi.isConnected() ? WiFi.RSSI() : -127);

    const String connectClientId = buildConnectClientId(device_id);
    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Client-ID ");
    DebugManager::println(DebugCategory::MQTT, connectClientId);
    DebugManager::printf(DebugCategory::MQTT,
                         "MQTTManager: User=%s PassLen=%u WillTopic=%s\n",
                         user.c_str(),
                         (unsigned int)password.length(),
                         availability_topic.c_str());
    
    bool connected = mqtt.connect(connectClientId.c_str(), user.c_str(), password.c_str(),
                                  availability_topic.c_str(), 0, true, "offline");
    int state = mqtt.state();
    DebugManager::printf(DebugCategory::MQTT,
                         "MQTTManager: Attempt #1 (with LWT) result=%s (%d)\n",
                         mqttStateText(state), state);
    if (!connected) {
        DebugManager::println(DebugCategory::MQTT,
                              "MQTTManager: Connect mit Last-Will fehlgeschlagen, retry ohne Last-Will");
        connected = mqtt.connect(connectClientId.c_str(), user.c_str(), password.c_str());
        state = mqtt.state();
        DebugManager::printf(DebugCategory::MQTT,
                             "MQTTManager: Attempt #2 (without LWT) result=%s (%d)\n",
                             mqttStateText(state), state);
    }

    if (connected) {
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Verbunden!");
        last_connect_ms = millis();
        reconnect_failures = 0;
        reconnect_interval_ms = RECONNECT_INTERVAL;

        mqtt.subscribe(name_request_topic.c_str());
        mqtt.subscribe(name_reply_topic.c_str());
        resolveDeviceName(false);

        // Publish device truth before accepting commands so HA resyncs to the clock's persisted state.
        publishDiscovery();
        publishState(StateManager::getInstance().getPowerState(),
                     StateManager::getInstance().getCurrentEffect(),
                     StateManager::getInstance().getColor(),
                     StateManager::getInstance().getBrightness());
        publishTelemetry();

        mqtt.subscribe(command_topic.c_str());
        mqtt.subscribe(reboot_command_topic.c_str());
        mqtt.subscribe(speed_command_topic.c_str());
        mqtt.subscribe(intensity_command_topic.c_str());
        mqtt.subscribe(density_command_topic.c_str());
        mqtt.subscribe(transition_command_topic.c_str());
        mqtt.subscribe(tuning_reset_command_topic.c_str());
        mqtt.subscribe(ota_check_command_topic.c_str());
    } else {
        reconnect_failures++;
        const int state = mqtt.state();
        if (state == MQTT_CONNECT_UNAUTHORIZED || state == MQTT_CONNECT_BAD_CREDENTIALS) {
            reconnect_interval_ms = 60000UL;
        } else {
            uint8_t shift = reconnect_failures > 3 ? 3 : reconnect_failures;
            unsigned long candidate = RECONNECT_INTERVAL << shift;
            reconnect_interval_ms = candidate > 60000UL ? 60000UL : candidate;
        }
        DebugManager::print(DebugCategory::MQTT, "MQTTManager: Verbindung fehlgeschlagen, Fehler: ");
        DebugManager::println(DebugCategory::MQTT, state);
        DebugManager::printf(DebugCategory::MQTT,
                             "MQTTManager: Fehlertext=%s\n",
                             mqttStateText(state));
        DebugManager::printf(DebugCategory::MQTT,
                             "MQTTManager: Reconnect backoff %lums (fails=%u)\n",
                             reconnect_interval_ms,
                             reconnect_failures);
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

void MQTTManager::publishTuningState() {
    if (!isConnected()) {
        return;
    }

    mqtt.publish(speed_state_topic.c_str(), String(effectSpeed).c_str(), true);
    mqtt.publish(intensity_state_topic.c_str(), String(effectIntensity).c_str(), true);
    mqtt.publish(density_state_topic.c_str(), String(effectDensity).c_str(), true);
    mqtt.publish(transition_state_topic.c_str(), String(transitionMs).c_str(), true);
}

void MQTTManager::publishDiscovery() {
    if (!isConnected()) {
        return;
    }

    DynamicJsonDocument doc(2048);
    doc["name"]                  = device_name;
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
    effect_list.add("waterdrop_r");
    effect_list.add("twinkle");
    effect_list.add("balls");
    effect_list.add("aurora");
    effect_list.add("enchantment");
    effect_list.add("snake");

    JsonObject device = doc.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(device_id);
    device["name"]         = device_name;
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
        dev["name"] = device_name;
        dev["model"] = "Seeed XIAO ESP32-C3";
        dev["manufacturer"] = "Custom";
    };

    auto publishEntityDiscovery = [this](const char* component,
                                         const String& objectId,
                                         const String& legacyObjectId,
                                         const String& payload) {
        mqtt.publish(buildDiscoveryConfigTopic(component, device_id, objectId).c_str(), payload.c_str(), true);
        mqtt.publish(buildDiscoveryConfigTopic(component, device_id, legacyObjectId).c_str(), "", true);
        mqtt.publish(buildDiscoveryConfigTopic(component, legacyObjectId).c_str(), "", true);
    };

    DynamicJsonDocument uptimeDoc(512);
    const String uptimeObjectId = "uptime";
    const String uptimeLegacyObjectId = device_id + "_uptime";
    uptimeDoc["name"] = device_name + " Uptime";
    uptimeDoc["object_id"] = uptimeObjectId;
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
    publishEntityDiscovery("sensor", uptimeObjectId, uptimeLegacyObjectId, uptimeCfg);

    DynamicJsonDocument rssiDoc(512);
    const String rssiObjectId = "rssi";
    const String rssiLegacyObjectId = device_id + "_rssi";
    rssiDoc["name"] = device_name + " RSSI";
    rssiDoc["object_id"] = rssiObjectId;
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
    publishEntityDiscovery("sensor", rssiObjectId, rssiLegacyObjectId, rssiCfg);

    DynamicJsonDocument ipDoc(512);
    const String ipObjectId = "ip";
    const String ipLegacyObjectId = device_id + "_ip";
    ipDoc["name"] = device_name + " IP";
    ipDoc["object_id"] = ipObjectId;
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
    publishEntityDiscovery("sensor", ipObjectId, ipLegacyObjectId, ipCfg);

    DynamicJsonDocument versionDoc(512);
    const String versionObjectId = "version";
    const String versionLegacyObjectId = device_id + "_version";
    versionDoc["name"] = device_name + " Version";
    versionDoc["object_id"] = versionObjectId;
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
    publishEntityDiscovery("sensor", versionObjectId, versionLegacyObjectId, versionCfg);

    // --- Update: Check-Button ---
    DynamicJsonDocument otaCheckDoc(512);
    const String otaCheckObjectId = "update_check";
    const String otaCheckLegacyObjectId = device_id + "_update_check";
    otaCheckDoc["name"] = device_name + " Update pr\u00fcfen";
    otaCheckDoc["object_id"] = otaCheckObjectId;
    otaCheckDoc["unique_id"] = device_id + "_ota_check";
    otaCheckDoc["command_topic"] = ota_check_command_topic;
    otaCheckDoc["payload_press"] = "CHECK";
    otaCheckDoc["entity_category"] = "config";
    otaCheckDoc["icon"] = "mdi:update";
    otaCheckDoc["availability_topic"] = availability_topic;
    attachDevice(otaCheckDoc);
    String otaCheckCfg;
    serializeJson(otaCheckDoc, otaCheckCfg);
    publishEntityDiscovery("button", otaCheckObjectId, otaCheckLegacyObjectId, otaCheckCfg);

    DynamicJsonDocument mqttStateDoc(512);
    const String mqttStateObjectId = "mqtt_state";
    const String mqttStateLegacyObjectId = device_id + "_mqtt_state";
    mqttStateDoc["name"] = device_name + " MQTT State";
    mqttStateDoc["object_id"] = mqttStateObjectId;
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
    publishEntityDiscovery("sensor", mqttStateObjectId, mqttStateLegacyObjectId, mqttStateCfg);

    DynamicJsonDocument rebootDoc(512);
    const String rebootObjectId = "reboot";
    const String rebootLegacyObjectId = device_id + "_reboot";
    rebootDoc["name"] = device_name + " Reboot";
    rebootDoc["object_id"] = rebootObjectId;
    rebootDoc["unique_id"] = device_id + "_reboot";
    rebootDoc["command_topic"] = reboot_command_topic;
    rebootDoc["payload_press"] = "REBOOT";
    rebootDoc["entity_category"] = "config";
    rebootDoc["icon"] = "mdi:restart";
    attachDevice(rebootDoc);
    String rebootCfg;
    serializeJson(rebootDoc, rebootCfg);
    publishEntityDiscovery("button", rebootObjectId, rebootLegacyObjectId, rebootCfg);

    // --- RTC Temperatur ---
    DynamicJsonDocument rtcTempDoc(512);
    const String rtcTempObjectId = "rtc_temp";
    const String rtcTempLegacyObjectId = device_id + "_rtc_temp";
    rtcTempDoc["name"] = device_name + " RTC Temperatur";
    rtcTempDoc["object_id"] = rtcTempObjectId;
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
    publishEntityDiscovery("sensor", rtcTempObjectId, rtcTempLegacyObjectId, rtcTempCfg);

    // --- RTC Batterie-Warnung ---
    DynamicJsonDocument rtcBatDoc(512);
    const String rtcBatObjectId = "rtc_battery";
    const String rtcBatLegacyObjectId = device_id + "_rtc_battery";
    rtcBatDoc["name"] = device_name + " RTC Batterie";
    rtcBatDoc["object_id"] = rtcBatObjectId;
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
    publishEntityDiscovery("binary_sensor", rtcBatObjectId, rtcBatLegacyObjectId, rtcBatCfg);

    DebugManager::println(DebugCategory::MQTT, "MQTTManager: Informationen discovery published");
}

void MQTTManager::publishTuningDiscovery() {
    if (!isConnected()) return;

    auto attachDevice = [this](JsonDocument& doc) {
        JsonObject dev = doc.createNestedObject("device");
        JsonArray ids = dev.createNestedArray("identifiers");
        ids.add(device_id);
        dev["name"] = device_name;
        dev["model"] = "Seeed XIAO ESP32-C3";
        dev["manufacturer"] = "Custom";
    };

    auto publishEntityDiscovery = [this](const char* component,
                                         const String& objectId,
                                         const String& legacyObjectId,
                                         const String& payload) {
        mqtt.publish(buildDiscoveryConfigTopic(component, device_id, objectId).c_str(), payload.c_str(), true);
        mqtt.publish(buildDiscoveryConfigTopic(component, device_id, legacyObjectId).c_str(), "", true);
        mqtt.publish(buildDiscoveryConfigTopic(component, legacyObjectId).c_str(), "", true);
    };

    // --- Speed ---
    DynamicJsonDocument speedDoc(512);
    const String speedObjectId = "geschwindigkeit";
    const String speedLegacyObjectId = device_id + "_geschwindigkeit";
    speedDoc["name"] = device_name + " Geschwindigkeit";
    speedDoc["object_id"] = speedObjectId;
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
    publishEntityDiscovery("number", speedObjectId, speedLegacyObjectId, speedCfg);

    // --- Intensity ---
    DynamicJsonDocument intDoc(512);
    const String intensityObjectId = "intensitaet";
    const String intensityLegacyObjectId = device_id + "_intensitaet";
    intDoc["name"] = device_name + " Intensit\u00e4t";
    intDoc["object_id"] = intensityObjectId;
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
    publishEntityDiscovery("number", intensityObjectId, intensityLegacyObjectId, intCfg);

    // --- Density ---
    DynamicJsonDocument denDoc(512);
    const String densityObjectId = "dichte";
    const String densityLegacyObjectId = device_id + "_dichte";
    denDoc["name"] = device_name + " Objekt-Dichte";
    denDoc["object_id"] = densityObjectId;
    denDoc["unique_id"] = device_id + "_density";
    denDoc["command_topic"] = density_command_topic;
    denDoc["state_topic"] = density_state_topic;
    denDoc["availability_topic"] = availability_topic;
    denDoc["min"] = ControlConfig::DENSITY_MIN;
    denDoc["max"] = ControlConfig::DENSITY_MAX;
    denDoc["step"] = 1;
    denDoc["unit_of_measurement"] = "%";
    denDoc["icon"] = "mdi:texture-box";
    denDoc["retain"] = true;
    attachDevice(denDoc);
    String denCfg;
    serializeJson(denDoc, denCfg);
    publishEntityDiscovery("number", densityObjectId, densityLegacyObjectId, denCfg);

    // --- Transition ---
    DynamicJsonDocument transDoc(512);
    const String transitionObjectId = "uebergang";
    const String transitionLegacyObjectId = device_id + "_uebergang";
    transDoc["name"] = device_name + " \u00dcbergang";
    transDoc["object_id"] = transitionObjectId;
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
    publishEntityDiscovery("number", transitionObjectId, transitionLegacyObjectId, transCfg);

    // --- Tuning Reset Button ---
    DynamicJsonDocument resetDoc(512);
    const String resetObjectId = "default";
    const String resetLegacyObjectId = device_id + "_default";
    resetDoc["name"] = device_name + " Default";
    resetDoc["object_id"] = resetObjectId;
    resetDoc["unique_id"] = device_id + "_tuning_reset";
    resetDoc["command_topic"] = tuning_reset_command_topic;
    resetDoc["payload_press"] = "RESET";
    resetDoc["entity_category"] = "config";
    resetDoc["icon"] = "mdi:restore";
    resetDoc["availability_topic"] = availability_topic;
    attachDevice(resetDoc);
    String resetCfg;
    serializeJson(resetDoc, resetCfg);
    publishEntityDiscovery("button", resetObjectId, resetLegacyObjectId, resetCfg);

    // Entfernte HA-Entities aktiv aus Discovery loeschen (retained empty payload).
    mqtt.publish(buildDiscoveryConfigTopic("text", "wordclock_service").c_str(), "", true);
    mqtt.publish(buildDiscoveryConfigTopic("select", "wordclock_palette").c_str(), "", true);
    mqtt.publish(buildDiscoveryConfigTopic("number", "wordclock_hueshift").c_str(), "", true);

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
    mqtt.publish(density_state_topic.c_str(), String(effectDensity).c_str(), true);
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

bool MQTTManager::randomizeDeviceName() {
    if (!isConnected()) {
        return false;
    }

    const String previousName = device_name;
    if (!resolveDeviceName(true)) {
        return false;
    }

    if (device_name != previousName) {
        publishDiscovery();
        publishState(StateManager::getInstance().getPowerState(),
                     StateManager::getInstance().getCurrentEffect(),
                     StateManager::getInstance().getColor(),
                     StateManager::getInstance().getBrightness());
        publishTelemetry();
    }
    return true;
}

void MQTTManager::setDeviceAlias(const String& alias) {
    String normalized = alias;
    normalized.trim();
    if (normalized.isEmpty()) {
        normalized = pickRandomDeviceAlias();
    }
    device_alias = normalized;
    device_name = buildDeviceDisplayName(normalized);
}

bool MQTTManager::persistDeviceAlias(const String& alias) {
    setDeviceAlias(alias);
    const bool ok = saveStoredDeviceAlias(device_alias);
    if (ok) {
        has_persisted_device_name = true;
    }
    return ok;
}

void MQTTManager::addPeerDeviceName(const String& alias) {
    String normalized = alias;
    normalized.trim();
    if (normalized.isEmpty()) {
        return;
    }

    for (size_t i = 0; i < name_reply_count; i++) {
        if (equalsIgnoreCaseTrimmed(name_reply_buffer[i], normalized)) {
            return;
        }
    }

    if (name_reply_count < MAX_NAME_REPLIES) {
        name_reply_buffer[name_reply_count++] = normalized;
    }
}

bool MQTTManager::collectPeerDeviceNames(unsigned long windowMs) {
    if (!isConnected()) {
        return false;
    }

    name_reply_count = 0;
    collecting_name_replies = true;
    mqtt.subscribe(name_request_topic.c_str());
    mqtt.subscribe(name_reply_topic.c_str());
    mqtt.publish(name_request_topic.c_str(), device_id.c_str(), false);

    const unsigned long startMs = millis();
    while ((millis() - startMs) < windowMs) {
        mqtt.loop();
        waitMs(10);
    }

    collecting_name_replies = false;
    return true;
}

bool MQTTManager::isDeviceAliasClaimed(const String& alias) const {
    for (size_t i = 0; i < name_reply_count; i++) {
        if (equalsIgnoreCaseTrimmed(name_reply_buffer[i], alias)) {
            return true;
        }
    }
    return false;
}

String MQTTManager::chooseAvailableDeviceAlias(const String& excludedAlias) const {
    static const char* kNames[] = {
        "Knut", "Rika", "Bernd", "Maike", "Günter", "Leyla", "Harald", "Maja",
        "Edmund", "Mira", "Rüdiger", "Ria", "Horst", "Tilda", "Konrad", "Hanne",
        "Malte", "Frieda", "Jannis", "Irmgard", "Sören", "Jassi", "Armin", "Nele",
        "Torben", "Thea", "Heiko", "Celina", "Henning", "Zoe", "Lennart", "Mara",
        "Jasper", "Gina", "Frank", "Hilda", "Werner", "Lea", "Taron", "Lotte",
        "Fredi", "Tia", "Georg", "Linda", "Kai", "Claudia", "Sebi", "Lena",
        "Peter", "Viola", "Lukas", "Anna", "Basti", "Sabrina", "Nick", "Maren",
        "Dieter", "Sabine", "Ulrich", "Beate"
    };
    static const size_t kNameCount = sizeof(kNames) / sizeof(kNames[0]);

    const size_t start = (size_t)(esp_random() % kNameCount);
    for (size_t offset = 0; offset < kNameCount; offset++) {
        const String candidate = String(kNames[(start + offset) % kNameCount]);
        if (!excludedAlias.isEmpty() && equalsIgnoreCaseTrimmed(candidate, excludedAlias)) {
            continue;
        }
        if (!isDeviceAliasClaimed(candidate)) {
            return candidate;
        }
    }

    if (!excludedAlias.isEmpty() && !isDeviceAliasClaimed(excludedAlias)) {
        return excludedAlias;
    }
    return pickRandomDeviceAlias(excludedAlias);
}

bool MQTTManager::resolveDeviceName(bool forceRandomize) {
    if (!has_persisted_device_name) {
        String storedAlias;
        if (loadStoredDeviceAlias(storedAlias)) {
            setDeviceAlias(storedAlias);
            has_persisted_device_name = true;
        }
    }

    const String previousAlias = device_alias;
    const bool needsNetworkProbe = forceRandomize || previousAlias.isEmpty() || !has_persisted_device_name;
    if (needsNetworkProbe) {
        collectPeerDeviceNames(350);
    }

    String targetAlias = previousAlias;
    if (forceRandomize || targetAlias.isEmpty() || !has_persisted_device_name) {
        targetAlias = chooseAvailableDeviceAlias(forceRandomize ? previousAlias : "");
    }

    if (targetAlias.isEmpty()) {
        targetAlias = pickRandomDeviceAlias(forceRandomize ? previousAlias : "");
    }

    const bool changed = !equalsIgnoreCaseTrimmed(previousAlias, targetAlias);
    const bool needsPersist = forceRandomize || changed || !has_persisted_device_name;
    const bool persistOk = needsPersist ? persistDeviceAlias(targetAlias) : true;
    if (!needsPersist) {
        setDeviceAlias(targetAlias);
    }

    if (changed || needsPersist) {
        DebugManager::printf(DebugCategory::MQTT,
                             "MQTTManager: Device name = %s (alias=%s)\n",
                             device_name.c_str(),
                             device_alias.c_str());
        Serial.printf("[NAME] %s\n", device_name.c_str());
    }
    return persistOk;
}

void MQTTManager::loop() {
    if (!mqtt.connected()) {
        unsigned long now = millis();
        if (now - last_reconnect_attempt > reconnect_interval_ms) {
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
        effectDensity   = ControlConfig::DEFAULT_DENSITY;
        transitionMs    = ControlConfig::DEFAULT_TRANSITION_MS;
        effectPalette   = ControlConfig::DEFAULT_PALETTE;
        stateManager.setSpeed(effectSpeed);
        stateManager.setIntensity(effectIntensity);
        stateManager.setDensity(effectDensity);
        stateManager.setTransitionMs(transitionMs);
        stateManager.setPalette(effectPalette);
        stateManager.scheduleSave();
        mqtt.publish(speed_state_topic.c_str(),      String(effectSpeed).c_str(),     true);
        mqtt.publish(intensity_state_topic.c_str(),  String(effectIntensity).c_str(), true);
        mqtt.publish(density_state_topic.c_str(),    String(effectDensity).c_str(),   true);
        mqtt.publish(transition_state_topic.c_str(), String(transitionMs).c_str(),   true);
        DebugManager::println(DebugCategory::MQTT, "MQTTManager: Tuning reset to defaults");
    };

    DebugManager::print(DebugCategory::MQTT, "MQTTManager: Message on ");
    DebugManager::print(DebugCategory::MQTT, topic);
    DebugManager::print(DebugCategory::MQTT, ": ");
    DebugManager::println(DebugCategory::MQTT, payload);

    if (topic == name_request_topic) {
        String requesterId = payload;
        requesterId.trim();
        if (!requesterId.isEmpty() && requesterId != device_id && !device_alias.isEmpty()) {
            const String replyTopic = buildNameReplyTopic(requesterId);
            mqtt.publish(replyTopic.c_str(), device_alias.c_str(), false);
        }
        logCallbackDuration("name_request");
        return;
    }

    if (topic == name_reply_topic) {
        if (collecting_name_replies) {
            addPeerDeviceName(payload);
        }
        logCallbackDuration("name_reply");
        return;
    }

    if (isCommandTopic(topic) && last_connect_ms != 0 && (millis() - last_connect_ms) < COMMAND_IGNORE_AFTER_CONNECT_MS) {
        DebugManager::print(DebugCategory::MQTT, "MQTTManager: Ignoring command during post-connect grace window on topic ");
        DebugManager::println(DebugCategory::MQTT, topic);
        logCallbackDuration("ignored_after_connect");
        return;
    }

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

    if (topic == density_command_topic) {
        int val = payload.toInt();
        if (val >= ControlConfig::DENSITY_MIN && val <= ControlConfig::DENSITY_MAX) {
            effectDensity = (uint8_t)val;
            stateManager.setDensity(effectDensity);
            stateManager.scheduleSave();
            mqtt.publish(density_state_topic.c_str(), String(effectDensity).c_str(), true);
            DebugManager::printf(DebugCategory::MQTT, "MQTTManager: Density set to %d\n", effectDensity);
        }
        logCallbackDuration("density");
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

bool MQTTManager::isCommandTopic(const String& topic) const {
    return topic == command_topic ||
           topic == reboot_command_topic ||
           topic == speed_command_topic ||
           topic == intensity_command_topic ||
           topic == density_command_topic ||
           topic == transition_command_topic ||
           topic == tuning_reset_command_topic ||
           topic == ota_check_command_topic;
}
