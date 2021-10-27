PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: abc
CREATE TABLE abc (
    id       INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    filepath VARCHAR UNIQUE
                     NOT NULL,
);

-- Table: job
CREATE TABLE job (
    id             INTEGER      PRIMARY KEY AUTOINCREMENT
                                UNIQUE
                                NOT NULL,
    multiple_hits  BOOLEAN      NOT NULL,
    hmmer3_compat  BOOLEAN      NOT NULL,
    abc            INTEGER      REFERENCES abc (id)
                                NOT NULL,
    db             INTEGER      REFERENCES db (id)
                                NOT NULL,
    state          VARCHAR (4)  CHECK (state IN ('pend', 'run', 'done', 'fail') )
                                NOT NULL
                                DEFAULT ('pend'),
    error          VARCHAR (31) NOT NULL
                                DEFAULT (''),
    submission     DATETIME     NOT NULL,
    exec_started   DATETIME     NOT NULL,
    exec_ended     DATETIME     NOT NULL,
);


-- Table: seq
CREATE TABLE [seq] (
    id     INTEGER PRIMARY KEY AUTOINCREMENT
                   NOT NULL
                   UNIQUE,
    value  VARCHAR NOT NULL,
    job    INTEGER REFERENCES job (id)
                   NOT NULL
);


-- Table: result
CREATE TABLE result (
    id         INTEGER PRIMARY KEY AUTOINCREMENT
                       UNIQUE
                       NOT NULL,
    job        INTEGER REFERENCES job (id)
                       NOT NULL
                       UNIQUE,
    amino_faa  TEXT    NOT NULL,
    codon_fna  TEXT    NOT NULL,
    output_gff TEXT    NOT NULL
);


-- Table: db
CREATE TABLE db (
    id       INTEGER PRIMARY KEY AUTOINCREMENT
                     UNIQUE
                     NOT NULL,
    filepath VARCHAR UNIQUE
                     NOT NULL,
);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
