import tempfile
from pathlib import Path
from fastapi import FastAPI, Query
from pydantic import BaseModel, Field
from .sched import ReturnCode
from . import sched

__all__ = ["app"]

app = FastAPI()


class DBAddResponse(BaseModel):
    db_id: int
    rc: ReturnCode
    error: str = ""


@app.post(
    "/db/add", response_model=DBAddResponse, summary="add a new deciphon database"
)
async def db_add(
    file_name: str = Query(
        ..., title="file name of a deciphon database", example="pfam.dcp"
    )
):
    rd = sched.add_db(file_name)
    return DBAddResponse(db_id=rd.val, rc=rd.rc)


class DBListResponse(BaseModel):
    dbs: list[sched.DB]
    rc: ReturnCode
    error: str = ""


@app.get("/db/list", response_model=DBListResponse, summary="list deciphon databases")
async def db_list():
    rd = sched.db_list()
    return DBListResponse(dbs=rd.val, rc=rd.rc)


class DBFilepathResponse(BaseModel):
    filepath: str
    rc: ReturnCode
    error: str = ""


@app.get(
    "/db/filepath",
    response_model=DBFilepathResponse,
    summary="get deciphon database filepath",
)
async def db_filepath(db_id: int):
    rd = sched.db_filepath(db_id)
    return DBFilepathResponse(filepath=rd.val, rc=rd.rc, error=rd.error)


fasta_example = """>Homoserine_dh-consensus
CCTATCATTTCGACGCTCAAGGAGTCGCTGACAGGTGACCGTATTACTCGAATCGAAGGG
ATATTAAACGGCACCCTGAATTACATTCTCACTGAGATGGAGGAAGAGGGGGCTTCATTC
TCTGAGGCGCTGAAGGAGGCACAGGAATTGGGCTACGCGGAAGCGGATCCTACGGACGAT
GTGGAAGGGCTAGATGCTGCTAGAAAGCTGGCAATTCTAGCCAGATTGGCATTTGGGTTA
GAGGTCGAGTTGGAGGACGTAGAGGTGGAAGGAATTGAAAAGCTGACTGCCGAAGATATT
GAAGAAGCGAAGGAAGAGGGTAAAGTTTTAAAACTAGTGGCAAGCGCCGTCGAAGCCAGG
GTCAAGCCTGAGCTGGTACCTAAGTCACATCCATTAGCCTCGGTAAAAGGCTCTGACAAC
GCCGTGGCTGTAGAAACGGAACGGGTAGGCGAACTCGTAGTGCAGGGACCAGGGGCTGGC
GCAGAGCCAACCGCATCCGCTGTACTCGCTGACCTTCTC"""


class JobSubmission(BaseModel):
    db_id: int = Field(None, example=1)
    multi_hits: bool = Field(None, example=True)
    hmmer3_compat: bool = Field(None, example=False)
    fasta_content: str = Field(None, example=fasta_example)
    alphabet: str = Field(None, example="dna")


class JobSubmissionResponse(BaseModel):
    job_id: int
    rc: ReturnCode
    error: str = ""


@app.post("/job/submit", summary="submit a new job")
async def job_submit(job_submission: JobSubmission):
    f = tempfile.NamedTemporaryFile(delete=False)
    filepath = Path(f.name)
    f.write(job_submission.fasta_content.encode())
    f.close()

    rd = sched.submit_job(
        job_submission.db_id,
        filepath,
        job_submission.multi_hits,
        job_submission.hmmer3_compat,
    )
    return JobSubmissionResponse(job_id=rd.val, rc=rd.rc, error=rd.error)


class JobStatusResponse(BaseModel):
    state: sched.JobState
    rc: ReturnCode
    error: str = ""


@app.get("/job/status", summary="query status of a job")
async def job_status(job_id: int):
    rd = sched.job_state(job_id)
    return JobStatusResponse(state=rd.val, rc=rd.rc, error=rd.error)


class JobNextPendResponse(BaseModel):
    job: sched.PendJob
    rc: ReturnCode
    error: str = ""


@app.get("/job/next_pend", summary="get next pending job")
async def job_next_pend():
    rd = sched.job_next_pend()
    return JobNextPendResponse(job=rd.val, rc=rd.rc, error=rd.error)


class SeqResponse(BaseModel):
    seq: sched.Seq
    rc: ReturnCode
    error: str = ""


@app.get("/seq/next", summary="get next seq")
async def seq_next(seq_id: int, job_id: int):
    rd = sched.next_seq(seq_id, job_id)
    return SeqResponse(seq=rd.val, rc=rd.rc, error=rd.error)
