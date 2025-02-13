#pragma once

//***************************************************************************
//  File name: SaveArrayFile.h
//
//  Description:
//  Saved array file class. Save CSPro numeric and alpha arrays
//  in a spec file. Load them in from same file.
//
//***************************************************************************

#include <zEngineO/zEngineO.h>
#include <zUtilO/Specfile.h>

class JsonWriter;
class LogicArray;
class PFF;
class SaveArray;
class SystemMessageIssuer;


// --------------------------------------------------------------------------
// SaveArrayFile
// --------------------------------------------------------------------------

class ZENGINEO_API SaveArrayFile
{
public:
    SaveArrayFile();

    // write all save arrays to file with given name.
    void WriteArrays(std::variant<std::wstring, const PFF*> filename_or_pff, const std::vector<LogicArray*>& logic_arrays,
                     std::shared_ptr<SystemMessageIssuer> system_message_issuer, size_t cases_read, bool increment_runs);

    // read arrays in from file with given name and put values into
    // arrays with same name that are marked as save
    void ReadArrays(std::variant<std::wstring, const PFF*> filename_or_pff, std::vector<LogicArray*>& logic_arrays,
                    std::shared_ptr<SystemMessageIssuer> system_message_issuer, bool loading_for_save_array_viewer = false);

private:
    enum class ReadVersionNumber { VersionPre72, Version72_77, Version80Beta };

    static std::wstring MakeArrayFileName(const std::variant<std::wstring, const PFF*>& filename_or_pff);

    std::set<const LogicArray*> ReadArrays(const std::wstring& sva_filename, std::vector<LogicArray*>& logic_arrays, bool loading_for_save_array_viewer);
    const LogicArray* ReadArray(const JsonNode<wchar_t>& json_node, std::vector<LogicArray*>& logic_arrays, bool loading_for_save_array_viewer);

    // for reading pre-JSON save array files
    std::set<const LogicArray*> ReadArraysPre80SpecFile(const std::wstring& sva_filename, std::vector<LogicArray*>& logic_arrays, bool loading_for_save_array_viewer);
    std::tuple<bool, ReadVersionNumber> ReadHeader();
    LogicArray* ReadArrayHeader(std::vector<LogicArray*>& logic_arrays, ReadVersionNumber read_version_number, bool loading_for_save_array_viewer);
    void SkipToNextArray();
    bool ReadArrayData(LogicArray& logic_array, ReadVersionNumber read_version_number);

    // for reading pre-7.2 save array files
    bool ReadOldFormatArrayData(LogicArray& logic_array);
    bool ReadCell(LogicArray& logic_array, int iRow, int iCol, int iSlice, CIMSAString& sLine);
    bool ReadCellAlpha(LogicArray& logic_array, const std::vector<size_t>& indices, CString& sLine);
    bool ReadCellNumeric(LogicArray& logic_array, const std::vector<size_t>& indices, CIMSAString& sLine);
    bool ReadStatistics(CString sMatchCmd, CIMSAString sArg, LogicArray& logic_array, int nRows, int nCols, int iLayer);

private:
    CSpecFile m_specFile;
    std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;
};


// --------------------------------------------------------------------------
// SaveArrayViewerHelpers
// --------------------------------------------------------------------------

namespace SaveArrayViewerHelpers
{
    ZENGINEO_API LogicArray* CreateLogicArray(std::wstring array_name, std::vector<size_t> dimensions);

    class ValueCopier
    {
    public:
        virtual ~ValueCopier() { }

        virtual std::tuple<std::wstring, size_t, size_t> GetValues(size_t index) const = 0;
        virtual void SetValues(size_t index, const std::wstring& value, size_t gets, size_t puts) = 0;
    };

    ZENGINEO_API void CopyValues(LogicArray& logic_array, ValueCopier& value_copier, bool clr_receiving_values);
}
