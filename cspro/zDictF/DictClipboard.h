#pragma once

#include <zJson/Json.h>


template<typename T>
struct DictPastedValues
{
    std::vector<T> values;
    CString parent_dict_element_name;
    std::vector<CString> value_set_names_added;
};


class DictClipboard
{
public:
    DictClipboard(CDDDoc& dictionary_doc);


    // returns whether the clipboard format is available
    template<typename T>
    bool IsAvailable() const { return IsClipboardFormatAvailable(GetClipboardFormat<T>()); }


    // puts a JSON array of the dictionary objects on the clipboard
    template<typename T>
    void PutOnClipboard(CWnd* pWnd, std::vector<const T*> dict_elements, const DictNamedBase* parent_dict_element = nullptr) const;


    // parses the JSON array on the clipboard, returning the dictionary objects;
    // if there is an error parsing the contents, a message will be displayed and
    // an empty vector will be returned
    template<typename T>
    std::vector<T> GetFromClipboard(CWnd* pWnd) const;


    // calls GetFromClipboard and then:
    // - ensures that names are unique
    // - drops aliases that are not unique
    // - ensures pasted records have unique record types
    // - gets a list of value sets that are part of this paste
    template<typename T>
    DictPastedValues<T> GetNamedElementsFromClipboard(CWnd* pWnd) const;


private:
    template<typename T>
    constexpr size_t GetFormatIndex() const
    {
        if      constexpr(std::is_same_v<T, DictLevel>)     return 0;
        else if constexpr(std::is_same_v<T, CDictRecord>)   return 1;
        else if constexpr(std::is_same_v<T, CDictItem>)     return 2;
        else if constexpr(std::is_same_v<T, DictValueSet>)  return 3;
        else if constexpr(std::is_same_v<T, DictValue>)     return 4;
        else if constexpr(std::is_same_v<T, DictValuePair>) return 5;
        else                                                static_assert_false();
    }

    template<typename T>
    unsigned GetClipboardFormat() const
    {
        return m_clipboardFormats[GetFormatIndex<T>()];
    }

    template<typename T>
    DictPastedValues<T> GetFromClipboardWorker(CWnd* pWnd) const;

private:
    static std::vector<unsigned> m_clipboardFormats;
    CDDDoc& m_dictionaryDoc;
};
