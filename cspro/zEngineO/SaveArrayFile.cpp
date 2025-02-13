//***************************************************************************
//  File name: SaveArrayFile.cpp
//
//  Description:
//  Save array file class. Save CSPro numeric/alpha/string arrays
//  in a spec file. Load them in from same file.
//
//***************************************************************************

#include "stdafx.h"
#include "SaveArrayFile.h"
#include "Array.h"
#include <zJson/JsonSpecFile.h>
#include <zMessageO/SystemMessageIssuer.h>


CREATE_JSON_KEY(arrays)
CREATE_JSON_KEY(cases)
CREATE_JSON_KEY(gets)
CREATE_JSON_KEY(puts)
CREATE_JSON_KEY(runs)
CREATE_JSON_VALUE(savedArrays)


namespace
{
    constexpr const TCHAR* sSectSavedArrays = _T("[Saved Arrays]");
    constexpr const TCHAR* sSectArray       = _T("[Array]");
    constexpr const TCHAR* sSectArrays      = _T("[Arrays]");
    constexpr const TCHAR* sSectSlice       = _T("[Slice]");
    constexpr const TCHAR* sCmdArrayName    = _T("Name");
    constexpr const TCHAR* sCmdDimensions   = _T("Dimension");
    constexpr const TCHAR* sCmdSize         = _T("Size");
    constexpr const TCHAR* sCmdType         = _T("Type");
    constexpr const TCHAR* sCmdRow          = _T("Row");
    constexpr const TCHAR* sTypeNumeric     = _T("numeric");
    constexpr const TCHAR* sTypeAlpha       = _T("alpha");

    constexpr const TCHAR* sCmdStatistics   = _T("Statistics");
    constexpr const TCHAR* sCmdRuns         = _T("Runs");
    constexpr const TCHAR* sCmdCases        = _T("Cases");
    constexpr const TCHAR* sCmdGet          = _T("Get");
    constexpr const TCHAR* sCmdPut          = _T("Put");

    constexpr const TCHAR* sCmdCell         = _T("Cell");


    std::wstring GetFileExceptionMessage(const CSpecFile& spec_file)
    {
#ifdef WIN_DESKTOP
        std::wstring cause(255, '\0');
        spec_file.GetFileException()->GetErrorMessage(cause.data(), cause.length());
        cause.resize(_tcslen(cause.data()));
        return cause;
#else
        return _T("Save Array File I/O");
#endif
    }


    class SaveArrayFileNullErrorReporter : public SystemMessageIssuer
    {
    protected:
        void OnIssue(MessageType /*message_type*/, int /*message_number*/, const std::wstring& /*message_text*/) override { }
    };
};


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::SaveArrayFile
// Constructor
/////////////////////////////////////////////////////////////////////////////////
SaveArrayFile::SaveArrayFile()
    :   m_specFile(TRUE) // silent so spec file doesn't pop up msg boxes on error
{
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::MakeArrayFileName
//
// If the filename is not specified, add ".sva" to end of app file name to get
// saved array filename so you'll get something like foo.bch.sva
/////////////////////////////////////////////////////////////////////////////////
std::wstring SaveArrayFile::MakeArrayFileName(const std::variant<std::wstring, const PFF*>& filename_or_pff)
{
    if( std::holds_alternative<std::wstring>(filename_or_pff) )
    {
        return std::get<std::wstring>(filename_or_pff);
    }

    else
    {
        const PFF* pff = std::get<const PFF*>(filename_or_pff);

        if( !pff->GetSaveArrayFilename().IsEmpty() )
        {
            return CS2WS(pff->GetSaveArrayFilename());
        }

        else
        {
            return PortableFunctions::PathAppendFileExtension(CS2WS(pff->GetAppFName()), FileExtensions::WithDot::SaveArray);
        }
    }
}



/////////////////////////////////////////////////////////////////////////////////
//
// Saved Arrays File JSON serialization
//
/////////////////////////////////////////////////////////////////////////////////

template<typename T>
class SaveArraySparseArrayWriter : LogicArray::SparseArrayWriter<T>
{
public:
    SaveArraySparseArrayWriter(JsonWriter& json_writer, const LogicArray& logic_array, const SaveArray& save_array)
        :   m_jsonWriter(json_writer),
            m_logicArray(logic_array),
            m_saveArray(save_array)
    {
    }

    void Write(const size_t cases_read, const bool increment_runs)
    {
        // write the metadata
        m_jsonWriter.BeginObject()
                    .Write(JK::name, m_logicArray.GetName());

        m_logicArray.WriteJsonMetadata_subclass(m_jsonWriter);

        m_jsonWriter.Write(JK::runs, m_saveArray.GetNumberRuns() + ( increment_runs ? 1 : 0 ))
                    .Write(JK::cases, m_saveArray.GetNumberCases() + cases_read);

        // write the array values, gets, and puts when
        // - the value is not default
        // - gets or puts is not 0
        m_jsonWriter.Key(JK::value);
        m_logicArray.WriteSparseArrayValueToJson(m_jsonWriter, this);

        m_jsonWriter.EndObject();
    }

protected:
    bool ShouldWriteNonDefaultValue(const std::vector<size_t>& indices) override
    {
        return ( m_saveArray.GetNumberGets(indices) != 0 ||
                 m_saveArray.GetNumberPuts(indices) != 0 );
    }

    void WriteValue(const std::vector<size_t>& indices, const T& value) override
    {
        m_jsonWriter.BeginObject()
                    .WriteEngineValue(JK::value, value)
                    .Write(JK::gets, m_saveArray.GetNumberGets(indices))
                    .Write(JK::puts, m_saveArray.GetNumberPuts(indices))
                    .EndObject();
    }

private:
    JsonWriter& m_jsonWriter;
    const LogicArray& m_logicArray;
    const SaveArray& m_saveArray;
};


void SaveArrayFile::WriteArrays(std::variant<std::wstring, const PFF*> filename_or_pff, const std::vector<LogicArray*>& logic_arrays,
                                std::shared_ptr<SystemMessageIssuer> system_message_issuer, const size_t cases_read, const bool increment_runs)
{
    m_systemMessageIssuer = std::move(system_message_issuer);
    ASSERT(m_systemMessageIssuer != nullptr);

    // add ".sva" to end of app file name to get saved array filename so
    // you'll get something like foo.bch.sva
    const std::wstring sva_filename = MakeArrayFileName(filename_or_pff);

    try
    {
        std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(sva_filename, JV::savedArrays);

        json_writer->BeginArray(JK::arrays);

        // write each array
        for( const LogicArray& logic_array : VI_V(logic_arrays) )
        {
            // write the array to the file if it is marked as a save array (compiler
            // sets this flag if the "save" keyword follows the array declaration).
            const SaveArray* save_array = logic_array.GetSaveArray();

            if( save_array != nullptr )
            {
                if( logic_array.IsNumeric() )
                {
                    SaveArraySparseArrayWriter<double> sparse_array_writer(*json_writer, logic_array, *save_array);
                    sparse_array_writer.Write(cases_read, increment_runs);
                }

                else
                {
                    ASSERT(logic_array.IsString());

                    SaveArraySparseArrayWriter<std::wstring> sparse_array_writer(*json_writer, logic_array, *save_array);
                    sparse_array_writer.Write(cases_read, increment_runs);
                }
            }
        }

        json_writer->EndArray();

        json_writer->EndObject();
    }

    catch( const CSProException& exception )
    {
        m_systemMessageIssuer->Issue(MessageType::Error, 19001,
                                     PortableFunctions::PathGetFilename(sva_filename),
                                     exception.GetErrorMessage().c_str());
    }
}


void SaveArrayFile::ReadArrays(std::variant<std::wstring, const PFF*> filename_or_pff, std::vector<LogicArray*>& logic_arrays,
                               std::shared_ptr<SystemMessageIssuer> system_message_issuer, const bool loading_for_save_array_viewer/* = false*/)
{
    m_systemMessageIssuer = std::move(system_message_issuer);

    if( m_systemMessageIssuer == nullptr )
        m_systemMessageIssuer = std::make_shared<SaveArrayFileNullErrorReporter>();

    const std::wstring sva_filename = MakeArrayFileName(filename_or_pff);
    std::set<const LogicArray*> successfully_read_arrays;

    if( !PortableFunctions::FileIsRegular(sva_filename) )
    {
        m_systemMessageIssuer->Issue(MessageType::Warning, 19002,
                                     sva_filename.c_str(),
                                     FileIO::Exception::FileNotFound(sva_filename).GetErrorMessage().c_str());
    }

    else if( JsonSpecFile::IsPre80SpecFile(sva_filename) )
    {
        successfully_read_arrays = ReadArraysPre80SpecFile(sva_filename, logic_arrays, loading_for_save_array_viewer);
    }

    else
    {
        successfully_read_arrays = ReadArrays(sva_filename, logic_arrays, loading_for_save_array_viewer);
    }

    // warn about save arrays in the program that are not in the file
    for( LogicArray* logic_array : logic_arrays )
    {
        SaveArray* save_array = logic_array->GetSaveArray();

        if( save_array != nullptr )
        {
            if( successfully_read_arrays.find(logic_array) == successfully_read_arrays.end() )
                m_systemMessageIssuer->Issue(MessageType::Warning, 19009, logic_array->GetName().c_str());

            // start keeping track of gets and puts
            if( !loading_for_save_array_viewer )
                save_array->StartTrackingAccess();
        }
    }
}


std::set<const LogicArray*> SaveArrayFile::ReadArrays(const std::wstring& sva_filename, std::vector<LogicArray*>& logic_arrays, const bool loading_for_save_array_viewer)
{
    std::set<const LogicArray*> successfully_read_arrays;

    // read the JSON specification file
    std::unique_ptr<JsonSpecFile::Reader> json_reader;

    try
    {
        json_reader = JsonSpecFile::CreateReader(sva_filename);

        json_reader->CheckVersion();
        json_reader->CheckFileType(JV::savedArrays);

        for( const JsonNode<wchar_t>& array_node : json_reader->GetArrayOrEmpty(JK::arrays) )
        {
            try
            {
                const LogicArray* read_array = ReadArray(array_node, logic_arrays, loading_for_save_array_viewer);

                if( read_array != nullptr )
                    successfully_read_arrays.insert(read_array);
            }

            catch( const CSProException& exception )
            {
                m_systemMessageIssuer->Issue(MessageType::Error, 19005, exception.GetErrorMessage().c_str());
            }
        }
    }

    catch( const CSProException& exception )
    {
        m_systemMessageIssuer->Issue(MessageType::Error, 19003,
                                     PortableFunctions::PathGetFilename(sva_filename),
                                     exception.GetErrorMessage().c_str());
    }

    // report any warnings
    if( json_reader != nullptr )
        json_reader->GetMessageLogger().DisplayWarnings();

    return successfully_read_arrays;
}


class SaveArraySparseArrayReader : public LogicArray::SparseArrayReader
{
public:
    SaveArraySparseArrayReader(SaveArray& save_array)
        :   m_saveArray(save_array)
    {
    }

    JsonNode<wchar_t> ProcessNodeAndGetValueNode(const std::vector<size_t>& indices, const JsonNode<wchar_t>& json_node) override
    {
        m_saveArray.SetNumberGets(indices, json_node.GetOrDefault(JK::gets, 0));
        m_saveArray.SetNumberPuts(indices, json_node.GetOrDefault(JK::puts, 0));

        return json_node.Get(JK::value);
    }

private:
    SaveArray& m_saveArray;
};


const LogicArray* SaveArrayFile::ReadArray(const JsonNode<wchar_t>& json_node, std::vector<LogicArray*>& logic_arrays, const bool loading_for_save_array_viewer)
{
    const std::wstring array_name = json_node.Get<wstring_view>(JK::name);

    if( !CIMSAString::IsName(array_name) )
        throw CSProException(_T("The Array name '%s' is invalid."), array_name.c_str());

    // find the save array
    bool creating_missing_array = false;
    LogicArray* logic_array = nullptr;
    std::unique_ptr<LogicArray> created_logic_array;

    auto array_lookup = std::find_if(logic_arrays.begin(), logic_arrays.end(),
                                     [&](const LogicArray* logic_array) { return SO::EqualsNoCase(array_name, logic_array->GetName()); });

    if( array_lookup == logic_arrays.end() )
    {
        if( loading_for_save_array_viewer )
        {
            // add the array (for the Save Array Viewer)
            creating_missing_array = true;
            created_logic_array = std::make_unique<LogicArray>(array_name);
            logic_array = created_logic_array.get();
            logic_array->SetUsingSaveArray();
        }

        else
        {
            // warn that the array in the file is not declared in the program
            m_systemMessageIssuer->Issue(MessageType::Warning, 19011, array_name.c_str());
            return nullptr;
        }
    }

    else
    {
        logic_array = *array_lookup;

        if( logic_array->GetSaveArray() == nullptr )
        {
            // the array is no longer a save array
            m_systemMessageIssuer->Issue(MessageType::Warning, 19013, array_name.c_str());
            return nullptr;
        }
    }


    // read the content type
    const DataType data_type = json_node.Get<DataType>(JK::contentType);

    if( ( !IsNumeric(data_type) && !IsString(data_type) ) ||
        ( !creating_missing_array && logic_array->GetDataType() != data_type ) )
    {
        m_systemMessageIssuer->Issue(MessageType::Warning, 19015, array_name.c_str());
        return nullptr;
    }

    else if( creating_missing_array )
    {
        if( data_type == DataType::Numeric )
        {
            logic_array->SetNumeric(true);
        }

        else
        {
            logic_array->SetNumeric(false);
            logic_array->SetPaddingStringLength(json_node.GetOrDefault(JK::length, 0));
        }
    }


    // read the dimensions, which are serialized as if the 0th dimension does not exist
    std::vector<size_t> dimensions = json_node.GetArray(JK::dimensions).GetVector<size_t>();
    std::for_each(dimensions.begin(), dimensions.end(), [](size_t& dimension) { ++dimension; });

    if( creating_missing_array )
    {
        if( dimensions.empty() )
            throw CSProException(_T("The Array '%s' cannot have 0 dimensions."), array_name.c_str());

        // the Save Array Viewer doesn't support Arrays with more than three dimensions
        if( dimensions.size() > 3 )
        {
            m_systemMessageIssuer->Issue(MessageType::Error, 19025, static_cast<int>(dimensions.size()), array_name.c_str());
            return nullptr;
        }

        logic_array->SetDimensions(std::move(dimensions));
    }

    else
    {
        // error when the number of dimensions don't match
        if( dimensions.size() != logic_array->GetNumberDimensions() )
        {
            m_systemMessageIssuer->Issue(MessageType::Error, 19014, array_name.c_str());
            return nullptr;
        }

        for( size_t i = 0; i < dimensions.size(); ++i )
        {
            // warn when the dimensions don't match
            if( dimensions[i] != logic_array->GetDimension(i) )
            {
                m_systemMessageIssuer->Issue(MessageType::Warning, 19014, array_name.c_str());
                break;
            }
        }
    }


    // read the runs/cases
    SaveArray* save_array = logic_array->GetSaveArray();
    save_array->SetNumberRuns(json_node.GetOrDefault<size_t>(JK::runs, 0));
    save_array->SetNumberCases(json_node.GetOrDefault<size_t>(JK::cases, 0));

    // read the array values, gets, and puts
    SaveArraySparseArrayReader sparse_array_reader(*save_array);
    logic_array->UpdateValueFromJson(json_node.Get(JK::value), &sparse_array_reader);

    // on success, add arrays created for the Save Array Viewer
    if( created_logic_array != nullptr )
        logic_arrays.emplace_back(created_logic_array.release());

    return logic_array;
}



/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::ReadArrays
//
// Read all arrays in file with name sAppFileName.  Copy data from
// those arrays into arrays in logic_arrays with same name.
/////////////////////////////////////////////////////////////////////////////////
std::set<const LogicArray*> SaveArrayFile::ReadArraysPre80SpecFile(const std::wstring& sva_filename, std::vector<LogicArray*>& logic_arrays, const bool loading_for_save_array_viewer)
{
    bool bNoErr = true;

    // open the file for reading
    if (m_specFile.Open(sva_filename.c_str(), CFile::modeRead) != TRUE) {
        // error opening array file, report i/o error using text from windows msg
        m_systemMessageIssuer->Issue(MessageType::Warning, 19002, sva_filename.c_str(), GetFileExceptionMessage(m_specFile).c_str());
        bNoErr = false;
    }

    // read the header section
    ReadVersionNumber read_version_number = ReadVersionNumber::Version80Beta;

    if (bNoErr) {
        std::tie(bNoErr, read_version_number) = ReadHeader();
    }

    std::set<const LogicArray*> successfully_read_arrays;

    // read the individual arrays
    if (bNoErr) {
        while (bNoErr && m_specFile.GetState() == SF_OK) {

            // read header section of array
            LogicArray* logic_array = ReadArrayHeader(logic_arrays, read_version_number, loading_for_save_array_viewer);

            if( logic_array == nullptr ) {
                // try to recover from error, move to next array
                SkipToNextArray();
            }

            else if( successfully_read_arrays.find(logic_array) != successfully_read_arrays.end() ) {
                // already found this array, issue warning and skip
                m_systemMessageIssuer->Issue(MessageType::Warning, 19024, m_specFile.GetLineNumber(), logic_array->GetName().c_str());
                SkipToNextArray();
            }

            // read values into the array
            else if( !ReadArrayData(*logic_array, read_version_number) ) {
                SkipToNextArray();
            }

            else {
                // the array was read successfully
                successfully_read_arrays.insert(logic_array);
            }
        }

        if (m_specFile.GetState() == SF_ABORT) {
            // hit a file i/o error somewhere along the way, report it
            // using windows generated text
            m_systemMessageIssuer->Issue(MessageType::Error, 19003, sva_filename.c_str(), GetFileExceptionMessage(m_specFile).c_str());
        }
    }

    // only close the file if it was actually opened without error
    if (m_specFile.m_pStream != NULL) {
        m_specFile.Close();
    }

    return successfully_read_arrays;
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::ReadHeader
//
// Read file header for saved array file.
/////////////////////////////////////////////////////////////////////////////////
std::tuple<bool, SaveArrayFile::ReadVersionNumber> SaveArrayFile::ReadHeader()
{
    std::tuple<bool, ReadVersionNumber> result(false, ReadVersionNumber::Version80Beta);

    if (!m_specFile.IsHeaderOK(sSectSavedArrays)) {
        m_systemMessageIssuer->Issue(MessageType::Error, 19007, m_specFile.GetFilePath().GetString(), m_specFile.GetLineNumber());
        return result;
    }

    double version_number = 0;

    if (!m_specFile.IsVersionOK(CSPRO_VERSION, &version_number)) {
        m_systemMessageIssuer->Issue(MessageType::Error, 19008, m_specFile.GetFilePath().GetString(), m_specFile.GetLineNumber());
        return result;
    }

    std::get<1>(result) = ( version_number < 7.2 ) ? ReadVersionNumber::VersionPre72 :
                          ( version_number < 8.0 ) ? ReadVersionNumber::Version72_77 :
                                                     ReadVersionNumber::Version80Beta;

    CString sCmd, sArg;
    while (m_specFile.GetLine(sCmd, sArg) == SF_OK) {

        if( sCmd.CompareNoCase(sCmdStatistics) == 0 ) {
            // ignore the Statistics line in pre-7.2 files
        }

        if (sCmd.CompareNoCase(sSectArray) == 0) {
            m_specFile.UngetLine();

            // past header, onto arrays section
            std::get<0>(result) = true;
            return result;
        }
    }

    // failed to find [Arrays] section
    m_systemMessageIssuer->Issue(MessageType::Error, 19010, m_specFile.GetFilePath().GetString());
    return result;
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::ReadArrayHeader
//
// Read header info of array from saved array file.
// Header info is name, dimensions, type...
// See WriteArray for info on file format.
/////////////////////////////////////////////////////////////////////////////////
LogicArray* SaveArrayFile::ReadArrayHeader(std::vector<LogicArray*>& logic_arrays, ReadVersionNumber read_version_number, const bool loading_for_save_array_viewer)
{
    CString sCmd;
    CIMSAString sArg;
    if (m_specFile.GetLine(sCmd, sArg) != SF_OK) {
        return nullptr; // EOF or io error that will get reported by caller
    }

    if (sCmd.CompareNoCase(sSectArray) != 0) {
        // error, expected [Array]
        m_systemMessageIssuer->Issue(MessageType::Error, 19006, m_specFile.GetFilePath().GetString(), m_specFile.GetLineNumber());
        return nullptr;
    }

    bool creating_missing_array = false;
    LogicArray* logic_array = nullptr;

    while (m_specFile.GetLine(sCmd, sArg) == SF_OK) {

        if (sCmd.CompareNoCase(sCmdArrayName) == 0) {
            if (SO::IsBlank(sArg)) {
                // invalid array name
                m_systemMessageIssuer->Issue(MessageType::Error, 19012, m_specFile.GetLineNumber());
                return nullptr;
            }

            // find the save array
            auto array_search = std::find_if(logic_arrays.begin(), logic_arrays.end(), [sArg](const auto& logic_array)
            {
                return SO::EqualsNoCase(sArg, logic_array->GetName());
            });

            if( array_search == logic_arrays.end() )
            {
                if( loading_for_save_array_viewer )
                {
                    // add the array (for the Save Array Viewer)
                    creating_missing_array = true;
                    logic_array = logic_arrays.emplace_back(new LogicArray(CS2WS(sArg)));
                    logic_array->SetUsingSaveArray();
                }

                else
                {
                    // warning array in file, but not declared in program
                    m_systemMessageIssuer->Issue(MessageType::Warning, 19011, sArg.GetString());
                    return nullptr;
                }
            }

            else
            {
                logic_array = *array_search;

                if( logic_array->GetSaveArray() == nullptr )
                {
                    // the array is no longer a save array
                    m_systemMessageIssuer->Issue(MessageType::Warning, 19013, sArg.GetString());
                    return nullptr;
                }
            }
        }
        else if( logic_array == nullptr ) {
            // the array name must be the first line processed
            m_systemMessageIssuer->Issue(MessageType::Error, 19016, m_specFile.GetLineNumber());
            return nullptr;
        }
        else if (sCmd.CompareNoCase(sCmdDimensions) == 0) {
            // ignore the Dimension line in pre-7.2 files
        }
        else if (sCmd.CompareNoCase(sCmdSize) == 0) {
            // read in each dimension
            std::vector<size_t> dimensions;

            while( true ) {
                CIMSAString sTemp;
                sTemp = sArg.GetToken();

                if( sTemp.IsNumeric() )
                {
                    if( creating_missing_array || read_version_number != ReadVersionNumber::VersionPre72 || dimensions.size() < logic_array->GetNumberDimensions() )
                        dimensions.emplace_back(static_cast<size_t>(sTemp.Val()));
                }

                else
                {
                    break;
                }
            }

            if( creating_missing_array )
            {
                // the Save Array Viewer doesn't support arrays with more than three dimensions
                if( dimensions.size() > 3 )
                {
                    m_systemMessageIssuer->Issue(MessageType::Error, 19025, static_cast<int>(dimensions.size()), logic_array->GetName().c_str());
                    logic_arrays.pop_back();
                    return nullptr;
                }

                logic_array->SetDimensions(dimensions);
            }

            bool number_dimensions_match = ( dimensions.size() == logic_array->GetNumberDimensions() );
            bool dimensions_match = number_dimensions_match;

            for( size_t i = 0; dimensions_match && i < dimensions.size(); i++ )
                dimensions_match = ( dimensions[i] == logic_array->GetDimension(i) );

            if( !dimensions_match ) {
                // warn that the size is bad or doesn't match dimensions
                m_systemMessageIssuer->Issue(number_dimensions_match ? MessageType::Warning : MessageType::Error, 19014,
                                             logic_array->GetName().c_str());

                if( !number_dimensions_match )
                    return nullptr;
            }
        }
        else if (sCmd.CompareNoCase(sCmdType) == 0) {
            bool numeric = ( sArg.CompareNoCase(sTypeNumeric) == 0 );
            bool string = ( sArg.CompareNoCase(sTypeAlpha) == 0 );

            if( creating_missing_array )
                logic_array->SetNumeric(numeric);

            if( ( !numeric && !string ) || ( logic_array->IsNumeric() != numeric ) ) {
                // array type not numeric or alpha or inconsistent with the array
                m_systemMessageIssuer->Issue(MessageType::Error, 19015, logic_array->GetName().c_str(), m_specFile.GetLineNumber());
                return nullptr;
            }
        }
        else if( sCmd.CompareNoCase(sCmdRuns) == 0 ) {
            if( logic_array->GetSaveArray() != nullptr )
                logic_array->GetSaveArray()->SetNumberRuns(static_cast<size_t>(sArg.Val()));
        }
        else if( sCmd.CompareNoCase(sCmdCases) == 0 ) {
            if( logic_array->GetSaveArray() != nullptr )
                logic_array->GetSaveArray()->SetNumberCases(static_cast<size_t>(sArg.Val()));
        }
        else {
            // unknown cmd, bail, if caller can't recognize it, see if caller can
            m_specFile.UngetLine();
            break;
        }
    }

    return logic_array;
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::SkipToNextArray
//
// Skip over lines in file until you find the start of the next array (marked
// by "[Array]" section marker).  Used when you hit an error in an array
// and you want to bail and move on to next one.
/////////////////////////////////////////////////////////////////////////////////
void SaveArrayFile::SkipToNextArray()
{
    UINT sfState = SF_OK;
    while (sfState == SF_OK) {
        sfState = m_specFile.SkipSection();
        if (sfState == SF_OK) {
            CString sCmd, sArg;
            sfState = m_specFile.GetLine(sCmd, sArg);
            if (sfState != SF_OK) {
                break;
            }
            if (sCmd.CompareNoCase(sSectArray) == 0) {
                m_specFile.UngetLine();
                break;
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//           SaveArrayFile::ReadArrayData + ReadOldFormatArrayData
//
// Read data of array from saved array file.
// Data section is slices, rows, cells
/////////////////////////////////////////////////////////////////////////////////

namespace
{
    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(SaveArrayFileReadException, _T("Save Array Read Error"))
}


bool SaveArrayFile::ReadArrayData(LogicArray& logic_array, ReadVersionNumber read_version_number)
{
    if( read_version_number == ReadVersionNumber::VersionPre72 )
        return ReadOldFormatArrayData(logic_array);

    SaveArray* save_array = logic_array.GetSaveArray();

    CString sCmd;
    CIMSAString sArg;

    const size_t numeric_components_expected = logic_array.GetNumberDimensions() + 2;

    // throw an exception if there are any problems with the lines
    try
    {
        while( m_specFile.GetLine(sCmd, sArg) == SF_OK )
        {
            if( sCmd.GetAt(0) == _T('[') )
            {
                // new command, assume end of table data
                m_specFile.UngetLine();
                break;
            }

            else if( sCmd.CompareNoCase(sCmdCell) != 0 )
            {
                // invalid command
                throw SaveArrayFileReadException();
            }

            // Format: Cell= dim1, dim2, ...; gets, puts; value
            std::vector<size_t> numeric_components;
            std::wstring cell_value;

            while( numeric_components.size() < numeric_components_expected || cell_value.empty() )
            {
                CIMSAString component = sArg.GetToken(_T(",;"));

                // anything other than the value should a valid number
                if( numeric_components.size() < numeric_components_expected )
                {
                    double numeric_value = -1;
                    component.Trim();

                    if( component.IsNumeric() )
                        numeric_value = static_cast<size_t>(component.Val());

                    if( numeric_value < 0 )
                        throw SaveArrayFileReadException();

                    numeric_components.emplace_back(static_cast<size_t>(numeric_value));
                }

                // the value should only have one space trimmed (in case an alpha value had
                // spaces before the value)
                else if( !component.IsEmpty() )
                {
                    if( read_version_number == ReadVersionNumber::Version80Beta )
                    {
                        cell_value = Encoders::FromEscapedString(wstring_view(component).substr(1));

                        // if semicolons appeared in the cell text, we must read the tokens until exhausted
                        while( true )
                        {
                            component = sArg.GetToken(_T(",;"));

                            if( SO::IsBlank(component) )
                                break;

                            SO::AppendWithSeparator(cell_value, Encoders::FromEscapedString(CS2WS(component)), ';');
                        }
                    }

                    else
                    {
                        cell_value = wstring_view(component).substr(1);
                    }

                    SO::MakeTrimRightSpace(cell_value);
                }

                if( SO::IsBlank(component) )
                    break;
            }

            if( numeric_components.size() != numeric_components_expected )
                throw SaveArrayFileReadException();

            // process the indices
            std::vector<size_t> indices;
            size_t numeric_components_index = 0;

            for( ; numeric_components_index < logic_array.GetNumberDimensions(); numeric_components_index++ )
                indices.emplace_back(numeric_components[numeric_components_index]);

            // skip entries with invalid indices
            if( !logic_array.IsValidIndex(indices) )
                continue;

            if( save_array != nullptr )
            {
                save_array->SetNumberGets(indices, numeric_components[numeric_components_index]);
                numeric_components_index++;
                save_array->SetNumberPuts(indices, numeric_components[numeric_components_index]);
            }

            if( logic_array.IsNumeric() )
            {
                logic_array.SetValue(indices, StringToNumber(cell_value));
            }

            else
            {
                logic_array.SetValue(indices, std::move(cell_value));
            }
        }
    }

    catch( const SaveArrayFileReadException& )
    {
        m_systemMessageIssuer->Issue(MessageType::Error, 19017, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        return false;
    }

    return true;
}



///////////////////////////////////////////////////////////////////////////////////////
//
// everything after this point is for reading the old save array format (pre-CSPro 7.2)
//
///////////////////////////////////////////////////////////////////////////////////////

namespace
{
    std::vector<size_t> OldDimensionsToIndices(const LogicArray& logic_array, int iRow, int iCol, int iSlice)
    {
        std::vector<size_t> indices;

        if( logic_array.GetNumberDimensions() >= 1 )
            indices.emplace_back(iRow);

        if( logic_array.GetNumberDimensions() >= 2 )
            indices.emplace_back(iCol);

        if( logic_array.GetNumberDimensions() >= 3 )
            indices.emplace_back(iSlice);

        return indices;
    }
}

bool SaveArrayFile::ReadOldFormatArrayData(LogicArray& logic_array)
{
    CString sCmd;
    CIMSAString sArg;
    if (m_specFile.GetLine(sCmd, sArg) != SF_OK) {
        // error, hit EOF before finding any data
        m_systemMessageIssuer->Issue(MessageType::Error, 19018, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        return false;
    }

    if (sCmd.CompareNoCase(sSectSlice) != 0) {
        // error expect "[Slice]" section marker for first slice
        m_systemMessageIssuer->Issue(MessageType::Error, 19018, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        m_specFile.UngetLine(); // case where you have start of new array instead of [Slice]
        return false;
    }

    int nRows = 1;
    int nCols = 1;
    int nSlices = 1;

    if( logic_array.GetNumberDimensions() >= 1 )
        nRows = logic_array.GetDimension(0);

    if( logic_array.GetNumberDimensions() >= 2 )
        nCols = logic_array.GetDimension(1);

    if( logic_array.GetNumberDimensions() >= 3 )
        nSlices = logic_array.GetDimension(2);

    // read slices
    // keep reading lines from file, increment row number each time
    // you get a new line starting with "Row=" and increment slice
    // number each time you get a line starting with "[Slice]"
    int iSlice = 0;
    int iRow = 0;
    int iGetSlice = 0;
    int iPutSlice = 0;

    while (m_specFile.GetLine(sCmd, sArg) == SF_OK) {

        if (sCmd.CompareNoCase(sCmdRow) == 0) {
            // start new row
            if (iRow >= nRows) {
                // too many rows
                m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
                return false;
            }
            // read each column (cell) within this row
            int iCol = 0;
            while (!sArg.IsEmpty())  {

                if (iCol >= nCols) {
                    // too many columns
                    m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
                    return false;
                }

                if (!ReadCell(logic_array, iRow, iCol, iSlice, sArg)) {
                    // error reading cell, will have been reported already
                    // by ReadCell.
                    return false;
                }
                ++iCol;
            }
            if (iCol != nCols) {
                // error if not enough cols in row
                m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
                return false;
            }
            ++iRow;
        }
        else if (sCmd.CompareNoCase(sSectSlice) == 0) {
            // start new slice
            if (iRow != nRows) {
                // error if not enough rows in last slice
                m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
                return false;
            }
            iRow = 0;
            ++iSlice;
            if (iSlice >= nSlices) {
                // too many slices
                m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
                return false;
            }
        }
        else if (sCmd.GetAt(0) == _T('[')) {
            // new command, assume end of table data
            m_specFile.UngetLine();
            break;
        }
        else if( !sCmd.CompareNoCase(sCmdGet) ) // 20130305 for save array statistics
        {
            if( !ReadStatistics(sCmdGet,sArg,logic_array,nRows,nCols,iGetSlice) )
                return false;

            iGetSlice++;
        }
        else if( !sCmd.CompareNoCase(sCmdPut) )
        {
            if( !ReadStatistics(sCmdPut,sArg,logic_array,nRows,nCols,iPutSlice) )
                return false;

            iPutSlice++;
        }
        else {
            // unrecognized line in table data
            m_systemMessageIssuer->Issue(MessageType::Error, 19019, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
            return false;
        }
    }

    if (iRow != nRows) {
        // error if not enough rows in last slice
        m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        return false;
    }
    if (iSlice != nSlices - 1) { // check nSlices - 1 since iSlices not incremented
                                   // after last slice
        // error if not enough slices in table
        m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        return false;
    }

    if( ( iGetSlice > 0 && iGetSlice != nSlices ) || ( iPutSlice > 0 && iPutSlice != nSlices ) )
    {
        m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        return false;
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::ReadCell
//
// Read cell of array from array file.
/////////////////////////////////////////////////////////////////////////////////
bool SaveArrayFile::ReadCell(LogicArray& logic_array, int iRow, int iCol, int iSlice, CIMSAString& sLine)
{
    std::vector<size_t> indices = OldDimensionsToIndices(logic_array, iRow, iCol, iSlice);
    ASSERT(logic_array.IsValidIndex(indices));

    if (logic_array.IsString()) {
        return ReadCellAlpha(logic_array, indices, sLine);
    }
    else if (logic_array.IsNumeric()) {
        return ReadCellNumeric(logic_array, indices, sLine);
    }

    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::ReadCellAlpha
//
// Read cell of alpha array from array file.
/////////////////////////////////////////////////////////////////////////////////
bool SaveArrayFile::ReadCellAlpha(LogicArray& logic_array, const std::vector<size_t>& indices, CString& sLine)
{
    int iCol = ( indices.size() == 1 ) ? 0 : indices[1];

    if( iCol == 0 ) {
        if (sLine.GetLength() < 1 || sLine.GetAt(0) != '|') {
            m_systemMessageIssuer->Issue(MessageType::Error, 19022, m_specFile.GetLineNumber(), m_specFile.GetFilePath().GetString(), iCol);
            return false; // need leading "|" bf first item in row
        }
        sLine = sLine.Right(sLine.GetLength() - 1);
    }

    UINT iCellSize = logic_array.GetPaddingStringLength();
    if ((UINT) sLine.GetLength() < iCellSize) {
        m_systemMessageIssuer->Issue(MessageType::Error, 19021, m_specFile.GetLineNumber(), m_specFile.GetFilePath().GetString(), iCol, iCellSize);
        return false;
    }

    if (sLine.GetAt(iCellSize) != '|') {
        m_systemMessageIssuer->Issue(MessageType::Error, 19022, m_specFile.GetLineNumber(), m_specFile.GetFilePath().GetString(), iCol);
        return false;
    }

    logic_array.SetValue(indices, CS2WS(sLine));

    sLine = sLine.Right(sLine.GetLength() - iCellSize -1);

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//                      SaveArrayFile::ReadCellNumeric
//
// Read cell of numeric array from array file.
/////////////////////////////////////////////////////////////////////////////////
bool SaveArrayFile::ReadCellNumeric(LogicArray& logic_array, const std::vector<size_t>& indices, CIMSAString& sLine)
{
    int iCol = ( indices.size() == 1 ) ? 0 : indices[1];

    CIMSAString sTok = sLine.GetToken(_T(" \t")); // allow space or tab delimited

    sTok.Trim();
    double fVal;

                if( sTok.CompareNoCase(L".000000000")==0)
                    sTok = sTok;

    if (sTok.CompareNoCase(_T("notappl")) == 0) {
        fVal = NOTAPPL;
    }
    else if (sTok.CompareNoCase(_T("default")) == 0) {
        fVal = DEFAULT;
    }
    else if (sTok.CompareNoCase(_T("missing")) == 0) {
        fVal = MISSING;
    }
    else if (sTok.CompareNoCase(_T("refused")) == 0) {
        fVal = REFUSED;
    }
    else if (sTok.IsNumeric()) {
        fVal = (double) sTok.fVal();
    }
    else {
        // error, not numeric
        m_systemMessageIssuer->Issue(MessageType::Error, 19020, m_specFile.GetLineNumber(), m_specFile.GetFilePath().GetString(), iCol);
        return false;
    }

    // assign to cell in array
    logic_array.SetValue(indices, fVal);

    return true;
}


bool SaveArrayFile::ReadStatistics(CString sMatchCmd, CIMSAString sArg, LogicArray& logic_array, int nRows, int nCols, int iLayer)
{
    SaveArray* save_array = logic_array.GetSaveArray();
    bool gets = ( sMatchCmd.CompareNoCase(sCmdGet) == 0 );

    int iRows = 0;

    CString sCmd = sMatchCmd;

    do
    {
        if( !sCmd.CompareNoCase(sMatchCmd) )
        {
            int iCols = 0;

            while( !sArg.IsEmpty() )
            {
                CIMSAString sTok = sArg.GetToken(_T(" \t"));
                sTok.Trim();

                std::vector<size_t> indices = OldDimensionsToIndices(logic_array, iRows, iCols, iLayer);

                if( save_array != nullptr && logic_array.IsValidIndex(indices) )
                {
                    size_t value = static_cast<size_t>(sTok.Val());

                    if( gets )
                    {
                        save_array->SetNumberGets(indices, value);
                    }

                    else
                    {
                        save_array->SetNumberPuts(indices, value);
                    }
                }

                iCols++;
            }

            if( iCols != nCols )
            {
                // error if not enough cols in row
                m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
                return false;
            }

            iRows++;
        }

        else if( !sCmd.CompareNoCase(sCmdGet) || !sCmd.CompareNoCase(sCmdPut) || sCmd.GetAt(0) == _T('[') )
        {
            m_specFile.UngetLine();
            break;
        }

        else
        {
            // unrecognized line in table data
            m_systemMessageIssuer->Issue(MessageType::Error, 19019, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
            return false;
        }

    } while( m_specFile.GetLine(sCmd,sArg) == SF_OK );

    if( iRows != nRows )
    {
        // error if not enough rows in last slice
        m_systemMessageIssuer->Issue(MessageType::Error, 19023, m_specFile.GetLineNumber(), logic_array.GetName().c_str());
        return false;
    }

    return true;
}



// --------------------------------------------------------------------------
// SaveArrayViewerHelpers
// --------------------------------------------------------------------------

LogicArray* SaveArrayViewerHelpers::CreateLogicArray(std::wstring array_name, std::vector<size_t> dimensions)
{
    LogicArray* logic_array = new LogicArray(std::move(array_name));
    logic_array->SetUsingSaveArray();
    logic_array->SetDimensions(std::move(dimensions));
    return logic_array;
}


void SaveArrayViewerHelpers::CopyValues(LogicArray& logic_array, ValueCopier& value_copier, bool clr_receiving_values)
{
    SaveArray* save_array = logic_array.GetSaveArray();
    ASSERT(save_array != nullptr);

    int rows = logic_array.GetDimension(0);
    int cols = ( logic_array.GetNumberDimensions() > 1 ) ? logic_array.GetDimension(1) : 1;

    std::function<void(const std::vector<size_t>&)> array_copier =
        [&](const std::vector<size_t>& indices)
        {
            size_t index = ( logic_array.GetNumberDimensions() > 2 ) ? indices[2] : 0;
            index *= cols;
            index += ( logic_array.GetNumberDimensions() > 1 ) ? indices[1] : 0;
            index *= rows;
            index += indices[0];

            if( clr_receiving_values )
            {
                const std::wstring& cell_value = logic_array.IsNumeric() ? CS2WS(NumberToString(logic_array.GetValue<double>(indices))) :
                                                                           logic_array.GetValue<std::wstring>(indices);

                value_copier.SetValues(index, cell_value, save_array->GetNumberGets(indices), save_array->GetNumberPuts(indices));
            }

            else
            {
                auto [cell_value, gets, puts] = value_copier.GetValues(index);

                if( logic_array.IsNumeric() )
                {
                    logic_array.SetValue(indices, StringToNumber(cell_value));
                }

                else
                {
                    logic_array.SetValue(indices, cell_value);
                }

                save_array->SetNumberGets(indices, gets);
                save_array->SetNumberPuts(indices, puts);
            }
        };

    logic_array.IterateCells(0, array_copier);
}
