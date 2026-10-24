#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>

// =====================================================
// 1. WIFI AND CLOUD CONFIGURATION
// =====================================================

const char* ssid = "realme 9";
const char* password = "12345678";

const char* serverUrl =
  "https://iot-smart-home-light-control.onrender.com/api/readings";

// Must match DEVICE_API_KEY configured in Render.
const char* deviceApiKey = "my-smart-home-secret-2026";
const char* deviceId = "esp32-smart-home-01";

// Official ISRG Root X1 certificate PEM (Let's Encrypt Root CA)
const char* ROOT_CA = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";

// Telemetry interval (10 seconds)
const unsigned long telemetryInterval = 10000;
unsigned long lastTelemetryTime = 0;

// Wi-Fi retry interval
const unsigned long reconnectInterval = 10000;
unsigned long lastReconnectTime = 0;

// =====================================================
// 2. HARDWARE PINS
// =====================================================

const int LDR_PIN = 27;  // HIGH = dark, LOW = light
const int LED_PIN = 23;  // External LED with 220-ohm resistor

WebServer server(80);

// =====================================================
// 3. CONTROL STATE
// =====================================================

bool automaticMode = true;
bool isDark = false;
bool ledOn = false;

void updateSensor() {
  isDark = (digitalRead(LDR_PIN) == HIGH);

  if (automaticMode) {
    ledOn = isDark;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  }
}

// =====================================================
// 4. LOCAL WEB DASHBOARD
// =====================================================

void handleRoot() {
  String page = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Home Light Control</title>
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
  box-sizing: border-box;
  padding: 13px;
  margin: 8px;
  border: none;
  border-radius: 9px;
  font-size: 16px;
}
select, button { width: 85%; }
select { background: #374151; color: white; }
button { color: white; cursor: pointer; }
button:hover { opacity: 0.85; }
.on { background: #16a34a; }
.off { background: #dc2626; }
.status {
  padding: 15px;
  border-radius: 10px;
  margin: 18px 0;
  font-weight: bold;
}
.bright { background: #14532d; color: #86efac; }
.dark { background: #713f12; color: #fde68a; }
.note { color: #cbd5e1; font-size: 14px; line-height: 1.5; }
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

  <div id="sensorStatus" class="status">Reading sensor...</div>
  <p id="recommendation">Please wait...</p>
  <h3 id="ledStatus">LED: --</h3>

  <div id="manualControls" class="hidden">
    <h3>Manual LED Control</h3>
    <button class="on" onclick="controlLED('on')">LIGHT ON</button>
    <button class="off" onclick="controlLED('off')">LIGHT OFF</button>
  </div>

  <p id="message" class="note">Connecting...</p>
  <p class="note">Refreshes every second. Cloud data is transmitted separately.</p>
</div>

<script>
let busy = false;

async function postCommand(url) {
  const response = await fetch(url, { method: 'POST' });
  if (!response.ok) {
    throw new Error('HTTP ' + response.status);
  }
}

async function changeMode() {
  if (busy) return;
  busy = true;

  const mode = document.getElementById('mode').value;
  document.getElementById('message').textContent = 'Changing mode...';

  try {
    await postCommand('/mode/' + mode);
    await updateDashboard();
    document.getElementById('message').textContent = 'Control mode updated';
  } catch (error) {
    document.getElementById('message').textContent =
      'Could not change mode: ' + error.message;
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
    document.getElementById('message').textContent = 'Manual command completed';
  } catch (error) {
    document.getElementById('message').textContent =
      'Could not control LED: ' + error.message;
  } finally {
    busy = false;
  }
}

async function updateDashboard() {
  try {
    const response = await fetch('/status', { cache: 'no-store' });
    if (!response.ok) throw new Error('Status request failed');

    const data = await response.json();
    const mode = data.automatic ? 'auto' : 'manual';

    document.getElementById('mode').value = mode;
    document.getElementById('manualControls').className =
      data.automatic ? 'hidden' : '';

    const status = document.getElementById('sensorStatus');
    status.textContent = data.dark ? 'Darkness detected' : 'Light detected';
    status.className = 'status ' + (data.dark ? 'dark' : 'bright');

    document.getElementById('recommendation').textContent =
      data.dark
        ? 'Recommendation: Consider turning the light ON.'
        : 'Recommendation: No need to turn on the light.';

    document.getElementById('ledStatus').textContent =
      'LED: ' + (data.ledOn ? 'ON' : 'OFF');

    document.getElementById('message').textContent = 'ESP32 connected';
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

// =====================================================
// 5. LOCAL CONTROL API
// =====================================================

void handleModeAuto() {
  automaticMode = true;
  updateSensor();
  Serial.println("[Control] Automatic mode");
  server.send(200, "text/plain", "Automatic mode");
}

void handleModeManual() {
  automaticMode = false;
  Serial.println("[Control] Manual mode");
  server.send(200, "text/plain", "Manual mode");
}

void handleLedOn() {
  if (automaticMode) {
    server.send(409, "text/plain", "Switch to Manual mode first");
    return;
  }

  ledOn = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("[Control] Manual LED ON");
  server.send(200, "text/plain", "LED ON");
}

void handleLedOff() {
  if (automaticMode) {
    server.send(409, "text/plain", "Switch to Manual mode first");
    return;
  }

  ledOn = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("[Control] Manual LED OFF");
  server.send(200, "text/plain", "LED OFF");
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

// =====================================================
// 6. WIFI CONNECTION MANAGEMENT
// =====================================================

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[Wi-Fi] Connecting");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[Wi-Fi] Connected!");
    Serial.print("[Wi-Fi] IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[Wi-Fi] Connection timed out. Will retry.");
  }
}

// =====================================================
// 7. CLOUD TELEMETRY
// =====================================================

void sendTelemetryToCloud() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Cloud] Wi-Fi offline. Skipping telemetry.");
    return;
  }

  updateSensor();

  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setTimeout(8000);

  HTTPClient https;
  https.setConnectTimeout(10000);
  https.setTimeout(10000);

  Serial.println("[Cloud] Sending sensor reading...");

  if (!https.begin(client, serverUrl)) {
    Serial.println("[Cloud] HTTPS initialization failed.");
    return;
  }

  https.addHeader("Content-Type", "application/json");
  https.addHeader("X-Device-Key", deviceApiKey);

  String payload = "{";
  payload += "\"device_id\":\"" + String(deviceId) + "\",";
  payload += "\"light_status\":\"" + String(isDark ? "DARK" : "LIGHT") + "\",";
  payload += "\"led_status\":";
  payload += ledOn ? "true" : "false";
  payload += ",\"mode\":\"";
  payload += automaticMode ? "AUTO" : "MANUAL";
  payload += "\"}";

  int httpCode = https.POST(payload);

  if (httpCode > 0) {
    Serial.printf("[Cloud] HTTP response: %d\n", httpCode);

    String responseBody = https.getString();
    if (httpCode == HTTP_CODE_OK ||
        httpCode == HTTP_CODE_CREATED) {
      Serial.println("[Cloud] Reading submitted successfully.");
    } else {
      Serial.println("[Cloud] Backend response:");
      Serial.println(responseBody);
    }
  } else {
    Serial.printf("[Cloud] HTTPS error: %s\n",
                  https.errorToString(httpCode).c_str());
    Serial.println("[Cloud] Check TLS certificate, URL and network.");
  }

  https.end();
}

// =====================================================
// 8. SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Set ESP32 clock via NTP so TLS certificate validity dates can be checked accurately.
  configTime(0, 0, "pool.ntp.org", "time.google.com");

  connectWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/mode/auto", HTTP_POST, handleModeAuto);
  server.on("/mode/manual", HTTP_POST, handleModeManual);
  server.on("/led/on", HTTP_POST, handleLedOn);
  server.on("/led/off", HTTP_POST, handleLedOff);

  server.begin();
  Serial.println("[System] Local web server started.");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[System] Open dashboard: http://");
    Serial.println(WiFi.localIP());
  }
}

// =====================================================
// 9. MAIN LOOP
// =====================================================

void loop() {
  server.handleClient();

  // Reconnect after a Wi-Fi disconnection.
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectTime >= reconnectInterval) {
      lastReconnectTime = millis();
      Serial.println("[Wi-Fi] Disconnected. Reconnecting...");
      WiFi.reconnect();
    }
  }

  updateSensor();

  // Send telemetry every 10 seconds.
  unsigned long now = millis();

  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;
    sendTelemetryToCloud();
  }

  delay(5);
}
