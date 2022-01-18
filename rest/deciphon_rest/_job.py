from typing import List, Any, Optional

from ._csched import lib, ffi
from ._prod import Prod, create_prod
from ._seq import Seq, create_seq
from pydantic import BaseModel
from fastapi import HTTPException, status, Body
from ._rc import ReturnCode, return_data
from ._prod import ProdIn, get_prod, prod_in_example
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


@app.get("/jobs/{job_id}")
def get_job(job_id: int):
    cjob = ffi.new("struct sched_job *")
    cjob[0].id = job_id

    rd = return_data(lib.sched_get_job(cjob))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_job(cjob)


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


@app.post("/jobs/{job_id}/prods")
def post_prod(job_id: int, prod_in: ProdIn = Body(None, example=prod_in_example)):
    cprod = prod_in._create_cdata()
    cprod[0].job_id = job_id
    rd = return_data(lib.sched_prod_submit(cprod))

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return get_prod(int(cprod[0].id))
