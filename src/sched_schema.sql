PRAGMA foreign_keys = off;

BEGIN TRANSACTION;

CREATE TABLE job (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    UNIQUE NOT NULL,
    db_id INTEGER REFERENCES db (id) NOT NULL,
    multi_hits INTEGER NOT NULL,
    hmmer3_compat INTEGER NOT NULL,
    state TEXT CHECK(state IN ('pend', 'run', 'done', 'fail')) NOT NULL DEFAULT ('pend'),
    error TEXT NOT NULL DEFAULT (''),
    submission INTEGER NOT NULL DEFAULT (0),
    exec_started INTEGER NOT NULL DEFAULT (0),
    exec_ended INTEGER NOT NULL DEFAULT (0)
);

CREATE TABLE seq (
    id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE NOT NULL,
    job_id INTEGER REFERENCES job (id) NOT NULL,
    name TEXT NOT NULL,
    data TEXT NOT NULL
);

CREATE TABLE prod (
    id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE NOT NULL,
    job_id INTEGER REFERENCES job (id) NOT NULL,
    seq_id INTEGER REFERENCES seq (id) NOT NULL,
    match_id INTEGER NOT NULL,
    prof_name TEXT NOT NULL,
    start_pos INTEGER NOT NULL,
    end_pos INTEGER NOT NULL,
    abc_id TEXT NOT NULL,
    loglik REAL NOT NULL,
    null_loglik REAL NOT NULL,
    model TEXT NOT NULL,
    version TEXT NOT NULL,
    match_data TEXT NOT NULL
);

CREATE TABLE db (
    id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    filepath TEXT UNIQUE NOT NULL
);

COMMIT TRANSACTION;

PRAGMA foreign_keys = ON;
