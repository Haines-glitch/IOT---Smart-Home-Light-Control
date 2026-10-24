from datetime import datetime
from typing import Literal, Optional
from pydantic import BaseModel, Field, ConfigDict


class ReadingCreate(BaseModel):
    """Schema for validating incoming sensor readings from the ESP32."""
    device_id: str = Field(..., min_length=1, max_length=100, description="Device identifier", examples=["esp32-smart-home-01"])
    light_status: Literal["LIGHT", "DARK"] = Field(..., description="Detected light condition: LIGHT or DARK")
    led_status: bool = Field(..., description="LED state: true for ON, false for OFF")
    mode: Literal["AUTO", "MANUAL"] = Field(..., description="Operating mode: AUTO or MANUAL")

    model_config = ConfigDict(
        json_schema_extra={
            "example": {
                "device_id": "esp32-smart-home-01",
                "light_status": "DARK",
                "led_status": True,
                "mode": "AUTO"
            }
        }
    )


class ReadingResponse(BaseModel):
    """Schema for returning sensor reading records."""
    id: int
    device_id: str
    timestamp: datetime
    light_status: str
    led_status: bool
    mode: str

    model_config = ConfigDict(from_attributes=True)


class HealthResponse(BaseModel):
    """Schema for health-check endpoint response."""
    status: str = Field(..., examples=["ok"])
    database: str = Field(..., examples=["connected"])
    timestamp: datetime
    version: Optional[str] = "1.0.0"
