import os
from functools import lru_cache
from typing import Optional
from dotenv import load_dotenv

# Load .env file if available
load_dotenv()


class Settings:
    """Application configuration settings loaded from environment variables."""

    PROJECT_NAME: str = "Smart Home Light Monitoring API"
    VERSION: str = "1.0.0"

    # Database configuration
    # Render provides postgres:// which SQLAlchemy 1.4+ requires to be postgresql://
    _raw_db_url: str = os.getenv("DATABASE_URL", "sqlite:///./sensor_data.db")
    if _raw_db_url.startswith("postgres://"):
        DATABASE_URL: str = _raw_db_url.replace("postgres://", "postgresql://", 1)
    else:
        DATABASE_URL: str = _raw_db_url

    # Device API Key for securing POST /api/readings
    DEVICE_API_KEY: Optional[str] = os.getenv("DEVICE_API_KEY")

    # Server port (used by Docker and Render)
    PORT: int = int(os.getenv("PORT", "8000"))

    # CORS configuration
    CORS_ORIGINS: list[str] = ["*"]


@lru_cache()
def get_settings() -> Settings:
    return Settings()
