from fastapi import FastAPI
from _sched import lib, ffi
from enum import Enum


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


@app.get("/db/add/{filepath:path}")
async def db_add(filepath: str):
    id = ffi.new("int64_t[]", 1)
    encoded = filepath.encode()
    rc = RC(lib.sched_add_db(filepath.encode(), id))
    error = ""
    if rc != RC.RC_DONE:
        error = logged.pop()
    return {
        "filepath": filepath,
        "encoded": encoded,
        "id": int(id[0]),
        "rc": str(rc),
        "error": error,
    }
