#pragma once

//------------------------------------------------------------------------
//  TABLES.h
//------------------------------------------------------------------------

#include <zLogicO/SymbolTable.h>
#include <zEngineO/EngineData.h>

#include <engine/Defines.h>
#include <engine/Settings.h>

#include <engine/Apl.h>
#include <Zissalib/CFlow.h>
#include <Zissalib/GroupT.h>
#include <engine/Form2.h>
#include <engine/BREAKID.H>

// dictionary symbols
#include <engine/Dict.h>
#include <Zissalib/SecT.h>
#include <engine/VarT.h>
#include <engine/relt.h>

// some other forward declarations
class CaseItemReference;
class UserFunctionArgumentEvaluator;

//-------------------------------------------------------------
//  Compatibility access macros
//-------------------------------------------------------------
#define NPT(i) (&NPT_Ref(i))

#define DIP(i) (m_engineData->dictionaries_pre80[i])
#define SIP(i) (m_engineData->sections[i])
#define VIP(i) (m_engineData->variables[i])
#define GIP(i) (m_engineData->groups[i])
