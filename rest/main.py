from fastapi import FastAPI, HTTPException
import io
from pydantic import BaseModel, Field, validator
from fastapi.responses import JSONResponse
from _sched import lib, ffi
from enum import Enum
import fasta_reader


@ffi.def_extern()
def logger_print(msg, arg):
    logged = ffi.from_handle(arg)
    logged.put(ffi.string(msg))


class Logged:
    def __init__(self):
        arg = ffi.new_handle(self)
        self._arg = arg
        lib.logger_setup(lib.logger_print, arg)
        self._msg = []

    def put(self, msg):
        self._msg.append(msg)

    def pop(self):
        return self._msg.pop()


logged = Logged()


class RC(Enum):
    RC_DONE = 0
    RC_END = 1
    RC_NEXT = 2
    RC_NOTFOUND = 3
    RC_EFAIL = 4
    RC_EINVAL = 5
    RC_EIO = 6
    RC_ENOMEM = 7
    RC_EPARSE = 8

    def __str__(self):
        return self.name


app = FastAPI()


class DeciphonException(Exception):
    def __init__(self, rc: RC, error: str):
        self.rc = rc
        self.error = error


@app.exception_handler(DeciphonException)
async def deciphon_exception_handler(_, exc: DeciphonException):
    return JSONResponse(
        status_code=418,
        content={"rc": str(exc.rc), "error": exc.error},
    )


@app.put("/db/add/{filepath:path}")
async def db_add(filepath: str):
    id = ffi.new("int64_t[]", 1)
    encoded = filepath.encode()
    rc = RC(lib.sched_add_db(filepath.encode(), id))
    if rc != RC.RC_DONE:
        raise DeciphonException(rc, logged.pop())
    return {
        "filepath": filepath,
        "encoded": encoded,
        "id": int(id[0]),
        "rc": str(rc),
    }


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


class SubmitJob(BaseModel):
    db_id: int = Field(None, example=1)
    multi_hits: bool = Field(None, example=True)
    hmmer3_compat: bool = Field(None, example=False)
    fasta_content: str = Field(None, example=fasta_example)
    alphabet: str = Field(None, example="dna")

    # void    sched_job_init(struct sched_job *job, int64_t db_id,
    #                        bool multi_hits, bool hmmer3_compat);


@app.post("/submit/job")
async def submit_job(submit_job: SubmitJob):
    job = ffi.new("struct sched_job[1]")
    lib.sched_job_init(
        job, submit_job.db_id, submit_job.multi_hits, submit_job.hmmer3_compat
    )

    rc = lib.sched_begin_job_submission(job)
    lib.sched_add_seq(job, "name", "data");
    # enum rc sched_rollback_job_submission(struct sched_job *job);
    # enum rc sched_end_job_submission(struct sched_job *job);
    pass
    # try:
    # queries = submit_job.parse_fasta_content()
    # except fasta_reader.ParsingError as e:
    #     raise HTTPException(
    #         status_code=404, detail=f"query parse error on line {e.line_number}"
    #     )
    return {}


# CREATE TABLE job (
#     id INTEGER PRIMARY KEY UNIQUE NOT NULL,
#
#     db_id INTEGER REFERENCES db (id) NOT NULL,
#     multi_hits INTEGER NOT NULL,
#     hmmer3_compat INTEGER NOT NULL,
#     state TEXT CHECK(state IN ('pend', 'run', 'done', 'fail')) NOT NULL,
#
#     error TEXT NOT NULL,
#     submission INTEGER NOT NULL,
#     exec_started INTEGER NOT NULL,
#     exec_ended INTEGER NOT NULL
# );
#
# CREATE TABLE seq (
#     id INTEGER PRIMARY KEY UNIQUE NOT NULL,
#     job_id INTEGER REFERENCES job (id) NOT NULL,
#     name TEXT NOT NULL,
#     data TEXT NOT NULL
# );
#
# CREATE TABLE prod (
#     id INTEGER PRIMARY KEY UNIQUE NOT NULL,
#
#     job_id INTEGER REFERENCES job (id) NOT NULL,
#     seq_id INTEGER REFERENCES seq (id) NOT NULL,
#
#     profile_name TEXT NOT NULL,
#     abc_name TEXT NOT NULL,
#
#     alt_loglik REAL NOT NULL,
#     null_loglik REAL NOT NULL,
#
#     profile_typeid TEXT NOT NULL,
#     version TEXT NOT NULL,
#
#     match TEXT NOT NULL,
#
#     UNIQUE(job_id, seq_id, profile_name)
# );
#
# CREATE TABLE db (
#     id INTEGER PRIMARY KEY UNIQUE NOT NULL,
#     xxh64 INTEGER UNIQUE NOT NULL,
#     filepath TEXT UNIQUE NOT NULL
# );
