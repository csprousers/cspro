#include "stdafx.h"
#include "BinaryCaseItem.h"
#include <zUtilO/BinaryDataAccessor.h>


BinaryCaseItem::BinaryCaseItem(const CDictItem& dict_item)
    :   CaseItem(dict_item, Type::Binary)
{
}


size_t BinaryCaseItem::GetSizeForMemoryAllocation() const
{
    return sizeof(BinaryDataAccessor);
}


void BinaryCaseItem::AllocateMemory(void* data_buffer) const
{
    new(data_buffer) BinaryDataAccessor();
}


void BinaryCaseItem::DeallocateMemory(void* data_buffer) const
{
    static_cast<BinaryDataAccessor*>(data_buffer)->~BinaryDataAccessor();
}


void BinaryCaseItem::CopyValue(void* data_buffer, const void* copy_data_buffer) const
{
    const BinaryDataAccessor& copy_binary_data_accessor = *static_cast<const BinaryDataAccessor*>(copy_data_buffer);
    BinaryDataAccessor& this_binary_data_accessor = *static_cast<BinaryDataAccessor*>(data_buffer);

    if( copy_binary_data_accessor.IsDefined() )
    {
        try
        {
            const BinaryData& binary_data = copy_binary_data_accessor.GetBinaryData();
            this_binary_data_accessor.SetBinaryData(binary_data);
            return;
        }
        catch(...) { } // ignore errors getting the content
    }

    this_binary_data_accessor.Clear();
}


size_t BinaryCaseItem::StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const
{
    const BinaryDataAccessor& binary_data_accessor = *static_cast<const BinaryDataAccessor*>(data_buffer);
    const bool binary_data_is_defined = ( binary_data_accessor.IsDefined() );
    const BinaryData* binary_data = nullptr;

    if( binary_data_is_defined )
    {
        try
        {
            binary_data = &binary_data_accessor.GetBinaryData();
        }
        catch(...) { } // ignore errors getting the content
    }

    size_t binary_size = 0;

    // routines for filling the binary buffer
    auto store_buffer = [&](const void* value, size_t value_size)
    {
        if( binary_buffer != nullptr )
        {
            memcpy(binary_buffer, value, value_size);
            binary_buffer += value_size;
        }

        binary_size += value_size;
    };

    auto store_value = [&](auto value)
    {
        store_buffer(&value, sizeof(value));
    };

    auto store_string = [&](wstring_view string_sv)
    {
        // int (string length)
        store_value(static_cast<int>(string_sv.length()));

        // the contents
        store_buffer(string_sv.data(), sizeof(TCHAR) * string_sv.length());
    };


    // bool (BinaryData is defined)
    store_value(static_cast<bool>(binary_data_is_defined));

    if( binary_data_is_defined )
    {
        // size_t (size of the contents)
        store_value(static_cast<size_t>(binary_data->GetContent().size()));

        // the contents
        store_buffer(binary_data->GetContent().data(), binary_data->GetContent().size());

        // size_t (metadata properties size)
        const std::vector<std::tuple<std::wstring, std::wstring>>& properties = binary_data->GetMetadata().GetProperties();

        store_value(static_cast<size_t>(properties.size()));

        // the metadata
        for( const auto& [attribute, value] : properties )
        {
            store_string(attribute);
            store_string(value);
        }
    }

    return binary_size;
}


size_t BinaryCaseItem::RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const
{
    size_t binary_size = 0;

    auto advance_binary_buffer = [&](size_t value_size)
    {
        binary_buffer += value_size;
        binary_size += value_size;
    };

    auto read_value = [&](size_t value_size) -> const void*
    {
        const std::byte* value = binary_buffer;
        advance_binary_buffer(value_size);
        return value;
    };

    auto read_string = [&]()
    {
        const int string_length = *static_cast<const int*>(read_value(sizeof(int)));
        std::wstring string(reinterpret_cast<const TCHAR*>(binary_buffer), string_length);
        advance_binary_buffer(sizeof(TCHAR) * string_length);
        return string;
    };

    const bool binary_data_defined = *static_cast<const bool*>(read_value(sizeof(bool)));

    if( !binary_data_defined )
    {
        ResetValue(data_buffer);
    }

    else
    {
        const size_t content_size = *static_cast<const size_t*>(read_value(sizeof(size_t)));

        std::vector<std::byte> content(binary_buffer, binary_buffer + content_size);
        advance_binary_buffer(content_size);

        BinaryDataMetadata binary_data_metadata;

        const size_t properties_size = *static_cast<const size_t*>(read_value(sizeof(size_t)));

        for( size_t i = 0; i < properties_size; ++i )
        {
            const std::wstring attribute = read_string();
            binary_data_metadata.SetProperty(attribute, read_string());
        }

        BinaryDataAccessor& binary_data_accessor = *static_cast<BinaryDataAccessor*>(data_buffer);
        binary_data_accessor.SetBinaryData(std::move(content), std::move(binary_data_metadata));
    }

    return binary_size;
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

    auto compare_size = [](auto size1, auto size2)
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
    const BinaryData* binary_data1 = GetBinaryData_noexcept(index1, binary_data_accessor1);
    const BinaryData* binary_data2 = GetBinaryData_noexcept(index2, binary_data_accessor2);

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


void BinaryCaseItem::HandleException(const CSProException& exception, const CaseItemIndex& index)
{
    // log the error
    const Case& data_case = index.GetCase();

    if( data_case.GetCaseConstructionReporter() != nullptr )
        data_case.GetCaseConstructionReporter()->BinaryDataIOError(data_case, true, exception.GetErrorMessage());
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


const BinaryDataMetadata* BinaryCaseItem::GetBinaryDataMetadata_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept
{
    try
    {
        if( binary_data_accessor.IsDefined() )
            return &binary_data_accessor.GetBinaryDataMetadata();
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
