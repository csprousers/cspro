#include "Stdafx.h"
#include "SimpleDictionaryCreator.h"


namespace
{
    CString GetTitleCaseLabel(System::String^ text)
    {
        return System::Globalization::CultureInfo::InvariantCulture->TextInfo->ToTitleCase(text->ToLower()->Replace('_', ' '));
    }
}


CSPro::Dictionary::SimpleDictionaryCreator::SimpleDictionaryCreator(System::String^ name_prefix_,
    bool decimal_char_default, bool zero_fill_default)
    :   m_dictionary(new CDataDict()),
        m_nextStartPos(1)
{
    CString name_prefix = name_prefix_;
    CString label_prefix = GetTitleCaseLabel(name_prefix_);

    m_dictionary->SetName(name_prefix + _T("_DICT"));
    m_dictionary->SetLabel(label_prefix + _T(" Dictionary"));

    m_dictionary->SetDecChar(decimal_char_default);
    m_dictionary->SetZeroFill(zero_fill_default);
    m_dictionary->SetRecTypeStart(0);
    m_dictionary->SetRecTypeLen(0);
    m_dictionary->SetPosRelative(true);
    m_dictionary->SetAllowDataViewerModifications(true);
    m_dictionary->SetAllowExport(true);

    DictLevel dict_level;
    dict_level.SetName(name_prefix + _T("_LEVEL"));
    dict_level.SetLabel(label_prefix + _T(" Level"));

    CDictRecord dict_record;
    dict_record.SetName(name_prefix + _T("_REC"));
    dict_record.SetLabel(label_prefix + _T(" Record"));

    dict_level.AddRecord(&dict_record);
    m_dictionary->AddLevel(std::move(dict_level));
}

CSPro::Dictionary::SimpleDictionaryCreator::!SimpleDictionaryCreator()
{
    delete m_dictionary;
}


void CSPro::Dictionary::SimpleDictionaryCreator::AddItem(bool is_id, System::String^ name,
    bool numeric, int length, int decimal, System::Collections::Generic::SortedSet<double>^ values)
{
    CDictItem dict_item;
    dict_item.SetName(CString(name));
    dict_item.SetLabel(GetTitleCaseLabel(name));
    dict_item.SetContentType(numeric ? ContentType::Numeric : ContentType::Alpha);
    dict_item.SetStart(m_nextStartPos);
    dict_item.SetLen(length);

    if( numeric )
    {
        dict_item.SetDecChar(m_dictionary->IsDecChar());
        dict_item.SetZeroFill(m_dictionary->IsZeroFill());
        dict_item.SetDecimal(decimal);
    }

    m_nextStartPos += dict_item.GetLen();

    if( values != nullptr ) // create a value set
    {
        DictValueSet dict_value_set;
        dict_value_set.SetName(dict_item.GetName() + "_VS1");
        dict_value_set.SetLabel(dict_item.GetLabel());

        for each( double value in values )
        {
            TCHAR pszTemp[30];
            CString formatted_double = dtoa(value, pszTemp, dict_item.GetDecimal(), _T('.'), false);

            DictValue dict_value;
            dict_value.SetLabel(_T("Value ") + formatted_double);
            dict_value.AddValuePair(DictValuePair(formatted_double));

            dict_value_set.AddValue(std::move(dict_value));
        }

        dict_item.AddValueSet(std::move(dict_value_set));
    }

    CDictRecord* dict_record = m_dictionary->GetLevel(0).GetRecord(is_id ? COMMON : 0);
    dict_record->AddItem(&dict_item);
    dict_record->SetRecLen(m_nextStartPos - 1);
}


void CSPro::Dictionary::SimpleDictionaryCreator::Save(System::String^ filename)
{
    try
    {
        m_dictionary->Save((CString)filename);
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}
