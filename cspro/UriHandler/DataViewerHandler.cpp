#include "stdafx.h"
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/WindowHelpers.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib,"Psapi.lib")


namespace
{
    constexpr wstring_view DataViewerPropertyFilename   = _T("file");
    constexpr wstring_view DataViewerPropertyDictionary = _T("dcf");
    constexpr wstring_view DataViewerPropertyKey        = _T("key");
    constexpr wstring_view DataViewerPropertyUuid       = _T("uuid");

    struct ProcessAndHandle
    {
        DWORD process_id;
        HWND hWnd;
    };


    BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM lParam)
    {
        // modified from https://stackoverflow.com/questions/1888863/how-to-get-main-window-handle-from-process-id
        ProcessAndHandle& process_and_handle = *reinterpret_cast<ProcessAndHandle*>(lParam);

        DWORD process_id;
        GetWindowThreadProcessId(hWnd, &process_id);

        if( process_and_handle.process_id == process_id && GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd) )
        {
            process_and_handle.hWnd = hWnd;
            return FALSE;
        }

        return TRUE;
    }


    HWND FindMainWindow(DWORD process_id)
    {
        ProcessAndHandle process_and_handle = { process_id, nullptr };
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&process_and_handle));
        return process_and_handle.hWnd;
    }


    std::vector<HWND> FindDataViewerWindowHandle(const std::wstring& dataviewer_exe)
    {
        // modified from https://stackoverflow.com/questions/53317608/get-full-path-from-a-process-in-windows
        HANDLE hSnapProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        std::vector<HWND> window_handles;

        if( hSnapProcess != INVALID_HANDLE_VALUE )
        {
            PROCESSENTRY32 process;
            process.dwSize = sizeof(PROCESSENTRY32);
            Process32First(hSnapProcess, &process);

            do
            {
                if( process.th32ProcessID != 0 )
                {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process.th32ProcessID);

                    if( hProcess != nullptr )
                    {
                        wchar_t file_path[MAX_PATH];

                        if( GetModuleFileNameEx(hProcess, nullptr, file_path, MAX_PATH) != 0 )
                        {
                            if( SO::EqualsNoCase(dataviewer_exe, file_path) )
                            {
                                HWND hWnd = FindMainWindow(process.th32ProcessID);

                                if( hWnd != nullptr )
                                    window_handles.emplace_back(hWnd);
                            }
                        }

                        CloseHandle(hProcess);
                    }
                }

            } while( Process32Next(hSnapProcess, &process) );

            CloseHandle(hSnapProcess);
        }

        return window_handles;
    }
}


void HandleDataViewerUri(const std::map<std::wstring, std::wstring>& properties)
{
    std::wstring command_line_arguments;
    std::wstring filename;
    std::wstring key;
    std::wstring uuid;

    // parse the properties
    auto add_command_line_argument = [&](wstring_view property_name, bool property_must_exist, std::wstring* out_value)
    {
        const auto& value_lookup = properties.find(property_name);

        if( value_lookup == properties.cend() )
        {
            if( property_must_exist )
            {
                throw CSProException(_T("The CSPro URI that launches Data Viewer must contain the property: %s"),
                                     std::wstring(property_name).c_str());
            }

            return;
        }

        if( out_value != nullptr )
            *out_value = value_lookup->second;

        std::wstring property_and_value = SO::Concatenate(property_name, _T("="), value_lookup->second);

        // escape any double quotes
        SO::Replace(property_and_value, _T("\""), _T("\"\""));

        SO::AppendFormat(command_line_arguments, _T("%s\"%s\""), command_line_arguments.empty() ? _T("") : _T(" "), property_and_value.c_str());
    };

    add_command_line_argument(DataViewerPropertyFilename, true, &filename);
    add_command_line_argument(DataViewerPropertyDictionary, false, nullptr);
    add_command_line_argument(DataViewerPropertyKey, false, &key);
    add_command_line_argument(DataViewerPropertyUuid, false, &uuid);


    const std::optional<std::wstring> dataviewer_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::DataViewer);

    if( !dataviewer_exe.has_value() )
        throw CSProException("Data Viewer could not be found.");


    // if Data Viewer is already open, open the case in that process if the same data file is open in that process
    std::vector<HWND> window_handles = FindDataViewerWindowHandle(*dataviewer_exe);

    if( !window_handles.empty() )
    {
        // pass the filename, the key, and the UUID
        // each string will be passed as: size (int), string contents
        std::vector<std::byte> message_data;

        auto add_to_message_data = [&](const void* data, size_t data_size)
        {
            size_t initial_size = message_data.size();
            message_data.resize(initial_size + data_size);
            memcpy(message_data.data() + initial_size, data, data_size);
        };

        auto add_string_to_message_data = [&](std::wstring& text)
        {
            int text_length = static_cast<int>(text.size());
            add_to_message_data(&text_length, sizeof(text_length));
            add_to_message_data(text.data(), sizeof(TCHAR) * text_length);
        };

        add_string_to_message_data(filename);
        add_string_to_message_data(key);
        add_string_to_message_data(uuid);

        COPYDATASTRUCT copy_data_struct { };
        copy_data_struct.cbData = message_data.size();
        copy_data_struct.lpData = message_data.data();

        for( HWND hWnd : window_handles )
        {
            // Data Viewer will return 1 if it could load this case
            if( SendMessage(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copy_data_struct)) == 1 )
            {
                WindowHelpers::BringToForefront(hWnd);
                return;
            }
        }
    }


    // if a current instance of Data Viewer with the filename wasn't available, launch a new instance of Data Viewer
    CSProExecutables::RunProgram(CSProExecutables::Program::DataViewer, command_line_arguments.c_str());
}
