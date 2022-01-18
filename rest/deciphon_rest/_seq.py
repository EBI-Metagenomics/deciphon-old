from typing import List
from ._csched import lib, ffi
from pydantic import BaseModel
from fastapi import HTTPException, status
from ._rc import ReturnCode, return_data
from ._app import app


class Seq(BaseModel):
    id: int = 0
    job_id: int = 0
    name: str = ""
    data: str = ""


# class SeqIn(BaseModel):
#     name: str = ""
#     data: str = ""
#
# class SeqInList(BaseModel):
#     job_id: int = 0
#     seqs: List[SeqIn]


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


# @app.post("/seq/")
# def post_seq(seq_in_list: SeqInList):
#     sched_seq = ffi.new("struct sched_seq[1]")
# sched_seq[0].id = 0
# sched_seq[0].job_id = seq.job_id
# sched_seq[0].name = seq.name
# sched_seq[0].data = seq.data
#
# rd = return_data(lib.sched_seq_db(filepath.encode(), seq_id))
#
# if rd.rc != ReturnCode.RC_DONE:
#     raise HTTPException(status.HTTP_500_INTERNAL_SERVER_ERROR, rd)
#
# return get_seq(int(seq_id[0]))

# def next_seq(seq_id: int, job_id: int):
#     sched_seq = ffi.new("struct sched_seq[1]")
#     sched_seq[0].id = seq_id
#     sched_seq[0].job_id = job_id
#     rc = RC(lib.sched_seq_next(sched_seq))
#
#     seq = Seq()
#     if rc == rc.RC_DONE:
#         return ReturnData(Return(rc=ReturnCode[rc.name], error=""), seq)
#
#     if rc != rc.RC_NEXT:
#         raise HTTPException(status_code=500, detail=f"failure at next_seq")
#
#     seq.id = sched_seq[0].id
#     seq.job_id = sched_seq[0].job_id
#     seq.name = ffi.string(sched_seq[0].name)
#     seq.data = ffi.string(sched_seq[0].data)
#
#     return ReturnData(Return(rc=ReturnCode[rc.name], error=""), seq)
