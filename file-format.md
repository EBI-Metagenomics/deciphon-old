# Deciphon file

The file is a serialized MessagePack Map object having three key-value pairs.

    String("header")  : ...
    String("metadata"): ...
    String("profile") : ...

Its content is roughly specified by the following JSON example,
where `null` has been used to denote Bin objects.

```json
{
  "header": {
    "magic_number": 50928,
    "profile_typeid": 2,
    "float_bytes": 4,
    "entry_dist": 1,
    "epsilon": 0.01,
    "abc": null,
    "amino": null,
    "profile_size": [3827, 2161]
  },
  "metadata": [null, null],
  "profile": [
    {
      "null": null,
      "alt": null,
      "R": 1,
      "S": 1,
      "N": 1,
      "B": 1,
      "E": 1,
      "J": 1,
      "C": 1,
      "T": 1
    },
    {
      "null": null,
      "alt": null,
      "R": 1,
      "S": 2,
      "N": 3,
      "B": 4,
      "E": 5,
      "J": 6,
      "C": 7,
      "T": 8
    }
  ]
}
```

Let N be the number of profiles.

## Header

header.magic_number: Integer (s16) (0xC6F0)
header.profile_typeid: Integer (s8) (1 or 2)
header.float_bytes : Integer (u8) (4 or 8)
header.entry_dist : Integer (u8) (1 or 2)
header.epsilon : Float (f4/f8) (optional)
header.abc : Bin (imm_abc)
header.amino : Bin (imm_amino) (optional)
header.profile_size: Array(Integer) (N u64 elements)

## Metadata

metadata: Array(Bin) (N profile-specific metadata elements described as follows)

    +---------------------------------------------------+   +-------------------------+
    |        Profile 1        |        Profile 2        |   |        Profile N        |
    |    Name    | Accession  |    Name    | Accession  |   |    Name    | Accession  |
    +============+============+============+============+...+============+============+
    |  c-string  |  c-string  |  c-string  |  c-string  |   |  c-string  |  c-string  |
    +============+============+============+============+   +============+============+

### Temporary metadata

    +---------------------------------------------------+   +-------------------------+
    |        Profile 1        |        Profile 2        |   |        Profile N        |
    |    Name    | Accession  |    Name    | Accession  |   |    Name    | Accession  |
    +============+============+============+============+...+============+============+
    |   String   |   String   |   String   |   String   |   |   String   |   String   |
    +============+============+============+============+   +============+============+

## Profile

profile: Array(Map) (N profile-compact definitions described as follows)

    null: Bin     (imm_dp)
    alt : Bin     (imm_dp)
    R   : Integer (u16)
    S   : Integer (u16)
    N   : Integer (u16)
    B   : Integer (u16)
    E   : Integer (u16)
    J   : Integer (u16)
    C   : Integer (u16)
    T   : Integer (u16)

Let `n` be the number of profiles.

|-----------------------------------------------------------|
| HEADER BLOCK |
| |
| |-------------------------------------------------------| |
| | magic_number: mp-u64 | |
| |-------------------------------------------------------| |
| | prof_type: mp-u8 (normal or protein) | |
| |-------------------------------------------------------| |
| | float_bytes: mp-u8 | |
| |-------------------------------------------------------| |
| | entry_dist: mp-u8 (active on protein prof_type) | |
| |-------------------------------------------------------| |
| | epsilon: mp-f4 or mp-f8 (active on protein prof_type) | |
| |-------------------------------------------------------| |
| | abc: imm_abc (nuclt on protein prof_type) | |
| |-------------------------------------------------------| |
| | amino: imm_amino (active on protein prof_type) | |
| |-------------------------------------------------------| |
| | n: mp-u32 | |
| |-------------------------------------------------------| |
| |
|-----------------------------------------------------------|

|-----------------------------------------------------------|
| PROFILE SIZES BLOCK |
| |
| |---------------------------------| |
| | profile_size[1] : mp-uint | |
| | profile_size[2] : mp-uint | |
| | ... | |
| | profile_size[n]: mp-uint | |
| |---------------------------------| |
| |
|-----------------------------------------------------------|

|-----------------------------------------------------------|
| METADATA BLOCK |
| |
| |-----------------------------------------------| |
| | size: mp-u32 (remaining size of this block) | |
| |-----------------------------------------------| |
| |
| |----------------| |
| | name: c-string | Profile 1 |
| | acc : c-string | |
| |----------------| |
| ... |
| |----------------| |
| | name: c-string | Profile n |
| | acc : c-string | |
| |----------------| |
| |
|-----------------------------------------------------------|

|-----------------------------------------------------------|
| DP block |
| |
| |---------------------------------------------| |
| | null: imm_dp | Profile 1 |
| |---------------------------------------------| |
| | alt : imm_dp | |
| |---------------------------------------------| |
| | R: mp-u16 | |
| | S: mp-u16 Special states | |
| | N: mp-u16 (active on protein prof_type) | |
| | B: mp-u16 | |
| | E: mp-u16 | |
| | J: mp-u16 | |
| | C: mp-u16 | |
| | T: mp-u16 | |
| |---------------------------------------------| |
| ... |
| |---------------------------------------------| |
| | null: imm_dp | Profile n |
| |---------------------------------------------| |
| | alt : imm_dp | |
| |---------------------------------------------| |
| | R: mp-u16 | |
| | S: mp-u16 Special states | |
| | N: mp-u16 (active on protein prof_type) | |
| | B: mp-u16 | |
| | E: mp-u16 | |
| | J: mp-u16 | |
| | C: mp-u16 | |
| | T: mp-u16 | |
| |---------------------------------------------| |
| |
|-----------------------------------------------------------|

|-------------------------------------------------------|
| null: imm_dp |
| alt : imm_dp |
| |---------------------------------------------------| | Profile 2
| | Special states | |
| |---------------------------------------------------| |
|-------------------------------------------------------|
...
|-------------------------------------------------------|
| null: imm_dp |
| alt : imm_dp |
| |---------------------------------------------------| | Profile `n`
| | Special states | |
| |---------------------------------------------------| |
|-------------------------------------------------------|

# Temporary metadata file

|-----------------|
| name: mp-string | Profile 1
| acc : mp-string |
|-----------------|
| name: mp-string | Profile 2
| acc : mp-string |
|-----------------|
...
|-----------------|
| name: mp-string | Profile `n`
| acc : mp-string |
|-----------------|

# Temporary DP file

Profile 1 : imm_dp, imm_dp
Profile 2 : imm_dp, imm_dp
...
Profile `n`: imm_dp, imm_dp

Hello world
Hello world
Hello world
Hello world
