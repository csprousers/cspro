#pragma once

#include <zDictO/DDClass.h>


struct COMMON_INFO
{
    int iCommonSect[MaxNumberLevels] = { 0 };         // Common Sections per level
    std::vector<const CDictItem*> arrayCommonItems;   // Common items for all levels. 1 array
    int iCommonFirstPos[MaxNumberLevels + 1] = { 0 };
    int iCommonLastPos[MaxNumberLevels + 1] = { 0 };
};
