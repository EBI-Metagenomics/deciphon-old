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


@dataclasses.dataclass
class ReturnData(Generic[T]):
    rc: ReturnCode
    error: str
    val: T


class DB(BaseModel):
    id: int
    name: str


class PendJob(BaseModel):
    id: int = 0
    db_id: int = 0
    multi_hits: bool = False
    hmmer3_compat: bool = False


class Seq(BaseModel):
    id: int = 0
    job_id: int = 0
    name: str = ""
    data: str = ""


@ffi.def_extern()
def db_peek(sched_db, arg: list[DB]):
    dbs: list[DB] = ffi.from_handle(arg)
    id = int(sched_db[0].id)
    name = ffi.string(sched_db[0].name).decode()
    dbs.append(DB(id=id, name=name))


def db_list():
    dbs: list[DB] = []
    arg = ffi.new_handle(dbs)
    rc = RC(lib.sched_db_list(lib.db_peek, arg))
    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at sched_db_list")

    return ReturnData(ReturnCode[rc.name], "", dbs)


def add_db(filepath: str):
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


def _submit_job(sched_job, filepath: str):
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


def submit_job(db_id: int, fasta_filepath: Path, multi_hits: bool, hmmer3_compat: bool):
    job = ffi.new("struct sched_job[1]")
    lib.sched_job_init(
        job,
        db_id,
        multi_hits,
        hmmer3_compat,
    )
    return _submit_job(job, str(fasta_filepath.absolute()))


def job_state(job_id: int):
    state = ffi.new("enum sched_job_state[1]")
    rc = RC(lib.sched_job_state(job_id, state))

    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at job_state")

    return ReturnData(ReturnCode[rc.name], "", JobState[JS(int(state[0])).name])


def job_next_pend():
    sched_job = ffi.new("struct sched_job[1]")
    rc = RC(lib.sched_next_pending_job(sched_job))

    job = PendJob()
    if rc == rc.RC_NOTFOUND:
        return ReturnData(ReturnCode[rc.name], "", job)

    if rc != rc.RC_DONE:
        raise HTTPException(status_code=500, detail=f"failure at next_pend_job")

    job.id = int(sched_job[0].id)
    job.db_id = int(sched_job[0].db_id)
    job.multi_hits = bool(sched_job[0].multi_hits)
    job.hmmer3_compat = bool(sched_job[0].hmmer3_compat)

    return ReturnData(ReturnCode[rc.name], "", job)


def next_seq(seq_id: int, job_id: int):
    sched_seq = ffi.new("struct sched_seq[1]")
    sched_seq[0].id = seq_id
    sched_seq[0].job_id = job_id
    rc = RC(lib.sched_seq_next(sched_seq))
    print(rc)
    print(rc)
    print(rc)

    seq = Seq()
    if rc == rc.RC_DONE:
        return ReturnData(ReturnCode[rc.name], "", seq)

    if rc != rc.RC_NEXT:
        raise HTTPException(status_code=500, detail=f"failure at next_seq")

    seq.id = sched_seq[0].id
    seq.job_id = sched_seq[0].job_id
    seq.name = ffi.string(sched_seq[0].name)
    seq.data = ffi.string(sched_seq[0].data)

    return ReturnData(ReturnCode[rc.name], "", seq)


class Sched:
    def __init__(self) -> None:
        sched_setup()
        sched_open()

    def close(self) -> None:
        sched_close()
