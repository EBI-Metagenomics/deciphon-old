import tempfile
from pathlib import Path
from typing import TypeVar, Generic
from fastapi import HTTPException
from pydantic import BaseModel
from ._sched import lib, ffi
from enum import Enum, IntEnum
import dataclasses

T = TypeVar("T")


class RC(IntEnum):
    RC_DONE = 0
    RC_END = 1
    RC_NEXT = 2
    RC_NOTFOUND = 3
    RC_EFAIL = 4
    RC_EINVAL = 5
    RC_EIO = 6
    RC_ENOMEM = 7
    RC_EPARSE = 8


class ReturnCode(str, Enum):
    RC_DONE = "done"
    RC_END = "end"
    RC_NEXT = "next"
    RC_NOTFOUND = "notfound"
    RC_EFAIL = "efail"
    RC_EINVAL = "einval"
    RC_EIO = "eio"
    RC_ENOMEM = "enomem"
    RC_EPARSE = "eparse"


@dataclasses.dataclass
class ReturnData(Generic[T]):
    rc: ReturnCode
    error: str
    val: T


class DB(BaseModel):
    id: int
    name: str


@ffi.def_extern()
def db_peek(sched_db, arg: list[DB]):
    dbs: list[DB] = ffi.from_handle(arg)
    id = int(sched_db[0].id)
    name = ffi.string(sched_db[0].name).decode()
    dbs.append(DB(id=id, name=name))


def sched_db_list():
    dbs: list[DB] = []
    arg = ffi.new_handle(dbs)
    rc = RC(lib.sched_db_list(lib.db_peek, arg))
    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at sched_db_list")

    return ReturnData(ReturnCode[rc.name], "", dbs)


def sched_add_db(filepath: str):
    db_id = ffi.new("int64_t[]", 1)

    rc = RC(lib.sched_add_db(filepath.encode(), db_id))
    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at sched_add_db")

    return ReturnData(ReturnCode[rc.name], "", int(db_id[0]))


def sched_setup():
    rc = RC(lib.sched_setup(b"deciphon.sched"))
    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at sched_setup")

    return ReturnData(ReturnCode[rc.name], "", None)


def sched_open():
    rc = RC(lib.sched_open())
    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at sched_open")

    return ReturnData(ReturnCode[rc.name], "", None)


def sched_close():
    rc = RC(lib.sched_close())
    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at sched_close")

    return ReturnData(ReturnCode[rc.name], "", None)


def _sched_submit_job(sched_job, filepath: str):
    error = ffi.new("char[128]")
    rc = RC(lib.sched_submit_job(sched_job, filepath.encode(), error))
    err = ffi.string(error)
    print(err)
    print(err)
    print(err)
    job_id = 0
    if rc == rc.RC_DONE:
        job_id = int(sched_job[0].id)
    elif len(err) == 0:
            raise HTTPException(status_code=500, detail=f"failure at sched_submit_job")

    return ReturnData(ReturnCode[rc.name], err, job_id)


def sched_submit_job(
    db_id: int, fasta_filepath: Path, multi_hits: bool, hmmer3_compat: bool
):
    job = ffi.new("struct sched_job[1]")
    lib.sched_job_init(
        job,
        db_id,
        multi_hits,
        hmmer3_compat,
    )
    return _sched_submit_job(job, str(fasta_filepath.absolute()))


class Sched:
    def __init__(self) -> None:
        sched_setup()
        sched_open()

    def close(self) -> None:
        sched_close()
