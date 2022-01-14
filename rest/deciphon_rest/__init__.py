from .sched import Sched
from fastapi import FastAPI
import dataclasses

__all__ = ["start", "Rest"]


@dataclasses.dataclass
class Rest:
    app: FastAPI
    sched: Sched


def start():
    from ._app import app

    return Rest(app, Sched())
