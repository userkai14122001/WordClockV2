#include "ota_https_update.h"
#include "DebugManager.h"
#include "SystemControl.h"
#include <WiFiClientSecure.h>
#include <esp_https_ota.h>

static inline void waitMs(uint32_t ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield();
    }
}

void performHttpsOtaUpdate() {
    DebugManager::println(DebugCategory::OTA, "Starte HTTPS OTA Update...");

    static const char *url =
        "https://raw.githubusercontent.com/userkai14122001/Word-Clock/main/firmware.bin";

    esp_http_client_config_t http_config = {};
    http_config.url = url;
    http_config.timeout_ms = 10000;
    http_config.transport_type = HTTP_TRANSPORT_OVER_TCP;
    http_config.skip_cert_common_name_check = true;
    http_config.use_global_ca_store = false;
    http_config.cert_pem = NULL;

    esp_err_t ret = esp_https_ota(&http_config);

    if (ret == ESP_OK) {
        DebugManager::println(DebugCategory::OTA, "OTA erfolgreich! Neustart...");
        rebootDevice("OTA success", 1000);
    } else {
        DebugManager::printf(DebugCategory::OTA, "OTA fehlgeschlagen! Fehlercode: %d\n", ret);
    }
}
