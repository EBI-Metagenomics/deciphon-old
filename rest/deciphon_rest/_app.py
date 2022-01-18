from fastapi import FastAPI

__all__ = ["app"]

app = FastAPI()


# class DBAddResponse(BaseModel):
#     db_id: int
#     ret: sched.Return
#
#
# @app.post(
#     "/db/add", response_model=DBAddResponse, summary="add a new deciphon database"
# )
# def db_add(
#     file_name: str = Query(
#         ..., title="file name of a deciphon database", example="pfam.dcp"
#     )
# ):
#     rd = sched.add_db(file_name)
#     return DBAddResponse(db_id=rd.val, ret=rd.ret)
#
#
# class DBListResponse(BaseModel):
#     dbs: list[sched.DB]
#     ret: sched.Return
#
#
# @app.get("/db/list", response_model=DBListResponse, summary="list deciphon databases")
# def db_list():
#     rd = sched.db_list()
#     return DBListResponse(dbs=rd.val, ret=rd.ret)
#
#
# class DBFilepathResponse(BaseModel):
#     filepath: str
#     ret: sched.Return
#
#
# @app.get(
#     "/db/filepath",
#     response_model=DBFilepathResponse,
#     summary="get deciphon database filepath",
# )
# def db_filepath(db_id: int):
#     rd = sched.db_filepath(db_id)
#     return DBFilepathResponse(filepath=rd.val, ret=rd.ret)
#
#
# fasta_example = """>Homoserine_dh-consensus
# CCTATCATTTCGACGCTCAAGGAGTCGCTGACAGGTGACCGTATTACTCGAATCGAAGGG
# ATATTAAACGGCACCCTGAATTACATTCTCACTGAGATGGAGGAAGAGGGGGCTTCATTC
# TCTGAGGCGCTGAAGGAGGCACAGGAATTGGGCTACGCGGAAGCGGATCCTACGGACGAT
# GTGGAAGGGCTAGATGCTGCTAGAAAGCTGGCAATTCTAGCCAGATTGGCATTTGGGTTA
# GAGGTCGAGTTGGAGGACGTAGAGGTGGAAGGAATTGAAAAGCTGACTGCCGAAGATATT
# GAAGAAGCGAAGGAAGAGGGTAAAGTTTTAAAACTAGTGGCAAGCGCCGTCGAAGCCAGG
# GTCAAGCCTGAGCTGGTACCTAAGTCACATCCATTAGCCTCGGTAAAAGGCTCTGACAAC
# GCCGTGGCTGTAGAAACGGAACGGGTAGGCGAACTCGTAGTGCAGGGACCAGGGGCTGGC
# GCAGAGCCAACCGCATCCGCTGTACTCGCTGACCTTCTC"""
#
#
# class JobSubmission(BaseModel):
#     db_id: int = Field(None, example=1)
#     multi_hits: bool = Field(None, example=True)
#     hmmer3_compat: bool = Field(None, example=False)
#     fasta_content: str = Field(None, example=fasta_example)
#     alphabet: str = Field(None, example="dna")
#
#
# class JobSubmissionResponse(BaseModel):
#     job_id: int
#     ret: sched.Return
#
#
# @app.post("/job/submit", summary="submit a new job")
# def job_submit(job_submission: JobSubmission):
#     f = tempfile.NamedTemporaryFile(delete=False)
#     filepath = Path(f.name)
#     f.write(job_submission.fasta_content.encode())
#     f.close()
#
#     rd = sched.submit_job(
#         job_submission.db_id,
#         filepath,
#         job_submission.multi_hits,
#         job_submission.hmmer3_compat,
#     )
#     return JobSubmissionResponse(job_id=rd.val, ret=rd.ret)
#
#
# class JobResponse(BaseModel):
#     job: sched.Job
#     ret: sched.Return
#
#
# @app.get("/job/{job_id}", summary="get job by id")
# def get_job(job_id: int):
#     rd = sched.get_job(job_id)
#     return JobResponse(job=rd.val, ret=rd.ret)
#
# class SeqResponse(BaseModel):
#     seq: sched.Seq
#     ret: sched.Return
#
# @app.get("/seq/{seq_id}", summary="get seq by id")
# def get_seq(seq_id: int):
#     return sched.get_seq(seq_id)
#
#
# class JobStatusResponse(BaseModel):
#     state: sched.JobState
#     ret: sched.Return
#
#
# @app.get("/job/status", summary="query status of a job")
# def job_status(job_id: int):
#     rd = sched.job_state(job_id)
#     return JobStatusResponse(state=rd.val, ret=rd.ret)
#
#
# class JobNextPendResponse(BaseModel):
#     job: sched.PendJob
#     ret: sched.Return
#
#
# @app.get("/job/next_pend", summary="get next pending job")
# def job_next_pend():
#     rd = sched.job_next_pend()
#     return JobNextPendResponse(job=rd.val, ret=rd.ret)
#
#
#
#
# @app.get("/seq/next", summary="get next seq")
# def seq_next(seq_id: int, job_id: int):
#     rd = sched.next_seq(seq_id, job_id)
#     return SeqResponse(seq=rd.val, ret=rd.ret)


# @app.put("/job/set_status")
# @app.patch("/job/{job_id}", response_model=Item, summary="update job")
# def job_set_status(job_id: int):
#     rd = sched.job_state(job_id)
#     return JobStatusResponse(state=rd.val, ret=rd.ret)
