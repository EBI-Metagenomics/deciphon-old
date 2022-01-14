import tempfile
from pathlib import Path
from fastapi import FastAPI, Query
from .sched import sched_add_db, sched_submit_job, sched_db_list
from pydantic import BaseModel, Field
from .sched import ReturnCode, DB

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
    rd = sched_add_db(file_name)
    return DBAddResponse(db_id=rd.val, rc=rd.rc)


class DBListResponse(BaseModel):
    dbs: list[DB]
    rc: ReturnCode
    error: str = ""


@app.get("/db/list", response_model=DBListResponse, summary="list deciphon databases")
async def db_list():
    rd = sched_db_list()
    return DBListResponse(dbs=rd.val, rc=rd.rc)


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


@app.post("/submit/job", summary="submit a new job")
async def submit_job(job_submission: JobSubmission):
    f = tempfile.NamedTemporaryFile(delete=False)
    filepath = Path(f.name)
    f.write(job_submission.fasta_content.encode())
    f.close()

    rd = sched_submit_job(
        job_submission.db_id,
        filepath,
        job_submission.multi_hits,
        job_submission.hmmer3_compat,
    )
    return JobSubmissionResponse(job_id=rd.val, rc=rd.rc, error=rd.error)
