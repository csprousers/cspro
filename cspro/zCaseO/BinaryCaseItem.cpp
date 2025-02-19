#include "stdafx.h"
#include "BinaryCaseItem.h"
#include <zUtilO/BinaryDataAccessor.h>


BinaryCaseItem::BinaryCaseItem(const CDictItem& dict_item)
    :   CaseItem(dict_item, Type::Binary, DataType::Binary, false)
{
}


size_t BinaryCaseItem::GetSizeForMemoryAllocation() const
{
    return sizeof(BinaryDataAccessor);
}


void BinaryCaseItem::AllocateMemory(void* const data_buffer) const
{
    new(data_buffer) BinaryDataAccessor();
}


void BinaryCaseItem::DeallocateMemory(void* const data_buffer) const
{
    static_cast<BinaryDataAccessor*>(data_buffer)->~BinaryDataAccessor();
}


void BinaryCaseItem::CopyValue(void* const data_buffer, const void* const copy_data_buffer) const
{
    const BinaryDataAccessor& copy_binary_data_accessor = *static_cast<const BinaryDataAccessor*>(copy_data_buffer);
    BinaryDataAccessor& this_binary_data_accessor = *static_cast<BinaryDataAccessor*>(data_buffer);
    this_binary_data_accessor = copy_binary_data_accessor;
}


size_t BinaryCaseItem::StoreBinaryValue(const void* const data_buffer, std::byte* binary_buffer) const
{
    const BinaryDataAccessor& binary_data_accessor = *static_cast<const BinaryDataAccessor*>(data_buffer);
    const BinaryData* binary_data = nullptr;

    if( binary_data_accessor.IsDefined() )
    {
        try
        {
            binary_data = &binary_data_accessor.GetBinaryData();
        }
        catch(...) { } // ignore errors getting the content
    }

    // the binary data is written as:
    // - bool (BinaryData is defined)
    // - if BinaryData is defined:
    //    - std::vector<std::byte> (content)
    //    - length (metadata properties size)
    //    - two strings for each metadata property

    const bool binary_data_defined = ( binary_data != nullptr );
    size_t binary_buffer_size = BinarySerializer::Write(binary_buffer, binary_data_defined);

    if( binary_data_defined )
    {
        auto get_adjusted_binary_buffer = [&]()
        {
            return ( binary_buffer == nullptr ) ? binary_buffer :
                                                  ( binary_buffer + binary_buffer_size );
        };

        // content
        binary_buffer_size += BinarySerializer::Write(get_adjusted_binary_buffer(), binary_data->GetContent());

        // metadata properties size
        const std::vector<std::tuple<std::string, std::string>>& properties = binary_data->GetMetadata().GetProperties();
        binary_buffer_size += BinarySerializer::WriteLength(get_adjusted_binary_buffer(), properties.size());

        // the metadata
        for( const auto& [attribute, value] : properties )
        {
            binary_buffer_size += BinarySerializer::Write(get_adjusted_binary_buffer(), attribute);
            binary_buffer_size += BinarySerializer::Write(get_adjusted_binary_buffer(), value);
        }
    }

    return binary_buffer_size;
}


void BinaryCaseItem::RetrieveBinaryValue(void* const data_buffer, const std::byte*& binary_buffer) const
{
    const bool binary_data_defined = BinarySerializer::Read<bool>(binary_buffer);

    if( !binary_data_defined )
    {
        ResetValue(data_buffer);
    }

    else
    {
        std::vector<std::byte> content = BinarySerializer::Read<std::vector<std::byte>>(binary_buffer);

        BinaryDataMetadata binary_data_metadata;

        const size_t properties_size = BinarySerializer::ReadLength(binary_buffer);

        for( size_t i = 0; i < properties_size; ++i )
        {
            const std::string attribute = BinarySerializer::Read<std::string>(binary_buffer);
            binary_data_metadata.SetProperty(attribute, BinarySerializer::Read<std::string>(binary_buffer));
        }

        BinaryDataAccessor& binary_data_accessor = *static_cast<BinaryDataAccessor*>(data_buffer);
        binary_data_accessor = BinaryData(std::move(content), std::move(binary_data_metadata));
    }
}


int BinaryCaseItem::CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const
{
    const BinaryDataAccessor& binary_data_accessor1 = GetBinaryDataAccessor(index1);
    const BinaryDataAccessor& binary_data_accessor2 = GetBinaryDataAccessor(index2);

    // not-defined will be considered less than
    auto compare_defined_state = [](const auto& optional1, const auto& optional2)
    {
        return !optional1 ? ( optional2 ? -1 : 0 ) :
               !optional2 ? 1 :
                            0;
    };

    auto compare_size = [](const auto size1, const auto size2)
    {
        return ( size1 < size2 ) ? -1 :
               ( size1 > size2 ) ?  1 :
                                    0;
    };

    const std::optional<uint64_t> size1 = GetBinaryDataSize_noexcept(index1, binary_data_accessor1);
    const std::optional<uint64_t> size2 = GetBinaryDataSize_noexcept(index2, binary_data_accessor2);

    int comparison = compare_defined_state(size1, size2);

    // if either value is not defined, the defined state comparison is sufficient
    if( comparison != 0 || !size1.has_value() )
        return comparison;

    // otherwise compare the file size
    comparison = compare_size(*size1, *size2);

    if( comparison != 0 )
        return comparison;

    // compare the file contents only when the file size was the same
    const BinaryData* const binary_data1 = GetBinaryData_noexcept(index1, binary_data_accessor1);
    const BinaryData* const binary_data2 = GetBinaryData_noexcept(index2, binary_data_accessor2);

    // if there were errors getting the contents, return the state of the data
    if( binary_data1 == nullptr || binary_data2 == nullptr )
        return compare_defined_state(binary_data1, binary_data2);

    // the size of the loaded data should be the same, but check just in case
    comparison = compare_size(binary_data1->GetContent().size(), binary_data2->GetContent().size());

    if( comparison != 0 )
        return ReturnProgrammingError(comparison);

    // compare the contents
    return memcmp(binary_data1->GetContent().data(), binary_data2->GetContent().data(), binary_data1->GetContent().size());
}


void BinaryCaseItem::Clear(CaseItemIndex& index) const
{
    ResetValue(GetDataBuffer(index));
}


void BinaryCaseItem::HandleException(const CSProException& exception, const CaseItemIndex& index)
{
    // log the error
    const Case& data_case = index.GetCase();

    if( data_case.GetCaseConstructionReporter() != nullptr )
        data_case.GetCaseConstructionReporter()->BinaryDataIOError(data_case, true, exception.what());
}


const BinaryData* BinaryCaseItem::GetBinaryData_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept
{
    try
    {
        if( binary_data_accessor.IsDefined() )
            return &binary_data_accessor.GetBinaryData();
    }

    catch( const CSProException& exception )
    {
        HandleException(exception, index);
    }

    return nullptr;
}


std::optional<uint64_t> BinaryCaseItem::GetBinaryDataSize_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept
{
    try
    {
        if( binary_data_accessor.IsDefined() )
            return binary_data_accessor.GetBinaryDataSize();
    }

    catch( const CSProException& exception )
    {
        HandleException(exception, index);
    }

    return std::nullopt;
}


std::string BinaryCaseItem::GetSuggestedFilename(const CaseItemIndex& index) const
{
    std::optional<std::string> filename;

    try
    {
         filename = GetBinaryDataAccessor(index).GetBinaryDataMetadata().GetFilename();
    }
    catch(...) { }

    // if there is no filename, use the item name with whatever extension the file was saved as
    if( !filename.has_value() )
    {
        filename = GetDictItem().GetName() + index.GetMinimalOccurrencesText(*this);

        try
        {
            const std::optional<std::string> extension = GetBinaryDataAccessor(index).GetBinaryDataMetadata().GetEvaluatedExtension();

            if( extension.has_value() )
                PortableFunctions::MakePathAppendFileExtension(*filename, *extension);
        }
        catch(...) { }
    }

    return std::move(*filename);
}
