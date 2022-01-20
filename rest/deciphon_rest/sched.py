from enum import Enum, IntEnum

from fastapi import HTTPException, status
from pydantic import BaseModel

from .csched import lib
from .rc import ReturnCode, return_data


class JS(IntEnum):
    SCHED_JOB_PEND = 0
    SCHED_JOB_RUN = 1
    SCHED_JOB_DONE = 2
    SCHED_JOB_FAIL = 3


class JobState(str, Enum):
    SCHED_JOB_PEND = "pend"
    SCHED_JOB_RUN = "run"
    SCHED_JOB_DONE = "done"
    SCHED_JOB_FAIL = "fail"


class DB(BaseModel):
    id: int
    name: str


class PendJob(BaseModel):
    id: int = 0
    db_id: int = 0
    multi_hits: bool = False
    hmmer3_compat: bool = False


# @ffi.def_extern()
# def db_peek(sched_db, arg: list[DB]):
#     dbs: list[DB] = ffi.from_handle(arg)
#     id = int(sched_db[0].id)
#     name = ffi.string(sched_db[0].name).decode()
#     dbs.append(DB(id=id, name=name))


# .decode() def db_list():
#     dbs: list[DB] = []
#     arg = ffi.new_handle(dbs)
#     rc = ReturnCode(lib.sched_db_list(lib.db_peek, arg))
#     if rc != rc.RC_DONE:
#         raise HTTPException(status_code=500, detail=f"failure at sched_db_list")
#
#     return ReturnData(Return(rc=ReturnCode[rc.name], error=""), dbs)
#
#
# def db_filepath(db_id: int):
#     filepath = ffi.new("char[4096]")
#     rc = ReturnCode(lib.sched_cpy_db_filepath(4096, filepath, db_id))
#
#     if rc == rc.RC_NOTFOUND:
#         return ReturnData(Return(rc=ReturnCode[rc.name], error=""), "")
#
#     if rc != rc.RC_DONE:
#         raise HTTPException(status_code=500, detail=f"failure at db_filepath")
#
#     return ReturnData(
#         Return(rc=ReturnCode[rc.name], error=""), ffi.string(filepath).decode()
#     )


def sched_setup():
    rd = return_data(lib.sched_setup(b"deciphon.sched"))

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

def sched_open():
    rd = return_data(lib.sched_open())

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

def sched_close():
    rd = return_data(lib.sched_close())

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return rd


# def job_next_pend():
#     sched_job = ffi.new("struct sched_job[1]")
#     rc = ReturnCode(lib.sched_next_pending_job(sched_job))
#
#     job = PendJob()
#     if rc == rc.RC_NOTFOUND:
#         return ReturnData(Return(rc=ReturnCode[rc.name], error=""), job)
#
#     if rc != rc.RC_DONE:
#         raise HTTPException(status_code=500, detail=f"failure at next_pend_job")
#
#     job.id = int(sched_job[0].id)
#     job.db_id = int(sched_job[0].db_id)
#     job.multi_hits = bool(sched_job[0].multi_hits)
#     job.hmmer3_compat = bool(sched_job[0].hmmer3_compat)
#
#     return ReturnData(Return(rc=ReturnCode[rc.name], error=""), job)


class Sched:
    def __init__(self) -> None:
        sched_setup()
        sched_open()

    def close(self) -> None:
        sched_close()

sched = Sched()
