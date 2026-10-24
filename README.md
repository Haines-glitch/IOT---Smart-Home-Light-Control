# IoT-Based Smart Home Light Monitoring and Recommendation System

A production-ready IoT cloud solution integrating an **ESP32 microcontroller**, a **FastAPI backend**, a **Render PostgreSQL database**, and an **interactive real-time web dashboard**.

---

## 🌟 Key Architecture & Flow

```
┌─────────────────────────┐
│     ESP32 Node          │
│ - LDR Sensor (GPIO 27)  │
│ - Smart LED  (GPIO 23)  │
│ - Auto/Manual Modes     │
└────────────┬────────────┘
             │ HTTPS POST (every 6s) with 'X-Device-Key' header
             ▼
┌────────────────────────────────────────────────────────┐
│             Render Docker Web Service                  │
│  FastAPI Backend (Uvicorn on 0.0.0.0:$PORT)            │
│  ├── POST /api/readings  (Protected write endpoint)    │
│  ├── GET  /api/latest    (Current telemetry)           │
│  ├── GET  /api/readings  (Paginated history log)       │
│  ├── GET  /health        (Health & DB connectivity)    │
│  └── GET  /              (Static Real-time Dashboard)  │
└────────────┬───────────────────────────────────────────┘
             │ SQLAlchemy ORM (Parameterized queries)
             ▼
┌────────────────────────────────────────────────────────┐
│           Render Managed PostgreSQL Database           │
│  Table: sensor_readings                                │
│  (id, device_id, timestamp, light_status, led, mode)   │
└────────────────────────────────────────────────────────┘
```

---

## 📁 Repository Structure

```
├── app/
│   ├── __init__.py          # Application package declaration
│   ├── config.py            # Settings with Render postgres:// URL normalization
│   ├── database.py          # SQLAlchemy engine, session maker, get_db dependency
│   ├── models.py            # SensorReading SQLAlchemy ORM table model
│   ├── schemas.py           # Pydantic validation schemas (ReadingCreate, Response, Health)
│   ├── security.py          # X-Device-Key authentication with constant-time verification
│   ├── main.py              # FastAPI app instance, CORS, routes & dashboard mounting
│   ├── routes/
│   │   ├── __init__.py
│   │   ├── readings.py      # POST /api/readings, GET /api/latest, GET /api/readings
│   │   └── health.py        # GET /health with database connectivity check
│   └── static/
│       ├── index.html       # Smart home real-time dashboard UI
│       ├── style.css        # Responsive dark-mode styling
│       └── app.js           # Live polling, telemetry updates & recommendation engine
├── esp32/
│   ├── esp32_smart_home.ino # Complete ESP32 Arduino sketch with TLS & local web UI
│   └── README.md            # Hardware wiring, pinout & flashing guide
├── tests/
│   ├── __init__.py
│   ├── conftest.py          # Test fixture with isolated in-memory SQLite (no prod risk)
│   └── test_api.py          # Complete pytest suite (13 test cases)
├── .env.example             # Template for local & production environment variables
├── .gitignore               # Excludes secrets, caches, and local virtualenvs
├── .dockerignore            # Excludes non-essential files from Docker image
├── Dockerfile               # Production Docker container with dynamic $PORT for Render
├── requirements.txt         # Python package dependencies
└── README.md                # Project documentation & deployment guide
```

---

## 🚀 Local Development Setup

### 1. Prerequisites
- Python 3.11+ (or 3.12+)
- Git

### 2. Setup Virtual Environment & Install Dependencies
```bash
# Create virtual environment
python -m venv venv

# Activate on Windows:
venv\Scripts\activate
# (On macOS/Linux: source venv/bin/activate)

# Install dependencies
pip install -r requirements.txt
```

### 3. Configure Local Environment
Copy the example environment file:
```bash
cp .env.example .env
```
Inside `.env`, configure:
```env
DATABASE_URL=sqlite:///./sensor_data.db
DEVICE_API_KEY=my-super-secret-key-12345
PORT=8000
```
*(If you have a local PostgreSQL instance, you can use `postgresql://user:pass@localhost:5432/dbname` instead).*

### 4. Run the Application
```bash
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```
- **Web Dashboard**: Open [http://localhost:8000](http://localhost:8000)
- **Interactive API Documentation (Swagger)**: Open [http://localhost:8000/docs](http://localhost:8000/docs)
- **API Health Check**: [http://localhost:8000/health](http://localhost:8000/health)

---

## 🧪 Automated Testing

The automated test suite uses `pytest` and FastAPI's `TestClient` with an in-memory SQLite database. **Tests never touch or modify production data.**

To execute all tests:
```bash
pytest -v
```

### Covered Test Cases:
1. `test_health_check`: Verifies `GET /health` returns status `ok` and `database: connected`.
2. `test_submit_valid_reading`: Validates that valid JSON with `X-Device-Key` is persisted with HTTP 201.
3. `test_submit_reading_missing_api_key`: Rejects submissions missing the `X-Device-Key` header with HTTP 401.
4. `test_submit_reading_invalid_api_key`: Rejects unauthorized keys with HTTP 401.
5. `test_submit_reading_invalid_light_status`: Rejects invalid sensor strings (not `LIGHT` or `DARK`) with HTTP 422.
6. `test_submit_reading_invalid_mode`: Rejects non-conforming modes with HTTP 422.
7. `test_submit_reading_missing_field`: Verifies Pydantic catches missing required payload fields (HTTP 422).
8. `test_get_latest_reading_when_empty`: Gracefully returns HTTP 404 when no records exist.
9. `test_get_latest_reading_success`: Successfully fetches the most recently timestamped reading.
10. `test_get_readings_history`: Confirms descending historical order and pagination limits.
11. `test_get_readings_limit_validation`: Enforces limits between 1 and 1000 records.
12. `test_database_persistence`: Direct ORM session verification of saved fields and timestamps.
13. `test_root_serves_dashboard`: Confirms `GET /` serves the HTML smart home dashboard.

---

## ☁️ Render Deployment Guide

Follow these exact steps to deploy both the PostgreSQL database and the Docker Web Service on [Render](https://render.com):

### Step 1: Create a PostgreSQL Database on Render
1. Log in to your [Render Dashboard](https://dashboard.render.com/).
2. Click **New +** and select **PostgreSQL**.
3. Fill in the database details:
   - **Name**: `smart-home-db`
   - **Database**: `smart_home`
   - **User**: `smart_home_user`
   - **Region**: Choose the region closest to you (e.g., Oregon, Frankfurt, Singapore).
   - **Plan**: Select **Free**.
4. Click **Create Database**.
5. Once provisioned, locate the **Connections** section on the database dashboard:
   - If deploying your backend in the same Render region, copy the **Internal Database URL** (recommended for lower latency and security).
   - Otherwise, copy the **External Database URL**.
   *(Note: FastAPI automatically normalizes `postgres://` to `postgresql://`).*

### Step 2: Create a Docker Web Service on Render
1. In the Render Dashboard, click **New +** and select **Web Service**.
2. Connect your Git repository containing this project.
3. Configure the Web Service:
   - **Name**: `smart-home-backend` (or your preferred name).
   - **Region**: Select the **same region** as your PostgreSQL database.
   - **Branch**: `main` (or your active branch).
   - **Runtime**: **Docker**.
   - **Root Directory**: Leave blank (defaults to root).
   - **Dockerfile Path**: `Dockerfile` (or leave default `./Dockerfile`).
   - **Instance Type**: Select **Free**.

### Step 3: Configure Environment Variables on Render
Scroll down to the **Environment Variables** section and add:

| Key | Value | Description |
| :--- | :--- | :--- |
| `DATABASE_URL` | `postgresql://...` | Paste your copied Render PostgreSQL database URL |
| `DEVICE_API_KEY` | `your-secure-random-32char-key` | A secret shared key for ESP32 authorization |

*(Note: Render will automatically set `PORT`. The `Dockerfile` dynamically binds Uvicorn to `0.0.0.0:$PORT`).*

Click **Create Web Service**.

### Step 4: Verify the Deployed Backend
Once deployment completes, Render will provide a public URL (e.g., `https://smart-home-backend.onrender.com`).

1. **Verify Health & Database Connectivity:**
   ```bash
   curl -s https://YOUR-APP.onrender.com/health
   ```
   Expected response:
   ```json
   {
     "status": "ok",
     "database": "connected",
     "timestamp": "...",
     "version": "1.0.0"
   }
   ```

2. **Test Submitting a Reading:**
   ```bash
   curl -X POST https://YOUR-APP.onrender.com/api/readings \
     -H "Content-Type: application/json" \
     -H "X-Device-Key: YOUR_CONFIGURED_DEVICE_API_KEY" \
     -d '{"device_id":"test-client","light_status":"DARK","led_status":true,"mode":"AUTO"}'
   ```

3. **Verify Latest Reading:**
   ```bash
   curl -s https://YOUR-APP.onrender.com/api/latest
   ```

4. **View Web Dashboard:**
   Open `https://YOUR-APP.onrender.com/` in any desktop or mobile browser to view live telemetry and recommendations!

---

## 🔌 Connecting the ESP32 to the Deployed Backend

1. Open [`esp32/esp32_smart_home.ino`](file:///c:/Users/haine/OneDrive/Desktop/Documents/IOT%20Project/esp32/esp32_smart_home.ino) in Arduino IDE.
2. Update the credentials on lines 33–42:
   ```cpp
   const char* WIFI_SSID     = "Your_WiFi_Network";
   const char* WIFI_PASSWORD = "Your_WiFi_Password";
   const char* SERVER_URL    = "https://YOUR-APP.onrender.com/api/readings";
   const char* DEVICE_KEY    = "YOUR_CONFIGURED_DEVICE_API_KEY";
   ```
3. Connect your ESP32 via USB and upload the sketch.
4. Open the Serial Monitor at `115200 baud`.
5. The ESP32 will connect to Wi-Fi and transmit readings every 6 seconds.
6. The web dashboard will automatically display live readings and recommendations!
