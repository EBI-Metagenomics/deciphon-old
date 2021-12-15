#ifndef PROFILE_TYPES_H
#define PROFILE_TYPES_H

enum profile_typeid
{
    PROFILE_NULL,
    PROFILE_STANDARD,
    PROFILE_PROTEIN,
};

char const *profile_typeid_name(enum profile_typeid typeid);

#endif
