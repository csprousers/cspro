#include "stdafx.h"
#include "FieldStatus.h"
#include "PFF.h"


DEFINE_ENUM_JSON_SERIALIZER_CLASS(FieldStatus,
    { FieldStatus::Unvisited, "unvisited" },
    { FieldStatus::Visited,   "visited" },
    { FieldStatus::Current,   "current" },
    { FieldStatus::Skipped,   "skipped" })


DEFINE_ENUM_JSON_SERIALIZER_CLASS(ShowInApplicationListing,
    { ShowInApplicationListing::Always, "always" },
    { ShowInApplicationListing::Hidden, "hidden" },
    { ShowInApplicationListing::Never,  "never" })


DEFINE_ENUM_JSON_SERIALIZER_CLASS(SyncDirection,
    { SyncDirection::Put,  ToString(SyncDirection::Put) },
    { SyncDirection::Get,  ToString(SyncDirection::Get) },
    { SyncDirection::Both, ToString(SyncDirection::Both) })
