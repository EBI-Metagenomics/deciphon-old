from fastapi import HTTPException
from fastapi.responses import JSONResponse

from .app import app


@app.exception_handler(HTTPException)
async def validation_exception_handler(_, exc: HTTPException):
    detail = exc.detail
    try:
        detail = exc.detail.dict()
    except TypeError:
        pass
    return JSONResponse(status_code=exc.status_code, content=detail)
