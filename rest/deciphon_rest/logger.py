from .csched import lib, ffi


class Logger:
    def __init__(self) -> None:
        self.handle = ffi.new_handle(self)
        self._msg: str = ""

    def put(self, msg):
        print(msg)
        self._msg = msg

    def pop(self):
        msg = self._msg
        self._msg = ""
        return msg


@ffi.def_extern()
def logger_callback(msg, arg):
    logger: Logger = ffi.from_handle(arg)
    logger.put(ffi.string(msg).decode())


logger = Logger()
lib.logger_setup(lib.logger_callback, logger.handle)
