from typing import List
from ._csched import lib, ffi
from ._prod import Prod, create_prod
from ._seq import Seq, create_seq
from pydantic import BaseModel
from fastapi import HTTPException, status
from ._rc import ReturnCode, return_data
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


@app.get("/jobs/{job_id}")
def get_job(job_id: int):
    sched_job = ffi.new("struct sched_job *")
    sched_job[0].id = job_id

    rd = return_data(lib.sched_get_job(sched_job))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    job = Job()

    job.id = int(sched_job[0].id)
    job.db_id = int(sched_job[0].db_id)
    job.multi_hits = bool(sched_job[0].multi_hits)
    job.hmmer3_compat = bool(sched_job[0].hmmer3_compat)
    job.state = ffi.string(sched_job[0].state).decode()

    job.error = ffi.string(sched_job[0].error).decode()
    job.submission = int(sched_job[0].submission)
    job.exec_started = int(sched_job[0].exec_started)
    job.exec_ended = int(sched_job[0].exec_ended)

    return job


@app.post("/jobs/")
def post_job(job: JobIn):
    pass


@ffi.def_extern()
def prod_set_cb(cprod, arg):
    prods = ffi.from_handle(arg)
    prods.append(create_prod(cprod))


@app.get("/jobs/{job_id}/prods")
def get_job_prods(job_id: int):
    cprod = ffi.new("struct sched_prod *")
    prods: List[Prod] = []
    rd = return_data(
        lib.sched_get_job_prods(job_id, lib.prod_set_cb, cprod, ffi.new_handle(prods))
    )

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return prods


@ffi.def_extern()
def seq_set_cb(cseq, arg):
    seqs = ffi.from_handle(arg)
    seqs.append(create_seq(cseq))


@app.get("/jobs/{job_id}/seqs")
def get_job_seqs(job_id: int):
    cseq = ffi.new("struct sched_seq *")
    seqs: List[Seq] = []
    rd = return_data(
        lib.sched_get_job_seqs(job_id, lib.seq_set_cb, cseq, ffi.new_handle(seqs))
    )

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return seqs


# def _submit_job(sched_job, filepath: str):
#     error = ffi.new("char[128]")
#     rc = ReturnCode(lib.sched_submit_job(sched_job, filepath.encode(), error))
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
