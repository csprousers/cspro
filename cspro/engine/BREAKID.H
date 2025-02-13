#pragma once
//---------------------------------------------------------------------------
//
//  BREAKID.h
//
//---------------------------------------------------------------------------

// RHF COM Jul 31, 2001 #define BREAKMAXIDLEN   20              // ???
#define BREAKMAXIDLEN   128
#define MAXBREAKVARS    16              // max # of vars in a break-id

#pragma pack( push, break_id_include ) // rcl, Jun 18 2004
#pragma pack(2)
typedef struct
  {
    short   ctnum;
    long    breakid[MAXBREAKVARS];
  } BREAK_ID;

#pragma pack( pop, break_id_include ) // rcl, Jun 18 2004

// RHF INIC Apr 16, 2003
class CBreakById {
public:
    int     m_iSymVar;
    int     m_iLen;
};
// RHF END Apr 16, 2003
