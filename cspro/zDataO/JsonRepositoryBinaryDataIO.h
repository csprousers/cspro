#pragma once

#include <zCaseO/CaseJsonSerializer.h>
#include <zUtilO/BinaryContentReader.h>


class JsonRepositoryCaseJsonParserHelper : public CaseJsonParserHelper
{
public:
    JsonRepositoryCaseJsonParserHelper(JsonRepository& json_repository);

    std::unique_ptr<BinaryContentReader> CreateBinaryContentReader(std::optional<uint64_t> size) override;

private:
    JsonRepository& m_jsonRepository;
};


class JsonRepositoryBinaryDataIO : public BinaryContentReader
{
public:
    JsonRepositoryBinaryDataIO(JsonRepository& json_repository);

    const UniqueId* GetUniqueId() const override;

    uint64_t GetSize(const std::string& signature) override;

    static void WriteBinaryData(JsonRepository& json_repository, JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);

protected:
    BinaryContentCacher::CacheableContent GetContentWorker(const std::string& signature) override;

private:
    const std::string& EvaluateFilePath(const std::string& signature);

private:
    JsonRepository& m_jsonRepository;
    std::string m_evaluatedFilePath;
};
