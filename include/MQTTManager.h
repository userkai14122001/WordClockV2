#ifndef MQTTMANAGER_CLASS_H
#define MQTTMANAGER_CLASS_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// Callback-Typ für MQTT-Nachrichten
typedef std::function<void(const String& topic, const String& payload)> MQTTCallback;

class MQTTManager {
public:
    MQTTManager(WiFiClient& wifi_client, const String& device_id = "wordclock");
    
    // Konfiguration
    void setConfig(const String& server, int port, const String& user, const String& password);
    void setTopics(const String& command_topic, const String& state_topic, const String& discover_topic);
    
    // Verbindung
    void connect();
    void disconnect();
    bool isConnected();
    
    // Kommunikation
    void publish(const String& topic, const String& payload);
    void publishState(bool power, const String& effect, uint32_t color, uint8_t brightness);
    void publishDiscovery();
    static constexpr uint32_t PUBLISH_WARN_US = 30000;
    static constexpr uint32_t LOOP_WARN_US = 25000;
    static constexpr uint32_t CALLBACK_WARN_US = 25000;
    
    // Loop für Ticker und Reconnect
    void loop();
    
    // Callback registrieren
    void setCallback(MQTTCallback callback) { on_message = callback; }
    
    // Getter
    String getServer() const { return server; }
    int getPort() const { return port; }
    String getUser() const { return user; }
    String getPassword() const { return password; }
    String getCommandTopic() const { return command_topic; }
    String getStateTopic() const { return state_topic; }
    String getDiscoverTopic() const { return discover_topic; }
    String getDeviceId() const { return device_id; }
    
private:
    PubSubClient mqtt;
    WiFiClient& wifi_client;
    
    String device_id;
    String server;
    int port;
    String user;
    String password;
    
    String command_topic;
    String state_topic;
    String discover_topic;

    String availability_topic;
    String uptime_topic;
    String rssi_topic;
    String ip_topic;
    String mqtt_state_topic;
    String version_topic;
    String reboot_command_topic;

    // Effect tuning control topics
    String speed_command_topic;
    String speed_state_topic;
    String intensity_command_topic;
    String intensity_state_topic;
    String transition_command_topic;
    String transition_state_topic;
    String tuning_reset_command_topic;
    String service_command_topic;

    // RTC telemetry topics
    String rtc_temp_topic;
    String rtc_battery_warning_topic;

    // Palette & hue shift topics
    String palette_command_topic;
    String palette_state_topic;
    String hue_shift_command_topic;
    String hue_shift_state_topic;

    // OTA update topics
    String ota_check_command_topic;

    unsigned long last_reconnect_attempt;
    unsigned long last_telemetry_publish;
    static const unsigned long RECONNECT_INTERVAL = 5000;
    static const unsigned long TELEMETRY_INTERVAL = 30000;
    
    MQTTCallback on_message;
    
    void internalCallback(const String& topic, const String& payload);
    void logSlowPublish(const char* label, const String& topic, uint32_t durationUs, bool ok);
    void publishDiagnosticsDiscovery();
    void publishTuningDiscovery();
    void publishTelemetry();
};

#endif
