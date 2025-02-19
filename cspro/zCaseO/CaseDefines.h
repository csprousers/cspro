#pragma once

#include <zCaseO/zCaseO.h>


// do not renumber these values
enum class PartialSaveMode
{
    None = 0,
    Add = 1,
    Modify = 2,
    Verify = 3
};

DECLARE_ENUM_JSON_SERIALIZER_CLASS(PartialSaveMode, ZCASEO_API)
