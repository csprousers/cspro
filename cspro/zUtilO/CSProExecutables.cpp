#include "StdAfx.h"
#include "CSProExecutables.h"
#include <zPlatformO/PlatformInterface.h>


#ifdef WIN_DESKTOP
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif


const std::wstring& CSProExecutables::GetApplicationDirectory()
{
#ifdef WIN_DESKTOP
    static const std::wstring cached_application_directory = []()
    {
        std::wstring application_directory(MAX_PATH + 1, '\0'); // + 1 to add the path character
        TCHAR* pf = application_directory.data();

        GetModuleFileName(reinterpret_cast<HINSTANCE>(&__ImageBase), pf, MAX_PATH);
        PathRemoveFileSpec(pf);

        const size_t path_len = _tcslen(pf);
        pf[path_len] = PATH_CHAR;

        application_directory.resize(path_len + 1);

        return application_directory;
    }();

    return cached_application_directory;

#else
    return PlatformInterface::GetInstance()->GetApplicationDirectory();
#endif
}


const std::wstring& CSProExecutables::GetApplicationOrAssetsDirectory()
{
#ifdef WIN_DESKTOP
    return CSProExecutables::GetApplicationDirectory();
#else
    return PlatformInterface::GetInstance()->GetAssetsDirectory();
#endif
}


#ifdef WIN_DESKTOP

const std::wstring& CSProExecutables::GetModuleFilename()
{
    auto get_module_filename = []()
    {
        auto exe_name = std::make_unique_for_overwrite<TCHAR[]>(_MAX_PATH);

        if( AfxGetApp() != nullptr && GetModuleFileName(AfxGetApp()->m_hInstance, exe_name.get(), _MAX_PATH) )
            return std::wstring(exe_name.get());

        return std::wstring();
    };

    static const std::wstring module_filename = get_module_filename();
    return module_filename;
}


const std::wstring& CSProExecutables::GetModuleDirectory()
{
    auto get_module_directory = []()
    {
        std::wstring module_filename = GetModuleFilename();

        if( module_filename.empty() )
        {
            // this code was used prior to GetModuleFilename existing
            const std::wstring& (&this_function)() = GetModuleDirectory;
            HMODULE hm = nullptr;

            module_filename.resize(_MAX_PATH);

            if( GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(this_function), &hm) )
                GetModuleFileName(hm, module_filename.data(), _MAX_PATH);
        }

        ASSERT(!module_filename.empty());
        return PortableFunctions::PathGetDirectory(module_filename);
    };

    static const std::wstring module_directory = get_module_directory();
    return module_directory;
}


std::wstring CSProExecutables::GetSolutionDirectory()
{
    std::wstring module_directory = GetModuleDirectory();

    // search directories, moving up to the root, to find the solution file
    while( PortableFunctions::FileIsDirectory(module_directory) )
    {
        const std::wstring solution_filename = PortableFunctions::PathAppendToPath(module_directory, _T("cspro.sln"));

        if( PortableFunctions::FileIsRegular(solution_filename) )
            return module_directory;

        module_directory = PortableFunctions::PathGetDirectory(PortableFunctions::PathRemoveTrailingSlash(module_directory));
    }

    throw CSProException("Could not find the CSPro solution file.");
}


const TCHAR* CSProExecutables::GetExecutableName(const CSProExecutables::Program program)
{
    constexpr const TCHAR* ExecutableNames[] =
    {
        _T("CSBatch.exe"),
        _T("CSCode.exe"),
        _T("CSConcat.exe"),
        _T("CSDeploy.exe"),
        _T("CSDiff.exe"),
        _T("CSDocument.exe"),
        _T("CSEntry.exe"),
        _T("CSExport.exe"),
        _T("CSFreq.exe"),
        _T("CSIndex.exe"),
        _T("CSPack.exe"),
        _T("CSPro.exe"),
        _T("CSReFmt.exe"),
        _T("CSSort.exe"),
        _T("CSTab.exe"),
        _T("CSView.exe"),
        _T("DataViewer.exe"),
        _T("Excel2CSPro.exe"),
        _T("Operator Statistics Viewer.exe"),
        _T("ParadataConcat.exe"),
        _T("ParadataViewer.exe"),
        _T("PFF Editor.exe"),
        _T("CSProProductionRunner.exe"),
        _T("RunPff.exe"),
        _T("Save Array Viewer.exe"),
        _T("TblView.exe"),
        _T("TextConverter.exe"),
        _T("TextView.exe"),
    };

    static_assert(_countof(ExecutableNames) == ( static_cast<size_t>(CSProExecutables::Program::TextView) + 1 ));

    return ExecutableNames[static_cast<size_t>(program)];
}


std::optional<std::wstring> CSProExecutables::GetExecutablePath(const CSProExecutables::Program program)
{
    const std::wstring& module_directory = GetModuleDirectory();

    if( !module_directory.empty() )
    {
        std::wstring module_filename = PortableFunctions::PathAppendToPath(module_directory, GetExecutableName(program));

        if( PortableFunctions::FileIsRegular(module_filename) )
            return module_filename;
    }

    return std::nullopt;
}


std::optional<std::wstring> CSProExecutables::GetExecutableHelpPath(const CSProExecutables::Program program)
{
    std::optional<std::wstring> module_filename = GetExecutablePath(program);

    if( module_filename.has_value() )
    {
        std::wstring help_filename = PortableFunctions::PathRemoveFileExtension(*module_filename) + FileExtensions::WithDot::CHM;

        if( PortableFunctions::FileIsRegular(help_filename) )
            return help_filename;
    }

    return std::nullopt;
}


void CSProExecutables::RunProgram(const CSProExecutables::Program program, const TCHAR* const argument/* = nullptr*/)
{
    const std::optional<std::wstring> module_filename = GetExecutablePath(program);

    if( module_filename.has_value() )
    {
        ShellExecute(nullptr, nullptr, module_filename->c_str(), argument, nullptr, SW_SHOW);
    }

    else
    {
        ErrorMessage::Display(FormatText(_T("The CSPro program '%s' could not be found"), GetExecutableName(program)));
    }
}


void CSProExecutables::RunProgramOpeningFile(const CSProExecutables::Program program, const std::wstring& filename)
{
    RunProgram(program, FormatText(_T("\"%s\""), filename.c_str()));
}

#endif // WIN_DESKTOP
