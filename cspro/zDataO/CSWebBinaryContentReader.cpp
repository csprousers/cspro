#include "stdafx.h"
#include "CSWebBinaryContentReader.h"
#include "CSWebRepository.h"
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/SyncException.h>


// --------------------------------------------------------------------------
// CSWebBinaryContentReader
// --------------------------------------------------------------------------

CSWebBinaryContentReader::CSWebBinaryContentReader(std::shared_ptr<Data> data, std::optional<uint64_t> size)
    :   SyncBinaryContentReader(std::move(size)),
        m_data(std::move(data))
{
    ASSERT(m_data != nullptr);
}


const UniqueId* CSWebBinaryContentReader::GetUniqueId() const
{
    return &m_data->repository_id;
}


BinaryContentCacher::CacheableContent CSWebBinaryContentReader::GetContentWorker(const std::string& signature)
{
    try
    {
        const std::string data = m_data->csweb_connection->DownloadDictionaryBinaryData(m_data->dictionary_name, signature);
        return SO::CreateByteVector(data);
    }

    catch( const std::exception& exception )
    {
        std::unique_ptr<SyncErrorFormatter> sync_error_formatter;
        CSWebRepository::RethrowException(exception, m_data->dictionary_name, sync_error_formatter);
    }
}



// --------------------------------------------------------------------------
// CSWebCaseJsonParserHelper
// --------------------------------------------------------------------------

CSWebCaseJsonParserHelper::CSWebCaseJsonParserHelper(UniqueId repository_id, std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CSWebConnection> csweb_connection)
    :   CaseJsonParserHelper(case_access),
        m_data(std::make_unique<CSWebBinaryContentReader::Data>(CSWebBinaryContentReader::Data { std::move(repository_id),
                                                                                                 case_access->GetDataDict().GetName(),
                                                                                                 std::move(csweb_connection) }))
{
    ASSERT(m_data->csweb_connection != nullptr);
}


std::unique_ptr<BinaryContentReader> CSWebCaseJsonParserHelper::CreateBinaryContentReader(std::optional<uint64_t> size)
{
    return std::make_unique<CSWebBinaryContentReader>(m_data, std::move(size));
}
