from sqlalchemy import Column, Integer, String, Boolean, DateTime, func
from app.database import Base


class SensorReading(Base):
    """
    SQLAlchemy model representing sensor readings sent from the ESP32.
    """
    __tablename__ = "sensor_readings"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    device_id = Column(String(100), nullable=False, index=True)
    timestamp = Column(
        DateTime(timezone=True),
        server_default=func.now(),
        nullable=False,
        index=True
    )
    light_status = Column(String(10), nullable=False)  # 'LIGHT' or 'DARK'
    led_status = Column(Boolean, nullable=False)       # True for ON, False for OFF
    mode = Column(String(10), nullable=False)          # 'AUTO' or 'MANUAL'

    def __repr__(self):
        return f"<SensorReading(id={self.id}, device={self.device_id}, light={self.light_status}, led={self.led_status}, mode={self.mode})>"


class DeviceControl(Base):
    """
    SQLAlchemy model representing desired control state (AUTO/MANUAL and manual LED on/off).
    Allows commands from web dashboard to be dispatched to the ESP32 node.
    """
    __tablename__ = "device_controls"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    device_id = Column(String(100), default="esp32-smart-home-01", unique=True, index=True)
    mode = Column(String(10), default="AUTO", nullable=False)       # 'AUTO' or 'MANUAL'
    led_status = Column(Boolean, default=False, nullable=False)    # Desired manual LED state
    updated_at = Column(
        DateTime(timezone=True),
        server_default=func.now(),
        onupdate=func.now(),
        nullable=False
    )

    def __repr__(self):
        return f"<DeviceControl(device={self.device_id}, mode={self.mode}, led={self.led_status})>"
