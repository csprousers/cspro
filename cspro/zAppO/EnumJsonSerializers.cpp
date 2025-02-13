#include "stdafx.h"
#include "FieldStatus.h"


DEFINE_ENUM_JSON_SERIALIZER_CLASS(FieldStatus,
    { FieldStatus::Unvisited, _T("unvisited") },
    { FieldStatus::Visited,   _T("visited") },
    { FieldStatus::Current,   _T("current") },
    { FieldStatus::Skipped,   _T("skipped") })
