#include "StdAfx.h"
#include "DictClipboard.h"
#include <zAppO/LanguageSerializerHelper.h>


std::vector<unsigned> DictClipboard::m_clipboardFormats;


namespace
{
    void DisplayException(const CSProException& exception)
    {
        ErrorMessage::Display(_T("Error pasting from the clipboard: ") + exception.GetErrorMessage());
    }

    template<typename CF>
    void Iterate(DictLevel& dict_level, const CF& callback_function)
    {
        callback_function(dict_level, nullptr);

        for( int r = -1; r < dict_level.GetNumRecords(); ++r )
        {
            CDictRecord* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);
            Iterate(*dict_record, callback_function);
        }
    }

    template<typename CF>
    void Iterate(CDictRecord& dict_record, const CF& callback_function)
    {
        callback_function(dict_record, nullptr);

        for( int i = 0; i < dict_record.GetNumItems(); ++i )
        {
            CDictItem* dict_item = dict_record.GetItem(i);
            Iterate(*dict_item, callback_function);
        }
    }

    template<typename CF>
    void Iterate(CDictItem& dict_item, const CF& callback_function)
    {
        callback_function(dict_item, nullptr);

        for( DictValueSet& dict_value_set : dict_item.GetValueSets() )
            Iterate(dict_value_set, callback_function, &dict_item);
    }

    template<typename CF>
    void Iterate(DictValueSet& dict_value_set, const CF& callback_function, const CDictItem* parent_dict_item = nullptr)
    {
        callback_function(dict_value_set, parent_dict_item);
    }


    class DictionaryPasteJsonReaderInterface : public JsonReaderInterface
    {
    public:
        void OnLogWarning(std::wstring message) override
        {
            if( std::find(m_messages.cbegin(), m_messages.cend(), message) == m_messages.cend() )
                m_messages.emplace_back(std::move(message));
        }

        bool DisplayWarningsAndPromptToContinue() const
        {
            if( m_messages.empty() )
                return true;

            std::wstring full_message = _T("There are language differences in the dictionary elements pasted. Continue?\n");

            for( const std::wstring& message : m_messages )
                SO::Append(full_message, _T("\n  • "), message);

            return ( AfxMessageBox(full_message.c_str(), MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON1) == IDYES );
        }

    private:
        std::vector<std::wstring> m_messages;
    };
}


DictClipboard::DictClipboard(CDDDoc& dictionary_doc)
    :   m_dictionaryDoc(dictionary_doc)
{
    if( m_clipboardFormats.empty() )
    {
        for( const TCHAR* format : { _T("CSPro DD Level"), _T("CSPro DD Rec"),   _T("CSPro DD Item"),
                                     _T("CSPro DD VSet"),  _T("CSPro DD Value"), _T("CSPro DD VPair") } )
        {
            m_clipboardFormats.emplace_back(RegisterClipboardFormat(format));
        }

        ASSERT(m_clipboardFormats.size() == ( GetFormatIndex<DictValuePair>() + 1 ));
    }
}


template<typename T>
void DictClipboard::PutOnClipboard(CWnd* pWnd, std::vector<const T*> dict_elements, const DictNamedBase* parent_dict_element/* = nullptr*/) const
{
    unsigned clipboard_format = GetClipboardFormat<T>();

    auto json_writer = Json::CreateStringWriter();

    // write all details
    json_writer->SetVerbose();

    json_writer->BeginObject();

    // write the parent name when applicable
    if( parent_dict_element != nullptr )
        json_writer->Write(JK::parent, parent_dict_element->GetName());

    // write labels with the proper languages
    const CDataDict& dictionary = m_dictionaryDoc.GetDictionary();
    auto language_serializer_holder = json_writer->GetSerializerHelper().Register(std::make_shared<LanguageSerializerHelper>(dictionary.GetLanguages()));

    json_writer->BeginArray(JK::values);

    for( const auto& dict_element : dict_elements )
        json_writer->Write(*dict_element);

    json_writer->EndArray();

    json_writer->EndObject();

    WinClipboard::PutTextWithFormat(clipboard_format, pWnd, json_writer->GetString());
}


template<typename T>
DictPastedValues<T> DictClipboard::GetFromClipboardWorker(CWnd* pWnd) const
{
    unsigned clipboard_format = GetClipboardFormat<T>();

    DictionaryPasteJsonReaderInterface dictionary_paste_json_reader_interface;
    JsonNode<wchar_t> json_node(WinClipboard::GetTextWithFormat(clipboard_format, pWnd), &dictionary_paste_json_reader_interface);

    // read labels with the proper languages
    const CDataDict& dictionary = m_dictionaryDoc.GetDictionary();
    auto language_serializer_holder = json_node.GetSerializerHelper().Register(std::make_shared<LanguageSerializerHelper>(dictionary.GetLanguages()));

    // read the values and get the name of the parent of these values (if applicable)
    DictPastedValues<T> pasted_values =
    {
        json_node.GetArray(JK::values).GetVector<T>(),
        json_node.GetOrDefault(JK::parent, CString())
    };

    // warn about issues from the paste, and clear the values if the user chooses not to continue
    if( !dictionary_paste_json_reader_interface.DisplayWarningsAndPromptToContinue() )
        pasted_values.values.clear();

    return pasted_values;
}


template<typename T>
std::vector<T> DictClipboard::GetFromClipboard(CWnd* pWnd) const
{
    try
    {
        return GetFromClipboardWorker<T>(pWnd).values;
    }

    catch( const CSProException& exception )
    {
        DisplayException(exception);
        return { };
    }
}


template<typename T>
DictPastedValues<T> DictClipboard::GetNamedElementsFromClipboard(CWnd* pWnd) const
{
    try
    {
        DictPastedValues<T> pasted_values = GetFromClipboardWorker<T>(pWnd);

        CDataDict& dictionary = m_dictionaryDoc.GetDictionary();
        std::set<CString> names_added;
        std::vector<CDictRecord*> non_id_records_added;

        for( T& dict_element : pasted_values.values )
        {
            Iterate(dict_element,
                [&](DictNamedBase& dict_element, const CDictItem* parent_dict_item)
                {
                    // 1. keep track of added records
                    // ------------------------------
                    if( dict_element.GetElementType() == DictElementType::Record )
                    {
                        CDictRecord& dict_record = assert_cast<CDictRecord&>(dict_element);

                        // don't process anything about the ID record
                        if( dict_record.IsIdRecord() )
                        {
                            ASSERT(dict_element.GetAliases().empty());
                            return;
                        }

                        non_id_records_added.emplace_back(&dict_record);
                    }


                    // 2. check the name
                    // -----------------
                    auto name_is_unique_and_add_to_set =
                        [&](CString name)
                        {
                            name.MakeUpper();

                            if( !dictionary.IsNameUnique(name) || names_added.find(name) != names_added.cend() )
                                return false;

                            names_added.insert(name);
                            return true;
                        };

                    // ensure the name is unique (with special override processing for value set names)
                    if( parent_dict_item != nullptr && !dictionary.IsNameUnique(dict_element.GetName()) )
                    {
                        const DictValueSet& dict_value_set = assert_cast<const DictValueSet&>(dict_element);
                        const auto& all_dict_value_sets = parent_dict_item->GetValueSets();
                        const auto& dict_value_set_lookup = std::find_if(all_dict_value_sets.cbegin(), all_dict_value_sets.cend(),
                            [&](const DictValueSet& this_dict_value_set) { return ( &this_dict_value_set == &dict_value_set ); });
                        size_t value_set_index = std::distance(all_dict_value_sets.cbegin(), dict_value_set_lookup);                            
                        dict_element.SetName(FormatText(_T("%s_VS%d"), (LPCTSTR)parent_dict_item->GetName(), (int)value_set_index + 1));
                    }

                    while( !name_is_unique_and_add_to_set(dict_element.GetName()) )
                        dict_element.SetName(dictionary.GetUniqueName(dict_element.GetName(), NONE, NONE, NONE, NONE, &names_added));

                    // drop aliases when the alias' name is in use
                    if( !dict_element.GetAliases().empty() )
                    {
                        std::set<CString> valid_aliases = dict_element.GetAliases();

                        for( auto alias_itr = valid_aliases.begin(); alias_itr != valid_aliases.end(); )
                        {
                            if( name_is_unique_and_add_to_set(*alias_itr) )
                                ++alias_itr;

                            else
                                alias_itr = valid_aliases.erase(alias_itr);
                        }

                        dict_element.SetAliases(valid_aliases);
                    }


                    // 3. keep track of value sets added
                    // ---------------------------------
                    if( dict_element.GetElementType() == DictElementType::ValueSet )
                        pasted_values.value_set_names_added.emplace_back(dict_element.GetName());
                });
        }


        // 4. if records were added, make sure that the record types are unique
        // --------------------------------------------------------------------
        if( !non_id_records_added.empty() )
        {
            size_t post_paste_num_records = dictionary.GetNumRecords() + non_id_records_added.size();

            if( post_paste_num_records > 1 )
            {
                if( dictionary.GetRecTypeLen() == 0 )
                {
                    // the dictionary editor (using the DictionaryValidator) clears the record type length
                    // when the dictionary is completely empty, so set it in that case
                    if( dictionary.GetNumRecords() == 0 )
                    {
                        dictionary.SetRecTypeStart(1);
                        dictionary.SetRecTypeLen(std::max(1, non_id_records_added.front()->GetRecTypeVal().GetLength()));
                    }

                    // otherwise disallow the operation
                    else
                    {
                        throw CSProException(_T("You cannot paste %d records until you make space for a record type"), (int)non_id_records_added.size());
                    }
                }

                std::set<CString> record_types_added;

                for( CDictRecord* dict_record : non_id_records_added )
                {
                    CString record_type = dict_record->GetRecTypeVal();

                    if( !DictionaryValidator::MakeRecordTypeUnique(dictionary, record_type, record_types_added) )
                        throw CSProException(_T("A unique record type could not be created for %s. Try pasting again after increasing the record type length"), (LPCTSTR)dict_record->GetName());

                    dict_record->SetRecTypeVal(record_type);
                    record_types_added.insert(record_type);
                }
            }
        }

        return pasted_values;
    }

    catch( const CSProException& exception )
    {
        DisplayException(exception);
        return { };
    }
}


#define INSTANTIATE(T, RT, FN)                                                                                 \
    template void DictClipboard::PutOnClipboard(CWnd* pWnd, std::vector<const T*> dict_elements,               \
                                                const DictNamedBase* parent_dict_element/* = nullptr*/) const; \
    template RT<T> DictClipboard::##FN##(CWnd* pWnd) const;

INSTANTIATE(DictLevel,     DictPastedValues, GetNamedElementsFromClipboard)
INSTANTIATE(CDictRecord,   DictPastedValues, GetNamedElementsFromClipboard)
INSTANTIATE(CDictItem,     DictPastedValues, GetNamedElementsFromClipboard)
INSTANTIATE(DictValueSet,  DictPastedValues, GetNamedElementsFromClipboard)
INSTANTIATE(DictValueSet,  std::vector,      GetFromClipboard) // for pasting value sets as linked value sets
INSTANTIATE(DictValue,     std::vector,      GetFromClipboard)
INSTANTIATE(DictValuePair, std::vector,      GetFromClipboard)
