PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

CREATE TABLE abc (
    name       VARCHAR(15)  PRIMARY KEY
                            UNIQUE
                            NOT NULL,
    size       INTEGER      NOT NULL
                            CONSTRAINT [non-negative] CHECK (size> 0),
    sym_idx64  VARCHAR(255) NOT NULL,
    symbols    VARCHAR(31)  NOT NULL,
    creation   DATETIME     NOT NULL,
    type       VARCHAR(7)   NOT NULL
                            CHECK (type IN ('dna', 'rna', 'amino')),
    any_symbol CHAR (1)     NOT NULL
);

CREATE TABLE target (
    sequence VARCHAR(2147483647) NOT NULL,
    task     INTEGER             REFERENCES task (id) ON DELETE CASCADE
                                 NOT NULL
);

CREATE TABLE task (
    id             INTEGER     PRIMARY KEY AUTOINCREMENT
                               UNIQUE
                               NOT NULL,
    cfg_loglik     BOOLEAN     NOT NULL,
    cfg_null       BOOLEAN     NOT NULL,
    multiple_hits  BOOLEAN     NOT NULL,
    hmmer3_compat  BOOLEAN     NOT NULL,
    abc            VARCHAR(15) REFERENCES abc (name)
                               NOT NULL,
    creation       DATETIME    NOT NULL
);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
