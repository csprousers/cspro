#pragma once

//***************************************************************************
//  File name: DDClass.h
//
//  Description:
//       Header for Data Dictionary classes
//
//  History:    Date       Author   Comment
//              ---------------------------
//              02 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zDictO/DictLevel.h>
#include <zDictO/DictRelation.h>
#include <zUtilO/imsaStr.h>
#include <zAppO/Language.h>

class CSpecFile;
class ProcessSummary;
namespace JsonSpecFile { class Reader; class ReaderMessageLogger; }


const int MAX_RECLEN = 32000;
const int MAX_NUMERIC_ITEM_LEN = 15;
const int MAX_ALPHA_ITEM_LEN = 999;
const int MAX_LABEL_LEN = 255;
const int MAX_RECTYPE_LEN = 9;

const int MAX_ITEM_START = 32000;
const int MAX_DECIMALS = 9;

constexpr const TCHAR* CRLF = _T("\r\n");

const int MAX_DICTKEYSIZE = 128;
constexpr size_t MaxNumberLevels = 3;


/////////////////////////////////////////////////////////////////////////////
//
//                               CDictName
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZDICTO CDictName
{
public:
    int m_iLevel;
    int m_iRec;
    int m_iItem;
    int m_iVSet;

// Constructor
    CDictName(int iLevel=NONE, int iRec=NONE, int iItem=NONE, int iVSet=NONE);

// Operators
    void operator=(const CDictName& n);
};


/////////////////////////////////////////////////////////////////////////////
//
//                               CDataDict
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZDICTO CDataDict : public DictNamedBase
{
public:
    bool UseNewSymbols() const { return ( m_note.Find(_T("UseNewSymbols")) == 0 ); } // ENGINECR_TODO remove 


// Methods
public:
    const CString& GetFullFileName() const        { return m_filename; }
    void SetFullFileName(const CString& filename) { m_filename = filename; }

    int64_t GetFileModifiedTime() const;

public:
// Construction/destruction
    CDataDict();
    CDataDict(const CDataDict& rhs);

    std::unique_ptr<DictNamedBase> Clone() const override { return std::make_unique<CDataDict>(*this); }

    DictElementType GetElementType() const override { return DictElementType::Dictionary; }

// Extraction
    const CString&       GetOldName       () const  { return m_csOldName; }
    UINT                 GetRecTypeStart  () const  { return m_uRecTypeStart; }
    UINT                 GetRecTypeLen    () const  { return m_uRecTypeLen; }
    bool                 IsPosRelative    () const  { return m_bPosRelative; }
    bool                 IsZeroFill       () const  { return m_bZeroFill; }
    bool                 IsDecChar        () const  { return m_bDecChar; }
    const DictNamedBase* GetChangedObject () const  { return m_pChangedObject; }

    // get the number records (not counting ID records)
    size_t GetNumRecords() const;

    bool GetAllowDataViewerModifications() const { return m_allowDataViewerModifications; }
    bool GetAllowExport() const                  { return m_allowExport; }
    int GetCachedPasswordMinutes() const         { return m_cachedPasswordMinutes; }

    bool GetReadOptimization() const                 { return m_readOptimization; }
    void SetReadOptimization(bool read_optimization) { m_readOptimization = read_optimization; }

    std::wstring GetStructureMd5() const;
    size_t GetIdStructureHashForKeyIndex(bool hash_name, bool hash_start) const;

    int GetSymbol() const { return m_iSymbol; }

    CString MakeQualifiedName(const CString& name) const { return FormatText(L"%s.%s", (LPCTSTR)GetName(), (LPCTSTR)name); }

// Assignment
    void SetOldName         (const CString& csOldName) { m_csOldName = csOldName; }
    void SetRecTypeStart    (UINT uRecTypeStart) { m_uRecTypeStart = uRecTypeStart; }
    void SetRecTypeLen      (UINT uRecTypeLen) { m_uRecTypeLen = uRecTypeLen; }
    void SetPosRelative     (bool bPosRelative) { m_bPosRelative = bPosRelative; }
    void SetZeroFill        (bool bZeroFill) { m_bZeroFill = bZeroFill; }
    void SetDecChar         (bool bDecChar) { m_bDecChar = bDecChar; }
    void SetSymbol          (const int iSymbol) { m_iSymbol = iSymbol; }                // Use unknown

    void SetChangedObject   (const DictNamedBase* dict_element) { m_pChangedObject = dict_element;}

    void SetAllowDataViewerModifications(bool allow) { m_allowDataViewerModifications = allow; }
    void SetAllowExport(bool allow)                  { m_allowExport = allow; }
    void SetCachedPasswordMinutes(int minutes)       { m_cachedPasswordMinutes = minutes; }

    void CopyDictionarySettings(const CDataDict& dictionary);

    std::unique_ptr<ProcessSummary> CreateProcessSummary() const;

    bool IsValid(CString& csError); // RHF Feb 03, 2005
    std::vector<CString> GetUniqueNames(const TCHAR* prefix, int iNumDigits, int iNumNames) const;

// Operators
    void operator=(CDataDict& dict);

// Name verification
    void BuildNameList();
    void UpdateNameList(const DictNamedBase& dict_element, int iLevel=NONE, int iRec=NONE, int iItem=NONE, int iVSet=NONE);
    void UpdateNameList(int iLevel, int iRec);              // redo a record
private:
    void RemoveFromNameList(int iLevel=NONE, int iRec=NONE, int iItem=NONE, int iVSet=NONE);
    void AddToNameList(const CString& name, int iLevel=NONE, int iRec=NONE, int iItem=NONE, int iVSet=NONE);
public:
    void AddToNameList(const DictNamedBase& dict_element, int iLevel=NONE, int iRec=NONE, int iItem=NONE, int iVSet=NONE);

    // lookup a name among levels, records, items, and value sets; any of the pointers can be null;
    // if the template value if set (and not void), then the function will only return true if a value of that type is found
    template<typename T = void>
    bool LookupName(const CString& name, const DictLevel** dict_level, const CDictRecord** dict_record = nullptr, const CDictItem** dict_item = nullptr, const DictValueSet** dict_value_set = nullptr) const;
    template<typename T = void>
    bool LookupName(const std::wstring& name, const DictLevel** dict_level, const CDictRecord** dict_record = nullptr, const CDictItem** dict_item = nullptr, const DictValueSet** dict_value_set = nullptr) const;
    template<typename T = void>
    bool LookupName(const CString& name, DictLevel** dict_level, CDictRecord** dict_record = nullptr, CDictItem** dict_item = nullptr, DictValueSet** dict_value_set = nullptr);

    // lookup a name of only a certain type
    template<typename T>
    const T* LookupName(const CString& name) const;

    bool LookupName(const CString& csName, int* iLevel, int* iRecord, int* iItem, int* iVSet) const;
    bool LookupName(const std::wstring& name, int* iLevel, int* iRecord, int* iItem, int* iVSet) const;

    const DictNamedBase* LookupName(const CString& name) const;
    bool IsNameUnique(const CString& name, int iLevel = NONE, int iRecord = NONE, int iItem = NONE, int iVSet = NONE) const;
    CString GetUniqueName(const CString& base_name, int iLevel = NONE, int iRecord = NONE, int iItem = NONE, int iVSet = NONE,
                          const std::set<CString>* additional_names_in_use = nullptr) const;

    const CDictRecord* FindRecord(wstring_view record_name) const;
    const CDictItem* FindItem(wstring_view item_name) const;

    bool Find(bool bNext, bool bCaseSensitive, const CString& csFindText,
              int& iLevel, int& iRec, int& iItem, int& iVSet, int& iValue);

    int GetParentItemNum(int iLevel, int iRec, int iItem) const;
    const CDictItem* GetParentItem(int iLevel, int iRec, int iItem) const;

    void UpdatePointers();

    // diagnostic support
    std::vector<const CDictItem*> GetIdItems(std::vector<int>* pAcumItemsByLevelIdx = nullptr, std::vector<const CDictRecord*>* paIdRecs = nullptr) const; //FABN Jan 2005
    std::vector<std::vector<const CDictItem*>> GetIdItemsByLevel() const;
    unsigned GetKeyLength() const;

    bool EnableBinaryItems() const    { return m_enableBinaryItems; }
    void EnableBinaryItems(bool flag) { m_enableBinaryItems = flag; }

// Data Members
private:
    UINT                                m_uRecTypeStart;    // Record type start Position (1-based)
    UINT                                m_uRecTypeLen;      // Record type length
    bool                                m_bPosRelative;     // Positions are relative
    bool                                m_bZeroFill;        // ZeroFill default
    bool                                m_bDecChar;         // DecChar default

    std::map<CString, CDictName>        m_mapNames;         // Map of all the names/aliases in the dictionary

    bool                                m_allowDataViewerModifications;
    bool                                m_allowExport;
    int                                 m_cachedPasswordMinutes; // Length of time to cache passwords

    bool                                m_readOptimization; // if true, only items used in logic are read

    CString m_filename;
    std::optional<int64_t> m_serializedFileModifiedTime;

    int                                 m_iSymbol;          // Use unknown

    const DictNamedBase*                m_pChangedObject;   // Last object whose name has changed
    CString                             m_csOldName;        // Last object's old name

    bool m_enableBinaryItems; // BINARY_TYPES_TO_ENGINE_TODO


public:
    // languages
    // --------------------------------------------------
    const std::vector<Language>& GetLanguages() const { return m_languages; }
    std::optional<size_t> IsLanguageDefined(wstring_view language_name) const;

    void AddLanguage(Language language);
    void ModifyLanguage(size_t language_index, Language language);
    void DeleteLanguage(size_t language_index);

    size_t GetCurrentLanguageIndex() const     { return m_label.GetCurrentLanguageIndex(); }
    const Language& GetCurrentLanguage() const { return m_languages[GetCurrentLanguageIndex()]; }
    void SetCurrentLanguage(size_t language_index) const;


    // levels
    // --------------------------------------------------
    size_t GetNumLevels() const { return m_dictLevels.size(); }

    const std::vector<DictLevel>& GetLevels() const { return m_dictLevels; }
    std::vector<DictLevel>& GetLevels()             { return m_dictLevels; }

    const DictLevel& GetLevel(size_t index) const { ASSERT(index < m_dictLevels.size()); return m_dictLevels[index]; }
    DictLevel& GetLevel(size_t index)             { ASSERT(index < m_dictLevels.size()); return m_dictLevels[index]; }

    void AddLevel(DictLevel dict_level);
    void InsertLevel(size_t index, DictLevel dict_level);
    void RemoveLevel(size_t index);


    // relations
    // --------------------------------------------------
    size_t GetNumRelations() const { return m_dictRelations.size(); }

    const std::vector<DictRelation>& GetRelations() const { return m_dictRelations; }

    const DictRelation& GetRelation(size_t index) const { ASSERT(index < m_dictRelations.size()); return m_dictRelations[index]; }
    DictRelation& GetRelation(size_t index)             { ASSERT(index < m_dictRelations.size()); return m_dictRelations[index]; }

    void AddRelation(DictRelation dict_relation);
    void RemoveRelation(size_t index);
    void SetRelations(std::vector<DictRelation> dict_relations);


    // linked value set management
    // --------------------------------------------------
    size_t CountValueSetLinks(const DictValueSet& dict_value_set) const;

    enum class SyncLinkedValueSetsAction { UpdateValuesFromLinks, OnPaste };
    void SyncLinkedValueSets(std::variant<SyncLinkedValueSetsAction, DictValueSet*> action_or_updated_dict_value_set = SyncLinkedValueSetsAction::UpdateValuesFromLinks,
                             const std::vector<CString>* value_set_names_added_on_paste = nullptr);


    // serialization
    // --------------------------------------------------

    // all serialization methods (with the exception of GetJson, WriteJson, and serialize) can throw exceptions
    static std::unique_ptr<CDataDict> InstantiateAndOpen(NullTerminatedString filename, bool silent = false, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger = nullptr);
    void Open(NullTerminatedString filename, bool silent = false, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger = nullptr);
    void OpenFromText(wstring_view text);

    void Save(NullTerminatedString filename, bool continue_using_filename = true) const;
    std::wstring GetJson(bool spec_file_format = true) const;

    static CDataDict CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

    void serialize(Serializer& ar);

    static std::wstring ConvertPre80SpecFile(NullTerminatedString filename);

private:
    void Open(JsonSpecFile::Reader& json_reader, bool silent);

    void CreateFromJsonWorker(const JsonNode<wchar_t>& json_node);


private:
    std::vector<Language> m_languages;
    std::vector<DictLevel> m_dictLevels;
    std::vector<DictRelation> m_dictRelations;
};


CLASS_DECL_ZDICTO UINT GetDictOccs(const CDictRecord* pRec, const CDictItem* pItem, int iItem);
CLASS_DECL_ZDICTO const CDictItem* GetDictOccItem(const CDictRecord* pRecord, const CDictItem* pItem, int iRec, int iItem);


template<typename T/* = void*/>
bool CDataDict::LookupName(const std::wstring& name, const DictLevel** dict_level, const CDictRecord** dict_record/* = nullptr*/,
                           const CDictItem** dict_item/* = nullptr*/, const DictValueSet** dict_value_set/* = nullptr*/) const
{
    return LookupName<T>(WS2CS(name), dict_level, dict_record, dict_item, dict_value_set);
}

template<typename T/* = void*/>
bool CDataDict::LookupName(const CString& name, DictLevel** dict_level, CDictRecord** dict_record/* = nullptr*/,
                           CDictItem** dict_item/* = nullptr*/, DictValueSet** dict_value_set/* = nullptr*/)
{
    return const_cast<const CDataDict*>(this)->LookupName<T>(name, const_cast<const DictLevel**>(dict_level),
                                                                   const_cast<const CDictRecord**>(dict_record),
                                                                   const_cast<const CDictItem**>(dict_item),
                                                                   const_cast<const DictValueSet**>(dict_value_set));
}
