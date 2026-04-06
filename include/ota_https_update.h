#pragma once

// Current firmware version compiled into the binary.
const char* getFirmwareVersion();

// Download and install a firmware image from a direct HTTPS URL.
void performHttpsOtaUpdate(const char* firmwareUrl);

// Check GitHub manifest for a newer version and update automatically.
// Returns true when an update was initiated.
bool checkForUpdateAndInstall(bool forceLog);
