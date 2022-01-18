from typing import List
from ._csched import lib, ffi
from pydantic import BaseModel
from fastapi import HTTPException, status
from ._rc import RC, return_data
from ._app import app


class ProdIn(BaseModel):
    seq_id: int = 0
    profile_name: str = ""
    alt_loglik: float = 0.0
    null_loglik: float = 0.0
    match: str = ""


class JobResult(BaseModel):
    id: int = 0
    exec_ended: int = 0
    prods: List[ProdIn]


@app.post("/job_result/")
def post_job_result(job_result: JobResult):
    pass
