#pragma once

#include <zUtilO/zUtilO.h>


namespace CSProExecutables
{
    enum class Program
    {
        CSBatch, CSCode, CSConcat, CSDeploy, CSDiff, CSDocument, CSEntry, CSExport, CSFreq, CSIndex, CSPack, CSPro, CSReFmt,
        CSSort, CSTab, CSView, DataManager, Excel2CSPro, OperatorStatisticsViewer, ParadataConcat, ParadataViewer,
        PffEditor, ProductionRunner, RunPff, SaveArrayViewer, TblView, TextConverter, TextView
    };


    CLASS_DECL_ZUTILO const std::string& GetApplicationDirectory();
    CLASS_DECL_ZUTILO const std::string& GetApplicationOrAssetsDirectory();


#ifdef WIN_DESKTOP
    CLASS_DECL_ZUTILO const std::string& GetModuleFilePath();
    CLASS_DECL_ZUTILO const std::string& GetModuleDirectory();
    CLASS_DECL_ZUTILO int64_t GetModuleModifiedTime();
#endif

    CLASS_DECL_ZUTILO const char* GetExecutableName(CSProExecutables::Program program);

#ifdef WIN_DESKTOP
    CLASS_DECL_ZUTILO std::optional<std::string> GetExecutablePath(CSProExecutables::Program program);

    CLASS_DECL_ZUTILO std::optional<std::string> GetExecutableHelpPath(CSProExecutables::Program program);

    CLASS_DECL_ZUTILO void RunProgram(CSProExecutables::Program program, const wchar_t* argument = nullptr,
                                      bool throw_exception_on_error = false);

    CLASS_DECL_ZUTILO void RunProgramOpeningFile(CSProExecutables::Program program, std::wstring file_path,
                                                 bool throw_exception_on_error = false);

    CLASS_DECL_ZUTILO void RunProgramOpeningFile(CSProExecutables::Program program, std::string file_path,
                                                 bool throw_exception_on_error = false);
#endif
}
