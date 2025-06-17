#pragma once

//-----------------------------------------------------------------------
//
//  COMPILAD.h  common data to every compiler functions
//
//-----------------------------------------------------------------------
#include <zLogicO/KeywordTable.h>
#include <engine/Comp.h>
#include <engine/NODES.H>
#include <zEngineO/Nodes/Set.h>

//-----------------------------------------------------------------------
//  generated object-code structures
//-----------------------------------------------------------------------

// extensions to support SET commands

#define BEHAVIOR_PATH                   1
#define BEHAVIOR_CANENTER_NOTAPPL       2
#define BEHAVIOR_CANENTER_OUTOFRANGE    3
#define BEHAVIOR_MESSAGES_DISPLAY       4
#define BEHAVIOR_MESSAGES_ERRMSG        5
#define BEHAVIOR_MOUSE                  6 // victor Feb 16, 00
//#define BEHAVIOR_PUSHMODE             7 // removed
#define BEHAVIOR_ENDGROUP               8 // victor Feb 16, 00
#define BEHAVIOR_ENDLEVEL               9 // victor Feb 16, 00
#define BEHAVIOR_MSGNUMBER_DISPLAY     10 // victor Jun 08, 00
#define BEHAVIOR_MSGNUMBER_ERRMSG      11 // victor Jun 08, 00
#define BEHAVIOR_EXPORT_DATA           21 // victor Dec 18, 00
#define BEHAVIOR_EXPORT_SPSS           22 // victor Dec 18, 00
#define BEHAVIOR_EXPORT_SAS            23 // victor Dec 18, 00
#define BEHAVIOR_EXPORT_STATA          24 // victor Dec 18, 00
#define BEHAVIOR_EXPORT_ALL            25 // RHF Oct 13, 2004
#define BEHAVIOR_EXPORT_CSPRO          26 // RHF Oct 08, 2004
#define BEHAVIOR_EXPORT_TABDELIM       27 // RHF Oct 13, 2004
#define BEHAVIOR_EXPORT_COMMADELIM     28 // RHF Oct 13, 2004
#define BEHAVIOR_EXPORT_SEMICOLONDELIM 29 // RHF Oct 13, 2004
#define BEHAVIOR_EXPORT_ALL4           30 // RHF Aug 02, 2006
#define BEHAVIOR_EXPORT_R              33 // 20120507
#define BEHAVIOR_EXPORT_ALL5           34

#define BEHAVIOR_CANENTER_NOTAPPL_NOSHOW 31

#define BEHAVIOR_EXPORT_ITEMONLY        1
#define BEHAVIOR_EXPORT_SUBITEMONLY     2
#define BEHAVIOR_EXPORT_ITEMSUBITEM     3

// flags for additional export metadata
// (bit flags since all can be enabled at once)
#define BEHAVIOR_EXPORT_UNICODE         1 // 20120416
#define BEHAVIOR_EXPORT_COMMA_DECIMAL   2

#define BEHAVIOR_CHECKRANGES           51               // victor Dec 07, 00
#define BEHAVIOR_SKIPSTRUC             52               // victor Mar 14, 01
#define BEHAVIOR_SKIPSTRUCIMPUTE       53               // RHF Nov 09, 2001


#define BEHAVIOR_EXIT                   71

#define BEHAVIOR_SPECIALVALUES          72

enum class GetLabelSearchType { ByCode = 1, ByLabel = 2 };

#define PPT(i) m_engineData->logic_byte_code.GetCodeAtPosition(i)
