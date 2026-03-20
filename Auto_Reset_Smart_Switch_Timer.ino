/*
 * TechsPassion Auto-Reset Smart Switch (Timer)
 * -------------------------------------------
 * A professional-grade smart relay switch featuring a real-time web interface, 
 * connectivity watchdog, and automated scheduling.
 * 
 * FEATURES:
 * - TechsPassion Branding: Custom AP SSID (TechsPassion-Switch), pass (techspassion), and mDNS hostname.
 * - WiFi Portal: Starts an AP for easy configuration with a network scanner if not connected.
 * - mDNS Friendly URL: Access via http://techspassion-switch.local
 * - Live Dark Mode UI: Real-time dashboard for time, connectivity, and switch state.
 * - Connectivity Watchdog: Automatically resets the relay (Off then On) if internet connection is lost.
 * - Smart Scheduling: Set daily ON/OFF times via the web interface.
 * - Configurable Timezone: Adjust GMT offset in the dashboard settings.
 * - Manual Control: Physical button (GPIO 0) and web dashboard toggle.
 * - Status LED: Onboard LED (GPIO 2) syncs with the relay state.
 * 
 * HARDWARE CONFIGURATION:
 * - Relay: GPIO 26
 * - Status LED: GPIO 2 (Onboard)
 * - Manual Button: GPIO 0 (Boot button)
 * 
 * HOW TO USE:
 * 1. Upload the code to your ESP32.
 * 2. Connect phone/PC to "TechsPassion-Switch" WiFi (password: techspassion).
 * 3. Open 192.168.4.1 in your browser, scan for your WiFi, and enter credentials.
 * 4. Once connected, access the dashboard at http://techspassion-switch.local
 * 5. Adjust the GMT Offset in settings to ensure the clock matches your local time.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <time.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// Pins
const int relayPin = 26;
const int ledPin = 2; // Onboard LED
const int buttonPin = 0; // Boot button

// Web server
WebServer server(80);
Preferences preferences;

// WiFi Config
String ssid = "";
String password = "";
bool apMode = false;

// Scheduling & Regional
struct Schedule {
  int onHour = -1;
  int onMin = -1;
  int offHour = -1;
  int offMin = -1;
  bool enabled = false;
};
Schedule currentSchedule;
long gmtOffset_sec = 0;
int daylightOffset_sec = 0;

// State
bool relayState = false;
bool internetConnected = false;
unsigned long lastConnectionCheck = 0;
const unsigned long connectionCheckInterval = 30000; // 30 seconds
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

const char* ntpServer = "pool.ntp.org";

void updateSwitchState() {
  digitalWrite(relayPin, relayState ? HIGH : LOW);
  digitalWrite(ledPin, relayState ? HIGH : LOW);
  preferences.putBool("relayState", relayState);
}

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  preferences.begin("smart-switch", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  gmtOffset_sec = preferences.getLong("gmtOffset", 0);
  daylightOffset_sec = preferences.getInt("dstOffset", 0);
  
  currentSchedule.onHour = preferences.getInt("onHour", -1);
  currentSchedule.onMin = preferences.getInt("onMin", -1);
  currentSchedule.offHour = preferences.getInt("offHour", -1);
  currentSchedule.offMin = preferences.getInt("offMin", -1);
  currentSchedule.enabled = preferences.getBool("schEnabled", false);
  relayState = preferences.getBool("relayState", false);
  
  updateSwitchState();

  if (ssid == "") {
    startAP();
  } else {
    if (!connectWiFi()) {
      startAP();
    } else {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      if (MDNS.begin("techspassion-switch")) {
        Serial.println("MDNS responder started: http://techspassion-switch.local");
      }
    }
  }

  setupServer();
  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
  handleButton();

  if (!apMode) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastConnectionCheck > connectionCheckInterval || lastConnectionCheck == 0) {
      lastConnectionCheck = currentMillis;
      checkConnectivity();
    }
    checkSchedule();
  }
}

bool connectWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    apMode = false;
    return true;
  } else {
    Serial.println("\nFailed to connect.");
    return false;
  }
}

void startAP() {
  apMode = true;
  // TechsPassion branding for AP
  WiFi.softAP("TechsPassion-Switch", "techspassion");
  Serial.println("Access Point started: TechsPassion-Switch");
}

void handleButton() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
      toggleRelay();
    }
  }
  lastButtonState = reading;
}

void toggleRelay() {
  relayState = !relayState;
  updateSwitchState();
}

void checkConnectivity() {
  if (WiFi.status() != WL_CONNECTED) {
    internetConnected = false;
    resetSwitch();
    connectWiFi();
  } else {
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    int httpCode = http.GET();
    if (httpCode <= 0) {
      internetConnected = false;
      resetSwitch();
    } else {
      internetConnected = true;
    }
    http.end();
  }
}

void resetSwitch() {
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, LOW);
  delay(2000);
  digitalWrite(relayPin, relayState ? HIGH : LOW);
  digitalWrite(ledPin, relayState ? HIGH : LOW);
}

void checkSchedule() {
  if (!currentSchedule.enabled) return;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour == currentSchedule.onHour && timeinfo.tm_min == currentSchedule.onMin) {
    if (!relayState) {
      relayState = true;
      updateSwitchState();
    }
  } else if (timeinfo.tm_hour == currentSchedule.offHour && timeinfo.tm_min == currentSchedule.offMin) {
    if (relayState) {
      relayState = false;
      updateSwitchState();
    }
  }
}

void setupServer() {
  server.on("/", HTTP_GET, []() {
    if (apMode) {
      int n = WiFi.scanNetworks();
      String html = "<html><head><title>TechsPassion Config</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:sans-serif;text-align:center;padding:20px;background:#1a1a1a;color:#eee;}select,input{padding:12px;margin:10px;width:90%;border-radius:8px;border:1px solid #444;background:#2d2d2d;color:#eee;}</style></head><body>";
      html += "<h1>TechsPassion WiFi Config</h1><form action='/save' method='POST'>";
      html += "Select Network: <br><select name='ssid'>";
      if (n == 0) html += "<option value=''>No networks found</option>";
      else {
        for (int i = 0; i < n; ++i) html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
      }
      html += "</select><br>Password: <br><input name='pass' type='password'><br>";
      html += "<input type='submit' value='Connect' style='background:#00b4d8;color:white;border:none;font-weight:bold;'></form></body></html>";
      server.send(200, "text/html", html);
    } else {
      String html = "<html><head><title>TechsPassion Smart Switch</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
      html += "body{font-family:'Segoe UI',Tahoma,sans-serif;text-align:center;padding:20px;background:#0f172a;color:#f8fafc;}";
      html += ".card{background:#1e293b;padding:25px;border-radius:20px;box-shadow:0 10px 15px -3px rgba(0,0,0,0.5);max-width:450px;margin:auto;}";
      html += "h1{color:#38bdf8;margin-bottom:5px;} .sub{color:#94a3b8;font-size:0.9em;margin-bottom:25px;}";
      html += ".btn{display:inline-block;padding:20px 40px;font-size:20px;font-weight:bold;cursor:pointer;text-align:center;text-decoration:none;color:#fff;background:#0ea5e9;border:none;border-radius:50px;box-shadow:0 0 20px rgba(14,165,233,0.3);transition:0.3s;margin:20px 0;}";
      html += ".btn:active{transform:scale(0.95);} .off{background:#64748b;box-shadow:none;}";
      html += ".status-row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #334155;}";
      html += ".dot{height:10px;width:10px;background-color:#ef4444;border-radius:50%;display:inline-block;margin-right:5px;} .online{background-color:#22c55e;}";
      html += "input[type='time'],input[type='number']{background:#334155;border:1px solid #475569;color:#fff;padding:8px;border-radius:8px;}";
      html += ".footer{margin-top:20px;font-size:0.8em;color:#64748b;} a{color:#38bdf8;text-decoration:none;}";
      html += "</style></head><body><div class='card'><h1>TechsPassion</h1><p class='sub'>http://techspassion-switch.local</p>";
      html += "<div class='status-row'><span>System Clock</span><span id='time'>--:--:--</span></div>";
      html += "<div class='status-row'><span>WiFi Link</span><span><span id='wifiDot' class='dot'></span><span id='wifiText'>...</span></span></div>";
      html += "<div class='status-row'><span>Internet</span><span><span id='netDot' class='dot'></span><span id='netText'>...</span></span></div>";
      html += "<div class='status-row'><span>Switch Status</span><strong id='relayLabel' style='color:#38bdf8'>OFF</strong></div>";
      html += "<button id='toggleBtn' class='btn' onclick='toggle()'>Turn ON</button>";
      html += "<h3>Schedule & Settings</h3><form action='/settings' method='POST' style='text-align:left;'>";
      html += String("ON Time: <input name='on' type='time' value='") + (currentSchedule.onHour < 10 ? "0" : "") + String(currentSchedule.onHour) + ":" + (currentSchedule.onMin < 10 ? "0" : "") + String(currentSchedule.onMin) + "' style='width:100px'><br><br>";
      html += String("OFF Time: <input name='off' type='time' value='") + (currentSchedule.offHour < 10 ? "0" : "") + String(currentSchedule.offHour) + ":" + (currentSchedule.offMin < 10 ? "0" : "") + String(currentSchedule.offMin) + "' style='width:100px'><br><br>";
      html += "Enabled: <input name='enabled' type='checkbox' " + String(currentSchedule.enabled ? "checked" : "") + "><br><br>";
      html += "GMT Offset (hrs): <input name='gmt' type='number' step='0.5' value='" + String(gmtOffset_sec / 3600.0) + "' style='width:80px'><br><br>";
      html += "<input type='submit' value='Apply Changes' style='width:100%;padding:10px;background:#22c55e;color:white;border:none;border-radius:8px;cursor:pointer;'></form>";
      html += "<div class='footer'>© TechsPassion | <a href='/reset_wifi'>Reset WiFi</a></div></div>";
      html += "<script>function update(){fetch('/status').then(r=>r.json()).then(d=>{";
      html += "document.getElementById('time').innerText=d.time;";
      html += "document.getElementById('wifiDot').className='dot '+(d.wifi?'online':'');";
      html += "document.getElementById('wifiText').innerText=d.wifi?'Connected':'Disconnected';";
      html += "document.getElementById('netDot').className='dot '+(d.internet?'online':'');";
      html += "document.getElementById('netText').innerText=d.internet?'Online':'Offline';";
      html += "document.getElementById('relayLabel').innerText=d.relay?'ON':'OFF';";
      html += "const b=document.getElementById('toggleBtn');b.innerText=d.relay?'Turn OFF':'Turn ON';";
      html += "if(d.relay)b.classList.remove('off');else b.classList.add('off');";
      html += "});} setInterval(update,2000);update();";
      html += "function toggle(){fetch('/toggle').then(()=>update());}</script></body></html>";
      server.send(200, "text/html", html);
    }
  });

  server.on("/status", HTTP_GET, []() {
    struct tm timeinfo;
    char buffer[10] = "--:--:--";
    if (getLocalTime(&timeinfo)) strftime(buffer, 10, "%H:%M:%S", &timeinfo);
    String json = "{\"relay\":"+String(relayState?"true":"false")+",\"wifi\":"+String(WiFi.status()==WL_CONNECTED?"true":"false")+",\"internet\":"+String(internetConnected?"true":"false")+",\"time\":\""+String(buffer)+"\"}";
    server.send(200, "application/json", json);
  });

  server.on("/toggle", HTTP_GET, []() {
    toggleRelay();
    server.send(200, "text/plain", "OK");
  });

  server.on("/save", HTTP_POST, []() {
    ssid = server.arg("ssid");
    password = server.arg("pass");
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    server.send(200, "text/html", "TechsPassion Config Saved.<br>IP: " + WiFi.localIP().toString() + "<br>Restarting...");
    delay(2000);
    ESP.restart();
  });

  server.on("/settings", HTTP_POST, []() {
    String onTime = server.arg("on");
    String offTime = server.arg("off");
    if(onTime.length()==5){currentSchedule.onHour=onTime.substring(0,2).toInt(); currentSchedule.onMin=onTime.substring(3,5).toInt();}
    if(offTime.length()==5){currentSchedule.offHour=offTime.substring(0,2).toInt(); currentSchedule.offMin=offTime.substring(3,5).toInt();}
    currentSchedule.enabled = server.hasArg("enabled");
    gmtOffset_sec = (long)(server.arg("gmt").toFloat() * 3600);
    
    preferences.putInt("onHour", currentSchedule.onHour);
    preferences.putInt("onMin", currentSchedule.onMin);
    preferences.putInt("offHour", currentSchedule.offHour);
    preferences.putInt("offMin", currentSchedule.offMin);
    preferences.putBool("schEnabled", currentSchedule.enabled);
    preferences.putLong("gmtOffset", gmtOffset_sec);
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/reset_wifi", HTTP_GET, []() {
    preferences.putString("ssid", "");
    preferences.putString("password", "");
    server.send(200, "text/html", "TechsPassion: WiFi reset. Restarting...");
    delay(2000);
    ESP.restart();
  });
}
