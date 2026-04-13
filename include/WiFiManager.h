#ifndef WIFIMANAGER_CLASS_H
#define WIFIMANAGER_CLASS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

class WiFiManager {
public:
    WiFiManager();
    
    // Konfiguration laden/speichern
    void loadConfig();
    void saveConfig(const String& ssid, const String& password,
                    const String& mqtt_server, const String& mqtt_user,
                    const String& mqtt_password, int mqtt_port);
    
    // Setup-Mode
    void startSetupMode();
    void handleSetup();
    void stopSetupMode();
    
    // WiFi Verbindung
    void connectToWiFi();
    bool isConnected();
    
    // Zugriff auf konfigurierte Werte
    String getSSID() const { return ssid; }
    String getPassword() const { return password; }
    String getMQTTServer() const { return mqtt_server; }
    String getMQTTUser() const { return mqtt_user; }
    String getMQTTPassword() const { return mqtt_password; }
    int getMQTTPort() const { return mqtt_port; }
    String getOtaProfile() const { return ota_profile; }
    unsigned long getOtaAutoCheckIntervalMs() const;
    void refreshOtaProfilePolicy();
    bool isSetupMode() const { return setup_mode; }
    
    // Web Server für Setup
    WebServer* getServer() { return &server; }
    
private:
    WebServer server;
    DNSServer dnsServer;
    Preferences prefs;
    bool config_loaded;
    bool routes_initialized;
    
    String ssid;
    String password;
    String mqtt_server;
    String mqtt_user;
    String mqtt_password;
    int mqtt_port;
    String ota_profile;
    uint32_t ota_profile_since_epoch;
    unsigned long ota_last_policy_check_ms;
    
    bool setup_mode;
    unsigned long setup_start_time;
    
    void setupWebRoutes();
    void handleScan();
    void handleSave();
    void handleStatus();
    void handleStatusLite();
    void handleLiveStatus();
    void handlePreview();
    void handleQuickTest();
    void handleDeviceNameRandomize();
    void handleOtaInfo();
    void handleOtaCheck();
    void handleOtaProfile();
    void handleLayoutGet();
    void handleLayoutSet();
    void handleZeitschaltungGet();
    void handleZeitschaltungSet();
    void setOtaProfile(const String& profile, bool resetSinceEpoch = true);
};

#endif
