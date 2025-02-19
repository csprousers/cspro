#include "StdAfx.h"
#include "CSProExecutables.h"
#include <zPlatformO/PlatformInterface.h>


#ifdef WIN_DESKTOP
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif


const std::string& CSProExecutables::GetApplicationDirectory()
{
#ifdef WIN_DESKTOP
    static const std::string cached_application_directory = []()
    {
        wchar_t application_directory[MAX_PATH];

        GetModuleFileName(reinterpret_cast<HINSTANCE>(&__ImageBase), application_directory, MAX_PATH);
        PathRemoveFileSpec(application_directory);

        return PortableFunctions::PathEnsureTrailingSlash(TC::ToUtf8(application_directory));
    }();

    return cached_application_directory;

#else
    return PlatformInterface::GetInstance()->GetApplicationDirectory();
#endif
}


const std::string& CSProExecutables::GetApplicationOrAssetsDirectory()
{
#ifdef WIN_DESKTOP
    return CSProExecutables::GetApplicationDirectory();
#else
    return PlatformInterface::GetInstance()->GetAssetsDirectory();
#endif
}


#ifdef WIN_DESKTOP

const std::string& CSProExecutables::GetModuleFilePath()
{
    static const std::string module_file_path =
        []()
        {
            wchar_t exe_name[MAX_PATH];

            // the second call, with null, works for the C# programs
            if( ( AfxGetApp() != nullptr && GetModuleFileName(AfxGetApp()->m_hInstance, exe_name, MAX_PATH) ) ||
                ( GetModuleFileName(nullptr, exe_name, MAX_PATH) ) )
            {
                return TC::ToUtf8(exe_name);
            }

            return std::string();
        }();

    return module_file_path;
}


const std::string& CSProExecutables::GetModuleDirectory()
{
    static const std::string module_directory =
        []()
        {
            std::string module_file_path = GetModuleFilePath();

            if( module_file_path.empty() )
            {
                // this code was used prior to GetModuleFilePath existing
                const std::string& (&this_function)() = GetModuleDirectory;
                HMODULE hm = nullptr;

                wchar_t exe_name[MAX_PATH];

                if( GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(this_function), &hm) )
                {
                    if( GetModuleFileName(hm, exe_name, MAX_PATH) )
                        module_file_path = TC::ToUtf8(exe_name);
                }
            }

            ASSERT(!module_file_path.empty());
            return PortableFunctions::PathGetDirectory(module_file_path);
        }();

    return module_directory;
}


int64_t CSProExecutables::GetModuleModifiedTime()
{
    return PortableFunctions::FileModifiedTime(GetModuleFilePath());
}

#endif // WIN_DESKTOP


const char* CSProExecutables::GetExecutableName(const CSProExecutables::Program program)
{
    constexpr const char* ExecutableNames[] =
    {
        "CSBatch.exe",
        "CSCode.exe",
        "CSConcat.exe",
        "CSDeploy.exe",
        "CSDiff.exe",
        "CSDocument.exe",
        "CSEntry.exe",
        "CSExport.exe",
        "CSFreq.exe",
        "CSIndex.exe",
        "CSPack.exe",
        "CSPro.exe",
        "CSReFmt.exe",
        "CSSort.exe",
        "CSTab.exe",
        "CSView.exe",
        "DataManager.exe",
        "Excel2CSPro.exe",
        "Operator Statistics Viewer.exe",
        "ParadataConcat.exe",
        "ParadataViewer.exe",
        "PFF Editor.exe",
        "CSProProductionRunner.exe",
        "RunPff.exe",
        "Save Array Viewer.exe",
        "TblView.exe",
        "TextConverter.exe",
        "TextView.exe",
    };

    static_assert(_countof(ExecutableNames) == ( static_cast<size_t>(CSProExecutables::Program::TextView) + 1 ));

    return ExecutableNames[static_cast<size_t>(program)];
}


#ifdef WIN_DESKTOP

std::optional<std::string> CSProExecutables::GetExecutablePath(const CSProExecutables::Program program)
{
    const std::string& module_directory = GetModuleDirectory();

    if( !module_directory.empty() )
    {
        std::string module_file_path = Path::Combine(module_directory, GetExecutableName(program));

        if( PortableFunctions::FileIsRegular(module_file_path) )
            return module_file_path;
    }

    return std::nullopt;
}


std::optional<std::string> CSProExecutables::GetExecutableHelpPath(const CSProExecutables::Program program)
{
    const std::optional<std::string> module_file_path = GetExecutablePath(program);

    if( module_file_path.has_value() )
    {
        std::string help_file_path = PortableFunctions::PathReplaceFileExtension(*module_file_path, FileExtensions::CHM);

        if( PortableFunctions::FileIsRegular(help_file_path) )
            return help_file_path;
    }

    return std::nullopt;
}


void CSProExecutables::RunProgram(const CSProExecutables::Program program, const wchar_t* const argument/* = nullptr*/,
                                  const bool throw_exception_on_error/* = false*/)
{
    try
    {
        const std::optional<std::wstring> module_file_path = UTF8_TODO::GetOptionalWide(GetExecutablePath(program));

        if( !module_file_path.has_value() )
        {
            throw CSProException("The CSPro program '%s' could not be found. Your CSPro installation may not be complete.",
                                 GetExecutableName(program));
        }

        ShellExecute(nullptr, nullptr, module_file_path->c_str(), argument, nullptr, SW_SHOW);
    }

    catch( const CSProException& exception )
    {
        if( throw_exception_on_error )
            throw exception;

        ErrorMessage::Display(exception);
    }
}


void CSProExecutables::RunProgramOpeningFile(const CSProExecutables::Program program, std::wstring file_path,
                                             const bool throw_exception_on_error/* = false*/)
{
    RunProgram(program, EscapeCommandLineArgument(std::move(file_path)).c_str(), throw_exception_on_error);
}


void CSProExecutables::RunProgramOpeningFile(const CSProExecutables::Program program, std::string file_path,
                                             const bool throw_exception_on_error/* = false*/)
{
    RunProgram(program, TC::ToWide(EscapeCommandLineArgument(std::move(file_path))).c_str(), throw_exception_on_error);
}


#endif // WIN_DESKTOP
