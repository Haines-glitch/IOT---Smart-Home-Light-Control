/**
 * Smart Home IoT Light Monitoring & Recommendation System
 * Frontend Controller
 */

let pollIntervalId = null;

// DOM Elements
const dbStatusBadge = document.getElementById("db-status-badge");
const systemStatusText = document.getElementById("system-status-text");
const recomIcon = document.getElementById("recom-icon");
const recomText = document.getElementById("recom-text");

const valLightStatus = document.getElementById("val-light-status");
const iconLight = document.getElementById("icon-light");
const valLedStatus = document.getElementById("val-led-status");
const iconLed = document.getElementById("icon-led");
const valMode = document.getElementById("val-mode");
const valDeviceId = document.getElementById("val-device-id");
const valTimestamp = document.getElementById("val-timestamp");

const limitSelect = document.getElementById("limit-select");
const autoRefreshSelect = document.getElementById("auto-refresh-select");
const btnRefresh = document.getElementById("btn-refresh");
const readingsTbody = document.getElementById("readings-tbody");

/**
 * Check API & Database Health
 */
async function checkHealth() {
  try {
    const res = await fetch("/health");
    if (!res.ok) throw new Error("Health check failed");
    const data = await res.json();
    if (data.status === "ok" && data.database === "connected") {
      dbStatusBadge.style.background = "rgba(16, 185, 129, 0.12)";
      dbStatusBadge.style.color = "var(--accent-emerald)";
      dbStatusBadge.style.borderColor = "rgba(16, 185, 129, 0.25)";
      systemStatusText.textContent = "Online & Database Connected";
    } else {
      dbStatusBadge.style.background = "rgba(244, 63, 94, 0.15)";
      dbStatusBadge.style.color = "var(--accent-rose)";
      dbStatusBadge.style.borderColor = "rgba(244, 63, 94, 0.3)";
      systemStatusText.textContent = `Degraded (${data.database})`;
    }
  } catch (err) {
    dbStatusBadge.style.background = "rgba(244, 63, 94, 0.15)";
    dbStatusBadge.style.color = "var(--accent-rose)";
    dbStatusBadge.style.borderColor = "rgba(244, 63, 94, 0.3)";
    systemStatusText.textContent = "Backend Offline";
  }
}

/**
 * Update UI Cards with the Latest Sensor Reading
 */
function updateLatestCards(reading) {
  if (!reading) return;

  // Ambient Light
  if (reading.light_status === "DARK") {
    valLightStatus.innerHTML = '<span class="badge-dark">DARK</span>';
    iconLight.textContent = "🌙";
  } else {
    valLightStatus.innerHTML = '<span class="badge-light">LIGHT</span>';
    iconLight.textContent = "☀️";
  }

  // LED State
  if (reading.led_status) {
    valLedStatus.innerHTML = '<span class="glow-pill glow-on">ON</span>';
    iconLed.textContent = "💡";
  } else {
    valLedStatus.innerHTML = '<span class="glow-pill glow-off">OFF</span>';
    iconLed.textContent = "⚫";
  }

  // Operating Mode
  if (reading.mode === "AUTO") {
    valMode.innerHTML = '<span class="badge-auto">AUTOMATIC</span>';
  } else {
    valMode.innerHTML = '<span class="badge-manual">MANUAL</span>';
  }

  // Device Info & Timestamp
  valDeviceId.textContent = reading.device_id || "ESP32";
  const dateObj = new Date(reading.timestamp);
  valTimestamp.textContent = `Last sync: ${dateObj.toLocaleTimeString()} (${dateObj.toLocaleDateString()})`;

  // Recommendation Engine
  generateRecommendation(reading.light_status, reading.led_status, reading.mode);
}

/**
 * Dynamic Recommendation Engine Logic
 */
function generateRecommendation(lightStatus, ledStatus, mode) {
  if (lightStatus === "LIGHT" && ledStatus === true) {
    recomIcon.textContent = "⚠️";
    recomText.innerHTML = "<strong>Energy Waste Alert:</strong> High ambient daylight is detected while the LED is powered ON. <em>Recommendation: Turn off the LED or switch to AUTO mode to conserve electrical power.</em>";
  } else if (lightStatus === "DARK" && ledStatus === true) {
    recomIcon.textContent = "✨";
    recomText.innerHTML = "<strong>Illumination Active:</strong> Ambient darkness detected. The smart lighting system is actively providing illumination for safe navigation.";
  } else if (lightStatus === "DARK" && ledStatus === false) {
    recomIcon.textContent = "💡";
    recomText.innerHTML = "<strong>Low Light Warning:</strong> The room is dark and the light is OFF. <em>Recommendation: Turn ON the light or switch mode to AUTO for comfortable visibility.</em>";
  } else {
    recomIcon.textContent = "🌿";
    recomText.innerHTML = "<strong>Optimal Energy Conservation:</strong> Ample natural daylight is available. The LED remains OFF, keeping power consumption at 0W.";
  }
}

/**
 * Fetch Latest Reading and History
 */
async function fetchTelemetry() {
  const limit = limitSelect.value || 50;

  try {
    // 1. Fetch latest reading
    const latestRes = await fetch("/api/latest");
    if (latestRes.ok) {
      const latestData = await latestRes.json();
      updateLatestCards(latestData);
    }

    // 2. Fetch history
    const historyRes = await fetch(`/api/readings?limit=${limit}`);
    if (historyRes.ok) {
      const historyData = await historyRes.json();
      renderHistoryTable(historyData);
    }
  } catch (err) {
    console.error("Telemetry sync error:", err);
  }
}

/**
 * Render Telemetry History in Table
 */
function renderHistoryTable(readings) {
  if (!readings || readings.length === 0) {
    readingsTbody.innerHTML = `
      <tr>
        <td colspan="6" style="text-align: center; color: var(--text-muted); padding: 24px;">
          No sensor readings found in database yet.
        </td>
      </tr>
    `;
    return;
  }

  readingsTbody.innerHTML = readings.map(r => {
    const formattedDate = new Date(r.timestamp).toLocaleString();
    const lightBadge = r.light_status === "DARK" 
      ? '<span class="badge-dark">🌙 DARK</span>' 
      : '<span class="badge-light">☀️ LIGHT</span>';
    const ledBadge = r.led_status 
      ? '<span class="glow-pill glow-on">ON</span>' 
      : '<span class="glow-pill glow-off">OFF</span>';
    const modeBadge = r.mode === "AUTO"
      ? '<span class="badge-auto">AUTO</span>'
      : '<span class="badge-manual">MANUAL</span>';

    return `
      <tr>
        <td>#${r.id}</td>
        <td><code>${escapeHtml(r.device_id)}</code></td>
        <td>${formattedDate}</td>
        <td>${lightBadge}</td>
        <td>${ledBadge}</td>
        <td>${modeBadge}</td>
      </tr>
    `;
  }).join("");
}

function escapeHtml(str) {
  if (!str) return "";
  return str.replace(/[&<>"']/g, function(m) {
    return {
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      '"': '&quot;',
      "'": '&#039;'
    }[m];
  });
}

/**
 * Manage Polling
 */
function setPolling(intervalMs) {
  if (pollIntervalId) {
    clearInterval(pollIntervalId);
    pollIntervalId = null;
  }

  const ms = parseInt(intervalMs, 10);
  if (ms > 0) {
    pollIntervalId = setInterval(() => {
      fetchTelemetry();
      checkHealth();
    }, ms);
  }
}

// Event Listeners
btnRefresh.addEventListener("click", () => {
  btnRefresh.disabled = true;
  btnRefresh.textContent = "Syncing...";
  Promise.all([fetchTelemetry(), checkHealth()]).finally(() => {
    btnRefresh.disabled = false;
    btnRefresh.textContent = "↻ Refresh Now";
  });
});

limitSelect.addEventListener("change", () => {
  fetchTelemetry();
});

autoRefreshSelect.addEventListener("change", (e) => {
  setPolling(e.target.value);
});

// Initialization
document.addEventListener("DOMContentLoaded", () => {
  checkHealth();
  fetchTelemetry();
  setPolling(autoRefreshSelect.value || 5000);
});
