#pragma once

#ifdef GENCODE
#include <engine/Exappl.h>
#else
#include <engine/Tables.h>
#endif
#include <engine/COMPILAD.H>
#include <engine/Engine.h>
#include <engine/Ctab.h>
#include <engine/COMPUTIL.H>
#include <engine/ParameterManager.h>
#include <zToolsO/Utf8Convert.h>
#include <zDictO/DDClass.h>
#include <zCaseO/CaseAccess.h>

#ifdef GENCODE
  #define GENERATE_CODE(x) if( Flagcomp ) x
#else
  #define GENERATE_CODE(x)
#endif

