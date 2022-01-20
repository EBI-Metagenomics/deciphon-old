from enum import Enum, IntEnum
from pydantic import BaseModel
from .logger import logger


class RC(IntEnum):
    RC_DONE = 0
    RC_END = 1
    RC_NEXT = 2
    RC_NOTFOUND = 3
    RC_EFAIL = 4
    RC_EINVAL = 5
    RC_EIO = 6
    RC_ENOMEM = 7
    RC_EPARSE = 8


class ReturnCode(str, Enum):
    RC_DONE = "done"
    RC_END = "end"
    RC_NEXT = "next"
    RC_NOTFOUND = "notfound"
    RC_EFAIL = "efail"
    RC_EINVAL = "einval"
    RC_EIO = "eio"
    RC_ENOMEM = "enomem"
    RC_EPARSE = "eparse"

    class Config:
        use_enum_values = True


class ReturnData(BaseModel):
    rc: ReturnCode = ReturnCode.RC_DONE
    error: str = ""


def return_data(rc):
    return ReturnData(rc=ReturnCode[RC(rc).name], error=logger.pop())
