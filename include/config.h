#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// ---------------------------------------------------------
// MATRIX / LED KONFIGURATION
// ---------------------------------------------------------
constexpr int WIDTH  = 11;
constexpr int HEIGHT = 10;

constexpr int LED_PIXEL_AMOUNT = WIDTH * HEIGHT;
constexpr int LED_PIN = 2;

// ---------------------------------------------------------
// GLOBALE VARIABLEN (extern)
// ---------------------------------------------------------

extern bool powerState;
extern String currentEffect;
extern uint32_t color;
extern uint8_t brightness;

extern bool setupMode;
extern unsigned long setupStartTime;

extern WebServer server;
extern DNSServer dnsServer;
extern Preferences prefs;

extern String wifi_ssid;
extern String wifi_pass;

extern WiFiClient espClient;
extern PubSubClient mqtt;

extern String mqtt_server;
extern String mqtt_user;
extern String mqtt_pass;

extern String device_id;
extern String command_topic;
extern String state_topic;

// ---------------------------------------------------------
// FUNKTIONEN
// ---------------------------------------------------------
void startSetupMode();
void handleSetupWeb();
void tryConnectWiFi();
void loadConfig();
void saveConfig(String ssid, String pass, String host, String user, String mpass);

#endif
