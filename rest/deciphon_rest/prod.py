from typing import List

from fastapi import HTTPException, status
from pydantic import BaseModel

from .app import app
from .csched import ffi, lib
from .rc import RC, return_data


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


class ProdIn(BaseModel):
    seq_id: int

    profile_name: str
    abc_name: str

    alt_loglik: float
    null_loglik: float

    profile_typeid: str
    version: str

    match: str

    def _create_cdata(self):
        cprod = ffi.new("struct sched_prod *")
        cprod[0].id = 0
        cprod[0].seq_id = self.seq_id
        cprod[0].profile_name = self.profile_name.encode()
        cprod[0].abc_name = self.abc_name.encode()
        cprod[0].alt_loglik = self.alt_loglik
        cprod[0].null_loglik = self.null_loglik
        cprod[0].profile_typeid = self.profile_typeid.encode()
        cprod[0].version = self.version.encode()
        cprod[0].match = self.match.encode()
        return cprod


prod_in_example = ProdIn(
    seq_id=1,
    profile_name="profname",
    abc_name="dna",
    alt_loglik=2.0,
    null_loglik=1.0,
    profile_typeid="protein",
    version="0.0.1",
    match="match",
)


def create_prod(cprod) -> Prod:
    prod = Prod()
    prod.id = int(cprod[0].id)
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


def create_prod_in(cprod) -> ProdIn:
    return ProdIn(
        seq_id=int(cprod[0].seq_id),
        profile_name=ffi.string(cprod[0].profile_name).decode(),
        abc_name=ffi.string(cprod[0].abc_name).decode(),
        alt_loglik=float(cprod[0].alt_loglik),
        null_loglik=float(cprod[0].null_loglik),
        profile_typeid=ffi.string(cprod[0].profile_typeid).decode(),
        version=ffi.string(cprod[0].version).decode(),
        match=ffi.string(cprod[0].match).decode(),
    )


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
