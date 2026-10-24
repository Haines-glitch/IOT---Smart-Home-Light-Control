import os
import pytest
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool
from fastapi.testclient import TestClient

# Ensure test environment variables are set before importing app
os.environ["DATABASE_URL"] = "sqlite:///:memory:"
os.environ["DEVICE_API_KEY"] = "test-secret-key-12345"

from app.database import Base, get_db
from app.config import get_settings
from app.main import app

# In-memory SQLite database specifically for test runs
SQLALCHEMY_DATABASE_URL = "sqlite:///:memory:"

test_engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args={"check_same_thread": False},
    poolclass=StaticPool,
)
TestingSessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=test_engine)


@pytest.fixture(scope="session", autouse=True)
def setup_test_db():
    """Create test tables once for the test session."""
    Base.metadata.create_all(bind=test_engine)
    yield
    Base.metadata.drop_all(bind=test_engine)


@pytest.fixture(name="db_session")
def fixture_db_session():
    """Yield a fresh database session for each test, rolled back afterwards."""
    connection = test_engine.connect()
    transaction = connection.begin()
    session = TestingSessionLocal(bind=connection)

    yield session

    session.close()
    transaction.rollback()
    connection.close()


@pytest.fixture(name="client")
def fixture_client(db_session):
    """
    FastAPI TestClient with overridden get_db dependency.
    Guarantees no production database is ever contacted or modified during tests.
    """
    def override_get_db():
        try:
            yield db_session
        finally:
            pass

    app.dependency_overrides[get_db] = override_get_db
    with TestClient(app) as test_client:
        yield test_client
    app.dependency_overrides.clear()


@pytest.fixture
def valid_api_key():
    return "test-secret-key-12345"
