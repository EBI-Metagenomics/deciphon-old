PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: job
CREATE TABLE job (
    id            INTEGER      PRIMARY KEY AUTOINCREMENT
                               UNIQUE
                               NOT NULL,
    multi_hits    BOOLEAN      NOT NULL,
    hmmer3_compat BOOLEAN      NOT NULL,
    db_id         INTEGER      REFERENCES db (id)
                               NOT NULL,
    state         VARCHAR (4)  CHECK (state IN ('pend', 'run', 'done', 'fail') )
                               NOT NULL
                               DEFAULT ('pend'),
    error         VARCHAR (31) NOT NULL
                               DEFAULT (''),
    submission    INTEGER      NOT NULL
                               DEFAULT (0),
    exec_started  INTEGER      NOT NULL
                               DEFAULT (0),
    exec_ended    INTEGER      NOT NULL
                               DEFAULT (0)
);


-- Table: seq
CREATE TABLE [seq] (
    id     INTEGER PRIMARY KEY AUTOINCREMENT
                   NOT NULL
                   UNIQUE,
    data   VARCHAR NOT NULL,
    job_id INTEGER REFERENCES job (id)
                   NOT NULL
);


-- Table: prod
CREATE TABLE prod (
    id              INTEGER PRIMARY KEY AUTOINCREMENT
                    UNIQUE
                    NOT NULL,
    job_id          INTEGER REFERENCES job (id)
                    NOT NULL
                    UNIQUE,
    amino_faa  TEXT NOT NULL,
    codon_fna  TEXT NOT NULL,
    output_gff TEXT NOT NULL
);


-- Table: db
CREATE TABLE db (
    id       INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    filepath VARCHAR NOT NULL
                     UNIQUE
);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
