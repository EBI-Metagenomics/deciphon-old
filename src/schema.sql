--
-- File generated with SQLiteStudio v3.3.3 on Tue Oct 19 14:15:37 2021
--
-- Text encoding used: UTF-8
--
PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: abc
CREATE TABLE abc (
    name       VARCHAR (15)  PRIMARY KEY
                             UNIQUE
                             NOT NULL,
    size       INTEGER       NOT NULL
                             CONSTRAINT [non-negative] CHECK (size> 0),
    sym_idx64  VARCHAR (255) NOT NULL,
    symbols    VARCHAR (31)  NOT NULL,
    creation   [DATETIME]    GENERATED ALWAYS AS (date('now')),
    type       VARCHAR (7)   NOT NULL
                             CHECK (type IN ('dna', 'rna', 'amino')),
    any_symbol CHAR (1)      NOT NULL
);

-- Table: target
CREATE TABLE target (
    sequence VARCHAR (2147483647) NOT NULL,
    task     INTEGER              REFERENCES task (id) ON DELETE CASCADE
                                  NOT NULL
);

-- Table: task
CREATE TABLE task (
    id             INTEGER      PRIMARY KEY AUTOINCREMENT
                                UNIQUE
                                NOT NULL,
    cfg_loglik     BOOLEAN      NOT NULL,
    cfg_null       BOOLEAN      NOT NULL,
    multiple_hits  BOOLEAN      NOT NULL,
    hmmer3_compat  BOOLEAN      NOT NULL,
    abc            VARCHAR (15) REFERENCES abc (name)
                                NOT NULL,
    local_creation DATETIME     NOT NULL
                                GENERATED ALWAYS AS (date('now'))
);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
