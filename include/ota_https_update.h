#pragma once

// Current firmware version compiled into the binary.
const char* getFirmwareVersion();

// Active OTA channel name used when selecting manifest entries.
const char* getOtaChannel();

// Download and install a firmware image from a direct HTTPS URL.
void performHttpsOtaUpdate(const char* firmwareUrl, const char* expectedSha256 = nullptr);

// Check GitHub manifest for a newer version and update automatically.
// Returns true when an update was initiated.
bool checkForUpdateAndInstall(bool forceLog);
