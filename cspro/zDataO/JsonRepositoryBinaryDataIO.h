#pragma once

#include <zCaseO/CaseJsonSerializer.h>
#include <zUtilO/BinaryDataReader.h>


class JsonRepositoryCaseJsonParserHelper : public CaseJsonParserHelper
{
public:
    JsonRepositoryCaseJsonParserHelper(JsonRepository& json_repository);

    std::unique_ptr<BinaryDataReader> CreateBinaryDataReader(BinaryDataMetadata binary_data_metadata, const JsonNode<wchar_t>& json_node) override;

private:
    JsonRepository& m_jsonRepository;
};


class JsonRepositoryBinaryDataIO : public BinaryDataReader
{
public:
    JsonRepositoryBinaryDataIO(JsonRepository& json_repository, BinaryDataMetadata binary_data_metadata);

    BinaryData GetBinaryData() override;

    const BinaryDataMetadata& GetMetadata() override { return m_binaryDataMetadata; }

    uint64_t GetSize() override;

    void OnBinaryDataChange() override { m_binaryDataChanged = true; }

    static void WriteBinaryData(JsonRepository& json_repository, JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);

private:
    const std::wstring& EvaluateFilename();

private:
    JsonRepository& m_jsonRepository;
    BinaryDataMetadata m_binaryDataMetadata;
    std::wstring m_evaluatedFilename;
    std::optional<BinaryData> m_binaryData;
    bool m_binaryDataChanged;
};
