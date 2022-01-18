from typing import List
from ._csched import lib, ffi
from pydantic import BaseModel
from fastapi import HTTPException, status
from ._rc import RC, return_data
from ._app import app


class Job(BaseModel):
    id: int = 0

    db_id: int = 0
    multi_hits: bool = False
    hmmer3_compat: bool = False
    state: str = ""

    error: str = ""
    submission: int = 0
    exec_started: int = 0
    exec_ended: int = 0


class SeqIn(BaseModel):
    name: str = ""
    data: str = ""


class JobIn(BaseModel):
    db_id: int = 0
    multi_hits: bool = False
    hmmer3_compat: bool = False
    seqs: List[SeqIn]


@app.get("/job/{job_id}")
def get_job(job_id: int):
    sched_job = ffi.new("struct sched_job *")
    sched_job[0].id = job_id

    rd = return_data(lib.sched_get_job(sched_job))

    if rd.rc == RC.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != RC.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    job = Job()

    job.db_id = int(sched_job[0].db_id)
    job.multi_hits = bool(sched_job[0].multi_hits)
    job.hmmer3_compat = bool(sched_job[0].hmmer3_compat)
    job.state = ffi.string(sched_job[0].state).decode()

    job.error = ffi.string(sched_job[0].error).decode()
    job.submission = int(sched_job[0].submission)
    job.exec_started = int(sched_job[0].exec_started)
    job.exec_ended = int(sched_job[0].exec_ended)

    return job


@app.post("/job/")
def post_job(job: JobIn):
    pass


# def _submit_job(sched_job, filepath: str):
#     error = ffi.new("char[128]")
#     rc = RC(lib.sched_submit_job(sched_job, filepath.encode(), error))
#     err = ffi.string(error)
#     job_id = 0
#     if rc == rc.RC_DONE:
#         job_id = int(sched_job[0].id)
#     elif len(err) == 0:
#         raise HTTPException(status_code=500, detail=f"failure at sched_submit_job")
#
#     return ReturnData(Return(rc=ReturnCode[rc.name], error=err), job_id)
#
#
# def submit_job(db_id: int, fasta_filepath: Path, multi_hits: bool, hmmer3_compat: bool):
#     job = ffi.new("struct sched_job[1]")
#     lib.sched_job_init(
#         job,
#         db_id,
#         multi_hits,
#         hmmer3_compat,
#     )
#     return _submit_job(job, str(fasta_filepath.absolute()))
