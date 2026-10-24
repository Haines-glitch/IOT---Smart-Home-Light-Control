import secrets
from fastapi import Security, HTTPException, status
from fastapi.security.api_key import APIKeyHeader
from app.config import get_settings

API_KEY_HEADER = APIKeyHeader(name="X-Device-Key", auto_error=False)


def verify_device_key(api_key: str = Security(API_KEY_HEADER)) -> str:
    """
    Validates that the incoming request contains an 'X-Device-Key' header matching DEVICE_API_KEY.
    Uses constant-time comparison (secrets.compare_digest) to prevent timing attacks.
    Fails safely if DEVICE_API_KEY is unset or missing.
    """
    settings = get_settings()
    configured_key = settings.DEVICE_API_KEY

    # If DEVICE_API_KEY is not set or empty, fail safely: do not allow open writes
    if not configured_key or not configured_key.strip():
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Server security configuration error: DEVICE_API_KEY is not set in environment."
        )

    if not api_key:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Missing 'X-Device-Key' header in request."
        )

    # Secure constant-time string comparison
    if not secrets.compare_digest(api_key.strip(), configured_key.strip()):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid 'X-Device-Key' provided."
        )

    return api_key
