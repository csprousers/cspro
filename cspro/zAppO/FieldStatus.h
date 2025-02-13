#pragma once

#include <zAppO/zAppO.h>
#include <zJson/JsonSerializer.h>

class CaseItem;
class CaseItemIndex;


enum class FieldStatus : int { Unvisited, Visited, Current, Skipped };

DECLARE_ENUM_JSON_SERIALIZER_CLASS(FieldStatus, ZAPPO_API)


using FieldStatusRetriever = std::function<FieldStatus(const CaseItem& case_item, const CaseItemIndex& index)>;
