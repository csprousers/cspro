#pragma once

#include <engine/CtDef.h>

#define TBL_NAMELEN    8
#define TBD_NAMELEN  128

typedef struct
  {
    csprochar    name[TBL_NAMELEN+2]; // RHF 10/3/98 Antes decia +1
    short   tblnum;
    short   tbltype;
    short   tblacum;
    short   tbldec;
    short   nrow;
    short   ncol;
    short   nlay;
    short   ndim;
    int     size;
    void    *ptab;  // Internal use for TAB pointer
  } TBLINFO;

#define MAXBREAK_VARS   16

typedef struct
  {
    csprochar    name[TBD_NAMELEN];
    short   keylen;
    short   ntables;
    short   nvarsbreak;
    short   nvars;
    csprochar    *vname;
    csprochar    *tname;
    csprochar    *bvname;
    short   varlen[MAXBREAK_VARS]; /* Largo de las variables de break */
  } TBDINFO;

#define KEY_NEXT        1
#define KEY_PREV        2
#define KEY_CURRENT     3
#define KEY_BEFOREFIRST 4
#define KEY_AFTERLAST   5
