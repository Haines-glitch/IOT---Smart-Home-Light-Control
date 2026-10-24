import pytest
from app.models import SensorReading


def test_health_check(client):
    """Test GET /health returns 200 with API status ok and database connected."""
    response = client.get("/health")
    assert response.status_code == 200
    data = response.json()
    assert data["status"] == "ok"
    assert data["database"] == "connected"
    assert "timestamp" in data
    assert data["version"] == "1.0.0"


def test_submit_valid_reading(client, valid_api_key):
    """Test POST /api/readings saves valid sensor reading with API key."""
    payload = {
        "device_id": "esp32-smart-home-01",
        "light_status": "DARK",
        "led_status": True,
        "mode": "AUTO"
    }
    headers = {"X-Device-Key": valid_api_key}

    response = client.post("/api/readings", json=payload, headers=headers)
    assert response.status_code == 201
    data = response.json()

    assert data["id"] is not None
    assert data["device_id"] == "esp32-smart-home-01"
    assert data["light_status"] == "DARK"
    assert data["led_status"] is True
    assert data["mode"] == "AUTO"
    assert "timestamp" in data


def test_submit_reading_missing_api_key(client):
    """Test POST /api/readings without X-Device-Key header is rejected with 401."""
    payload = {
        "device_id": "esp32-smart-home-01",
        "light_status": "DARK",
        "led_status": True,
        "mode": "AUTO"
    }
    response = client.post("/api/readings", json=payload)
    assert response.status_code == 401
    assert "Missing 'X-Device-Key'" in response.json()["detail"]


def test_submit_reading_invalid_api_key(client):
    """Test POST /api/readings with incorrect X-Device-Key header is rejected with 401."""
    payload = {
        "device_id": "esp32-smart-home-01",
        "light_status": "DARK",
        "led_status": True,
        "mode": "AUTO"
    }
    headers = {"X-Device-Key": "incorrect-secret-key"}
    response = client.post("/api/readings", json=payload, headers=headers)
    assert response.status_code == 401
    assert "Invalid 'X-Device-Key'" in response.json()["detail"]


def test_submit_reading_invalid_light_status(client, valid_api_key):
    """Test POST /api/readings with invalid light_status fails Pydantic validation (422)."""
    payload = {
        "device_id": "esp32-smart-home-01",
        "light_status": "DIM",  # Invalid: must be LIGHT or DARK
        "led_status": True,
        "mode": "AUTO"
    }
    headers = {"X-Device-Key": valid_api_key}
    response = client.post("/api/readings", json=payload, headers=headers)
    assert response.status_code == 422


def test_submit_reading_invalid_mode(client, valid_api_key):
    """Test POST /api/readings with invalid mode fails Pydantic validation (422)."""
    payload = {
        "device_id": "esp32-smart-home-01",
        "light_status": "LIGHT",
        "led_status": False,
        "mode": "INVALID_MODE"  # Invalid: must be AUTO or MANUAL
    }
    headers = {"X-Device-Key": valid_api_key}
    response = client.post("/api/readings", json=payload, headers=headers)
    assert response.status_code == 422


def test_submit_reading_missing_field(client, valid_api_key):
    """Test POST /api/readings with missing field fails Pydantic validation (422)."""
    payload = {
        "device_id": "esp32-smart-home-01",
        "light_status": "LIGHT"
        # missing led_status and mode
    }
    headers = {"X-Device-Key": valid_api_key}
    response = client.post("/api/readings", json=payload, headers=headers)
    assert response.status_code == 422


def test_get_latest_reading_when_empty(client):
    """Test GET /api/latest returns 404 when no readings exist."""
    response = client.get("/api/latest")
    assert response.status_code == 404


def test_get_latest_reading_success(client, valid_api_key):
    """Test GET /api/latest returns the most recent reading."""
    headers = {"X-Device-Key": valid_api_key}

    # Post reading 1
    client.post("/api/readings", json={
        "device_id": "esp32-smart-home-01",
        "light_status": "LIGHT",
        "led_status": False,
        "mode": "AUTO"
    }, headers=headers)

    # Post reading 2
    client.post("/api/readings", json={
        "device_id": "esp32-smart-home-01",
        "light_status": "DARK",
        "led_status": True,
        "mode": "MANUAL"
    }, headers=headers)

    response = client.get("/api/latest")
    assert response.status_code == 200
    data = response.json()
    assert data["light_status"] == "DARK"
    assert data["led_status"] is True
    assert data["mode"] == "MANUAL"


def test_get_readings_history(client, valid_api_key):
    """Test GET /api/readings returns descending historical entries."""
    headers = {"X-Device-Key": valid_api_key}

    for i in range(3):
        client.post("/api/readings", json={
            "device_id": f"device-{i}",
            "light_status": "LIGHT" if i % 2 == 0 else "DARK",
            "led_status": i % 2 != 0,
            "mode": "AUTO"
        }, headers=headers)

    response = client.get("/api/readings?limit=2")
    assert response.status_code == 200
    readings = response.json()
    assert len(readings) == 2


def test_get_readings_limit_validation(client):
    """Test GET /api/readings rejects invalid limit parameters."""
    # Limit < 1
    resp_zero = client.get("/api/readings?limit=0")
    assert resp_zero.status_code == 422

    # Limit > 1000
    resp_excess = client.get("/api/readings?limit=1500")
    assert resp_excess.status_code == 422


def test_database_persistence(client, valid_api_key, db_session):
    """Test that saved readings are genuinely persisted in the database session."""
    headers = {"X-Device-Key": valid_api_key}
    client.post("/api/readings", json={
        "device_id": "esp32-test-persist",
        "light_status": "LIGHT",
        "led_status": False,
        "mode": "AUTO"
    }, headers=headers)

    record = db_session.query(SensorReading).filter_by(device_id="esp32-test-persist").first()
    assert record is not None
    assert record.light_status == "LIGHT"
    assert record.led_status is False
    assert record.mode == "AUTO"
    assert record.timestamp is not None


def test_root_serves_dashboard(client):
    """Test GET / serves the HTML dashboard."""
    response = client.get("/")
    assert response.status_code == 200
    assert "text/html" in response.headers.get("content-type", "")
    assert "Smart Home Light Monitoring" in response.text
