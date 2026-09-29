#pragma once

#include <zToolsO/zToolsO.h>


namespace CSProExecutables
{
    enum class Program
    {
        CSBatch, CSCode, CSConcat, CSDeploy, CSDiff, CSDocument, CSEntry, CSExport, CSFreq, CSIndex, CSPack, CSPro, CSReFmt,
        CSSort, CSTab, CSView, DataManager, Excel2CSPro, OperatorStatisticsViewer, ParadataConcat, ParadataViewer,
        PffEditor, ProductionRunner, RunPff, SaveArrayViewer, TblView, TextConverter, TextView
    };


    CLASS_DECL_ZTOOLSO const std::string& GetApplicationDirectory();
    CLASS_DECL_ZTOOLSO const std::string& GetApplicationOrAssetsDirectory();


#ifdef WIN_DESKTOP
    CLASS_DECL_ZTOOLSO const std::string& GetModuleFilePath();
    CLASS_DECL_ZTOOLSO const std::string& GetModuleDirectory();
    CLASS_DECL_ZTOOLSO int64_t GetModuleModifiedTime();
#endif

    CLASS_DECL_ZTOOLSO const char* GetExecutableName(Program program);

#ifdef WIN_DESKTOP
    CLASS_DECL_ZTOOLSO std::optional<std::string> GetExecutablePath(Program program);

    CLASS_DECL_ZTOOLSO std::optional<std::string> GetExecutableHelpPath(Program program);

    CLASS_DECL_ZTOOLSO void RunProgram(Program program, const wchar_t* argument = nullptr, bool throw_exception_on_error = false);

    CLASS_DECL_ZTOOLSO void RunProgramOpeningFile(Program program, std::wstring file_path, bool throw_exception_on_error = false);

    CLASS_DECL_ZTOOLSO void RunProgramOpeningFile(Program program, std::string file_path, bool throw_exception_on_error = false);
#endif
}
