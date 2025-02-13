#pragma once

#include <zUtilO/zUtilO.h>


namespace CSProExecutables
{
    enum class Program
    {
        CSBatch, CSCode, CSConcat, CSDeploy, CSDiff, CSDocument, CSEntry, CSExport, CSFreq, CSIndex, CSPack, CSPro, CSReFmt,
        CSSort, CSTab, CSView, DataViewer, Excel2CSPro, OperatorStatisticsViewer, ParadataConcat, ParadataViewer,
        PffEditor, ProductionRunner, RunPff, SaveArrayViewer, TblView, TextConverter, TextView
    };


    CLASS_DECL_ZUTILO const std::wstring& GetApplicationDirectory();
    CLASS_DECL_ZUTILO const std::wstring& GetApplicationOrAssetsDirectory();


#ifdef WIN_DESKTOP
    CLASS_DECL_ZUTILO const std::wstring& GetModuleFilename();
    CLASS_DECL_ZUTILO const std::wstring& GetModuleDirectory();

    CLASS_DECL_ZUTILO std::wstring GetSolutionDirectory();

    CLASS_DECL_ZUTILO const TCHAR* GetExecutableName(CSProExecutables::Program program);

    CLASS_DECL_ZUTILO std::optional<std::wstring> GetExecutablePath(CSProExecutables::Program program);

    CLASS_DECL_ZUTILO std::optional<std::wstring> GetExecutableHelpPath(CSProExecutables::Program program);

    CLASS_DECL_ZUTILO void RunProgram(CSProExecutables::Program program, const TCHAR* argument = nullptr);
    CLASS_DECL_ZUTILO void RunProgramOpeningFile(CSProExecutables::Program program, const std::wstring& filename);
#endif
}
