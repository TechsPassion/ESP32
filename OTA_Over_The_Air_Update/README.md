# ESP32 Over The Air (OTA) Update

Upload new code to your ESP32 over WiFi, no USB cable needed, and see how the ESP32 protects itself from a bad update with automatic rollback.

Watch the video on my YouTube channel: https://www.youtube.com/@techs.passion

| Sketch | What it shows |
| --- | --- |
| [`OTA_Basic`](OTA_Basic/OTA_Basic.ino) | The simplest OTA setup. Change the blink speed, upload over WiFi, watch the LED change. |
| [`OTA_Rollback_Demo`](OTA_Rollback_Demo/OTA_Rollback_Demo.ino) | Which partition is running, the update states, and a bad update rolling back to the last working version. |

For the Arduino IDE 2 with the ESP32 Arduino core 3.x, on a standard ESP32 dev board (4MB flash). No extra libraries needed.

---

## How it works

### 1. Two slots for your code

![ESP32 flash layout with two app slots](images/01-flash-layout.png)

With an OTA partition scheme, the flash has **two app slots** of the same size:

- **app0 (ota_0)** and **app1 (ota_1)**: one holds the code that is running, the other receives the next update.
- **otadata**: a tiny partition that tells the bootloader which slot to start from.

Partition schemes marked "No OTA" (like *Huge APP*) have only one slot, so OTA can't work with them.

### 2. The update process

![How an OTA update works](images/02-ota-update-flow.png)

1. **Send**: the Arduino IDE sends the new firmware to the ESP32 over WiFi.
2. **Write**: the ESP32 writes it into the free slot while the current code keeps running.
3. **Check**: it checks the file arrived complete (MD5 and image check). If the check fails, nothing changes.
4. **Switch**: it updates otadata to point at the new slot and restarts into the new code.

The default check makes sure the file is not corrupted. It does **not** check who made the firmware. For that you need signed updates or Secure Boot.

### 3. The ping-pong effect

![Each update goes into the slot that is not running](images/03-ping-pong.png)

Each update goes into the slot that is **not** running, so the ESP32 bounces between app0 and app1. The previous version is always kept as a backup in the other slot, until the next update replaces it.

### 4. The safety net: rollback

![Rollback flow](images/04-rollback.png)

A new update starts as a **trial run** (`PENDING_VERIFY`). The sketch runs a health check, for example "did WiFi connect?":

- **Passed**: the sketch calls `esp_ota_mark_app_valid_cancel_rollback()` and the update stays (`VALID`).
- **Failed**: the sketch calls `esp_ota_mark_app_invalid_rollback_and_reboot()`.
- **Crashed** before it was marked valid: on the next restart the bootloader sees the trial never finished.

In both bad cases the ESP32 starts the **previous version** again from the other slot.

Two things to know:

- **Rollback is not automatic in Arduino by default.** The Arduino core marks every new update as good *before* `setup()` runs. The rollback sketch turns this off with:

  ```cpp
  extern "C" bool verifyRollbackLater() {
    return true;
  }
  ```

  `extern "C"` is required. Without it, the Arduino core ignores your function and rollback never happens.
- **A freeze is not a crash.** Rollback needs the ESP32 to restart. If your code hangs forever without restarting, it stays on the bad version. A watchdog timer can turn a hang into a restart.

---

## Setup in the Arduino IDE

![OTA steps in the Arduino IDE](images/05-arduino-ide-steps.png)

1. Install the ESP32 boards: **Tools > Board > Boards Manager**, search for *esp32* (by Espressif) and install it.
2. Select your board, for example **ESP32 Dev Module**.
3. **Tools > Partition Scheme > Default 4MB with spiffs** (any scheme with OTA works).
4. Open `OTA_Basic`, enter your WiFi name and password, and **upload once over USB**.
5. Open the **Serial Monitor at 115200** and note the IP address.
6. Change `BLINK_MS` (for example `1000` to `150`).
7. **Tools > Port**: under *Network ports*, pick **techspassion-esp32 at 192.168.x.x**.
8. Click **Upload** and enter the OTA password (`techspassion` in the sketch, change it to your own).

The LED now blinks faster: the new code arrived over WiFi.

### Network port not showing?

- The computer and the ESP32 must be on the **same WiFi network**.
- Restart the Arduino IDE after the ESP32 connects to WiFi.
- On Windows, allow the Arduino IDE through the firewall on **private networks**.
- Some routers and guest networks block device discovery (mDNS). Try a different network or your phone's hotspot.

---

## Rollback demo

Open `OTA_Rollback_Demo`, enter your WiFi details, and open the Serial Monitor at 115200.

| Step | Settings | Upload | What you see |
| --- | --- | --- | --- |
| 1 | `FIRMWARE_VERSION = 1` | USB | Running from app0, LED blinks 1 time |
| 2 | `FIRMWARE_VERSION = 2` | WiFi | Running from app1, trial run passes, marked **VALID**, LED blinks 2 times |
| 3 | `FIRMWARE_VERSION = 3`, `SIMULATE_CRASH = true` | WiFi | Version 3 crashes, the ESP32 restarts and is **back on version 2** |
| 4 | `FIRMWARE_VERSION = 3`, `SIMULATE_HEALTH_CHECK_FAIL = true` | WiFi | Version 3 starts, fails its check and **rolls itself back** to version 2 |

After a rollback the Serial Monitor shows which slot held the rejected update:

```
=== TechsPassion OTA Rollback Demo ===
Firmware version:  2
Running from:      app1
Next update goes:  app0
State:             VALID
Last rejected update was in: app0
```

---

## Tips

- **Every sketch must include the OTA code.** Upload one without it and the next update needs USB again.
- **Keep `loop()` fast.** `ArduinoOTA.handle()` must run often. Use `millis()` instead of long `delay()` calls.
- **Robots, motors and relays:** stop them in `ArduinoOTA.onStart()`. The ESP32 restarts at the end of the update.
- **Change the OTA password** before using this on a real project.
- **Updating from anywhere (not the same WiFi):** let the ESP32 download a `.bin` file from a web server with the `HTTPUpdate` library. The same two slots and rollback apply.

You can use this code however you like. Please consider liking and subscribing to my YouTube channel: https://www.youtube.com/@techs.passion
