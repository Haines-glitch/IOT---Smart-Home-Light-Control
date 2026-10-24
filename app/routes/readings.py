import logging
from typing import List, Optional
from fastapi import APIRouter, Depends, HTTPException, Query, status
from sqlalchemy.orm import Session
from sqlalchemy.exc import SQLAlchemyError

from app.database import get_db
from app.models import SensorReading
from app.schemas import ReadingCreate, ReadingResponse
from app.security import verify_device_key

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/api", tags=["Sensor Readings"])


@router.post(
    "/readings",
    response_model=ReadingResponse,
    status_code=status.HTTP_201_CREATED,
    summary="Receive and store a new sensor reading from the ESP32",
    description="Validates incoming sensor data and saves it to PostgreSQL with a server-side timestamp. Requires X-Device-Key header."
)
def create_reading(
    reading_in: ReadingCreate,
    db: Session = Depends(get_db),
    _key: str = Depends(verify_device_key)
):
    """
    Store sensor reading in PostgreSQL.
    Guarded by X-Device-Key header.
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
        return new_reading
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
    return latest


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
