# Test Release Plan v0.2.47

Purpose: validate the hardened OTA release workflow after the manifest update failures on v0.2.45 and v0.2.46.

## Preconditions

1. Current branch is `main` and clean.
2. Current workflow fix is included on `main`.
3. GitHub repository secret `RELEASE_PUSH_TOKEN` exists.
4. `RELEASE_PUSH_TOKEN` has at least `contents:write` permission for this repository.
5. At least one pilot device is available for OTA verification.

## Test Goal

The tag-triggered release workflow must:

1. Build the firmware successfully.
2. Publish `firmware.bin` as a GitHub release asset for `v0.2.47`.
3. Update `ota_manifest.json` on `main` automatically.
4. Verify remote manifest alignment successfully.
5. Allow a pilot device to update once without entering an OTA loop.

## Execution Steps

1. Confirm local build:
   - `.venv\Scripts\python.exe -m platformio run -e seeed_xiao_esp32c3_usb`
2. Create the tag:
   - `git tag v0.2.47`
3. Push the tag:
   - `git push origin v0.2.47`
4. Wait for workflow `Build & Release Firmware` to finish.

## Expected GitHub Results

1. Workflow result is green.
2. GitHub release `v0.2.47` exists.
3. Release asset `firmware.bin` is downloadable.
4. `ota_manifest.json` on `main` contains:
   - `channels.stable.version = 0.2.47`
   - `channels.stable.firmware_url` points to `/releases/download/v0.2.47/firmware.bin`
   - `channels.stable.sha256` matches the published asset
   - `channels.stable.status = ready`

## Expected Device Results

1. Pilot device detects remote version `0.2.47`.
2. OTA update downloads and installs successfully.
3. Device reboots successfully.
4. Reported local version becomes `0.2.47`.
5. No repeated OTA update after reboot.

## Failure Interpretation

1. If workflow fails before release creation:
   - build or workflow syntax problem
2. If release exists but manifest update fails:
   - `RELEASE_PUSH_TOKEN` missing, invalid, or insufficient permissions
3. If manifest is correct but device loops:
   - firmware version embedded in build does not match manifest version
4. If device refuses update:
   - manifest `status` not `ready`, bad SHA, or asset URL mismatch

## Immediate Recovery

1. Do not continue fleet rollout.
2. Keep `stable` at the last known good version.
3. Fix root cause.
4. Repeat with the next patch tag only after the cause is understood.