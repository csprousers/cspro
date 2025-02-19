#include "StdAfx.h"
#include "BinaryDataAccessor.h"
#include "BinaryContentReader.h"


BinaryDataAccessor::BinaryDataAccessor(BinaryData binary_data)
    :   m_data(std::move(binary_data))
{
}


BinaryDataAccessor::BinaryDataAccessor(BinaryDataMetadata binary_data_metadata, std::string signature, std::shared_ptr<BinaryContentReader> binary_content_reader)
    :   m_data(ReaderData(std::move(binary_data_metadata), std::move(binary_content_reader))),
        m_signature(std::move(signature))
{
    ASSERT(IsValidSignature(m_signature) && GetBinaryContentReader() != nullptr);
}


const BinaryDataMetadata& BinaryDataAccessor::GetBinaryDataMetadata() const
{
    if( !IsDefined() )
        throw ProgrammingErrorException();

    return std::holds_alternative<BinaryData>(*m_data) ? std::get<BinaryData>(*m_data).GetMetadata() :
                                                         std::get<0>(std::get<ReaderData>(*m_data));
}


BinaryDataMetadata& BinaryDataAccessor::GetBinaryDataMetadata()
{
    if( !IsDefined() )
        throw ProgrammingErrorException();

    return std::holds_alternative<BinaryData>(*m_data) ? std::get<BinaryData>(*m_data).GetMetadata() :
                                                         std::get<0>(std::get<ReaderData>(*m_data));
}


const BinaryContentReader* BinaryDataAccessor::GetBinaryContentReader() const noexcept
{
    if( m_data.has_value() && std::holds_alternative<ReaderData>(*m_data) )
        return std::get<1>(std::get<ReaderData>(*m_data)).get();

    return nullptr;
}


BinaryContentReader* BinaryDataAccessor::GetBinaryContentReader() noexcept
{
    return const_cast<BinaryContentReader*>(const_cast<const BinaryDataAccessor*>(this)->GetBinaryContentReader());
}


uint64_t BinaryDataAccessor::GetBinaryDataSize() const
{
    if( !IsDefined() )
    {
        throw ProgrammingErrorException();
    }

    else if( std::holds_alternative<BinaryData>(*m_data) )
    {
        return std::get<BinaryData>(*m_data).GetContent().size();
    }

    else
    {
        const ReaderData& reader_data = std::get<ReaderData>(*m_data);
        return std::get<1>(reader_data)->GetSize(m_signature);
    }
}


const BinaryData& BinaryDataAccessor::GetBinaryData() const
{
    if( !IsDefined() )
    {
        throw ProgrammingErrorException();
    }

    else if( std::holds_alternative<ReaderData>(*m_data) )
    {
        // load the content from the reader and use the metadata stored by this object
        ReaderData& reader_data = std::get<ReaderData>(*m_data);
        std::shared_ptr<const std::vector<std::byte>> content = std::get<1>(reader_data)->GetContent(m_signature);

        m_data.emplace(BinaryData(std::move(content),
                                  std::move(std::get<0>(reader_data))));
    }

    return std::get<BinaryData>(*m_data);
}


BinaryData& BinaryDataAccessor::GetBinaryData()
{
    return const_cast<BinaryData&>(const_cast<const BinaryDataAccessor&>(*this).GetBinaryData());
}


const std::string& BinaryDataAccessor::GetSignature() const
{
    if( m_signature.empty() && std::holds_alternative<BinaryData>(*m_data) )
        m_signature = PortableFunctions::BinaryMd5(std::get<BinaryData>(*m_data).GetContent());

    ASSERT(( IsDefined() && IsValidSignature(m_signature) ) || ( !IsDefined() && m_signature.empty() ));

    return m_signature;
}


bool BinaryDataAccessor::IsValidSignature(const std::string_view signature_sv)
{
    return ( signature_sv.length() == 32 &&
             signature_sv == SO::ToLower(signature_sv) );
}
