import logging
from typing import List, Optional
from fastapi import APIRouter, Depends, HTTPException, Query, status
from sqlalchemy.orm import Session
from sqlalchemy.exc import SQLAlchemyError

from app.database import get_db
from app.models import SensorReading, DeviceControl
from app.schemas import ReadingCreate, ReadingResponse, ControlCommand, ControlResponse
from app.security import verify_device_key

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/api", tags=["Sensor Readings & Control"])


def get_or_create_control_state(db: Session, device_id: str = "esp32-smart-home-01") -> DeviceControl:
    """Helper to fetch or initialize the device control state."""
    control = db.query(DeviceControl).filter_by(device_id=device_id).first()
    if not control:
        control = DeviceControl(device_id=device_id, mode="AUTO", led_status=False)
        db.add(control)
        db.commit()
        db.refresh(control)
    return control


@router.post(
    "/readings",
    response_model=ReadingResponse,
    status_code=status.HTTP_201_CREATED,
    summary="Receive and store a new sensor reading from the ESP32",
    description="Validates incoming sensor data, saves to PostgreSQL, and returns current desired control state (target_mode, target_led)."
)
def create_reading(
    reading_in: ReadingCreate,
    db: Session = Depends(get_db),
    _key: str = Depends(verify_device_key)
):
    """
    Store sensor reading in PostgreSQL.
    Guarded by X-Device-Key header.
    Returns the latest target control state so the ESP32 knows whether to switch mode or toggle LED.
    """
    try:
        new_reading = SensorReading(
            device_id=reading_in.device_id,
            light_status=reading_in.light_status,
            led_status=reading_in.led_status,
            mode=reading_in.mode
        )
        db.add(new_reading)
        db.commit()
        db.refresh(new_reading)

        # Retrieve current target control state
        ctrl = get_or_create_control_state(db, reading_in.device_id)

        # Build response with target commands
        return ReadingResponse(
            id=new_reading.id,
            device_id=new_reading.device_id,
            timestamp=new_reading.timestamp,
            light_status=new_reading.light_status,
            led_status=new_reading.led_status,
            mode=new_reading.mode,
            target_mode=ctrl.mode,
            target_led=ctrl.led_status
        )
    except SQLAlchemyError as exc:
        db.rollback()
        logger.error(f"Database error while saving sensor reading: {exc}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Database persistence error. Could not record reading."
        )


@router.get(
    "/latest",
    response_model=ReadingResponse,
    summary="Get the most recent sensor reading",
    description="Returns the latest recorded reading across all devices or filtered by device_id."
)
def get_latest_reading(
    device_id: Optional[str] = Query(None, description="Filter latest by specific device ID"),
    db: Session = Depends(get_db)
):
    """Fetch the single most recent sensor reading."""
    query = db.query(SensorReading)
    if device_id:
        query = query.filter(SensorReading.device_id == device_id)

    latest = query.order_by(SensorReading.timestamp.desc(), SensorReading.id.desc()).first()

    if not latest:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="No sensor readings found."
        )

    # Attach current target mode/led
    target_device = device_id or latest.device_id
    ctrl = db.query(DeviceControl).filter_by(device_id=target_device).first()

    return ReadingResponse(
        id=latest.id,
        device_id=latest.device_id,
        timestamp=latest.timestamp,
        light_status=latest.light_status,
        led_status=latest.led_status,
        mode=latest.mode,
        target_mode=ctrl.mode if ctrl else latest.mode,
        target_led=ctrl.led_status if ctrl else latest.led_status
    )


@router.get(
    "/readings",
    response_model=List[ReadingResponse],
    summary="Get recent sensor readings history",
    description="Returns readings ordered by newest timestamp first. Query parameter 'limit' prevents excessive queries."
)
def get_readings_history(
    limit: int = Query(default=50, ge=1, le=1000, description="Number of readings to fetch (1-1000)"),
    device_id: Optional[str] = Query(None, description="Optional filter by device ID"),
    db: Session = Depends(get_db)
):
    """Fetch recent readings in descending timestamp order."""
    query = db.query(SensorReading)
    if device_id:
        query = query.filter(SensorReading.device_id == device_id)

    readings = query.order_by(SensorReading.timestamp.desc(), SensorReading.id.desc()).limit(limit).all()
    return readings


@router.get(
    "/control",
    response_model=ControlResponse,
    summary="Get current control state",
    description="Returns current operating mode (AUTO/MANUAL) and desired manual LED status."
)
def get_control_state(
    device_id: str = Query("esp32-smart-home-01", description="Device ID to query"),
    db: Session = Depends(get_db)
):
    """Fetch current control state."""
    control = get_or_create_control_state(db, device_id)
    return control


@router.post(
    "/control",
    response_model=ControlResponse,
    summary="Update device control mode or manual LED state",
    description="Allows the web dashboard to set operating mode (AUTO/MANUAL) and toggle the LED (ON/OFF) in MANUAL mode."
)
def set_control_state(
    cmd: ControlCommand,
    device_id: str = Query("esp32-smart-home-01", description="Device ID to update"),
    db: Session = Depends(get_db)
):
    """Set operating mode or manual LED status."""
    control = get_or_create_control_state(db, device_id)

    if cmd.mode is not None:
        control.mode = cmd.mode

    if cmd.led_status is not None:
        control.led_status = cmd.led_status

    db.commit()
    db.refresh(control)
    return control
