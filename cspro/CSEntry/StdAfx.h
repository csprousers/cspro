#pragma once

#include <engine/StandardSystemIncludes.h>

#include <CSEntry/DEEdit.h>
#include <CSEntry/resource.h>
#include <CSEntry/UWM.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilO/Specfile.h>
#include <zDictO/DDClass.h>
#include <zCaseO/CaseSummary.h>
#include <zCapiO/UWM.h>
#include <zCaseTreeF/CaseTree.h>
#include <zCaseTreeF/UWM.h>
#include <zLogicO/SpecialFunction.h>

enum END_MODE { PARTIAL_SAVE = 1 , FINISH_CASE = 2 , DISCARD_CASE, CANCEL_MODE };
