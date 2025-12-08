//---------------------------------------------------------------------------
//  File name: RunWait.cpp
//
//  Description:
//          This utiliy is intended to wait a windows program.
//
//  History:    Date       Author   Comment
//              ---------------------------
//              29 Aug 00   RHF     Creation
//              05 Oct 01   RHF/TC  Modify for ISSAW compatibility
//
//---------------------------------------------------------------------------

#include <StandardIncludes/minimal.h>
#include <StandardIncludes/strict_errors.h>
#include <process.h>
#include <windows.h>
#include <vector>


int wmain(const int argc, const wchar_t* const argv[])
{
    if( argc <= 1 )
        return 0;

    std::vector<std::vector<wchar_t>> argument_data;
    std::vector<const wchar_t*> argument_pointers;

    for( int i = 1; i < argc; ++i )
    {
        const wchar_t* const this_argument = argv[i];

        // check if the argument need to be escaped
        constexpr const wchar_t* EscapeCharacters = L" \t\n;,\'\"()[]{}";
        bool needs_escaping = false;

        for( const wchar_t* escape_itr = EscapeCharacters; *escape_itr != 0; ++escape_itr )
        {
            if( wcschr(this_argument, *escape_itr) != nullptr )
            {
                needs_escaping = true;
                break;
            }
        }

        const size_t argument_length = wcslen(this_argument);

        std::vector<wchar_t>& argument_data_vector = argument_data.emplace_back(argument_length + 1 + ( needs_escaping ? 2 : 0 ));
        wchar_t* argument_pointer = argument_data_vector.data();
        argument_pointers.emplace_back(argument_pointer);

        if( needs_escaping )
            *(argument_pointer++) = '"';

        wcscpy(argument_pointer, this_argument);

        if( needs_escaping )
            *(argument_pointer + argument_length) = '"';
    }

    argument_pointers.emplace_back(nullptr);

    // run the program
    const intptr_t return_value = _wspawnvp(_P_WAIT, argv[1], argument_pointers.data());

    // refocus the foreground window
    const HWND hWnd = GetForegroundWindow();

    if( hWnd != nullptr )
        ShowWindow(hWnd, SW_RESTORE);

    return int32_cast(return_value);
}
