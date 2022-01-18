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


def create_prod(cprod) -> Prod:
    prod = Prod()
    prod.job_id = int(cprod[0].job_id)
    prod.seq_id = int(cprod[0].seq_id)

    prod.profile_name = ffi.string(cprod[0].profile_name).decode()
    prod.abc_name = ffi.string(cprod[0].abc_name).decode()

    prod.alt_loglik = float(cprod[0].alt_loglik)
    prod.null_loglik = float(cprod[0].null_loglik)

    prod.profile_typeid = ffi.string(cprod[0].profile_typeid).decode()
    prod.version = ffi.string(cprod[0].version).decode()

    prod.match = ffi.string(cprod[0].match).decode()
    return prod


@app.get("/prods/{prod_id}")
def get_prod(prod_id: int):
    cprod = ffi.new("struct sched_prod *")
    cprod[0].id = prod_id

    rd = return_data(lib.sched_get_prod(cprod))

    if rd.rc == RC.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != RC.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_prod(cprod)
