from enum import Enum
import os
from typing import List

from fastapi import Body, File, HTTPException, UploadFile, status
from pydantic import BaseModel

from .app import app
from .csched import ffi, lib
from .prod import Prod, create_prod
from .prod import ProdIn, get_prod, prod_in_example
from .rc import ReturnCode, ReturnData, return_data
from .seq import Seq, create_seq


class JobState(str, Enum):
    pend = "pend"
    run = "run"
    done = "done"
    fail = "fail"


class Job(BaseModel):
    id: int = 0

    db_id: int = 0
    multi_hits: bool = False
    hmmer3_compat: bool = False
    state: JobState = JobState.pend

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
    seqs: List[SeqIn] = []


job_in_example = JobIn(
    db_id=1,
    multi_hits=True,
    hmmer3_compat=False,
    seqs=[
        SeqIn(
            name="Homoserine_dh-consensus",
            data="CCTATCATTTCGACGCTCAAGGAGTCGCTGACAGGTGACCGTATTACTCGAATCGAAGGG"
            "ATATTAAACGGCACCCTGAATTACATTCTCACTGAGATGGAGGAAGAGGGGGCTTCATTC"
            "TCTGAGGCGCTGAAGGAGGCACAGGAATTGGGCTACGCGGAAGCGGATCCTACGGACGAT"
            "GTGGAAGGGCTAGATGCTGCTAGAAAGCTGGCAATTCTAGCCAGATTGGCATTTGGGTTA"
            "GAGGTCGAGTTGGAGGACGTAGAGGTGGAAGGAATTGAAAAGCTGACTGCCGAAGATATT"
            "GAAGAAGCGAAGGAAGAGGGTAAAGTTTTAAAACTAGTGGCAAGCGCCGTCGAAGCCAGG"
            "GTCAAGCCTGAGCTGGTACCTAAGTCACATCCATTAGCCTCGGTAAAAGGCTCTGACAAC"
            "GCCGTGGCTGTAGAAACGGAACGGGTAGGCGAACTCGTAGTGCAGGGACCAGGGGCTGGC"
            "GCAGAGCCAACCGCATCCGCTGTACTCGCTGACCTTCTC",
        ),
        SeqIn(
            name="AA_kinase-consensus",
            data="AAACGTGTAGTTGTAAAGCTTGGGGGTAGTTCTCTGACAGATAAGGAAGAGGCATCACTC"
            "AGGCGTTTAGCTGAGCAGATTGCAGCATTAAAAGAGAGTGGCAATAAACTAGTGGTCGTG"
            "CATGGAGGCGGCAGCTTCACTGATGGTCTGCTGGCATTGAAAAGTGGCCTGAGCTCGGGC"
            "GAATTAGCTGCGGGGTTGAGGAGCACGTTAGAAGAGGCCGGAGAAGTAGCGACGAGGGAC"
            "GCCCTAGCTAGCTTAGGGGAACGGCTTGTTGCAGCGCTGCTGGCGGCGGGTCTCCCTGCT"
            "GTAGGACTCAGCGCCGCTGCGTTAGATGCGACGGAGGCGGGCCGGGATGAAGGCAGCGAC"
            "GGGAACGTCGAGTCCGTGGACGCAGAAGCAATTGAGGAGTTGCTTGAGGCCGGGGTGGTC"
            "CCCGTCCTAACAGGATTTATCGGCTTAGACGAAGAAGGGGAACTGGGAAGGGGATCTTCT"
            "GACACCATCGCTGCGTTACTCGCTGAAGCTTTAGGCGCGGACAAACTCATAATACTGACC"
            "GACGTAGACGGCGTTTACGATGCCGACCCTAAAAAGGTCCCAGACGCGAGGCTCTTGCCA"
            "GAGATAAGTGTGGACGAGGCCGAGGAAAGCGCCTCCGAATTAGCGACCGGTGGGATGAAG"
            "GTCAAACATCCAGCGGCTCTTGCTGCAGCTAGACGGGGGGGTATTCCGGTCGTGATAACG"
            "AAT",
        ),
        SeqIn(
            name="23ISL-consensus",
            data="CAGGGTCTGGATAACGCTAATCGTTCGCTAGTTCGCGCTACAAAAGCAGAAAGTTCAGAT"
            "ATACGGAAAGAGGTGACTAACGGCATCGCTAAAGGGCTGAAGCTAGACAGTCTGGAAACA"
            "GCTGCAGAGTCGAAGAACTGCTCAAGCGCACAGAAAGGCGGATCGCTAGCTTGGGCAACC"
            "AACTCCCAACCACAGCCTCTCCGTGAAAGTAAGCTTGAGCCATTGGAAGACTCCCCACGT"
            "AAGGCTTTAAAAACACCTGTGTTGCAAAAGACATCCAGTACCATAACTTTACAAGCAGTC"
            "AAGGTTCAACCTGAACCCCGCGCTCCCGTCTCCGGGGCGCTGTCCCCGAGCGGGGAGGAA"
            "CGCAAGCGCCCAGCTGCGTCTGCTCCCGCTACCTTACCGACACGACAGAGTGGTCTAGGT"
            "TCTCAGGAAGTCGTTTCGAAGGTGGCGACTCGCAAAATTCCAATGGAGTCACAACGCGAG"
            "TCGACT",
        ),
    ],
)


def create_job(cjob):
    job = Job()

    job.id = int(cjob[0].id)
    job.db_id = int(cjob[0].db_id)
    job.multi_hits = bool(cjob[0].multi_hits)
    job.hmmer3_compat = bool(cjob[0].hmmer3_compat)
    job.state = ffi.string(cjob[0].state).decode()

    job.error = ffi.string(cjob[0].error).decode()
    job.submission = int(cjob[0].submission)
    job.exec_started = int(cjob[0].exec_started)
    job.exec_ended = int(cjob[0].exec_ended)

    return job


@app.get("/jobs/next_pend", response_model=Job)
def get_next_pend_job():
    cjob = ffi.new("struct sched_job *")
    rd = return_data(lib.sched_next_pend_job(cjob))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_job(cjob)


@app.get("/jobs/{job_id}", response_model=Job)
def get_job(job_id: int):
    cjob = ffi.new("struct sched_job *")
    cjob[0].id = job_id

    rd = return_data(lib.sched_get_job(cjob))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_job(cjob)


@app.patch("/jobs/{job_id}", response_model=Job)
def update_job(job_id: int, state: JobState):
    job = get_job(job_id)

    if job.state == state:
        raise HTTPException(
            status.HTTP_403_FORBIDDEN,
            ReturnData(rc=ReturnCode.RC_EINVAL, error="redundant job state update"),
        )

    if job.state == JobState.pend and state == JobState.run:
        rd = return_data(lib.sched_set_job_run(job_id))
        if rd.rc != ReturnCode.RC_DONE:
            raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)
        return get_job(job_id)

    raise HTTPException(
        status.HTTP_403_FORBIDDEN,
        ReturnData(rc=ReturnCode.RC_EINVAL, error="invalid job state update"),
    )


@app.post("/jobs")
def post_job(job: JobIn = Body(None, example=job_in_example)):
    cjob = ffi.new("struct sched_job *")

    cjob[0].id = 0
    cjob[0].db_id = job.db_id
    cjob[0].multi_hits = job.multi_hits
    cjob[0].hmmer3_compat = job.hmmer3_compat

    rd = return_data(lib.sched_begin_job_submission(cjob))
    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    for seq in job.seqs:
        lib.sched_add_seq(cjob, seq.name.encode(), seq.data.encode())

    rd = return_data(lib.sched_end_job_submission(cjob))
    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_job(cjob)


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


@app.get("/jobs/{job_id}/seqs/next/{seq_id}")
def get_next_job_seq(job_id: int, seq_id: int):
    cseq = ffi.new("struct sched_seq *")
    cseq[0].id = seq_id
    cseq[0].job_id = job_id
    rd = return_data(lib.sched_seq_next(cseq))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_seq(cseq)


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


@app.post("/prods/upload")
def upload_prods_file(prods_file: UploadFile = File(...)):
    print(f"prods_file.filename: {prods_file.filename}")
    prods_file.file.flush()
    fd = os.dup(prods_file.file.fileno())
    fp = lib.fdopen(fd, b"rb")
    rd = return_data(lib.sched_submit_prod_file(fp))

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)


@app.post("/jobs/{job_id}/prods")
def post_prod(job_id: int, prod_in: ProdIn = Body(None, example=prod_in_example)):
    cprod = prod_in._create_cdata()
    cprod[0].job_id = job_id
    rd = return_data(lib.sched_prod_submit(cprod))

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return get_prod(int(cprod[0].id))
