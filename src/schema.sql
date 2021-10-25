--
-- File generated with SQLiteStudio v3.3.3 on Mon Oct 25 10:23:38 2021
--
-- Text encoding used: UTF-8
--
PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: abc
CREATE TABLE abc (
    id         INTEGER       PRIMARY KEY AUTOINCREMENT
                             UNIQUE
                             NOT NULL,
    name       VARCHAR (15)  UNIQUE
                             NOT NULL,
    size       INTEGER       NOT NULL
                             CONSTRAINT [non-negative] CHECK (size > 0),
    sym_idx64  VARCHAR (255) NOT NULL,
    symbols    VARCHAR (31)  NOT NULL,
    creation   DATETIME      NOT NULL,
    type       VARCHAR (7)   NOT NULL
                             CHECK (type IN ('dna', 'rna', 'amino')),
    any_symbol CHAR (1)      NOT NULL,
    user       INTEGER REFERENCES user (id)
);


-- Table: email
CREATE TABLE email (
    id      INTEGER PRIMARY KEY AUTOINCREMENT
                    UNIQUE
                    NOT NULL,
    user    INTEGER REFERENCES user (id),
    address VARCHAR UNIQUE
                    NOT NULL
);


-- Table: job
CREATE TABLE job (
    id             INTEGER      PRIMARY KEY AUTOINCREMENT
                                UNIQUE
                                NOT NULL,
    sid            VARCHAR (19) UNIQUE
                                NOT NULL,
    multiple_hits  BOOLEAN      NOT NULL,
    hmmer3_compat  BOOLEAN      NOT NULL,
    abc            INTEGER      REFERENCES abc (id)
                                NOT NULL,
    target         INTEGER      REFERENCES target (id)
                                NOT NULL,
    status         VARCHAR (4)  CHECK (status IN ('pend', 'run', 'done', 'fail') )
                                NOT NULL
                                DEFAULT ('pend'),
    status_log     VARCHAR,
    submission     DATETIME     NOT NULL,
    exec_started   DATETIME,
    exec_ended     DATETIME,
    contacted      INTEGER      NOT NULL
                                DEFAULT (0),
    last_contacted DATETIME,
    user           INTEGER      REFERENCES user (id)
);


-- Table: query
CREATE TABLE [query] (
    id   INTEGER PRIMARY KEY AUTOINCREMENT
                 NOT NULL
                 UNIQUE,
    name VARCHAR NOT NULL,
    seq  VARCHAR NOT NULL,
    job  INTEGER REFERENCES job (id)
                 NOT NULL
);


-- Table: result
CREATE TABLE result (
    id         INTEGER PRIMARY KEY AUTOINCREMENT
                       UNIQUE
                       NOT NULL,
    job        INTEGER REFERENCES job (id)
                       NOT NULL
                       UNIQUE
                       DEFAULT dwdw,
    amino_faa  TEXT    NOT NULL,
    codon_fna  TEXT    NOT NULL,
    output_gff TEXT    NOT NULL
);


-- Table: target
CREATE TABLE target (
    id       INTEGER     PRIMARY KEY AUTOINCREMENT
                         UNIQUE
                         NOT NULL,
    name     VARCHAR     UNIQUE
                         NOT NULL,
    filepath VARCHAR     UNIQUE
                         NOT NULL,
    xxh3     INTEGER     UNIQUE
                         NOT NULL,
    type     VARCHAR (3) NOT NULL
                         CHECK (type IN ('std', 'pro')),
    user     INTEGER     REFERENCES user (id)
);


-- Table: user
CREATE TABLE user (
    id       INTEGER      PRIMARY KEY AUTOINCREMENT
                          UNIQUE
                          NOT NULL,
    username VARCHAR (31) UNIQUE
                          NOT NULL,
    name     VARCHAR      NOT NULL,
    admin    BOOLEAN      NOT NULL
                          DEFAULT (FALSE)
);


COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
