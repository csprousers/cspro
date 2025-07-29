#pragma once

#include <zDataO/CSWebRepository.h>
#include <zDataO/SyncBinaryContentReader.h>
#include <zCaseO/CaseJsonSerializer.h>


// --------------------------------------------------------------------------
// CSWebBinaryContentReader
// --------------------------------------------------------------------------

class CSWebBinaryContentReader : public SyncBinaryContentReader
{
    friend class CSWebCaseJsonParserHelper;
    struct Data;

public:
    CSWebBinaryContentReader(std::shared_ptr<Data> data, std::optional<uint64_t> size);

    // BinaryContentReader overrides
    const UniqueId* GetUniqueId() const override;

protected:
    BinaryContentCacher::CacheableContent GetContentWorker(const std::string& signature) override;

private:
    struct Data
    {
        std::variant<CSWebRepository*, UniqueId> repository_or_repository_id;
        std::string dictionary_name;
        std::shared_ptr<CSWebConnection> csweb_connection;
    };

    std::shared_ptr<Data> m_data;
};


// --------------------------------------------------------------------------
// CSWebCaseJsonParserHelper
// --------------------------------------------------------------------------

class CSWebCaseJsonParserHelper : public CaseJsonParserHelper
{
public:
    CSWebCaseJsonParserHelper(std::variant<CSWebRepository*, UniqueId> repository_or_repository_id,
                              std::shared_ptr<const CaseAccess> case_access,
                              std::shared_ptr<CSWebConnection> csweb_connection);

    std::unique_ptr<BinaryContentReader> CreateBinaryContentReader(std::optional<uint64_t> size) override;

private:
    std::shared_ptr<CSWebBinaryContentReader::Data> m_data;
};
