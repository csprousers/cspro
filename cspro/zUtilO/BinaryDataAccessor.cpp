#include "StdAfx.h"
#include "BinaryDataAccessor.h"
#include "BinaryDataReader.h"


BinaryDataAccessor::BinaryDataAccessor(const BinaryDataAccessor& rhs)
    :   m_binaryDataReader(rhs.m_binaryDataReader),
        m_binaryDataReaderRequiresQuerying(rhs.m_binaryDataReaderRequiresQuerying)
{
    if( rhs.m_binaryData != nullptr )
        m_binaryData = std::make_unique<BinaryData>(*rhs.m_binaryData);
}


void BinaryDataAccessor::Clear()
{
    if( m_binaryDataReader != nullptr )
    {
        m_binaryDataReader->OnBinaryDataChange();
        m_binaryDataReaderRequiresQuerying = false;
    }

    m_binaryData.reset();
}


BinaryData& BinaryDataAccessor::GetBinaryDataUsingBinaryDataReader()
{
    ASSERT(m_binaryData == nullptr);

    if( !m_binaryDataReaderRequiresQuerying )
        throw ProgrammingErrorException();

    ASSERT(m_binaryDataReader != nullptr);

    m_binaryData = std::make_unique<BinaryData>(m_binaryDataReader->GetBinaryData());
    m_binaryDataReaderRequiresQuerying = false;

    return *m_binaryData;
}


const BinaryDataMetadata& BinaryDataAccessor::GetBinaryDataMetadataUsingBinaryDataReader()
{
    ASSERT(m_binaryData == nullptr);

    if( m_binaryDataReader == nullptr )
        throw ProgrammingErrorException();

    return m_binaryDataReader->GetMetadata();
}


uint64_t BinaryDataAccessor::GetBinaryDataSizeUsingBinaryDataReader()
{
    ASSERT(m_binaryData == nullptr);

    if( m_binaryDataReader == nullptr )
        throw ProgrammingErrorException();

    return m_binaryDataReader->GetSize();
}


void BinaryDataAccessor::SetBinaryData(BinaryData binary_data)
{
    if( m_binaryDataReader != nullptr )
    {
        m_binaryDataReader->OnBinaryDataChange();
        m_binaryDataReaderRequiresQuerying = false;
    }

    m_binaryData = std::make_unique<BinaryData>(std::move(binary_data));
}


void BinaryDataAccessor::SetBinaryDataReader(std::shared_ptr<BinaryDataReader> binary_data_reader)
{
    ASSERT(binary_data_reader != nullptr);

    if( m_binaryDataReader != nullptr )
    {
        if( m_binaryDataReader == binary_data_reader )
            return;

        m_binaryDataReader->OnBinaryDataChange();
    }

    m_binaryData.reset();
    m_binaryDataReader = std::move(binary_data_reader);
    m_binaryDataReaderRequiresQuerying = true;
}
