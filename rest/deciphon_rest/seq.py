from fastapi import HTTPException, status
from pydantic import BaseModel

from .csched import ffi, lib
from .app import app
from .rc import ReturnCode, return_data


class Seq(BaseModel):
    id: int = 0
    job_id: int = 0
    name: str = ""
    data: str = ""


def create_seq(cseq) -> Seq:
    seq = Seq()
    seq.id = int(cseq[0].id)
    seq.job_id = int(cseq[0].job_id)
    seq.name = ffi.string(cseq[0].name).decode()
    seq.data = ffi.string(cseq[0].data).decode()
    return seq


@app.get("/seqs/{seq_id}")
def get_seq(seq_id: int):
    cseq = ffi.new("struct sched_seq *")
    cseq[0].id = seq_id

    rd = return_data(lib.sched_get_seq(cseq))

    if rd.rc == ReturnCode.RC_NOTFOUND:
        raise HTTPException(status.HTTP_404_NOT_FOUND, rd)

    if rd.rc != ReturnCode.RC_DONE:
        raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)

    return create_seq(cseq)
