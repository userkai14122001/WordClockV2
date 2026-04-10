# Release Checklist (WordClock OTA)

This checklist is mandatory for every production release.

## 1) Pre-release (before tag)

1. Working tree is clean.
2. Main branch is up to date.
3. Firmware builds locally:
   - .venv\Scripts\python.exe -m platformio run -e seeed_xiao_esp32c3_usb
4. No known OTA blockers in open issues.
5. Planned version follows SemVer (for example 0.2.46).
6. Device test plan is ready (at least 1-2 pilot devices).

## 2) Trigger release (tag only)

1. Create and push tag from main:
   - git tag vX.Y.Z
   - git push origin vX.Y.Z
2. Do not manually edit ota_manifest.json for normal releases.
3. Wait for GitHub workflow Build and Release Firmware to finish.

## 3) CI workflow must pass

1. Workflow status is green.
2. GitHub release for tag vX.Y.Z exists.
3. Release asset firmware.bin exists and is downloadable.
4. Workflow has updated ota_manifest.json on main.
5. Stable channel values are correct:
   - version equals X.Y.Z
   - firmware_url points to release asset for vX.Y.Z
   - sha256 is present and matches release binary
   - status is ready

## 4) Post-release device validation

1. Pilot device reports remote newer version.
2. OTA update starts and completes.
3. Device reboots successfully.
4. Reported fw_version equals X.Y.Z.
5. No update loop after reboot.
6. Core functions healthy: time sync, display updates, WiFi, MQTT.

## 5) Rollout decision

1. If pilot is good, allow normal fleet rollout.
2. If issue appears, stop rollout immediately by setting stable status to hold in ota_manifest.json.
3. If needed, point stable channel back to last known good version.

## 6) Incident quick actions

1. Set stable status to hold.
2. Publish manifest fix to main.
3. Re-check that devices no longer start OTA.
4. Prepare hotfix release as next tag.

## 7) Hard rules

1. Never release from uncommitted local changes.
2. Never ship without release asset and matching manifest metadata.
3. Never call release done before pilot devices are verified.
4. Keep firmware version, tag, and manifest stable version aligned.
