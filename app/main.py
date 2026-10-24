import os
from contextlib import asynccontextmanager
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse

from app.config import get_settings
from app.database import init_db
from app.routes.readings import router as readings_router
from app.routes.health import router as health_router

settings = get_settings()


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    Lifespan context manager that handles startup and shutdown events.
    Initializes PostgreSQL tables automatically upon application startup.
    """
    # Startup: create database tables if they do not already exist
    init_db()
    yield
    # Shutdown logic (if any) can be added here


app = FastAPI(
    title=settings.PROJECT_NAME,
    version=settings.VERSION,
    description="Backend API and Real-Time Dashboard for ESP32 Smart Home Light Monitoring & Recommendation System",
    lifespan=lifespan
)

# CORS Middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include API Routers
app.include_router(readings_router)
app.include_router(health_router)

# Mount static files directory
STATIC_DIR = os.path.join(os.path.dirname(__file__), "static")
if os.path.exists(STATIC_DIR):
    app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


@app.get("/", summary="Serve Smart Home Web Dashboard", include_in_schema=False)
def serve_dashboard():
    """Serves the front-end Smart Home Light Monitoring & Recommendation dashboard."""
    index_file = os.path.join(STATIC_DIR, "index.html")
    if os.path.exists(index_file):
        return FileResponse(index_file)
    return {"message": "Smart Home IoT API is running. Access /docs for API documentation."}
