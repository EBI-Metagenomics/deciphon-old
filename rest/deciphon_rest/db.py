from fastapi import HTTPException, status
from pydantic import BaseModel

from .csched import ffi, lib
from .app import app
from .rc import ReturnCode, return_data


class DB(BaseModel):
    id: int = 0
    xxh64: int = 0
    filepath: str = ""


@app.get("/dbs/{db_id}")
def get_db(db_id: int):
    sched_db = ffi.new("struct sched_db *")
    sched_db[0].id = db_id

    rd = return_data(lib.sched_get_db(sched_db))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return DB(
        id=int(sched_db[0].id),
        xxh64=int(sched_db[0].xxh64),
        filepath=ffi.string(sched_db[0].filepath).decode(),
    )


@app.post("/dbs")
def post_db(filepath: str):
    db_id = ffi.new("int64_t *")
    rd = return_data(lib.sched_add_db(filepath.encode(), db_id))

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return get_db(int(db_id[0]))
