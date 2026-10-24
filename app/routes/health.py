from datetime import datetime, timezone
import logging
from fastapi import APIRouter, Depends, status, Response
from sqlalchemy.orm import Session
from sqlalchemy import text
from app.database import get_db
from app.schemas import HealthResponse

logger = logging.getLogger(__name__)

router = APIRouter(tags=["Health"])


@router.get(
    "/health",
    response_model=HealthResponse,
    summary="Health check endpoint",
    description="Confirms that the FastAPI service is running and tests connectivity to the PostgreSQL database."
)
def health_check(response: Response, db: Session = Depends(get_db)):
    """
    Checks API status and executes a lightweight query ('SELECT 1') against PostgreSQL.
    """
    db_status = "connected"
    api_status = "ok"

    try:
        db.execute(text("SELECT 1"))
    except Exception as exc:
        logger.error(f"Database health check failed: {exc}")
        db_status = "unreachable"
        api_status = "degraded"
        response.status_code = status.HTTP_503_SERVICE_UNAVAILABLE

    return HealthResponse(
        status=api_status,
        database=db_status,
        timestamp=datetime.now(timezone.utc),
        version="1.0.0"
    )
