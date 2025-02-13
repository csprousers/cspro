#pragma once

#include <zReformatO/CaseItemConverter.h>
#include <zUtilO/MimeType.h>


class BinaryCaseItemConverter : public CaseItemConverter
{
public:
    BinaryCaseItemConverter(const BinaryCaseItem& input_binary_case_item, const CaseItemIndex& input_index);

    bool ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index) override;
    bool ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index) override;
    bool ToBinary(const BinaryCaseItem& output_binary_case_item, CaseItemIndex& output_index) override;

private:
    const BinaryCaseItem& m_inputBinaryCaseItem;
    const CaseItemIndex& m_inputIndex;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline BinaryCaseItemConverter::BinaryCaseItemConverter(const BinaryCaseItem& input_binary_case_item, const CaseItemIndex& input_index)
    :   m_inputBinaryCaseItem(input_binary_case_item),
        m_inputIndex(input_index)
{
}


inline bool BinaryCaseItemConverter::ToNumber(const NumericCaseItem& /*output_numeric_case_item*/, CaseItemIndex& /*output_index*/)
{
    // BINARY_TYPES_TO_ENGINE_TODO for 8.1, potentially Document (text/plain) -> number
    return false;
}


inline bool BinaryCaseItemConverter::ToString(const StringCaseItem& /*output_string_case_item*/, CaseItemIndex& /*output_index*/)
{
    // BINARY_TYPES_TO_ENGINE_TODO for 8.1, potentially Document (text/plain) -> string
    return false;
}


inline bool BinaryCaseItemConverter::ToBinary(const BinaryCaseItem& output_binary_case_item, CaseItemIndex& output_index)
{
    const ContentType output_content_type = output_binary_case_item.GetDictionaryItem().GetContentType();
    ASSERT(output_content_type == ContentType::Audio ||
           output_content_type == ContentType::Document ||
           output_content_type == ContentType::Geometry ||
           output_content_type == ContentType::Image);

    const BinaryData* binary_data = m_inputBinaryCaseItem.GetBinaryData_noexcept(m_inputIndex);

    // if there was an error getting the binary data, return success
    // (because the error will already have been logged by GetBinaryData_noexcept)
    if( binary_data == nullptr )
        return true;

    // check if the conversion is suitable
    bool can_convert = ( output_content_type == ContentType::Document );

    if( !can_convert )
    {
        const std::optional<std::wstring> mime_type = binary_data->GetMetadata().GetEvaluatedMimeType();

        if( mime_type.has_value() )
        {
            if( output_content_type == ContentType::Audio )
            {
                can_convert = MimeType::IsAudioType(*mime_type);
            }

            else if( output_content_type == ContentType::Geometry )
            {
                can_convert = ( mime_type == MimeType::Type::GeoJson );
            }

            else if( output_content_type == ContentType::Image )
            {
                can_convert = MimeType::IsImageType(*mime_type);
            }
        }
    }

    if( can_convert )
        output_binary_case_item.GetBinaryDataAccessor(output_index).SetBinaryData(*binary_data);

    return can_convert;
}
