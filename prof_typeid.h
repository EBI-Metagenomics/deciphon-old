#ifndef PROF_TYPEID_H
#define PROF_TYPEID_H

enum prof_typeid
{
  PROF_NULL,
  PROF_STD,
  PROF_PROT,
};

char const *prof_typeid_name(enum prof_typeid typeid);

#endif
