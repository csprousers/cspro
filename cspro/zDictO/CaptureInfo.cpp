#include "StdAfx.h"
#include "CaptureInfo.h"
#include "ValueProcessor.h"
#include <zToolsO/Serializer.h>
#include <zUtilO/Specfile.h>


// --------------------------------------------------------------------------
//  CaptureInfo
// --------------------------------------------------------------------------

namespace
{
    const TCHAR* const CaptureTypeNames[]
    {
        _T("TextBox"),
        _T("RadioButton"),
        _T("CheckBox"),
        _T("DropDown"),
        _T("ComboBox"),
        _T("Date"),
        _T("NumberPad"),
        _T("Barcode"),
        _T("Slider"),
        _T("ToggleButton"),
        _T("Photo"),
        _T("Signature"),
        _T("Audio"),
    };

    const TCHAR* const UnspecifiedCaptureTypeName = _T("Unspecified");


    bool CaptureTypeSupportedOnPlatform(CaptureType capture_type)
    {
#ifdef WIN_DESKTOP
        if( capture_type == CaptureType::Barcode ||
            capture_type == CaptureType::Slider ||
            capture_type == CaptureType::Photo ||
            capture_type == CaptureType::Signature ||
            capture_type == CaptureType::Audio )
#else
        if( capture_type == CaptureType::NumberPad )
#endif
        {
            return false;
        }

        return true;
    }
}


CaptureInfo::CaptureInfo() noexcept
    :   m_captureType(CaptureType::Unspecified)
{
}


CaptureInfo::CaptureInfo(const CaptureInfo& rhs) noexcept
{
    *this = rhs;
}


CaptureInfo::CaptureInfo(CaptureType capture_type) noexcept
{
    SetCaptureType(capture_type);
}


CaptureInfo& CaptureInfo::operator=(const CaptureInfo& rhs) noexcept
{
    SetCaptureType(rhs.m_captureType);

    ASSERT(( m_extendedCaptureInfo != nullptr ) == ( rhs.m_extendedCaptureInfo != nullptr ));

    if( m_captureType == CaptureType::Date )
        GetExtended<DateCaptureInfo>() = rhs.GetExtended<DateCaptureInfo>();

    return *this;
}


bool CaptureInfo::operator==(const CaptureInfo& rhs) const noexcept
{
    if( m_captureType != rhs.m_captureType )
        return false;

    ASSERT(( m_extendedCaptureInfo != nullptr ) == ( rhs.m_extendedCaptureInfo != nullptr ));

    if( m_captureType == CaptureType::Date )
        return ( GetExtended<DateCaptureInfo>() == rhs.GetExtended<DateCaptureInfo>() );

    return true;
}


namespace
{
    template<typename T>
    void CreateExtendedCaptureInfo(std::unique_ptr<ExtendedCaptureInfo>& extended_capture_info)
    {
        if( dynamic_cast<T*>(extended_capture_info.get()) == nullptr )
            extended_capture_info = std::make_unique<T>();
    }
}

void CaptureInfo::SetCaptureType(CaptureType capture_type)
{
    m_captureType = capture_type;

    if( m_captureType == CaptureType::Date )
    {
        CreateExtendedCaptureInfo<DateCaptureInfo>(m_extendedCaptureInfo);
    }

    else
    {
        m_extendedCaptureInfo.reset();
    }
}


void CaptureInfo::Build(CSpecFile& spec_file, const CString& argument)
{
    SetCaptureType(GetCaptureTypeFromSerializableName(argument).value_or(CaptureType::Unspecified));

    if( m_extendedCaptureInfo != nullptr )
        m_extendedCaptureInfo->Build(spec_file);
}


void CaptureInfo::Save(CSpecFile& spec_file, bool use_pre77_command_names) const
{
    const TCHAR* command = CMD_CAPTURE_TYPE;

    // to keep the form file from changing too much from 7.6 to 7.7, we will
    // use the old command name when the capture type is a <= 7.6 capture type
    if( use_pre77_command_names )
    {
        switch( m_captureType )
        {
            case CaptureType::TextBox:
            case CaptureType::RadioButton:
            case CaptureType::CheckBox:
            case CaptureType::DropDown:
            case CaptureType::ComboBox:
            case CaptureType::Date:
            case CaptureType::NumberPad:
            case CaptureType::Barcode:
            case CaptureType::Slider:
                command = _T("DataCaptureType");
                break;
        }
    }
    
    spec_file.PutLine(command, GetCaptureTypeName(m_captureType));

    if( m_extendedCaptureInfo != nullptr )
        m_extendedCaptureInfo->Save(spec_file);
}


CaptureInfo CaptureInfo::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    CaptureInfo capture_info;
    ASSERT(!capture_info.IsSpecified());

    if( !json_node.IsNull() )
    {
        wstring_view type_text = json_node.Get<wstring_view>(JK::type);
        std::optional<CaptureType> capture_type = CaptureInfo::GetCaptureTypeFromSerializableName(type_text);

        if( !capture_type.has_value() )
            throw JsonParseException(_T("'%s' is not a valid capture type"), std::wstring(type_text).c_str());

        capture_info.SetCaptureType(*capture_type);

        if( capture_info.m_extendedCaptureInfo != nullptr )
            capture_info.m_extendedCaptureInfo->ParseJsonInput(json_node);
    }

    return capture_info;
}

void CaptureInfo::WriteJson(JsonWriter& json_writer) const
{
    if( IsSpecified() )
    {
        json_writer.BeginObject();

        json_writer.Write(JK::type, SO::TitleToCamelCase(GetCaptureTypeName(m_captureType)));

        if( m_extendedCaptureInfo != nullptr )
            m_extendedCaptureInfo->WriteJson(json_writer);

        json_writer.EndObject();
    }

    else
    {
        json_writer.WriteNull();
    }
}


void CaptureInfo::serialize(Serializer& ar)
{
    ar.SerializeEnum(m_captureType);

    if( ar.IsLoading() )
        SetCaptureType(m_captureType);

    if( m_extendedCaptureInfo != nullptr )
        m_extendedCaptureInfo->serialize(ar);
}


const TCHAR* CaptureInfo::GetCaptureTypeName(CaptureType capture_type, bool display_name/* = false*/)
{
    static_assert(_countof(CaptureTypeNames) == ( (size_t)CaptureType::LastDefined + 1 ));

    if( display_name )
    {
             if( capture_type == CaptureType::TextBox )      return _T("Text Box");
        else if( capture_type == CaptureType::RadioButton )  return _T("Radio Button");
        else if( capture_type == CaptureType::CheckBox )     return _T("Check Box");
        else if( capture_type == CaptureType::DropDown )     return _T("Drop Down");
        else if( capture_type == CaptureType::ComboBox )     return _T("Combo Box");
        else if( capture_type == CaptureType::NumberPad )    return _T("Number Pad");
        else if( capture_type == CaptureType::ToggleButton ) return _T("Toggle Button");
    }

    else if( capture_type == CaptureType::Unspecified )
    {
        return UnspecifiedCaptureTypeName;
    }

    return CaptureTypeNames[(size_t)capture_type];
}


std::optional<CaptureType> CaptureInfo::GetCaptureTypeFromSerializableName(wstring_view name)
{
    for( size_t i = 0; i < _countof(CaptureTypeNames); ++i )
    {
        if( SO::EqualsNoCase(name, CaptureTypeNames[i]) )
            return (CaptureType)i;
    }

    if( SO::EqualsNoCase(name, UnspecifiedCaptureTypeName) )
        return CaptureType::Unspecified;

    return std::nullopt;
}


CaptureType CaptureInfo::GetBaseCaptureType(const CDictItem& dict_item)
{
    switch( dict_item.GetContentType() )
    {
        case ContentType::Numeric:
        case ContentType::Alpha:
            return CaptureType::TextBox;

        case ContentType::Image:
            return CaptureType::Photo;

        case ContentType::Audio:
            return CaptureType::Audio;

        default:
            ASSERT(false);
            return CaptureType::TextBox;
    }
}


CaptureInfo CaptureInfo::GetDefaultCaptureInfo(const CDictItem& dict_item)
{
    const DictValueSet* dict_value_set = dict_item.GetFirstValueSetOrNull();
    bool has_value_set = ( dict_value_set != nullptr );
    size_t num_values = has_value_set ? dict_value_set->GetNumValues() : 0;
    size_t num_to_values = has_value_set ? dict_value_set->GetNumToValues() : 0;

    CaptureType default_capture_type = GetBaseCaptureType(dict_item);

    // when there is a value set with values, override the base capture type to be a
    // radio button, with some exceptions for...
    if( num_values > 0 )
    {
        default_capture_type = CaptureType::RadioButton;

        // ...numeric fields
        if( dict_item.GetContentType() == ContentType::Numeric )
        {
            // to avoid setting too many fields to combo boxes, only make the default a combo box if there is just one
            // non-range (so that it works for value sets like ages [0 - 130 years, -99 unknown])
            if( num_to_values > 0 )
            {
                default_capture_type = ( ( num_to_values + 1 ) == num_values ) ? CaptureType::ComboBox :
                                                                                 CaptureType::TextBox;
            }
        }

        // ...alpha fields
        else if( dict_item.GetContentType() == ContentType::Alpha )
        {
            // don't suggest a checkbox unless there are at least two values and the
            // length of the field doesn't match the checkbox length
            if( num_values >= 2 )
            {
                unsigned checkbox_length = CheckBoxCaptureInfo::GetCheckBoxLength(dict_item, *dict_value_set);

                if( checkbox_length != 0 && checkbox_length != dict_item.GetLen() )
                    default_capture_type = CaptureType::CheckBox;
            }
        }
    }

    CaptureInfo default_capture_info = default_capture_type;

    ASSERT(default_capture_info == default_capture_info.MakeValid(dict_item, dict_value_set));

    return default_capture_info;
}


bool CaptureInfo::IsCaptureTypePossible(const CDictItem& dict_item, CaptureType capture_type)
{
    ASSERT(capture_type != CaptureType::Unspecified);

    switch( capture_type )
    {
        // numeric and alpha (of any length)
        case CaptureType::TextBox:
        case CaptureType::RadioButton:
        case CaptureType::DropDown:
        case CaptureType::ComboBox:
        case CaptureType::Barcode:
        case CaptureType::ToggleButton:
            return ( dict_item.GetContentType() == ContentType::Numeric ||
                     dict_item.GetContentType() == ContentType::Alpha );

        // numeric and alpha (of specific length)
        case CaptureType::Date:
            return DateCaptureInfo::IsCaptureTypePossible(dict_item);

        // numeric only
        case CaptureType::NumberPad:
        case CaptureType::Slider:
            return ( dict_item.GetContentType() == ContentType::Numeric );

        // alpha only
        case CaptureType::CheckBox:
            return ( dict_item.GetContentType() == ContentType::Alpha );

        // image only
        case CaptureType::Photo:
        case CaptureType::Signature:
            return ( dict_item.GetContentType() == ContentType::Image );

        // audio only
        case CaptureType::Audio:
            return ( dict_item.GetContentType() == ContentType::Audio );

        default:
            ASSERT(false);
            return false;
    }
}


std::vector<CaptureType> CaptureInfo::GetPossibleCaptureTypes(const CDictItem& dict_item, CaptureTypeSortOrder sort_order)
{
    std::vector<CaptureType> possible_capture_types;

    for( CaptureType capture_type = CaptureType::FirstDefined;
         capture_type <= CaptureType::LastDefined;
         capture_type = (CaptureType)( (int)capture_type + 1 ) )
    {
        if( IsCaptureTypePossible(dict_item, capture_type) )
            possible_capture_types.emplace_back(capture_type);
    }

    if( sort_order == CaptureTypeSortOrder::Name )
    {
        std::sort(possible_capture_types.begin(), possible_capture_types.end(),
            [](CaptureType ct1, CaptureType ct2)
            {
                return ( _tcscmp(GetCaptureTypeName(ct1, true), GetCaptureTypeName(ct2, true)) < 0 );
            });
    }

    return possible_capture_types;
}


CString CaptureInfo::GetDescription() const
{
    CString description = GetCaptureTypeName(m_captureType, true);

    if( m_extendedCaptureInfo != nullptr )
        m_extendedCaptureInfo->AddToDescription(description);

    return description;
}


void CaptureInfo::Validate(const CDictItem& dict_item) const
{
    if( m_captureType == CaptureType::Unspecified )
        return;

    if( !IsCaptureTypePossible(dict_item, m_captureType) )
    {
        CString extra_message;

        if( m_captureType == CaptureType::Date )
            extra_message = FormatText(_T(" and length %u"), dict_item.GetLen());

        throw ValidationException(_T("The capture type %s is not valid for an item of type %s%s."),
            GetCaptureTypeName(m_captureType, true), ToString(dict_item.GetContentType()), (LPCTSTR)extra_message);
    }

    if( m_extendedCaptureInfo != nullptr )
        m_extendedCaptureInfo->Validate(dict_item);
}


CaptureInfo CaptureInfo::MakeValid(const CDictItem& dict_item, const DictValueSet* dict_value_set,
    bool get_capture_type_supported_on_current_platform/* = true*/) const
{
    bool has_value_set = ( dict_value_set != nullptr );
    size_t num_values = has_value_set ? dict_value_set->GetNumValues() : 0;
    size_t num_to_values = has_value_set ? dict_value_set->GetNumToValues() : 0;
    bool format_is_valid = false;

    // perform additional checks only when the capture type is possible on the platform
    if( m_captureType != CaptureType::Unspecified && IsCaptureTypePossible(dict_item, m_captureType) &&
        ( !get_capture_type_supported_on_current_platform || CaptureTypeSupportedOnPlatform(m_captureType) ) )
    {
        // beyond checking the possibility that an item can have a specific capture type, use the value set
        // to determine if the capture type is actually valid (for a few capture types)
        if( m_captureType == CaptureType::RadioButton ||
            m_captureType == CaptureType::DropDown )
        {
            format_is_valid = ( num_values > 0 && num_to_values == 0 );
        }

        else if( m_captureType == CaptureType::ComboBox )
        {
            format_is_valid = ( num_values > 0 );
        }

        else if( m_captureType == CaptureType::CheckBox )
        {
            format_is_valid = ( has_value_set && CheckBoxCaptureInfo::GetCheckBoxLength(dict_item, *dict_value_set) != 0 );
        }

        else if( m_captureType == CaptureType::Slider )
        {
            // a slider is valid if there is a single value set value which is a range value
            format_is_valid = ( num_values == num_to_values && num_to_values == 1 );
        }

        else if( m_captureType == CaptureType::ToggleButton )
        {
            // a toggle button is valid if there is one value set value...
            if( num_values >= 1 && num_to_values == 0 && dict_value_set->GetValue(0).GetNumValuePairs() == 1 )
            {
                if( num_values == 1 )
                {
                    format_is_valid = true;
                }

                // ...or if there are two value set values with one being notappl/""
                else if( num_values == 2 && dict_value_set->GetValue(1).GetNumValuePairs() == 1 )
                {
                    auto check_dict_value = [&](const DictValue& dict_value) -> bool
                    {
                        if( dict_item.GetContentType() == ContentType::Numeric )
                        {
                            return dict_value.IsSpecialValue(NOTAPPL);
                        }

                        else
                        {
                            return SO::IsBlank(dict_value.GetValuePair(0).GetFrom());
                        }
                    };

                    format_is_valid = check_dict_value(dict_value_set->GetValue(0)) ||
                                      check_dict_value(dict_value_set->GetValue(1));
                }
            }
        }

        else
        {
            format_is_valid = true;
        }
    }


    // if the initial format check is not valid, use the base setting
    CaptureInfo valid_capture_info = format_is_valid ? *this :
                                                        GetBaseCaptureType(dict_item);

    // make sure any extended information is also valid
    if( valid_capture_info.m_extendedCaptureInfo != nullptr )
        valid_capture_info.m_extendedCaptureInfo->MakeValid(dict_item);

    return valid_capture_info;
}



// --------------------------------------------------------------------------
//  DateCaptureInfo
// --------------------------------------------------------------------------

namespace
{
    const TCHAR* const DateFormats[]
    {
        _T("DDMMYYYY"),
        _T("DDYYYYMM"),
        _T("YYYYDDMM"),
        _T("YYYYMMDD"),
        _T("MMDDYYYY"),
        _T("MMYYYYDD"),
        _T("DDMMYY"),
        _T("DDYYMM"),
        _T("YYDDMM"),
        _T("YYMMDD"),
        _T("MMDDYY"),
        _T("MMYYDD"),
        _T("DDYYYY"),
        _T("DDYY"),
        _T("DDMM"),
        _T("MMYYYY"),
        _T("MMYY"),
        _T("MMDD"),
        _T("YYMM"),
        _T("YYDD"),
        _T("YYYYMM"),
        _T("YYYYDD")
    };
}


bool DateCaptureInfo::operator==(const DateCaptureInfo& rhs) const
{
    return ( m_format == rhs.m_format );
}


void DateCaptureInfo::Build(CSpecFile& spec_file)
{
    CString command;
    CString argument;

    while( spec_file.GetLine(command, argument) == SF_OK )
    {
        if( command.CompareNoCase(CMD_CAPTURE_TYPE_DATE) == 0 )
        {
            m_format = argument;
        }

        else
        {
            spec_file.UngetLine();
            break;
        }
    }
}


void DateCaptureInfo::Save(CSpecFile& spec_file) const
{
    if( !m_format.IsEmpty() )
        spec_file.PutLine(CMD_CAPTURE_TYPE_DATE, m_format);
}


void DateCaptureInfo::ParseJsonInput(const JsonNode<wchar_t>& json_node)
{
    m_format = json_node.GetOrDefault(JK::dateFormat, SO::EmptyCString);
}


void DateCaptureInfo::WriteJson(JsonWriter& json_writer) const
{
    if( json_writer.Verbose() || !m_format.IsEmpty() )
        json_writer.Write(JK::dateFormat, m_format);
}


void DateCaptureInfo::serialize(Serializer& ar)
{
    ar & m_format;
}


bool DateCaptureInfo::IsCaptureTypePossible(const CDictItem& dict_item)
{
    if( dict_item.GetContentType() == ContentType::Numeric ||
        dict_item.GetContentType() == ContentType::Alpha )
    {
        int field_length = dict_item.GetLen();

        return ( ( dict_item.GetDecimal() == 0 ) &&
                 ( field_length == 4 || field_length == 6 || field_length == 8 ) );
    }

    return false;
}


std::vector<const TCHAR*> DateCaptureInfo::GetPossibleFormats(const CDictItem& dict_item)
{
    std::vector<const TCHAR*> formats;

    for( size_t i = 0; i < _countof(DateFormats); ++i )
    {
        if( _tcslen(DateFormats[i]) == dict_item.GetLen() )
            formats.emplace_back(DateFormats[i]);
    }

    return formats;
}


const TCHAR* DateCaptureInfo::GetDefaultFormat(const CDictItem& dict_item)
{
    switch( dict_item.GetLen() )
    {
        case 4:
            return DateFormats[17]; // MMDD
        case 6:
            return DateFormats[9];  // YYMMDD
        case 8:
        default:
            return DateFormats[3];  // YYYYMMDD
    }
}


void DateCaptureInfo::AddToDescription(CString& description) const
{
    description.AppendFormat(_T(" (format: \"%s\")"), (LPCTSTR)m_format);
}


bool DateCaptureInfo::IsFormatValid(const CDictItem& dict_item) const
{
    // if the format is specified, make sure it is valid
    if( dict_item.GetLen() == (unsigned)m_format.GetLength() )
    {
        for( size_t i = 0; i < _countof(DateFormats); ++i )
        {
            if( m_format == DateFormats[i] )
                return true;
        }
    }

    return false;
}


void DateCaptureInfo::Validate(const CDictItem& dict_item) const
{
    ASSERT(IsCaptureTypePossible(dict_item));

    if( !m_format.IsEmpty() && !IsFormatValid(dict_item) )
        throw CaptureInfo::ValidationException(_T("The date format %s is not valid."), (LPCTSTR)m_format);
}


void DateCaptureInfo::MakeValid(const CDictItem& dict_item)
{
    ASSERT(IsCaptureTypePossible(dict_item));

    // if the date format is not valid, set it to a default setting
    if( !IsFormatValid(dict_item) )
        m_format = GetDefaultFormat(dict_item);
}


bool DateCaptureInfo::IsResponseValid(const CString& date_text) const
{
   ASSERT(m_format.GetLength() == date_text.GetLength());

    // use a leap year and a month with 31 days as the default values to
    // make this the most comprehensive when not all values are supplied
    int year = 2020;
    int month = 1;
    int day = 1;

    for( int date_index = 0; date_index < m_format.GetLength(); )
    {
        int component_length = 2;
        int* numeric_component;

        if( m_format[date_index] == _T('Y') )
        {
            if( ( date_index + 2 ) < m_format.GetLength() && m_format[date_index + 2] == _T('Y') )
                component_length = 4;

            numeric_component = &year;
        }

        else if( m_format[date_index] == _T('M') )
        {
            numeric_component = &month;
        }

        else
        {
            ASSERT(m_format[date_index] == _T('D'));
            numeric_component = &day;
        }

        CIMSAString component = date_text.Mid(date_index, component_length);

        *numeric_component = (int)component.Val();

        if( numeric_component == &year && component_length == 2 )
            year += 1900;

        date_index += component_length;
    }

    return DateHelper::IsValid(year, month, day);
}



// --------------------------------------------------------------------------
// CheckBoxCaptureInfo
// --------------------------------------------------------------------------

unsigned CheckBoxCaptureInfo::GetCheckBoxLength(const CDictItem& dict_item, const DictValueSet& dict_value_set)
{
    // for checkboxes, each value must be of the same trimmed length, and the length of
    // the checkbox field must be a multiple of the trimmed length
    std::optional<unsigned> trimmed_length;

    for( const DictValue& dict_value : dict_value_set.GetValues() )
    {
        unsigned this_trimmed_length = SO::Trim(dict_value.GetValuePair(0).GetFrom()).length();
        bool length_is_valid;

        if( !trimmed_length.has_value() )
        {
            trimmed_length = this_trimmed_length;
            length_is_valid = ( *trimmed_length > 0 && ( dict_item.GetLen() % *trimmed_length == 0 ) );
        }

        else
        {
            length_is_valid = ( this_trimmed_length == *trimmed_length );
        }

        if( !length_is_valid )
            return 0;
    }

    return trimmed_length.value_or(0);
}


template<typename T>
T CheckBoxCaptureInfo::SharedResponseProcessor(const CString& checkbox_text, const ValueProcessor& value_processor)
{
    // for IsResponseValid, T will be bool
    // for GetResponseLabel, T will be std::vector<const DictValue*>

    ASSERT(value_processor.GetDictItem().GetContentType() == ContentType::Alpha &&
           value_processor.GetDictValueSet() != nullptr);

    int checkbox_length = CString(value_processor.GetDictValueSet()->GetValue(0).GetValuePair(0).GetFrom()).Trim().GetLength();
    ASSERT(checkbox_length == (int)CheckBoxCaptureInfo::GetCheckBoxLength(value_processor.GetDictItem(), *value_processor.GetDictValueSet()));
    ASSERT(checkbox_text.GetLength() % checkbox_length == 0);

    std::vector<const DictValue*> selected_values;

    for( const TCHAR* checkbox_itr = checkbox_text; *checkbox_itr != 0; checkbox_itr += checkbox_length )
    {
        // only check non-blank values
        bool has_non_blank_values = false;

        for( int i = 0; i < checkbox_length; ++i )
        {
            if( checkbox_itr[i] != _T(' ') )
            {
                has_non_blank_values = true;
                break;
            }
        }

        if( !has_non_blank_values )
            continue;

        CString component(checkbox_itr, checkbox_length);

        const DictValue* selected_value = value_processor.GetDictValue(component);

        // IsResponseValid processing
        if constexpr(std::is_same_v<T, bool>)
        {
            // don't allow duplicate selections
            if( selected_value == nullptr || std::find(selected_values.cbegin(), selected_values.cend(), selected_value) != selected_values.cend() )
                return false;
        }

        selected_values.emplace_back(selected_value);
    }

    // IsResponseValid processing
    if constexpr(std::is_same_v<T, bool>)
    {
        return true;
    }

    // GetResponseLabel processing
    else
    {
        return selected_values;
    }
}

bool CheckBoxCaptureInfo::IsResponseValid(const CString& checkbox_text, const ValueProcessor& value_processor)
{
    return SharedResponseProcessor<bool>(checkbox_text, value_processor);
}


CString CheckBoxCaptureInfo::GetResponseLabel(const CString& checkbox_text, const ValueProcessor& value_processor)
{
    std::vector<const DictValue*> selected_values =
        SharedResponseProcessor<std::vector<const DictValue*>>(checkbox_text, value_processor);

    CString checkbox_label;

    for( const DictValue* dict_value : selected_values )
    {
        if( dict_value != nullptr )
        {
            if( !checkbox_label.IsEmpty() )
                checkbox_label.Append(_T(", "));

            checkbox_label.Append(dict_value->GetLabel());
        }
    }

    return checkbox_label;
}
