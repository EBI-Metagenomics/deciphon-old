from typing import List
from ._csched import lib, ffi
from pydantic import BaseModel
from fastapi import HTTPException, status
from ._rc import RC, return_data
from ._app import app


class Prod(BaseModel):
    id: int = 0

    job_id: int = 0
    seq_id: int = 0

    profile_name: str = ""
    abc_name: str = ""

    alt_loglik: float = 0.0
    null_loglik: float = 0.0

    profile_typeid: str = ""
    version: str = ""

    match: str = ""




@app.get("/prod/{prod_id}")
def get_prod(prod_id: int):
    sched_prod = ffi.new("struct sched_prod *")
    sched_prod[0].id = prod_id

    rd = return_data(lib.sched_get_prod(sched_prod))

    if rd.rc == RC.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != RC.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    prod = Prod()

    prod.job_id = int(sched_prod[0].job_id)
    prod.seq_id = int(sched_prod[0].seq_id)

    prod.profile_name = ffi.string(sched_prod[0].profile_name).decode()
    prod.abc_name = ffi.string(sched_prod[0].abc_name).decode()

    prod.alt_loglik = float(sched_prod[0].alt_loglik)
    prod.null_loglik = float(sched_prod[0].null_loglik)

    prod.profile_typeid = ffi.string(sched_prod[0].profile_typeid).decode()
    prod.version = ffi.string(sched_prod[0].version).decode()

    prod.match = ffi.string(sched_prod[0].match).decode()

    return prod
