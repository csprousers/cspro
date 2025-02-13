#pragma once


struct ExportItemMapping
{
    const CaseItem* case_item;
    size_t item_subitem_occurrence;

    std::wstring formatted_item_name;

    // if an ID item, the level number of the ID record
    std::optional<size_t> id_level_number;

    void* tag;
};


struct ExportRecordMapping
{
    const CaseRecordMetadata* case_record_metadata;

    std::wstring formatted_record_name;

    // the level number of the record
    size_t level_number;

    // the record number on the level
    size_t record_number;

    std::vector<ExportItemMapping> item_mappings;

    void* tag;
};
