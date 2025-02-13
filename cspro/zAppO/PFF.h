#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/Application.h>
#include <zToolsO/StringNoCase.h>
#include <zUtilO/ConnectionString.h>


template<typename T>
struct MultipleFilenames
{
    std::vector<T> evaluated_filenames;
    std::vector<T> serializable_filenames;
    void clear();
};

struct FlagNameValue
{
    const TCHAR* const name;
    int value;
    bool serialize = true;
    FlagNameValue(const TCHAR* const _name, int _value, bool _serialize = true);
};

enum APPTYPE
{
    ENTRY_TYPE = 0,         BATCH_TYPE,             TAB_TYPE,
    FREQ_TYPE,              SORT_TYPE,              EXPORT_TYPE,
    REFORMAT_TYPE,          COMPARE_TYPE,           CONCAT_TYPE,
    PACK_TYPE,              INDEX_TYPE,             SYNC_TYPE,
    PARADATA_CONCAT_TYPE,   EXCEL2CSPRO_TYPE,       DEPLOY_TYPE,
    VIEW_TYPE,
    INVALID_TYPE
};
extern const TCHAR* const AppTypeNames[];

enum PROCESS { PROCESS_INVALID = -1, ALL_STUFF = 0, CS_TAB, CS_CON, CS_CALC, CS_PREP };
extern const TCHAR* const ProcessNames[];

enum class StartMode { None, Add, Modify, Verify };
extern const TCHAR* const StartModeNames[];

namespace LockFlag
{
    constexpr int Add         = 0x01;
    constexpr int Modify      = 0x02;
    constexpr int Delete      = 0x04;
    constexpr int Verify      = 0x08;
    constexpr int View        = 0x10;
    constexpr int Statistics  = 0x20;
    constexpr int CaseListing = 0x40;
};
extern const FlagNameValue LockFlagNameValues[];

enum class ShowInApplicationListing { Always, Hidden, Never };
extern const TCHAR* const ShowInApplicationListingNames[];

enum FULLSCREEN { FULLSCREEN_NO = 0, FULLSCREEN_YES, FULLSCREEN_NOMENUS };
extern const TCHAR* const FullScreenNames[];

enum class InteractiveEditMode { ErrMsg = 0, Range, Both, Ask, Off };
extern const TCHAR* const InteractiveEditModeNames[];

enum VIEWLISTING { NEVER = 0, ALWAYS , ONERROR };
extern const TCHAR* const ViewListingNames[];

enum class ErrMsgOverride { No = 0, Summary, Case };
extern const TCHAR* const ErrMsgOverrideNames[];

enum class InputOrder { Indexed, Sequential };
extern const TCHAR* const InputOrderNames[];

namespace PackIncludeFlag
{
    const int ValueSetImages = 0x01;
    const int Resources= 0x02;
    const int InputFile = 0x04;
    const int ExternalFiles = 0x08;
    const int UserFiles = 0x10;
};
extern const FlagNameValue PackIncludeFlagNameValues[];

enum class ConcatenateMethod { Case, Text };
extern const TCHAR* const ConcatenateMethodNames[];

enum class DuplicateCase { List, View, Prompt, PromptIfDifferent, KeepFirst };
extern const TCHAR* const DuplicateCaseNames[];

extern const TCHAR* const SyncServerTypeNames[];
extern const TCHAR* const SyncDirectionNames[];

enum DeployToOverride { None, CSWeb, Dropbox, FTP, LocalFile, LocalFolder };
extern const TCHAR* const DeployToOverrideNames[];


#define PFF_COMMAND_INPUT_DICT  _T("InputDict")
#define PFF_COMMAND_OUTPUT_DICT _T("OutputDict")



class ZAPPO_API PFF
{
protected:
    CString         m_sPifFileName;     // PifFile file name that is opened
    CString         m_sVersion;         // Version number
    APPTYPE         m_eAppType;         // Application type
    PROCESS         m_eTabProcess;      // Used by Tabulations to specify which Tab to run
    CString         m_sAppDescription;  // Application description (optional, used in Android display)

    CString         m_sOpID;                    // operator ID string
    StartMode       m_eStartMode;               // Start Mode
    CString         m_sStartKey;                // Starting key
    CString         m_csKey;
    int             m_iLockFlags;
    CString         m_csCaseListingFilter;
    ShowInApplicationListing m_eShowInApplicationListing;
    FULLSCREEN      m_eFullScreen;              // 20140405 there is a tablet option (for no menu bars)
    bool            m_bAutoAdd;                 // Set auto add mode
    bool            m_bDisableFileOpen;         // Disable File open
    InteractiveEditMode m_eInteractiveEditMode; // Interactive mode
    bool            m_bInteractiveEditDialogLocked;

    CString         m_sAppFName;                        // Application file name
    MultipleFilenames<ConnectionString> m_inputDataConnectionStrings;
    CString         m_excelFilename;
    MultipleFilenames<CString> m_exportFiles;
    MultipleFilenames<ConnectionString> m_outputDataConnectionStrings;
    CString         m_sInputDictFName;                  // Used in Reformat, Index and Concat
    CString         m_sOutputDictFName;                 // Used in Reformat
    ConnectionString m_referenceDataConnectionString;   // Used in Compare
    CString         m_sTabOutputFName;                  // CSTab Tab Output file name .tbd
    MultipleFilenames<CString> m_conInputFilenames;     // Array of con input file names
    CString         m_sConOutputFName;                  // Con output file name
    CString         m_sPrepInputFName;                  // Prep input file name
    CString         m_sPrepOutputFName;                 // Prep output file name
    CString         m_sAreaFName;
    CString         m_sSPSSCodeFName;
    CString         m_sSASCodeFName;
    CString         m_sSTATACodeFName;
    CString         m_sSTATADOFName;
    CString         m_sCSPROCodeFName;
    CString         m_sRCodeFName;
    CString         m_sPackOutputFName;                 // ZIP file to be created by CSPack
    MultipleFilenames<CString> m_extraFiles;            // Extra files to be packed by CSPack
    MultipleFilenames<CString> m_inputParadata;
    CString         m_outputParadata;
    CString         m_csParadataFilename;
    CString         m_sListingFName;                    // Listing file name
    CString         m_frequenciesFilename;
    CString         m_imputeFrequenciesFilename;
    ConnectionString m_imputeStatConnectionString;
    CString         m_sWriteDataFName;                  // Write data file name
    CString         m_saveArrayFilename;
    CString         m_csCommonStoreFName;
    CString         m_htmlDialogsDirectory;
    std::optional<BaseMapSelection> m_baseMapSelection;

    // map of external dictionaries
    std::map<CString, ConnectionString> m_mapExternalDataConnectionStrings;

    // map of user files
    std::map<CString, std::tuple<CString, CString>> m_mapUserFiles;

    // parameters
    CString           m_sStartLanguage;
    CString           m_sParameter;           // SysParam string
    VIEWLISTING       m_eViewListing;
    bool              m_bViewResults;
    int               m_iPackIncludes;
    DuplicateCase     m_eDuplicateCase;       // for deleting duplicates in CSIndex
    bool              m_bSkipStructure;
    bool              m_bChkRanges;
    int               m_iListingWidth;
    bool              m_bMessageWrap;
    ErrMsgOverride    m_errMsgOverride;
    InputOrder        m_eInputOrder;          // used for applications that use the batch input loop
    bool              m_bDisplayNames;        // for CSRefmt
    ConcatenateMethod m_eConcatenateMethod;
    SyncServerType    m_eSyncServerType;
    SyncDirection     m_eSyncDirection;
    CString           m_sSyncUrl;
    DeployToOverride  m_eDeployToOverride;
    bool              m_bSilent;              // run CSPack silently
    CString           m_csOnExitFilename;
    std::map<StringNoCase, std::vector<std::wstring>> m_customParameters; // a map of custom parameters

    std::map<CString, CString> m_mapPersistent; // Map of Persistent Variable names to data


public:
    PFF(CString sFileName = CString());
    PFF& operator=(const PFF& rhs) = default;

    virtual ~PFF() { }

    void ResetContents();

protected:
    CString GetFilename(const CString& filename, bool absolute_path) const;
    void MakeAbsolutePath(CString& filename_object);

    void SetFilename(CString& filename_object, const TCHAR* filename);

    // for multiple filenames
    void AddFilename(MultipleFilenames<CString>& multiple_filenames_object, const TCHAR* filename);

    // for connection strings
    CString GetConnectionStringText(const ConnectionString& connection_string, bool absolute_path) const;
    void SetConnectionString(ConnectionString& connection_string_object, const TCHAR* connection_string_text);

    // for multiple connection strings
    std::vector<CString> GeConnectionStringsSerializableText(const MultipleFilenames<ConnectionString>& multiple_connection_strings) const;
    void AddConnectionString(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, ConnectionString connection_string);
    void AddFilename(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, const TCHAR* connection_string_text);
    void ClearAndAddConnectionStrings(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, const std::vector<ConnectionString>& connection_strings);
    const ConnectionString& GetSingleConnectionString(const MultipleFilenames<ConnectionString>& multiple_connection_strings) const;
    const ConnectionString& GetConnectionString(const MultipleFilenames<ConnectionString>& multiple_connection_strings, size_t index) const;
    void SetSingleConnectionString(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, ConnectionString connection_string);
    void SetSingleConnectionString(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, wstring_view connection_string_text);

    static CString GetBooleanText(bool boolean_object);
    static void SetBoolean(bool& boolean_object, const TCHAR* text);

    template<typename T>
    static CString GetEnumText(T enum_object, const TCHAR* const type_names[]);
    template<typename T>
    static void SetEnum(T& enum_object, const TCHAR* const type_names[], const TCHAR* text, T default_value);

    static CString GetFlagsText(int flag_object, const FlagNameValue flag_name_values[]);
    static void SetFlags(int& flag_object, const FlagNameValue flag_name_values[], wstring_view text_sv);

#define DefineGetFilenameMethods(function_name, object_name)                                                  \
    CString Get##function_name() const                    { return object_name; }                             \
    CString Get##function_name(bool absolute_path) const  { return GetFilename(object_name, absolute_path); }
#define DefineSetFilenameMethods(function_name, object_name)                                                  \
    void Set##function_name(const TCHAR* filename)        { return SetFilename(object_name, filename); }
#define DefineGetSetFilenameMethods(function_name, object_name)                                               \
    DefineGetFilenameMethods(function_name, object_name)                                                      \
    DefineSetFilenameMethods(function_name, object_name)

#define DefineGetSetMultipleFilenamesMethods(function_name, object_name)                                                 \
    void Clear##function_name()                                          { object_name.clear(); }                        \
    const std::vector<CString>& Get##function_name() const               { return object_name.evaluated_filenames; }     \
    const std::vector<CString>& Get##function_name##Serializable() const { return object_name.serializable_filenames; }  \
    void Add##function_name(const TCHAR* filename)                       { AddFilename(object_name, filename); }

#define DefineGetSetConnectionStringMethods(function_name, object_name)                                                                      \
protected:                                                                                                                                   \
    CString Get##function_name##Text() const                            { return GetConnectionStringText(object_name, true); }               \
public:                                                                                                                                      \
    const ConnectionString& Get##function_name() const                  { return object_name; }                                              \
    void Set##function_name(const TCHAR* connection_string_text)        { return SetConnectionString(object_name, connection_string_text); } \
    void Set##function_name(const ConnectionString& connection_string)  { object_name = connection_string; }

#define DefineGetSetMultipleConnectionStringMethods(singular_function_name, object_name)                                                                                    \
    void Clear##singular_function_name##s()                                                 { object_name.clear(); }                                                        \
    const std::vector<ConnectionString>& Get##singular_function_name##s() const             { return object_name.evaluated_filenames; }                                     \
    const std::vector<ConnectionString>& Get##singular_function_name##sSerializable() const { return object_name.serializable_filenames; }                                  \
protected:                                                                                                                                                                  \
    std::vector<CString> Get##singular_function_name##sSerializableText() const { return GeConnectionStringsSerializableText(object_name); }                                \
public:                                                                                                                                                                     \
    void Add##singular_function_name(const ConnectionString& connection_string) { AddConnectionString(object_name, connection_string); }                                    \
    void Add##singular_function_name(const TCHAR* connection_string_text)       { AddFilename(object_name, connection_string_text); }                                       \
    void ClearAndAdd##singular_function_name##s(const std::vector<ConnectionString>& connection_strings) { ClearAndAddConnectionStrings(object_name, connection_strings); } \
    const ConnectionString& GetSingle##singular_function_name() const { return GetSingleConnectionString(object_name); }                                                    \
    const ConnectionString& Get##singular_function_name(size_t index) const { return GetConnectionString(object_name, index); }                                             \
    void SetSingle##singular_function_name(const ConnectionString& connection_string) { SetSingleConnectionString(object_name, connection_string); }                        \
    void SetSingle##singular_function_name(const TCHAR* connection_string_text) { SetSingleConnectionString(object_name, connection_string_text); }

#define DefineGetSetStringMethods(function_name, object_name)                                                      \
    CString Get##function_name() const                  { return object_name; }                                    \
    void Set##function_name(const TCHAR* text)          { object_name = text; }

#define DefineGetSetBooleanMethods(function_name, object_name)                                                     \
protected:                                                                                                         \
    CString Get##function_name##Text() const            { return GetBooleanText(object_name); }                    \
    void Set##function_name##Text(const TCHAR* text)    { SetBoolean(object_name,  text); }                        \
public:                                                                                                            \
    bool Get##function_name() const                     { return object_name; }                                    \
    void Set##function_name(bool value)                 { object_name = value; }

#define DefineGetSetEnumMethods(function_name, object_name, type, type_names, default_value)                       \
protected:                                                                                                         \
    CString Get##function_name##Text() const            { return GetEnumText(object_name, type_names); }           \
    void Set##function_name##Text(const TCHAR* text)    { SetEnum(object_name, type_names, text, default_value); } \
public:                                                                                                            \
    type Get##function_name() const                     { return object_name; }                                    \
    void Set##function_name(type value)                 { object_name = value; }

#define DefineGetSetFlagsMethods(function_name, object_name, flag_name_values)                                     \
protected:                                                                                                         \
    CString Get##function_name##Text() const            { return GetFlagsText(object_name, flag_name_values); }    \
    void Set##function_name##Text(const TCHAR* text)    { SetFlags(object_name, flag_name_values, text); }         \
public:                                                                                                            \
    bool Get##function_name(int parameter) const        { return ( object_name & parameter ) != 0; }               \
    void Set##function_name(int parameter, bool value)                                                             \
    {                                                                                                              \
        if( value )                                                                                                \
            object_name |= parameter;                                                                              \
        else                                                                                                       \
            object_name &= ~parameter;                                                                             \
    }

public:
    // the header
    DefineGetSetStringMethods(PifFileName, m_sPifFileName);
    DefineGetSetStringMethods(Version, m_sVersion);

    DefineGetSetEnumMethods(AppType, m_eAppType, APPTYPE, AppTypeNames, APPTYPE::INVALID_TYPE);
    CString GetAppTypeString() const { return GetAppTypeText(); }

    DefineGetSetEnumMethods(TabProcess, m_eTabProcess, PROCESS, ProcessNames, PROCESS::PROCESS_INVALID);

    DefineGetSetStringMethods(AppDescription, m_sAppDescription);
    CString GetEvaluatedAppDescription(bool bAddApplicationPrefix = false) const;


    // data entry init section
    DefineGetSetStringMethods(OpID, m_sOpID);

protected:
    CString GetStartModeText() const;
    void SetStartModeText(const TCHAR* text);
public:
    StartMode GetStartMode() const { return m_eStartMode; }
    CString GetStartModeString() const;
    void SetStartMode(StartMode start_mode, const CString& start_mode_text);
    DefineGetSetStringMethods(StartKeyString, m_sStartKey);

    DefineGetSetStringMethods(Key, m_csKey);

    DefineGetSetFlagsMethods(LockFlag, m_iLockFlags, LockFlagNameValues);
    bool GetAddLockFlag() const         { return GetLockFlag(LockFlag::Add); }
    bool GetModifyLockFlag() const      { return GetLockFlag(LockFlag::Modify); }
    bool GetDeleteLockFlag() const      { return GetLockFlag(LockFlag::Delete); }
    bool GetVerifyLockFlag() const      { return GetLockFlag(LockFlag::Verify); }
    bool GetViewLockFlag() const        { return GetLockFlag(LockFlag::View); }
    bool GetStatsLockFlag() const       { return GetLockFlag(LockFlag::Statistics); }
    bool GetCaseListingLockFlag() const { return GetLockFlag(LockFlag::CaseListing); }

    DefineGetSetStringMethods(CaseListingFilter, m_csCaseListingFilter);
    DefineGetSetEnumMethods(ShowInApplicationListing, m_eShowInApplicationListing, ShowInApplicationListing, ShowInApplicationListingNames, ShowInApplicationListing::Always);
    DefineGetSetEnumMethods(FullScreenFlag, m_eFullScreen, FULLSCREEN, FullScreenNames, FULLSCREEN::FULLSCREEN_NO);
    DefineGetSetBooleanMethods(AutoAddFlag, m_bAutoAdd);
    DefineGetSetBooleanMethods(FileOpenFlag, m_bDisableFileOpen);

protected:
    CString GetInteractiveEditModeText() const;
    void SetInteractiveEditModeText(const TCHAR* text);
public:
    InteractiveEditMode GetInteractiveEditMode() const      { return m_eInteractiveEditMode; }
    void SetInteractiveEditMode(InteractiveEditMode mode)   { m_eInteractiveEditMode = mode; }
    DefineGetSetBooleanMethods(InteractiveEditDialogLocked, m_bInteractiveEditDialogLocked);


    // the files section
    DefineGetFilenameMethods(AppFName, m_sAppFName);
    void SetAppFName(const TCHAR* filename);

    CString GetApplicationErrorsFilename() const;

    DefineGetSetMultipleConnectionStringMethods(InputDataConnectionString, m_inputDataConnectionStrings);
    DefineGetSetFilenameMethods(ExcelFilename, m_excelFilename);
    DefineGetSetMultipleFilenamesMethods(ExportFilenames, m_exportFiles);
    DefineGetSetMultipleConnectionStringMethods(OutputDataConnectionString, m_outputDataConnectionStrings)
    DefineGetSetFilenameMethods(InputDictFName, m_sInputDictFName);
    DefineGetSetFilenameMethods(OutputDictFName, m_sOutputDictFName);
    DefineGetSetConnectionStringMethods(ReferenceDataConnectionString, m_referenceDataConnectionString);
    DefineGetSetFilenameMethods(TabOutputFName, m_sTabOutputFName);
    DefineGetSetMultipleFilenamesMethods(ConInputFilenames, m_conInputFilenames);
    DefineGetSetFilenameMethods(ConOutputFName, m_sConOutputFName);
    DefineGetSetFilenameMethods(PrepInputFName, m_sPrepInputFName);
    DefineGetSetFilenameMethods(PrepOutputFName, m_sPrepOutputFName);
    DefineGetSetFilenameMethods(AreaFName, m_sAreaFName);
    DefineGetSetFilenameMethods(SPSSSyntaxFName, m_sSPSSCodeFName);
    DefineGetSetFilenameMethods(SASSyntaxFName, m_sSASCodeFName);
    DefineGetSetFilenameMethods(STATASyntaxFName, m_sSTATACodeFName);
    DefineGetSetFilenameMethods(STATADOFName, m_sSTATADOFName);
    DefineGetSetFilenameMethods(CSPROSyntaxFName, m_sCSPROCodeFName);
    DefineGetSetFilenameMethods(RSyntaxFName, m_sRCodeFName);
    DefineGetSetFilenameMethods(PackOutputFName, m_sPackOutputFName);
    DefineGetSetMultipleFilenamesMethods(PackExtraFiles, m_extraFiles);
    DefineGetSetMultipleFilenamesMethods(InputParadataFilenames, m_inputParadata);
    DefineGetSetFilenameMethods(OutputParadataFilename, m_outputParadata);
    DefineGetSetFilenameMethods(ParadataFilename, m_csParadataFilename);
    DefineGetSetFilenameMethods(ListingFName, m_sListingFName);
    DefineGetSetFilenameMethods(FrequenciesFilename, m_frequenciesFilename);
    DefineGetSetFilenameMethods(ImputeFrequenciesFilename, m_imputeFrequenciesFilename);
    DefineGetSetConnectionStringMethods(ImputeStatConnectionString, m_imputeStatConnectionString);
    DefineGetSetFilenameMethods(WriteFName, m_sWriteDataFName);
    DefineGetSetFilenameMethods(SaveArrayFilename, m_saveArrayFilename);
    DefineGetSetFilenameMethods(CommonStoreFName, m_csCommonStoreFName);
    DefineGetSetFilenameMethods(HtmlDialogsDirectory, m_htmlDialogsDirectory);

    const std::optional<BaseMapSelection>& GetBaseMapSelection() const { return m_baseMapSelection; }
    CString GetBaseMapSelectionText() const;
    void SetBaseMapSelection(const TCHAR* text);

    // external (data) files
    void SetExternalDataConnectionString(CString dictionary_name, const ConnectionString& connection_string);
    void SetExternalDataConnectionString(CString dictionary_name, const TCHAR* connection_string_text);
    const ConnectionString& GetExternalDataConnectionString(CString dictionary_name) const;
    const std::map<CString, ConnectionString>& GetExternalDataConnectionStrings() const { return m_mapExternalDataConnectionStrings; }


    // user files
    void SetUsrDatAssoc(CString file_handler_name, CString filename);
    CString LookUpUsrDatFile(CString file_handler_name, bool absolute_path = true) const;
    std::vector<CString> GetUserFiles() const;
    void ClearUserFilesMap() { m_mapUserFiles.clear(); }


    // the parameters section
    DefineGetSetStringMethods(StartLanguageString, m_sStartLanguage);
    DefineGetSetStringMethods(ParamString, m_sParameter);
    DefineGetSetEnumMethods(ViewListing, m_eViewListing, VIEWLISTING, ViewListingNames, VIEWLISTING::ALWAYS);
    DefineGetSetBooleanMethods(ViewResultsFlag, m_bViewResults);
    DefineGetSetFlagsMethods(PackInclude, m_iPackIncludes, PackIncludeFlagNameValues);
    DefineGetSetEnumMethods(DuplicateCase, m_eDuplicateCase, DuplicateCase, DuplicateCaseNames, DuplicateCase::List);
    DefineGetSetBooleanMethods(SkipStructFlag, m_bSkipStructure);
    DefineGetSetBooleanMethods(ChkRangesFlag, m_bChkRanges);

protected:
    CString GetListingWidthText() const;
    void SetListingWidthText(const TCHAR* text);
public:
    int GetListingWidth() const                 { return m_iListingWidth; }
    void SetListingWidth(int iListingWidth)     { m_iListingWidth = iListingWidth; }

    DefineGetSetBooleanMethods(MessageWrap, m_bMessageWrap);
    DefineGetSetEnumMethods(ErrMsgOverride, m_errMsgOverride, ErrMsgOverride, ErrMsgOverrideNames, ErrMsgOverride::No);

    DefineGetSetEnumMethods(InputOrder, m_eInputOrder, InputOrder, InputOrderNames, InputOrder::Sequential);
    bool GetInputOrderIndexed() const { return ( m_eInputOrder == InputOrder::Indexed ); }

    DefineGetSetBooleanMethods(DisplayNames, m_bDisplayNames);
    DefineGetSetEnumMethods(ConcatenateMethod, m_eConcatenateMethod, ConcatenateMethod, ConcatenateMethodNames, ConcatenateMethod::Text);
    DefineGetSetEnumMethods(SyncServerType, m_eSyncServerType, SyncServerType, SyncServerTypeNames, SyncServerType::FTP);
    DefineGetSetEnumMethods(SyncDirection, m_eSyncDirection, SyncDirection, SyncDirectionNames, SyncDirection::Get);
    DefineGetSetStringMethods(SyncUrl, m_sSyncUrl);
    DefineGetSetEnumMethods(DeployToOverride, m_eDeployToOverride, DeployToOverride, DeployToOverrideNames, DeployToOverride::None);
    DefineGetSetBooleanMethods(Silent, m_bSilent);
    DefineGetSetFilenameMethods(OnExitFilename, m_csOnExitFilename);

    void SetCustomParamString(StringNoCase attribute, std::wstring value);
    const std::wstring& GetCustomParamString(const StringNoCase& attribute) const;
    std::vector<std::tuple<std::wstring, std::wstring>> GetCustomParams() const;
    std::vector<std::wstring> GetCustomParamMappings() const;


    // the data entry IDs section
    void SetPersistentData(CString field_name, CString value);
    CString GetPersistentData(CString field_name) const;


    bool EntryConnectionStringsContainWildcards() const;

    bool UsingOutputData() const;

    // properties used by CSTab but not serialized to the PFF
protected:
    std::vector<CString> m_asCalcInputFNames;   // Array of calc input file names
    CString m_sCalcOutputFName;                 // Calc output file name
public:
    DefineGetSetFilenameMethods(CalcOutputFName, m_sCalcOutputFName);
    std::vector<CString>& GetCalcInputFNamesArr() { return m_asCalcInputFNames; }

    ///////////////////////////////////////////////////////////////////////////////////
    //            Build/Save/Internal functions
    ////////////////////////////////////////////////////////////////////////////////////

    std::optional<std::wstring> GetExecutableProgram() const;

#ifdef WIN_DESKTOP
    static void ExecutePff(const std::wstring& pff_filename, const std::optional<NullTerminatedString> extra_arguments = std::nullopt);
    void ExecuteOnExitPff() const;
#endif

    static void ViewResults(NullTerminatedString filename);
    static void ViewResults(const ConnectionString& connection_string);

    static void ViewListing(const TCHAR* listing_filename);
    void ViewListing() const;

    bool LoadPifFile(bool silently_load_the_spec_file = false);
    // loading with a filename is not supported, but it is declared here so that a linker
    // error prevents code from automatically being cast to use the above method
    void LoadPifFile(const TCHAR*);

    bool Save(bool silently_save_the_spec_file = false) const;

    void SetProperties(const std::wstring& command, const std::vector<std::wstring>& arguments);
    std::vector<std::wstring> GetProperties(CString command) const;

protected:
    const TCHAR* GetSectionHeaderForProperties(CString* command) const;

    typedef CString (PFF::*GetterFunction)() const;
    typedef void (PFF::*SetterFunction)(const TCHAR* text);
    typedef const std::vector<CString>& (PFF::*MultipleGetterFunction)() const;
    typedef std::vector<CString> (PFF::*MultipleGetterTransformedFunction)() const;
    typedef void (PFF::*ClearFunction)();
    struct PffFunctions;
    static const std::vector<PffFunctions>& GetPffFunctionsArray();
    const PffFunctions* GetPffFunctions(const TCHAR* command) const;

    void SetCommandArgumentPair(const TCHAR* last_section_header, CString command, CString argument, bool clear_existing_multiple_filenames);

    void AdjustAttributesFromOldFiles();

public:
    // for the Help Generator
    static std::vector<const TCHAR*> GetAppTypeWords();
    static std::vector<const TCHAR*> GetHeadingWords();
    static std::vector<const TCHAR*> GetAttributeWords();


protected:
    std::shared_ptr<Application> m_application; // application object

public:
    const Application* GetApplication() const                     { return m_application.get(); }
    Application* GetApplication()                                 { return m_application.get(); }
    void SetApplication(std::shared_ptr<Application> application) { m_application = std::move(application); }
};
