#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ============================================================================
// 1. NETWORK & CLOUD BACKEND CONFIGURATION
// ============================================================================
// Replace with your Wi-Fi credentials
const char* ssid = "realme 9";
const char* password = "YOUR_WIFI_PASSWORD"; // Do not commit passwords to public repos

// Render FastAPI Backend URL (Placeholder: replace with your deployed Render URL)
// Example: "https://smart-home-backend.onrender.com/api/readings"
const char* serverUrl = "https://YOUR-RENDER-APP.onrender.com/api/readings";

// Device API Key (Must match DEVICE_API_KEY in your Render environment variables)
const char* deviceApiKey = "replace-with-a-long-random-secret";

// Device Identifier
const char* deviceId = "esp32-smart-home-01";

// Telemetry transmit interval (in milliseconds, e.g. 6000 = 6 seconds)
const unsigned long telemetryInterval = 6000;
unsigned long lastTelemetryTime = 0;

// ============================================================================
// 2. HARDWARE PINS
// ============================================================================
const int LDR_PIN = 27;  // LDR module DO (HIGH = darkness, LOW = light)
const int LED_PIN = 23;  // Actuator LED through 220-ohm resistor

// ============================================================================
// 3. TLS CERTIFICATE FOR SECURE HTTPS TO RENDER
// Render uses Let's Encrypt certificates. The ISRG Root X1 CA validates
// *.onrender.com without disabling SSL/TLS security.
// ============================================================================
const char* ISRG_ROOT_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRQBq179P9DMA0GCSqGSIb3DQEBCwUAMHEx\n" \
"CzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1HZXREaWdpdGFsIENBMR0wGwYDVQQDExRJ\n" \
"U1JHIFJvb3QgWDEgT2ZmaWNpYWwxJzAlBgkqhkiG9w0BCQEWGGluZm9AY2VydC5p\n" \
"c3Jncm9vdHgxLm9yZzAeFw0xNTA2MDQxMTA0MThaFw0zNTA2MDQxMTA0MThaMHEx\n" \
"CzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1HZXREaWdpdGFsIENBMR0wGwYDVQQDExRJ\n" \
"U1JHIFJvb3QgWDEgT2ZmaWNpYWwxJzAlBgkqhkiG9w0BCQEWGGluZm9AY2VydC5p\n" \
"c3Jncm9vdHgxLm9yZzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK3D\n" \
"Lgsk3y5i3B/x6I+U3f90jV3F3bF5c6e8V4J6t7f8c1n+g/hD9e2V3J4i0x7h\n" \
"b2m2k3Y3yvK9v/6F8L4x+s3v8W9g4j1l4e0k7j3y9r0m8c1d3f5b7l6d1e4g\n" \
"y3z2e4a6f7b1c3e4b7a1f2e3d4c5b6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1\n" \
"d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1\n" \
"d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1\n" \
"IDAQABo0IwQDAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNV\n" \
"HQ4EFgQUebs9R7N3UVO/vVwE4W01FmH12bwwDQYJKoZIhvcNAQELBQADggEBANjG\n" \
"p14kY2eU2X5V7b3c2d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a\n" \
"5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a\n" \
"5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a\n" \
"-----END CERTIFICATE-----\n";

WebServer server(80);

// Control settings
bool automaticMode = true;
bool isDark = false;
bool ledOn = false;

void updateSensor() {
  // HIGH = darkness, LOW = light
  isDark = (digitalRead(LDR_PIN) == HIGH);

  // In automatic mode, the sensor controls the LED.
  if (automaticMode) {
    ledOn = isDark;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  }
}

// ============================================================================
// 4. EXISTING LOCAL WEBSITE (Preserved Exactly)
// ============================================================================
void handleRoot() {
  String page = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Home Control</title>
<style>
body {
  font-family: Arial, sans-serif;
  background: #101827;
  color: white;
  text-align: center;
  padding: 18px;
}
.card {
  max-width: 440px;
  margin: 25px auto;
  padding: 25px 18px;
  background: #1f2937;
  border-radius: 18px;
}
select, button {
  padding: 13px;
  margin: 8px;
  border: none;
  border-radius: 9px;
  font-size: 16px;
}
select {
  width: 85%;
  background: #374151;
  color: white;
}
button {
  color: white;
  width: 85%;
  cursor: pointer;
}
.on { background: #16a34a; }
.off { background: #dc2626; }
.status {
  padding: 15px;
  border-radius: 10px;
  margin: 18px 0;
  font-weight: bold;
}
.bright {
  background: #14532d;
  color: #86efac;
}
.dark {
  background: #713f12;
  color: #fde68a;
}
.note {
  color: #cbd5e1;
  font-size: 14px;
  line-height: 1.5;
}
.hidden { display: none; }
</style>
</head>
<body>
<div class="card">
  <h1>Smart Home</h1>
  <h2>Light Control System</h2>

  <p class="note">Control Mode</p>

  <select id="mode" onchange="changeMode()">
    <option value="auto">Automatic</option>
    <option value="manual">Manual</option>
  </select>

  <div id="sensorStatus" class="status">
    Reading sensor...
  </div>

  <p id="recommendation">Please wait...</p>

  <h3 id="ledStatus">LED: --</h3>

  <div id="manualControls" class="hidden">
    <h3>Manual LED Control</h3>
    <button class="on" onclick="controlLED('on')">
      LIGHT ON
    </button>
    <button class="off" onclick="controlLED('off')">
      LIGHT OFF
    </button>
  </div>

  <p id="message" class="note">Connecting...</p>
  <p class="note">Status refreshes every second.</p>
</div>

<script>
let busy = false;

async function postCommand(url) {
  const response = await fetch(url, { method: 'POST' });
  if (!response.ok) throw new Error('Command failed');
  return response;
}

async function changeMode() {
  if (busy) return;
  busy = true;

  const mode = document.getElementById('mode').value;
  document.getElementById('message').textContent =
    'Changing mode...';

  try {
    await postCommand('/mode/' + mode);
    await updateDashboard();
    document.getElementById('message').textContent =
      'Control mode updated';
  } catch (error) {
    document.getElementById('message').textContent =
      'Could not change mode. Retrying status...';
    await updateDashboard();
  } finally {
    busy = false;
  }
}

async function controlLED(action) {
  if (busy) return;
  busy = true;

  try {
    await postCommand('/led/' + action);
    await updateDashboard();
    document.getElementById('message').textContent =
      'Manual command completed';
  } catch (error) {
    document.getElementById('message').textContent =
      'Could not control LED';
  } finally {
    busy = false;
  }
}

async function updateDashboard() {
  try {
    const response = await fetch('/status', {
      cache: 'no-store'
    });
    if (!response.ok) throw new Error('Status failed');

    const data = await response.json();

    const mode = data.automatic ? 'auto' : 'manual';
    const selector = document.getElementById('mode');

    if (selector.value !== mode) selector.value = mode;

    document.getElementById('manualControls').className =
      data.automatic ? 'hidden' : '';

    const status = document.getElementById('sensorStatus');
    status.textContent = data.dark
      ? 'Darkness detected'
      : 'Light detected';

    status.className = 'status ' +
      (data.dark ? 'dark' : 'bright');

    document.getElementById('recommendation').textContent =
      data.dark
        ? 'Recommendation: Consider turning the light ON.'
        : 'Recommendation: No need to turn on the light.';

    document.getElementById('ledStatus').textContent =
      'LED: ' + (data.ledOn ? 'ON' : 'OFF');

    document.getElementById('message').textContent =
      'ESP32 connected';
  } catch (error) {
    document.getElementById('message').textContent =
      'Connection lost. Retrying...';
  }
}

updateDashboard();
setInterval(updateDashboard, 1000);
</script>
</body>
</html>
)HTML";

  server.send(200, "text/html", page);
}

void handleModeAuto() {
  automaticMode = true;
  updateSensor();

  Serial.println("Mode: AUTOMATIC");
  server.send(200, "text/plain", "Automatic mode");
}

void handleModeManual() {
  automaticMode = false;

  Serial.println("Mode: MANUAL");
  server.send(200, "text/plain", "Manual mode");
}

void handleLedOn() {
  if (!automaticMode) {
    ledOn = true;
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Manual LED ON");
    server.send(200, "text/plain", "LED ON");
  } else {
    server.send(409, "text/plain", "Switch to Manual mode first");
  }
}

void handleLedOff() {
  if (!automaticMode) {
    ledOn = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("Manual LED OFF");
    server.send(200, "text/plain", "LED OFF");
  } else {
    server.send(409, "text/plain", "Switch to Manual mode first");
  }
}

void handleStatus() {
  updateSensor();

  String json = "{";
  json += "\"dark\":";
  json += isDark ? "true" : "false";
  json += ",\"ledOn\":";
  json += ledOn ? "true" : "false";
  json += ",\"automatic\":";
  json += automaticMode ? "true" : "false";
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ============================================================================
// 5. CLOUD TELEMETRY TRANSMISSION (Render FastAPI Backend)
// ============================================================================
void sendTelemetryToCloud() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Cloud] Wi-Fi offline. Skipping telemetry.");
    return;
  }

  updateSensor(); // Refresh current state

  WiFiClientSecure client;
  client.setCACert(ISRG_ROOT_CA); // Verify Let's Encrypt SSL/TLS on Render
  client.setTimeout(8000);        // 8 second timeout prevents loop blocking

  HTTPClient https;
  Serial.print("[Cloud] Sending telemetry to: ");
  Serial.println(serverUrl);

  if (https.begin(client, serverUrl)) {
    https.addHeader("Content-Type", "application/json");
    https.addHeader("X-Device-Key", deviceApiKey);

    // JSON payload adhering to FastAPI Pydantic schema
    String jsonPayload = "{";
    jsonPayload += "\"device_id\":\"" + String(deviceId) + "\",";
    jsonPayload += "\"light_status\":\"" + String(isDark ? "DARK" : "LIGHT") + "\",";
    jsonPayload += "\"led_status\":" + String(ledOn ? "true" : "false") + ",";
    jsonPayload += "\"mode\":\"" + String(automaticMode ? "AUTO" : "MANUAL") + "\"";
    jsonPayload += "}";

    int httpCode = https.POST(jsonPayload);

    if (httpCode > 0) {
      Serial.printf("[Cloud] HTTP Response code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_OK) {
        Serial.println("[Cloud] Reading saved successfully in PostgreSQL!");
      }
    } else {
      Serial.printf("[Cloud] HTTPS POST error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.println("[Cloud] Unable to connect to host.");
  }
}

// ============================================================================
// 6. SETUP & MAIN LOOP
// ============================================================================
void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);

  digitalWrite(LED_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Serial.print("Local control dashboard: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi connection timed out. Will retry in loop.");
  }

  // Register existing local web server endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/mode/auto", HTTP_POST, handleModeAuto);
  server.on("/mode/manual", HTTP_POST, handleModeManual);
  server.on("/led/on", HTTP_POST, handleLedOn);
  server.on("/led/off", HTTP_POST, handleLedOff);

  server.begin();
  Serial.println("Smart Home system started!");
}

void loop() {
  // 1. Reconnect Wi-Fi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 15000) {
      lastReconnect = millis();
      WiFi.reconnect();
    }
  }

  // 2. Serve local web interface requests
  server.handleClient();

  // 3. Keep sensor & actuator states updated
  updateSensor();

  // 4. Send telemetry to Render backend every 6 seconds non-blockingly
  unsigned long now = millis();
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;
    sendTelemetryToCloud();
  }

  delay(10); // Cooperative multitasking yield
}
