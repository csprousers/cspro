#include "stdafx.h"
#include "PFF.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/ArrUtil.h> // remove once CString is gone
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilO/Specfile.h>
#include <zUtilO/Viewers.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    constexpr const TCHAR* RUNINFO                  = _T("[Run Information]");
    constexpr const TCHAR* APPTYPESTRING            = _T("AppType");
    constexpr const TCHAR* OPERATION                = _T("Operation");
    constexpr const TCHAR* APPDESCRIPTION           = _T("Description");

    constexpr const TCHAR* DATAENTRYINIT            = _T("[DataEntryInit]");
    constexpr const TCHAR* OPID                     = _T("OperatorID");
    constexpr const TCHAR* STARTMODE                = _T("StartMode");
    constexpr const TCHAR* KEY                      = _T("Key");
    constexpr const TCHAR* LOCK                     = _T("Lock");
    constexpr const TCHAR* CASELISTINGFILTER        = _T("CaseListingFilter");
    constexpr const TCHAR* SHOWINAPPLICATIONLISTING = _T("ShowInApplicationListing");
    constexpr const TCHAR* FULLSCREEN_STR           = _T("FullScreen");
    constexpr const TCHAR* AUTOADD                  = _T("AutoAdd");
    constexpr const TCHAR* NOFILEOPEN               = _T("NoFileOpen");
    constexpr const TCHAR* INTERACTIVE              = _T("Interactive");

    constexpr const TCHAR* FILESSTRING              = _T("[Files]");
    constexpr const TCHAR* APPSTRING                = _T("Application");
    constexpr const TCHAR* INPUTDATA                = _T("InputData");
    constexpr const TCHAR* EXCEL                    = _T("Excel");
    constexpr const TCHAR* EXPTOUTPUT               = _T("ExportedData");
    constexpr const TCHAR* OUTPUTDATA               = _T("OutputData");
    constexpr const TCHAR* INPUTDICT                = PFF_COMMAND_INPUT_DICT;
    constexpr const TCHAR* OUTPUTDICT               = PFF_COMMAND_OUTPUT_DICT;
    constexpr const TCHAR* REFERENCEDATA            = _T("ReferenceData");
    constexpr const TCHAR* TABOUTPUT                = _T("TabOutputTAB");
    constexpr const TCHAR* CONINPUT                 = _T("ConInputTAB");
    constexpr const TCHAR* CONOUTPUT                = _T("ConOutputTAB");
    constexpr const TCHAR* PREPINPUT                = _T("FormatInputTAB");
    constexpr const TCHAR* PREPOUTPUT               = _T("OutputTBW");
    constexpr const TCHAR* AREANAMES                = _T("AreaNames");
    constexpr const TCHAR* SPSSSYNTAXFNAME          = _T("SPSSDescFile");
    constexpr const TCHAR* SASSYNTAXFNAME           = _T("SASDescFile");
    constexpr const TCHAR* STATASYNTAXFNAME         = _T("STATADescFile");
    constexpr const TCHAR* STATADOFNAME             = _T("STATALabelsFile");
    constexpr const TCHAR* CSPROSYNTAXFNAME         = _T("CSProDescFile");
    constexpr const TCHAR* RSYNTAXFNAME             = _T("RDescFile");
    constexpr const TCHAR* PACKOUTPUT               = _T("ZipFile");
    constexpr const TCHAR* EXTRAFILE                = _T("ExtraFile");
    constexpr const TCHAR* INPUTPARADATA            = _T("InputParadata");
    constexpr const TCHAR* OUTPUTPARADATA           = _T("OutputParadata");
    constexpr const TCHAR* PARADATA                 = _T("Paradata");
    constexpr const TCHAR* LISTING                  = _T("Listing");
    constexpr const TCHAR* FREQS                    = _T("Freqs");
    constexpr const TCHAR* IMPUTEFREQS              = _T("ImputeFreqs");
    constexpr const TCHAR* IMPUTESTAT               = _T("ImputeStat");
    constexpr const TCHAR* WRITEDATA                = _T("WriteData");
    constexpr const TCHAR* SAVEARRAY                = _T("SaveArray");
    constexpr const TCHAR* COMMONSTORE              = _T("CommonStore");
    constexpr const TCHAR* HTMLDIALOGS              = _T("HtmlDialogs");
    constexpr const TCHAR* BASEMAP                  = _T("BaseMap");

    constexpr const TCHAR* EXTERNALFILES            = _T("[ExternalFiles]");

    constexpr const TCHAR* USERFILES                = _T("[UserFiles]");

    constexpr const TCHAR* PARAMETERS               = _T("[Parameters]");
    constexpr const TCHAR* LANGUAGE                 = _T("Language");
    constexpr const TCHAR* PARAMSTRING              = _T("Parameter");
    constexpr const TCHAR* VIEWLISTING_STR          = _T("ViewListing");
    constexpr const TCHAR* VIEWRESULTS              = _T("ViewResults");
    constexpr const TCHAR* PACK_INCLUDES_STR        = _T("PackInclude");
    constexpr const TCHAR* DUPLICATE_CASE           = _T("DuplicateCase");
    constexpr const TCHAR* SKIPSTRUCT               = _T("SkipStructure");
    constexpr const TCHAR* CHKRANGES                = _T("CheckRanges");
    constexpr const TCHAR* LISTINGWIDTH             = _T("ListingWidth");
    constexpr const TCHAR* MESSAGEWRAP              = _T("MessageWrap");
    constexpr const TCHAR* ERRMSGOVERRIDE           = _T("ErrmsgOverride");
    constexpr const TCHAR* INPUT_ORDER              = _T("InputOrder");
    constexpr const TCHAR* DISPLAYNAMES             = _T("DisplayNames");
    constexpr const TCHAR* CONCAT_METHOD            = _T("ConcatMethod");
    constexpr const TCHAR* SYNC_SERVER_TYPE         = _T("SyncType");
    constexpr const TCHAR* SYNC_DIRECTION           = _T("SyncDirection");
    constexpr const TCHAR* SYNC_URL                 = _T("SyncUrl");
    constexpr const TCHAR* DEPLOY_TO_OVERRIDE       = _T("DeployToOverride");
    constexpr const TCHAR* SILENT                   = _T("Silent");
    constexpr const TCHAR* ONEXIT                   = _T("OnExit");

    constexpr const TCHAR* DATAENTRYIDS             = _T("[DataEntryIds]");
}

const TCHAR* const AppTypeNames[] =
{
    _T("Entry"),                _T("Batch"),        _T("Tabulation"),
    _T("Frequencies"),          _T("Sort"),         _T("Export"),
    _T("Reformat"),             _T("Compare"),      _T("Concatenate"),
    _T("Pack"),                 _T("Index"),        _T("Sync"),
    _T("ParadataConcatenate"),  _T("Excel2CSPro"),  _T("Deploy"),
    _T("View"),
    nullptr
};

const TCHAR* const ProcessNames[] = { _T("All"), _T("Tab"), _T("Con"), _T("Calc"), _T("Format"), nullptr };

const TCHAR* const StartModeNames[] = { _T("None"), _T("Add"), _T("Modify"), _T("Verify"), nullptr };

const FlagNameValue LockFlagNameValues[]
{
    { _T("Add"),         LockFlag::Add },
    { _T("Modify"),      LockFlag::Modify },
    { _T("Delete"),      LockFlag::Delete },
    { _T("Verify"),      LockFlag::Verify },
    { _T("View"),        LockFlag::View },
    { _T("Stats"),       LockFlag::Statistics },
    { _T("CaseListing"), LockFlag::CaseListing },
    { nullptr,           0 }
};

const TCHAR* const ShowInApplicationListingNames[] = { _T("Always"), _T("Hidden"), _T("Never"), nullptr };

const TCHAR* const FullScreenNames[] = { CSPRO_ARG_NO, CSPRO_ARG_YES, _T("NoMenus"), nullptr };

const TCHAR* const InteractiveEditModeNames[] = { _T("ErrMsg"), _T("Range"), _T("Both"), _T("Ask"), _T("Off"), nullptr, };
const TCHAR* const InteractiveEditLock = _T("Lock");

const TCHAR* const ViewListingNames[] = { _T("Never"), _T("Always"), _T("OnError"), nullptr };

const FlagNameValue PackIncludeFlagNameValues[]
{
    { _T("VSImages"),       PackIncludeFlag::ValueSetImages },
    { _T("Resources"),      PackIncludeFlag::Resources },
    { _T("InputFiles"),     ( PackIncludeFlag::InputFile | PackIncludeFlag::ExternalFiles ), false },
    { _T("InputFile"),      PackIncludeFlag::InputFile },
    { _T("ExternalFiles"),  PackIncludeFlag::ExternalFiles },
    { _T("UserFiles"),      PackIncludeFlag::UserFiles },
    { nullptr,              0 }
};

const TCHAR* const ErrMsgOverrideNames[] = { CSPRO_ARG_NO, _T("Summary"), _T("Case"), nullptr };

const TCHAR* const InputOrderNames[] = { _T("Indexed"), _T("Sequential"), nullptr };

const TCHAR* const ConcatenateMethodNames[] = { _T("Case"), _T("Text"), nullptr };

const TCHAR* const DuplicateCaseNames[] = { _T("List"), _T("View"), _T("Prompt"), _T("PromptIfDifferent"), _T("KeepFirst"), nullptr };

const TCHAR* const SyncServerTypeNames[] = { _T("CSWeb"), _T("Dropbox"), _T("FTP"), _T("LocalDropbox"), _T("LocalFiles"), nullptr };

const TCHAR* const SyncDirectionNames[] = { _T("=unused"), _T("Put"), _T("Get"), _T("Both"), nullptr };

const TCHAR* const DeployToOverrideNames[] = { _T("None"), _T("CSWeb"), _T("Dropbox"), _T("FTP"), _T("LocalFile"), _T("LocalFolder"), nullptr };

namespace
{
    static ConnectionString UndefinedConnectionString;
}

template<typename T>
void MultipleFilenames<T>::clear()
{
    evaluated_filenames.clear();
    serializable_filenames.clear();
}


FlagNameValue::FlagNameValue(const TCHAR* const _name, int _value, bool _serialize/* = true*/)
    :   name(_name),
        value(_value),
        serialize(_serialize)
{
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PFF::PFF(CString sFileName/* = CString()*/)
{
    ResetContents();

    m_sPifFileName = sFileName;
}


void PFF::ResetContents()
{
    m_sVersion = CSPRO_VERSION;
    m_eAppType = APPTYPE::ENTRY_TYPE;
    m_eTabProcess = ALL_STUFF;
    m_sAppDescription.Empty();

    m_sOpID.Empty();
    m_eStartMode = StartMode::None;
    m_sStartKey.Empty();
    m_csKey.Empty();
    m_iLockFlags = 0;
    m_csCaseListingFilter.Empty();
    m_eShowInApplicationListing = ShowInApplicationListing::Always;
    m_eFullScreen = FULLSCREEN::FULLSCREEN_NO;
    m_bAutoAdd = true;
    m_bDisableFileOpen = false;
    m_eInteractiveEditMode = InteractiveEditMode::Ask;
    m_bInteractiveEditDialogLocked = false;

    m_sAppFName.Empty();
    m_inputDataConnectionStrings.clear();
    m_excelFilename.Empty();
    m_exportFiles.clear();
    m_outputDataConnectionStrings.clear();
    m_sInputDictFName.Empty();
    m_sOutputDictFName.Empty();
    m_referenceDataConnectionString.Clear();
    m_sTabOutputFName.Empty();
    m_conInputFilenames.clear();
    m_sConOutputFName.Empty();
    m_sPrepInputFName.Empty();
    m_sPrepOutputFName.Empty();
    m_sAreaFName.Empty();
    m_sSPSSCodeFName.Empty();
    m_sSASCodeFName.Empty();
    m_sSTATACodeFName.Empty();
    m_sSTATADOFName.Empty();
    m_sCSPROCodeFName.Empty();
    m_sRCodeFName.Empty();
    m_sPackOutputFName.Empty();
    m_extraFiles.clear();
    m_inputParadata.clear();
    m_outputParadata.Empty();
    m_csParadataFilename.Empty();
    m_sListingFName.Empty();
    m_frequenciesFilename.Empty();
    m_imputeFrequenciesFilename.Empty();
    m_imputeStatConnectionString.Clear();
    m_sWriteDataFName.Empty();
    m_saveArrayFilename.Empty();
    m_csCommonStoreFName.Empty();
    m_htmlDialogsDirectory.Empty();
    m_baseMapSelection.reset();

    m_mapExternalDataConnectionStrings.clear();

    m_mapUserFiles.clear();

    m_sStartLanguage.Empty();
    m_sParameter.Empty();
    m_eViewListing = VIEWLISTING::ALWAYS;
    m_bViewResults = true;
    m_iPackIncludes = 0;
    m_eDuplicateCase = DuplicateCase::List;
    m_bSkipStructure = false;
    m_bChkRanges = false;
    m_iListingWidth = 120;
    m_bMessageWrap = false;
    m_errMsgOverride = ErrMsgOverride::No;
    m_eInputOrder = InputOrder::Sequential;
    m_bDisplayNames = false;
    m_eConcatenateMethod = ConcatenateMethod::Text;
    m_eSyncServerType = SyncServerType::FTP;
    m_eSyncDirection = SyncDirection::Get;
    m_sSyncUrl.Empty();
    m_eDeployToOverride = DeployToOverride::None;
    m_bSilent = false;
    m_csOnExitFilename.Empty();
    m_customParameters.clear();

    m_mapPersistent.clear();

    m_asCalcInputFNames.clear();
    m_sCalcOutputFName.Empty();
}


CString PFF::GetFilename(const CString& filename, bool absolute_path) const
{
    return filename.IsEmpty() ? CString() :
           absolute_path      ? filename :
                                GetRelativeFName<CString>(m_sPifFileName, filename);
}

void PFF::MakeAbsolutePath(CString& filename_object)
{
    if( !filename_object.IsEmpty() )
        filename_object = WS2CS(MakeFullPath(GetWorkingFolder(m_sPifFileName), CS2WS(filename_object)));
}

void PFF::SetFilename(CString& filename_object, const TCHAR* filename)
{
    filename_object = filename;
    MakeAbsolutePath(filename_object);
}

void PFF::AddFilename(MultipleFilenames<CString>& multiple_filenames_object, const TCHAR* filename)
{
    CString serializable_filename;
    SetFilename(serializable_filename, filename);

    if( !serializable_filename.IsEmpty() )
    {
        multiple_filenames_object.serializable_filenames.emplace_back(serializable_filename);

        // evaluate the filename in case it uses wildcards
        for( const std::wstring& this_filename : DirectoryLister::GetFilenamesWithPossibleWildcard(serializable_filename, true) )
            multiple_filenames_object.evaluated_filenames.emplace_back(WS2CS(this_filename));
    }
}


CString PFF::GetConnectionStringText(const ConnectionString& connection_string, bool absolute_path) const
{
    if( connection_string.IsDefined() )
    {
        return WS2CS(absolute_path ? connection_string.ToString() :
                                     connection_string.ToRelativeString(GetWorkingFolder(m_sPifFileName)));
    }

    else
    {
        return CString();
    }
}

void PFF::SetConnectionString(ConnectionString& connection_string_object, const TCHAR* connection_string_text)
{
    CString text = connection_string_text;

    // if blank, set the connection string to undefined
    if( SO::IsBlank(text) )
    {
        connection_string_object = UndefinedConnectionString;
    }

    else
    {
        connection_string_object = ConnectionString(text);
        connection_string_object.AdjustRelativePath(GetWorkingFolder(m_sPifFileName));
    }
}


std::vector<CString> PFF::GeConnectionStringsSerializableText(const MultipleFilenames<ConnectionString>& multiple_connection_strings) const
{
    std::vector<CString> connection_string_texts;

    for( const ConnectionString& connection_string : multiple_connection_strings.serializable_filenames )
        connection_string_texts.emplace_back(WS2CS(connection_string.ToString()));

    return connection_string_texts;
}

void PFF::AddConnectionString(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, ConnectionString connection_string)
{
    if( connection_string.IsDefined() )
    {
        ConnectionString& added_connection_string = multiple_connection_strings_object.serializable_filenames.emplace_back(std::move(connection_string));

        added_connection_string.AdjustRelativePath(GetWorkingFolder(m_sPifFileName));

        // expand wildcards only for for non-output data
        if( &multiple_connection_strings_object == &m_outputDataConnectionStrings )
        {
            multiple_connection_strings_object.evaluated_filenames.emplace_back(added_connection_string);
        }

        else
        {
            ASSERT(&multiple_connection_strings_object == &m_inputDataConnectionStrings);
            PathHelpers::ExpandConnectionStringWildcards(multiple_connection_strings_object.evaluated_filenames, added_connection_string);
        }
    }
}

void PFF::AddFilename(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, const TCHAR* connection_string_text)
{
    CString text = connection_string_text;
    AddConnectionString(multiple_connection_strings_object, SO::IsBlank(text) ? UndefinedConnectionString : ConnectionString(text));
}

void PFF::ClearAndAddConnectionStrings(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, const std::vector<ConnectionString>& connection_strings)
{
    multiple_connection_strings_object.clear();

    for( const ConnectionString& connection_string : connection_strings )
        AddConnectionString(multiple_connection_strings_object, connection_string);
}

const ConnectionString& PFF::GetSingleConnectionString(const MultipleFilenames<ConnectionString>& multiple_connection_strings) const
{
    return multiple_connection_strings.evaluated_filenames.empty() ? UndefinedConnectionString :
                                                                     multiple_connection_strings.evaluated_filenames.front();
}

const ConnectionString& PFF::GetConnectionString(const MultipleFilenames<ConnectionString>& multiple_connection_strings, size_t index) const
{
    if( index < multiple_connection_strings.evaluated_filenames.size() )
        return multiple_connection_strings.evaluated_filenames[index];

    else
    {
        ASSERT(false);
        return UndefinedConnectionString;
    }
}

void PFF::SetSingleConnectionString(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, ConnectionString connection_string)
{
    multiple_connection_strings_object.clear();
    AddConnectionString(multiple_connection_strings_object, std::move(connection_string));
}

void PFF::SetSingleConnectionString(MultipleFilenames<ConnectionString>& multiple_connection_strings_object, wstring_view connection_string_text)
{
    SetSingleConnectionString(multiple_connection_strings_object, ConnectionString(connection_string_text));
}


CString PFF::GetBooleanText(bool boolean_object)
{
    return boolean_object ? CSPRO_ARG_YES : CSPRO_ARG_NO;
}

void PFF::SetBoolean(bool& boolean_object, const TCHAR* text)
{
    boolean_object = SO::EqualsNoCase(text, CSPRO_ARG_YES);
}


template<typename T>
CString PFF::GetEnumText(T enum_object, const TCHAR* const type_names[])
{
    return type_names[static_cast<size_t>(enum_object)];
}

template CString PFF::GetEnumText(APPTYPE enum_object, const TCHAR* const type_names[]);

template<typename T>
void PFF::SetEnum(T& enum_object, const TCHAR* const type_names[], const TCHAR* text, T default_value)
{
    for( size_t i = 0; type_names[i] != nullptr; i++ )
    {
        if( SO::EqualsNoCase(text, type_names[i]) )
        {
            enum_object = static_cast<T>(i);
            return;
        }
    }

    enum_object = default_value;
}


CString PFF::GetFlagsText(int flag_object, const FlagNameValue flag_name_values[])
{
    CString text;

    for( size_t i = 0; flag_name_values[i].name != nullptr; i++ )
    {
        if( flag_name_values[i].serialize && ( flag_object & flag_name_values[i].value ) != 0 )
            text.AppendFormat(_T("%s%s"), text.IsEmpty() ? _T("") : _T(","), flag_name_values[i].name);
    }

    return text;
}


void PFF::SetFlags(int& flag_object, const FlagNameValue flag_name_values[], const wstring_view text_sv)
{
    SO::ForeachSection(text_sv, _T(","),
        [&](wstring_view flag_sv)
        {
            flag_sv = SO::Trim(flag_sv);

            for( size_t i = 0; flag_name_values[i].name != nullptr; ++i )
            {
                if( SO::EqualsNoCase(flag_sv, flag_name_values[i].name) )
                {
                    flag_object |= flag_name_values[i].value;
                    break;
                }
            }

            return true;
        });
}


CString PFF::GetEvaluatedAppDescription(bool bAddApplicationPrefix/* = false*/) const
{
    if( !m_sAppDescription.IsEmpty() )
    {
        return m_sAppDescription;
    }

    else
    {
        // Use pff filename if it is not empty, otherwise use app filename
        CString csApplicationName = !m_sPifFileName.IsEmpty() ? m_sPifFileName : m_sAppFName;
        csApplicationName = PortableFunctions::PathRemoveFileExtension<CString>(PortableFunctions::PathGetFilename(csApplicationName));

        if( bAddApplicationPrefix )
            csApplicationName.Format(_T("Application: %s"), csApplicationName.GetString());

        return csApplicationName;
    }
}


CString PFF::GetStartModeText() const
{
    CString text;

    if( m_eStartMode != StartMode::None )
        text.Format(_T("%s%s%s"), StartModeNames[static_cast<size_t>(m_eStartMode)], m_sStartKey.IsEmpty() ? _T("") : _T(";"), m_sStartKey.GetString());

    return text;
}

void PFF::SetStartModeText(const TCHAR* text)
{
    CString command(text);
    CString key;

    int semicolon_pos = command.Find(';');

    if( semicolon_pos > 0 )
    {
        key = command.Mid(semicolon_pos + 1);
        command = command.Left(semicolon_pos).Trim();
    }

    SetEnum(m_eStartMode, StartModeNames, command, StartMode::None);

    if( m_eStartMode == StartMode::None )
        key.Empty();

    SetStartKeyString(key);
}

CString PFF::GetStartModeString() const
{
    return ( m_eStartMode != StartMode::None ) ? (CString)StartModeNames[static_cast<size_t>(m_eStartMode)] : CString();
}

void PFF::SetStartMode(StartMode start_mode, const CString& start_mode_text)
{
    m_eStartMode = start_mode;
    m_sStartKey = start_mode_text;
}


CString PFF::GetInteractiveEditModeText() const
{
    CString text = InteractiveEditModeNames[static_cast<size_t>(m_eInteractiveEditMode)];

    if( m_eInteractiveEditMode != InteractiveEditMode::Ask && m_bInteractiveEditDialogLocked )
        text.AppendFormat(_T(",%s"), InteractiveEditLock);

    return text;
}

void PFF::SetInteractiveEditModeText(const TCHAR* text)
{
    CString command(text);
    bool locked = false;

    // check if the locked flag is set
    int comma_pos = command.Find(_T(","));

    if( comma_pos > 0 )
    {
        CString lock_text = command.Mid(comma_pos + 1).Trim();
        locked = ( lock_text.CompareNoCase(InteractiveEditLock) == 0 );
        command = command.Left(comma_pos).Trim();
    }

    SetEnum(m_eInteractiveEditMode, InteractiveEditModeNames, command, InteractiveEditMode::Ask);

    if( m_eInteractiveEditMode == InteractiveEditMode::Ask )
        locked = false;

    SetInteractiveEditDialogLocked(locked);
}


void PFF::SetAppFName(const TCHAR* filename)
{
    SetFilename(m_sAppFName, filename);

    // adjust the app type based on the filename (if it is not an input to CSPack or CSView)
    if( GetAppType() != APPTYPE::PACK_TYPE && GetAppType() != APPTYPE::VIEW_TYPE )
    {
        auto matches = [extension = PortableFunctions::PathGetFileExtension(m_sAppFName)](const TCHAR* test_extension)
        {
            return SO::EqualsNoCase(extension, test_extension);
        };

        APPTYPE new_app_type =
            matches(FileExtensions::EntryApplication)      ? APPTYPE::ENTRY_TYPE :
            matches(FileExtensions::BinaryEntryPen)        ? APPTYPE::ENTRY_TYPE :
            matches(FileExtensions::BatchApplication)      ? APPTYPE::BATCH_TYPE :
            matches(FileExtensions::TabulationApplication) ? APPTYPE::TAB_TYPE :
            matches(FileExtensions::FrequencySpec)         ? APPTYPE::FREQ_TYPE :
            matches(FileExtensions::SortSpec)              ? APPTYPE::SORT_TYPE :
            matches(FileExtensions::ExportSpec)            ? APPTYPE::EXPORT_TYPE :
            matches(FileExtensions::CompareSpec)           ? APPTYPE::COMPARE_TYPE :
            matches(FileExtensions::PackSpec)              ? APPTYPE::PACK_TYPE :
                                                             GetAppType();
        SetAppType(new_app_type);
    }
}


CString PFF::GetApplicationErrorsFilename() const
{
    ASSERT(!m_sAppFName.IsEmpty());
    return m_sAppFName + _T(".err");
}


CString PFF::GetBaseMapSelectionText() const
{
    return m_baseMapSelection.has_value() ? WS2CS(ToString(*m_baseMapSelection)) :
                                            CString();
}

void PFF::SetBaseMapSelection(const TCHAR* text)
{
    m_baseMapSelection = FromString(text, m_sPifFileName);
}


void PFF::SetExternalDataConnectionString(CString dictionary_name, const ConnectionString& connection_string)
{
    dictionary_name.MakeUpper();

    if( connection_string.IsDefined() )
        m_mapExternalDataConnectionStrings[dictionary_name] = connection_string;

    else
        m_mapExternalDataConnectionStrings.erase(dictionary_name);
}

void PFF::SetExternalDataConnectionString(CString dictionary_name, const TCHAR* connection_string_text)
{
    ConnectionString connection_string;
    SetConnectionString(connection_string, connection_string_text);
    SetExternalDataConnectionString(dictionary_name, connection_string);
}

const ConnectionString& PFF::GetExternalDataConnectionString(CString dictionary_name) const
{
    dictionary_name.MakeUpper();
    const auto& itr = m_mapExternalDataConnectionStrings.find(dictionary_name);
    return ( itr != m_mapExternalDataConnectionStrings.end() ) ? itr->second : UndefinedConnectionString;
}


void PFF::SetUsrDatAssoc(CString file_handler_name, CString filename)
{
    CString original_case_file_handler_name = file_handler_name;
    file_handler_name.MakeUpper();

    MakeAbsolutePath(filename);
    m_mapUserFiles[file_handler_name] = std::make_tuple(original_case_file_handler_name, filename);
}

CString PFF::LookUpUsrDatFile(CString file_handler_name, bool absolute_path/* = true*/) const
{
    const auto& itr = m_mapUserFiles.find(file_handler_name.MakeUpper());
    return ( itr != m_mapUserFiles.end() ) ? GetFilename(std::get<1>(itr->second), absolute_path) : CString();
}

std::vector<CString> PFF::GetUserFiles() const
{
    std::vector<CString> filenames;

    for( auto itr = m_mapUserFiles.cbegin(); itr != m_mapUserFiles.cend(); ++itr )
        filenames.emplace_back(std::get<1>(itr->second));

    return filenames;
}


CString PFF::GetListingWidthText() const
{
    return IntToString(m_iListingWidth);
}

void PFF::SetListingWidthText(const TCHAR* text)
{
    constexpr int MinimumListingWidth = 60;
    const int parsed_value = static_cast<int>(CIMSAString(text).Val());
    SetListingWidth(std::max(MinimumListingWidth, parsed_value));
}


void PFF::SetCustomParamString(StringNoCase attribute, std::wstring value)
{
    auto lookup = m_customParameters.find(attribute);

    if( lookup != m_customParameters.end() )
    {
        lookup->second.emplace_back(std::move(value));
    }

    else
    {
        m_customParameters.try_emplace(std::move(attribute), std::vector<std::wstring> { std::move(value) });
    }
}


const std::wstring& PFF::GetCustomParamString(const StringNoCase& attribute) const
{
    const auto& lookup = m_customParameters.find(attribute);

    // if the attribute was associated with multiple pieces of data, then only the last is returned
    return ( lookup != m_customParameters.end() ) ? lookup->second.back() :
                                                    SO::EmptyString;
}


std::vector<std::tuple<std::wstring, std::wstring>> PFF::GetCustomParams() const
{
    std::vector<std::tuple<std::wstring, std::wstring>> custom_params;

    for( const auto& [attribute, values] : m_customParameters )
        custom_params.emplace_back(attribute, values.back());

    return custom_params;
}


std::vector<std::wstring> PFF::GetCustomParamMappings() const
{
    std::vector<std::wstring> custom_params;

    for( const auto& [attribute, values] : m_customParameters )
    {
        for( const std::wstring value : values )
            custom_params.emplace_back(SO::Concatenate(attribute, _T("="), value));
    }

    return custom_params;
}


void PFF::SetPersistentData(CString field_name, CString value)
{
    m_mapPersistent[field_name.MakeUpper()] = value;
}

CString PFF::GetPersistentData(CString field_name) const
{
    const auto& itr = m_mapPersistent.find(field_name.MakeUpper());
    return ( itr != m_mapPersistent.end() ) ?  itr->second : CString();
}














/////////////////////////////////////////////////////////////////////////////////
//
//  PFF FILE LOADING
//
/////////////////////////////////////////////////////////////////////////////////
struct PFF::PffFunctions
{
    const TCHAR* const command;
    const GetterFunction getter_function;
    const SetterFunction setter_function;
    const MultipleGetterFunction multiple_getter_function;
    const MultipleGetterTransformedFunction multiple_getter_transformed_function;
    const ClearFunction clear_function;

    PffFunctions(const TCHAR* const _command,
        const GetterFunction _getter_function,
        const SetterFunction _setter_function,
        const MultipleGetterFunction _multiple_getter_function = nullptr,
        const MultipleGetterTransformedFunction _multiple_getter_transformed_function = nullptr,
        const ClearFunction _clear_function = nullptr)
        :   command(_command),
            getter_function(_getter_function),
            setter_function(_setter_function),
            multiple_getter_function(_multiple_getter_function),
            multiple_getter_transformed_function(_multiple_getter_transformed_function),
            clear_function(_clear_function)
    {
    }
};

namespace
{
    constexpr const TCHAR* FileSections[] =
    {
        DATAENTRYINIT, FILESSTRING, EXTERNALFILES, USERFILES, PARAMETERS, DATAENTRYIDS
    };

    const TCHAR* GetSectionHeader(CString header)
    {
        for( size_t i = 0; i < _countof(FileSections); i++ )
        {
            if( header.CompareNoCase(FileSections[i]) == 0 )
                return FileSections[i];
        }

        return nullptr;
    }
}



const std::vector<PFF::PffFunctions>& PFF::GetPffFunctionsArray()
{
    static const std::vector<PffFunctions> PffFunctions =
    {
        // the header
        { CMD_VERSION,              &PFF::GetVersion,                            &PFF::SetVersion },
        { APPTYPESTRING,            &PFF::GetAppTypeText,                        &PFF::SetAppTypeText },
        { OPERATION,                &PFF::GetTabProcessText,                     &PFF::SetTabProcessText },
        { APPDESCRIPTION,           &PFF::GetAppDescription,                     &PFF::SetAppDescription },

        // data entry init
        { OPID,                     &PFF::GetOpID,                               &PFF::SetOpID },
        { STARTMODE,                &PFF::GetStartModeText,                      &PFF::SetStartModeText },
        { KEY,                      &PFF::GetKey,                                &PFF::SetKey },
        { LOCK,                     &PFF::GetLockFlagText,                       &PFF::SetLockFlagText },
        { CASELISTINGFILTER,        &PFF::GetCaseListingFilter,                  &PFF::SetCaseListingFilter },
        { SHOWINAPPLICATIONLISTING, &PFF::GetShowInApplicationListingText,       &PFF::SetShowInApplicationListingText },
        { FULLSCREEN_STR,           &PFF::GetFullScreenFlagText,                 &PFF::SetFullScreenFlagText },
        { AUTOADD,                  &PFF::GetAutoAddFlagText,                    &PFF::SetAutoAddFlagText },
        { NOFILEOPEN,               &PFF::GetFileOpenFlagText,                   &PFF::SetFileOpenFlagText },
        { INTERACTIVE,              &PFF::GetInteractiveEditModeText,            &PFF::SetInteractiveEditModeText },

        // files
        { APPSTRING,                &PFF::GetAppFName,                           &PFF::SetAppFName },
        { INPUTDATA,                nullptr,                                               &PFF::AddInputDataConnectionString,
                                    nullptr,                                               &PFF::GetInputDataConnectionStringsSerializableText,
                                    &PFF::ClearInputDataConnectionStrings },
        { EXCEL,                    &PFF::GetExcelFilename,                      &PFF::SetExcelFilename },
        { EXPTOUTPUT,               nullptr,                                               &PFF::AddExportFilenames,
                                    &PFF::GetExportFilenamesSerializable,        nullptr,
                                    &PFF::ClearExportFilenames },
        { OUTPUTDATA,               nullptr,                                               &PFF::AddOutputDataConnectionString,
                                    nullptr,                                               &PFF::GetOutputDataConnectionStringsSerializableText,
                                    &PFF::ClearOutputDataConnectionStrings },
        { INPUTDICT,                &PFF::GetInputDictFName,                     &PFF::SetInputDictFName },
        { OUTPUTDICT,               &PFF::GetOutputDictFName,                    &PFF::SetOutputDictFName },
        { REFERENCEDATA,            &PFF::GetReferenceDataConnectionStringText,  &PFF::SetReferenceDataConnectionString },
        { TABOUTPUT,                &PFF::GetTabOutputFName,                     &PFF::SetTabOutputFName },
        { CONINPUT,                 nullptr,                                               &PFF::AddConInputFilenames,
                                    &PFF::GetConInputFilenamesSerializable,      nullptr,
                                    &PFF::ClearConInputFilenames },

        { CONOUTPUT,                &PFF::GetConOutputFName,                     &PFF::SetConOutputFName },
        { PREPINPUT,                &PFF::GetPrepInputFName,                     &PFF::SetPrepInputFName },
        { PREPOUTPUT,               &PFF::GetPrepOutputFName,                    &PFF::SetPrepOutputFName },
        { AREANAMES,                &PFF::GetAreaFName,                          &PFF::SetAreaFName },
        { SPSSSYNTAXFNAME,          &PFF::GetSPSSSyntaxFName,                    &PFF::SetSPSSSyntaxFName },
        { SASSYNTAXFNAME,           &PFF::GetSASSyntaxFName,                     &PFF::SetSASSyntaxFName },
        { STATASYNTAXFNAME,         &PFF::GetSTATASyntaxFName,                   &PFF::SetSTATASyntaxFName },
        { STATADOFNAME,             &PFF::GetSTATADOFName,                       &PFF::SetSTATADOFName },
        { CSPROSYNTAXFNAME,         &PFF::GetCSPROSyntaxFName,                   &PFF::SetCSPROSyntaxFName },
        { RSYNTAXFNAME,             &PFF::GetRSyntaxFName,                       &PFF::SetRSyntaxFName },
        { PACKOUTPUT,               &PFF::GetPackOutputFName,                    &PFF::SetPackOutputFName },
        { EXTRAFILE,                nullptr,                                               &PFF::AddPackExtraFiles,
                                    &PFF::GetPackExtraFilesSerializable,         nullptr,
                                    &PFF::ClearPackExtraFiles },
        { INPUTPARADATA,            nullptr,                                               &PFF::AddInputParadataFilenames,
                                    &PFF::GetInputParadataFilenamesSerializable, nullptr,
                                    &PFF::ClearInputParadataFilenames },
        { OUTPUTPARADATA,           &PFF::GetOutputParadataFilename,             &PFF::SetOutputParadataFilename },
        { PARADATA,                 &PFF::GetParadataFilename,                   &PFF::SetParadataFilename },
        { LISTING,                  &PFF::GetListingFName,                       &PFF::SetListingFName },
        { FREQS,                    &PFF::GetFrequenciesFilename,                &PFF::SetFrequenciesFilename },
        { IMPUTEFREQS,              &PFF::GetImputeFrequenciesFilename,          &PFF::SetImputeFrequenciesFilename },
        { IMPUTESTAT,               &PFF::GetImputeStatConnectionStringText,     &PFF::SetImputeStatConnectionString },
        { WRITEDATA,                &PFF::GetWriteFName,                         &PFF::SetWriteFName },
        { SAVEARRAY,                &PFF::GetSaveArrayFilename,                  &PFF::SetSaveArrayFilename },
        { COMMONSTORE,              &PFF::GetCommonStoreFName,                   &PFF::SetCommonStoreFName },
        { HTMLDIALOGS,              &PFF::GetHtmlDialogsDirectory,               &PFF::SetHtmlDialogsDirectory },
        { BASEMAP,                  &PFF::GetBaseMapSelectionText,               &PFF::SetBaseMapSelection },

        // parameters
        { LANGUAGE,                 &PFF::GetStartLanguageString,                &PFF::SetStartLanguageString },
        { PARAMSTRING,              &PFF::GetParamString,                        &PFF::SetParamString },
        { VIEWLISTING_STR,          &PFF::GetViewListingText,                    &PFF::SetViewListingText },
        { VIEWRESULTS,              &PFF::GetViewResultsFlagText,                &PFF::SetViewResultsFlagText },
        { PACK_INCLUDES_STR,        &PFF::GetPackIncludeText,                    &PFF::SetPackIncludeText },
        { DUPLICATE_CASE,           &PFF::GetDuplicateCaseText,                  &PFF::SetDuplicateCaseText },
        { SKIPSTRUCT,               &PFF::GetSkipStructFlagText,                 &PFF::SetSkipStructFlagText },
        { CHKRANGES,                &PFF::GetChkRangesFlagText,                  &PFF::SetChkRangesFlagText },
        { LISTINGWIDTH,             &PFF::GetListingWidthText,                   &PFF::SetListingWidthText },
        { MESSAGEWRAP,              &PFF::GetMessageWrapText,                    &PFF::SetMessageWrapText },
        { ERRMSGOVERRIDE,           &PFF::GetErrMsgOverrideText,                 &PFF::SetErrMsgOverrideText },
        { INPUT_ORDER,              &PFF::GetInputOrderText,                     &PFF::SetInputOrderText },
        { DISPLAYNAMES,             &PFF::GetDisplayNamesText,                   &PFF::SetDisplayNamesText },
        { CONCAT_METHOD,            &PFF::GetConcatenateMethodText,              &PFF::SetConcatenateMethodText },
        { SYNC_SERVER_TYPE,         &PFF::GetSyncServerTypeText,                 &PFF::SetSyncServerTypeText },
        { SYNC_DIRECTION,           &PFF::GetSyncDirectionText,                  &PFF::SetSyncDirectionText },
        { SYNC_URL,                 &PFF::GetSyncUrl,                            &PFF::SetSyncUrl },
        { DEPLOY_TO_OVERRIDE,       &PFF::GetDeployToOverrideText,               &PFF::SetDeployToOverrideText },
        { SILENT,                   &PFF::GetSilentText,                         &PFF::SetSilentText },
        { ONEXIT,                   &PFF::GetOnExitFilename,                     &PFF::SetOnExitFilename },
    };

    return PffFunctions;
}

const PFF::PffFunctions* PFF::GetPffFunctions(const TCHAR* command) const
{
    for( const PffFunctions& pff_function : GetPffFunctionsArray() )
    {
        if( SO::EqualsNoCase(command, pff_function.command) )
            return &pff_function;
    }

    return nullptr;
}


bool PFF::LoadPifFile(bool silently_load_the_spec_file/* = false*/)
{
    // open the specfile
    CSpecFile nPifFile;
    nPifFile.SetSilent(silently_load_the_spec_file);

    // try to open the pifFile
    if( !nPifFile.Open(m_sPifFileName, CFile::modeRead) )
        return false;

    CString command; // left side of =
    CString argument; // right side of =

    const TCHAR* last_section_header = nullptr;
    bool read_run_info = false;

    while( nPifFile.GetLine(command, argument, false) == SF_OK )
    {
        command.Trim();

        if( command.IsEmpty() )
            continue;

        // turn ␤ -> \n
        NewlineSubstitutor::MakeUnicodeNLToNewline(command);
        NewlineSubstitutor::MakeUnicodeNLToNewline(argument);

        if( read_run_info )
        {
            // changing sections
            if( command[0] == _T('[') )
            {
                last_section_header = GetSectionHeader(command);

                if( last_section_header == nullptr )
                {
#ifdef WIN_DESKTOP
                    CString message;
                    message.Format(_T("Invalid section heading at line %d: %s"), nPifFile.GetLineNumber(), command.GetString());
                    AfxMessageBox(message);
#endif
                    nPifFile.SkipSection();
                }
            }

            // a command / argument pair
            else
            {
                SetCommandArgumentPair(last_section_header, command, argument, false);
            }
        }

        else
        {
            read_run_info |= ( command.CompareNoCase(RUNINFO) == 0 );
        }
    }

    nPifFile.Close();

    AdjustAttributesFromOldFiles();

    // If key is specified, ignore the start mode. This is probably a confused user who didn't read the docs well.
    if (!GetKey().IsEmpty()) {
        m_eStartMode = StartMode::None;
        m_sStartKey.Empty();
    }

    return true;
}


void PFF::SetCommandArgumentPair(const TCHAR* last_section_header, CString command, CString argument, bool clear_existing_multiple_filenames)
{
    // trim the argument (for nearly all commands)
    if( !SO::EqualsOneOfNoCase(command, APPDESCRIPTION,
                                        KEY) )
    {
        argument.Trim();
    }

    // ----------------------------------------------
    // process commands that have generalized functions
    const PffFunctions* pff_function = GetPffFunctions(command);

    if( pff_function != nullptr )
    {
        if( clear_existing_multiple_filenames && pff_function->clear_function != nullptr )
            (this->*pff_function->clear_function)();

        (this->*pff_function->setter_function)(argument);

        return;
    }

    // ----------------------------------------------
    // external (data) files
    if( last_section_header == EXTERNALFILES )
    {
        SetExternalDataConnectionString(command, argument);
        return;
    }

    // ----------------------------------------------
    //  user files
    if( last_section_header == USERFILES )
    {
        SetUsrDatAssoc(command, argument);
        return;
    }

    // ----------------------------------------------
    // parameters
    if( last_section_header == PARAMETERS )
    {
        SetCustomParamString(CS2WS(command), CS2WS(argument));
        return;
    }

    // ----------------------------------------------
    // data entry IDs
    if( last_section_header == DATAENTRYIDS )
    {
        SetPersistentData(command, argument);
        return;
    }
}


bool PFF::Save(bool silently_save_the_spec_file/* = false*/) const
{
    ASSERT(!m_sPifFileName.IsEmpty() && !PathIsRelative(m_sPifFileName));

    CSpecFile nPifFile;
    nPifFile.SetSilent(silently_save_the_spec_file);

    if( !nPifFile.Open(m_sPifFileName, CFile::modeWrite) )
        return false;

    const TCHAR* last_section_header = nullptr;

    auto write_argument_to_file = [&](const TCHAR* section_header, std::wstring command, std::wstring argument)
    {
        if( section_header != last_section_header )
        {
            // separate sections with a blank line
            if( last_section_header != nullptr )
                nPifFile.PutLine(_T(""));

            nPifFile.PutLine(section_header);
            last_section_header = section_header;
        }

        // turn \n -> ␤
        NewlineSubstitutor::MakeNewlineToUnicodeNL(command);
        NewlineSubstitutor::MakeNewlineToUnicodeNL(argument);

        nPifFile.PutLine(command, argument);
    };

    auto write_arguments_to_file = [&](const TCHAR* section_header, NullTerminatedString command, const std::vector<std::wstring>& arguments)
    {
        for( const std::wstring& argument : arguments )
            write_argument_to_file(section_header, command, argument);
    };

    auto write_argument = [&](const TCHAR* section_header, NullTerminatedString command, bool write_condition = true)
    {
        if( write_condition )
            write_arguments_to_file(section_header, command, GetProperties(command));
    };

    auto write_non_blank_string_argument = [&](const TCHAR* section_header, NullTerminatedString command, bool write_condition = true)
    {
        if( write_condition )
        {
            const std::vector<std::wstring> arguments = GetProperties(command);
            ASSERT(arguments.size() == 1);

            if( !arguments.front().empty() )
                write_arguments_to_file(section_header, command, arguments);
        }
    };

    auto write_filename = [&](const TCHAR* section_header, NullTerminatedString command, const CString& filename)
    {
        write_argument_to_file(section_header, command, CS2WS(GetFilename(filename, false)));
    };

    auto write_non_blank_filename_argument = [&](const TCHAR* section_header, NullTerminatedString command)
    {
        const std::vector<std::wstring> arguments = GetProperties(command);
        ASSERT(arguments.size() == 1);

        if( !arguments.front().empty() )
            write_filename(section_header, command, WS2CS(arguments.front()));
    };

    auto write_filenames_argument = [&](const TCHAR* section_header, NullTerminatedString command)
    {
        for( const std::wstring& filename : GetProperties(command) )
            write_filename(section_header, command, WS2CS(filename));
    };

    auto write_defined_connection_string = [&](const TCHAR* section_header, NullTerminatedString command, const ConnectionString& connection_string)
    {
        if( connection_string.IsDefined() )
            write_argument_to_file(section_header, command, CS2WS(GetConnectionStringText(connection_string, false)));
    };

    auto write_defined_connection_strings = [&](const TCHAR* section_header, NullTerminatedString command, const auto& multiple_connection_strings)
    {
        for( const ConnectionString& connection_string : multiple_connection_strings.serializable_filenames )
            write_defined_connection_string(section_header, command, connection_string);
    };


    // the header
    write_argument_to_file(RUNINFO, CMD_VERSION, CSPRO_VERSION);
    write_argument(RUNINFO, APPTYPESTRING);

    if( m_eAppType == APPTYPE::TAB_TYPE )
        write_argument(RUNINFO, OPERATION);

    write_non_blank_string_argument(RUNINFO, APPDESCRIPTION);
    write_argument(RUNINFO, SHOWINAPPLICATIONLISTING, m_eShowInApplicationListing != ShowInApplicationListing::Always);


    // data entry init section
    if( m_eAppType == APPTYPE::ENTRY_TYPE )
    {
        write_non_blank_string_argument(DATAENTRYINIT, OPID);
        write_non_blank_string_argument(DATAENTRYINIT, STARTMODE);
        write_non_blank_string_argument(DATAENTRYINIT, KEY);
        write_non_blank_string_argument(DATAENTRYINIT, CASELISTINGFILTER);
        write_non_blank_string_argument(DATAENTRYINIT, LOCK);
        write_argument(DATAENTRYINIT, FULLSCREEN_STR, m_eFullScreen != FULLSCREEN::FULLSCREEN_NO);
        write_argument(DATAENTRYINIT, AUTOADD, !m_bAutoAdd);
        write_argument(DATAENTRYINIT, NOFILEOPEN, m_bDisableFileOpen);
        write_argument(DATAENTRYINIT, INTERACTIVE, m_eInteractiveEditMode != InteractiveEditMode::Ask || m_bInteractiveEditDialogLocked);
    }

    // the files section
    write_non_blank_filename_argument(FILESSTRING, APPSTRING);

    write_defined_connection_strings(FILESSTRING, INPUTDATA, m_inputDataConnectionStrings);

    if( m_eAppType == APPTYPE::EXCEL2CSPRO_TYPE )
        write_non_blank_filename_argument(FILESSTRING, EXCEL);

    if( m_eAppType == APPTYPE::EXPORT_TYPE || m_eAppType == APPTYPE::BATCH_TYPE )
        write_filenames_argument(FILESSTRING, EXPTOUTPUT);

    write_defined_connection_strings(FILESSTRING, OUTPUTDATA, m_outputDataConnectionStrings);
    write_non_blank_filename_argument(FILESSTRING, INPUTDICT);
    write_non_blank_filename_argument(FILESSTRING, OUTPUTDICT);
    write_defined_connection_string(FILESSTRING, REFERENCEDATA, m_referenceDataConnectionString);

    if( m_eTabProcess == PROCESS::CS_TAB )
        write_non_blank_filename_argument(FILESSTRING, TABOUTPUT);

    if( m_eTabProcess == PROCESS::CS_CON )
    {
        write_filenames_argument(FILESSTRING, CONINPUT);
        write_non_blank_filename_argument(FILESSTRING, CONOUTPUT);
    }

    if( m_eTabProcess == PROCESS::CS_PREP )
        write_non_blank_filename_argument(FILESSTRING, PREPINPUT);

    if( m_eTabProcess == PROCESS::ALL_STUFF || m_eTabProcess == PROCESS::CS_PREP )
        write_non_blank_filename_argument(FILESSTRING, PREPOUTPUT);

    write_non_blank_filename_argument(FILESSTRING, AREANAMES);
    write_non_blank_filename_argument(FILESSTRING, SPSSSYNTAXFNAME);
    write_non_blank_filename_argument(FILESSTRING, SASSYNTAXFNAME);
    write_non_blank_filename_argument(FILESSTRING, STATASYNTAXFNAME);
    write_non_blank_filename_argument(FILESSTRING, STATADOFNAME);
    write_non_blank_filename_argument(FILESSTRING, CSPROSYNTAXFNAME);
    write_non_blank_filename_argument(FILESSTRING, RSYNTAXFNAME);
    write_non_blank_filename_argument(FILESSTRING, PACKOUTPUT);
    write_filenames_argument(FILESSTRING, EXTRAFILE);

    if( m_eAppType == APPTYPE::PARADATA_CONCAT_TYPE )
    {
        write_filenames_argument(FILESSTRING, INPUTPARADATA);
        write_non_blank_filename_argument(FILESSTRING, OUTPUTPARADATA);
    }

    write_non_blank_filename_argument(FILESSTRING, PARADATA);
    write_non_blank_filename_argument(FILESSTRING, LISTING);
    write_non_blank_filename_argument(FILESSTRING, FREQS);
    write_non_blank_filename_argument(FILESSTRING, IMPUTEFREQS);
    write_defined_connection_string(FILESSTRING, IMPUTESTAT, m_imputeStatConnectionString);
    write_non_blank_filename_argument(FILESSTRING, WRITEDATA);
    write_non_blank_filename_argument(FILESSTRING, SAVEARRAY);
    write_non_blank_filename_argument(FILESSTRING, COMMONSTORE);
    write_non_blank_filename_argument(FILESSTRING, HTMLDIALOGS);

    if( m_baseMapSelection.has_value() )
        write_argument_to_file(FILESSTRING, BASEMAP, ToString(*m_baseMapSelection, m_sPifFileName));


    // external (data) files
    for( const auto& [dictionary_name, connection_string] : m_mapExternalDataConnectionStrings )
        write_argument_to_file(EXTERNALFILES, CS2WS(dictionary_name), CS2WS(GetConnectionStringText(connection_string, false)));


    // user files
    for( const auto& [file_handler_name, original_case_file_handler_name_and_filename] : m_mapUserFiles )
        write_filename(USERFILES, std::get<0>(original_case_file_handler_name_and_filename), std::get<1>(original_case_file_handler_name_and_filename));


    // the parameters section
    write_non_blank_string_argument(PARAMETERS, LANGUAGE);
    write_non_blank_string_argument(PARAMETERS, PARAMSTRING);

    if( m_eAppType != APPTYPE::DEPLOY_TYPE && m_eAppType != APPTYPE::ENTRY_TYPE && m_eAppType != APPTYPE::EXCEL2CSPRO_TYPE )
    {
        write_argument(PARAMETERS, VIEWLISTING_STR);

        if( m_eAppType != APPTYPE::COMPARE_TYPE && m_eAppType != APPTYPE::INDEX_TYPE &&
            m_eAppType != APPTYPE::PACK_TYPE    && m_eAppType != APPTYPE::PARADATA_CONCAT_TYPE )
        {
            write_argument(PARAMETERS, VIEWRESULTS);
        }
    }

    if( m_eAppType == APPTYPE::PACK_TYPE )
        write_non_blank_string_argument(PARAMETERS, PACK_INCLUDES_STR);

    if( m_eAppType == APPTYPE::INDEX_TYPE )
        write_argument(PARAMETERS, DUPLICATE_CASE);

    if( m_eAppType == APPTYPE::BATCH_TYPE )
    {
        write_argument(PARAMETERS, SKIPSTRUCT, m_bSkipStructure);
        write_argument(PARAMETERS, CHKRANGES, m_bChkRanges);
        write_argument(PARAMETERS, LISTINGWIDTH);
        write_argument(PARAMETERS, MESSAGEWRAP);
        write_argument(PARAMETERS, ERRMSGOVERRIDE);
    }

    if( m_eAppType == APPTYPE::BATCH_TYPE || m_eAppType == APPTYPE::TAB_TYPE || m_eAppType == APPTYPE::FREQ_TYPE || m_eAppType == APPTYPE::EXPORT_TYPE )
        write_argument(PARAMETERS, INPUT_ORDER);

    if( m_eAppType == APPTYPE::REFORMAT_TYPE )
        write_argument(PARAMETERS, DISPLAYNAMES, m_bDisplayNames);

    if( m_eAppType == APPTYPE::CONCAT_TYPE )
        write_argument(PARAMETERS, CONCAT_METHOD);

    if( m_eAppType == APPTYPE::SYNC_TYPE )
    {
        write_argument(PARAMETERS, SYNC_SERVER_TYPE);
        write_argument(PARAMETERS, SYNC_DIRECTION);
    }

    if( m_eAppType == APPTYPE::SYNC_TYPE || m_eAppType == APPTYPE::DEPLOY_TYPE )
        write_non_blank_string_argument(PARAMETERS, SYNC_URL);

    if( m_eAppType == APPTYPE::SYNC_TYPE || ( m_bSilent && m_eAppType == APPTYPE::PACK_TYPE ) )
        write_argument(PARAMETERS, SILENT);

    if( m_eAppType == APPTYPE::DEPLOY_TYPE && m_eDeployToOverride != DeployToOverride::None )
        write_argument(PARAMETERS, DEPLOY_TO_OVERRIDE);

    write_non_blank_filename_argument(PARAMETERS, ONEXIT);

    for( const auto& [argument, values] : m_customParameters )
        write_arguments_to_file(PARAMETERS, argument, values);


    // the data entry IDs section
    for( auto itr = m_mapPersistent.cbegin(); itr != m_mapPersistent.cend(); ++itr )
        write_argument(DATAENTRYIDS, itr->first, itr->second);


    nPifFile.Close();

    return true;
}


const TCHAR* PFF::GetSectionHeaderForProperties(CString* command) const
{
    command->Trim();

    // when called from logic, first check if the section is specified
    const TCHAR* last_section_header = nullptr;
    int dot_pos = command->Find(_T('.'));

    if( dot_pos > 0 )
    {
        CString section_name;
        section_name.Format(_T("[%s]"), command->Left(dot_pos).GetString());
        last_section_header = GetSectionHeader(section_name);

        if( last_section_header != nullptr )
            *command = command->Mid(dot_pos + 1);
    }

    // otherwise check if the command is specifying something in the external (data) files,
    // user files, or data entry IDs maps
    else if( GetExternalDataConnectionString(*command).IsDefined() )
        last_section_header = EXTERNALFILES;

    else if( !LookUpUsrDatFile(*command).IsEmpty() )
        last_section_header = USERFILES;

    else if( !GetPersistentData(*command).IsEmpty() )
        last_section_header = DATAENTRYIDS;

    // default to parameters for any command that isn't recognized
    else
        last_section_header = PARAMETERS;

    return last_section_header;
}


void PFF::SetProperties(const std::wstring& command, const std::vector<std::wstring>& arguments)
{
    CString cs_command = WS2CS(command);
    const TCHAR* last_section_header = GetSectionHeaderForProperties(&cs_command);

    for( size_t i = 0; i < arguments.size(); ++i )
    {
        bool clear_existing_multiple_filenames = ( i == 0 );
        SetCommandArgumentPair(last_section_header, cs_command, WS2CS(arguments[i]), clear_existing_multiple_filenames);
    }
}


std::vector<std::wstring> PFF::GetProperties(CString command) const
{
    // strip any section specification, as it won't be used when getting properties
    GetSectionHeaderForProperties(&command);

    const PffFunctions* pff_function = GetPffFunctions(command);

    if( pff_function != nullptr )
    {
        if( pff_function->getter_function != nullptr )
        {
            return { CS2WS((this->*pff_function->getter_function)()) };
        }

        else if( pff_function->multiple_getter_function != nullptr )
        {
            return CS2WS_Vector((this->*pff_function->multiple_getter_function)());
        }

        else if( pff_function->multiple_getter_transformed_function != nullptr )
        {
            return CS2WS_Vector((this->*pff_function->multiple_getter_transformed_function)());
        }

        else
        {
            ASSERT(false);
        }
    }

    std::wstring argument;

    if( GetExternalDataConnectionString(command).IsDefined() )
    {
        argument = GetExternalDataConnectionString(command).ToString();
    }

    else if( ( argument = CS2WS(LookUpUsrDatFile(command)) ).empty() )
    {
        if( ( argument = CS2WS(GetPersistentData(command)) ).empty() )
        {
            argument = GetCustomParamString(CS2WS(command));
        }
    }

    return { std::move(argument) };
}


void PFF::AdjustAttributesFromOldFiles()
{
    // if PFF attributes change, old PFFs can be upgraded to the new attributes here
    double version = GetCSProVersionNumeric(m_sVersion);

    if( version < 7.3 )
    {
        if( m_eAppType == APPTYPE::EXCEL2CSPRO_TYPE )
        {
            // InputData -> Excel
            if( !GetInputDataConnectionStrings().empty() )
            {
                SetExcelFilename(GetSingleInputDataConnectionString().GetFilename().c_str());
                ClearInputDataConnectionStrings();
            }
        }

        else if( m_eAppType == APPTYPE::PARADATA_CONCAT_TYPE )
        {
            // InputData -> InputParadata
            for( const ConnectionString& connection_string : GetInputDataConnectionStringsSerializable() )
                AddInputParadataFilenames(WS2CS(connection_string.GetFilename()));

            ClearInputDataConnectionStrings();

            // OutputData -> OutputParadata
            if( GetSingleOutputDataConnectionString().IsDefined() )
            {
                SetOutputParadataFilename(WS2CS(GetSingleOutputDataConnectionString().GetFilename()));
                ClearOutputDataConnectionStrings();
            }
        }
    }

    if( version < 7.4 )
    {
        if( m_eAppType == APPTYPE::INDEX_TYPE )
        {
            // DeletePrompt -> DuplicateCase
            std::wstring delete_prompt_value = GetCustomParamString(_T("DeletePrompt"));

            if( !delete_prompt_value.empty() )
            {
                DuplicateCase duplicate_case = DuplicateCase::List;

                if( GetSingleOutputDataConnectionString().IsDefined() )
                {
                    duplicate_case = SO::EqualsNoCase(delete_prompt_value, CSPRO_ARG_YES) ? DuplicateCase::Prompt :
                                                                                            DuplicateCase::KeepFirst;
                }

                SetDuplicateCase(duplicate_case);
            }
        }
    }
}


std::optional<std::wstring> PFF::GetExecutableProgram() const // 20111012 for execpff and runpff
{
#ifdef WIN_DESKTOP
    CSProExecutables::Program program;

    switch( GetAppType() )
    {
        case APPTYPE::ENTRY_TYPE:
            program = CSProExecutables::Program::CSEntry;
            break;
        case APPTYPE::BATCH_TYPE:
            program = CSProExecutables::Program::CSBatch;
            break;
        case APPTYPE::TAB_TYPE:
            program = CSProExecutables::Program::CSTab;
            break;
        case APPTYPE::EXPORT_TYPE:
            program = CSProExecutables::Program::CSExport;
            break;
        case APPTYPE::FREQ_TYPE:
            program = CSProExecutables::Program::CSFreq;
            break;
        case APPTYPE::SORT_TYPE:
            program = CSProExecutables::Program::CSSort;
            break;
        case APPTYPE::REFORMAT_TYPE:
            program = CSProExecutables::Program::CSReFmt;
            break;
        case APPTYPE::COMPARE_TYPE:
            program = CSProExecutables::Program::CSDiff;
            break;
        case APPTYPE::CONCAT_TYPE:
            program = CSProExecutables::Program::CSConcat;
            break;
        case APPTYPE::PACK_TYPE:
            program = CSProExecutables::Program::CSPack;
            break;
        case APPTYPE::INDEX_TYPE:
            program = CSProExecutables::Program::CSIndex;
            break;
        case APPTYPE::SYNC_TYPE:
            program = CSProExecutables::Program::DataViewer;
            break;
        case APPTYPE::PARADATA_CONCAT_TYPE:
            program = CSProExecutables::Program::ParadataConcat;
            break;
        case APPTYPE::EXCEL2CSPRO_TYPE:
            program = CSProExecutables::Program::Excel2CSPro;
            break;
        case APPTYPE::DEPLOY_TYPE:
            program = CSProExecutables::Program::CSDeploy;
            break;
        case APPTYPE::VIEW_TYPE:
            program = CSProExecutables::Program::CSView;
            break;
        default:
            return ReturnProgrammingError(std::nullopt);
    }

    return CSProExecutables::GetExecutablePath(program);

#else
    return std::nullopt;

#endif
}


bool PFF::EntryConnectionStringsContainWildcards() const
{
    ASSERT(m_eAppType == ENTRY_TYPE);

    // process the input data
    for( const ConnectionString& connection_string : m_inputDataConnectionStrings.serializable_filenames )
    {
        if( connection_string.IsFilenamePresent() && PathHasWildcardCharacters(connection_string.GetFilename()) )
            return true;
    }

    // process the external (data) files map
    for( auto itr = m_mapExternalDataConnectionStrings.cbegin(); itr != m_mapExternalDataConnectionStrings.cend(); ++itr )
    {
        if( itr->second.IsFilenamePresent() && PathHasWildcardCharacters(itr->second.GetFilename()) )
            return true;
    }

    return false;
}


bool PFF::UsingOutputData() const
{
    for( const ConnectionString& connection_string : m_outputDataConnectionStrings.evaluated_filenames )
    {
        if( connection_string.IsDefined() && connection_string.GetType() != DataRepositoryType::Null )
            return true;
    }

    return false;
}


#ifdef WIN_DESKTOP

void PFF::ExecutePff(const std::wstring& pff_filename, const std::optional<NullTerminatedString> extra_arguments/* = std::nullopt*/)
{
    PFF pff(WS2CS(pff_filename));

    if( !pff.LoadPifFile() )
        return;

    std::optional<std::wstring> exe_name = pff.GetExecutableProgram();

    if( !exe_name.has_value() )
    {
        AfxMessageBox(_T("The PFF file is invalid or there is no program that can run it."));
        return;
    }

    // 20120614 calling this using relative paths didn't work, so we'll convert them to absolute paths
    std::wstring path(MAX_PATH, '\0');
    GetCurrentDirectory(_MAX_PATH, path.data());
    path.resize(_tcslen(path.data()));

    std::wstring argument = DOUBLEQUOTE + MakeFullPath(path, pff_filename) + DOUBLEQUOTE;

    if( extra_arguments.has_value() )
        SO::Append(argument, _T(" "), *extra_arguments);

    ShellExecute(NULL, NULL, exe_name->c_str(), argument.c_str(), NULL, SW_SHOW);
}


void PFF::ExecuteOnExitPff() const
{
    if( !m_csOnExitFilename.IsEmpty() )
        ExecutePff(CS2WS(GetOnExitFilename()));
}

#endif


namespace
{
    void ViewResultsRunner(CSProExecutables::Program program, NullTerminatedString filename)
    {
        if( !PortableFunctions::FileIsRegular(filename) )
            return;

#ifdef WIN_DESKTOP
        if( program == CSProExecutables::Program::TextView )
        {
            ViewFileInTextViewer(filename);
        }

        else
        {
            CSProExecutables::RunProgramOpeningFile(program, filename);
        }

#else
        if( program == CSProExecutables::Program::TextView )
        {
            try
            {
                // read the file and display it as preformatted text, ignoring file read errors
                constexpr int64_t MaxBytesToRead = 64 * 1024;
                CString file_contents = FileIO::ReadText(filename, MaxBytesToRead, _T("\r\n\r\n... file too large to fully display ..."));

                std::wstring title = PortableFunctions::PathGetFilename(filename);
                std::wstring html = Encoders::ToPreformattedTextHtml(title, file_contents);

                Viewer viewer;
                viewer.UseEmbeddedViewer()
                      .SetTitle(title)
                      .ViewHtmlContent(html, PortableFunctions::PathGetDirectory(filename));
            }
            catch(...) { }
        }
#endif
    }
}


void PFF::ViewResults(NullTerminatedString filename)
{
    std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    if( SO::EqualsOneOfNoCase(extension, FileExtensions::CSV,
                                         FileExtensions::Excel,
                                         FileExtensions::HTML,
                                         FileExtensions::HTM) )
    {
        Viewer().ViewFile(filename);
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::Table) )
    {
        ViewResultsRunner(CSProExecutables::Program::TblView, filename);
    }

    else
    {
        ViewResultsRunner(CSProExecutables::Program::TextView, filename);
    }
}

void PFF::ViewResults(const ConnectionString& connection_string)
{
    if( connection_string.IsDefined() )
    {
        if( DataRepositoryHelpers::IsTypeSQLiteOrDerived(connection_string.GetType()) )
        {
            ViewResultsRunner(CSProExecutables::Program::DataViewer, connection_string.GetFilename());
        }

        else if( connection_string.IsFilenamePresent() )
        {
            ViewResults(connection_string.GetFilename());
        }
    }
}

void PFF::ViewListing(const TCHAR* listing_filename)
{
    ViewResults(listing_filename);
}

void PFF::ViewListing() const
{
    ViewListing(GetListingFName());
}


std::vector<const TCHAR*> PFF::GetAppTypeWords()
{
    std::vector<const TCHAR*> words;

    for( size_t i = 0; AppTypeNames[i] != nullptr; i++ )
        words.emplace_back(AppTypeNames[i]);

    return words;
}

std::vector<const TCHAR*> PFF::GetHeadingWords()
{
    std::vector<const TCHAR*> words = { RUNINFO };

    for( size_t i = 0; i < _countof(FileSections); i++ )
        words.emplace_back(FileSections[i]);

    return words;
}

std::vector<const TCHAR*> PFF::GetAttributeWords()
{
    std::vector<const TCHAR*> words;

    for( const PffFunctions& pff_function : GetPffFunctionsArray() )
        words.emplace_back(pff_function.command);

    return words;
}
