//---------------------------------------------------------------------------
//  File name: runwait.cpp
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

#include <process.h>
#include <tchar.h>
#include <Windows.h>
#include <vector>


int _tmain(int argc, TCHAR* argv[])
{
    if( argc <= 1 )
        return 0;

    std::vector<std::vector<TCHAR>> argument_data;
    std::vector<const TCHAR*> argument_pointers;

    for( int i = 1; i < argc; ++i )
    {
        const TCHAR* this_argument = argv[i];

        // check if the argument need to be escaped
        const TCHAR* EscapeCharacters = _T(" \t\n;,\'\"()[]{}");
        bool needs_escaping = false;

        for( const TCHAR* escape_itr = EscapeCharacters; *escape_itr != 0; ++escape_itr )
        {
            if( _tcschr(this_argument, *escape_itr) != nullptr )
            {
                needs_escaping = true;
                break;
            }
        }

        size_t argument_length = _tcslen(this_argument);

        auto& argument_data_vector = argument_data.emplace_back(argument_length + 1 + ( needs_escaping ? 2 : 0 ));
        TCHAR* argument_pointer = argument_data_vector.data();
        argument_pointers.emplace_back(argument_pointer);

        if( needs_escaping )
            *(argument_pointer++) = _T('"');

        _tcscpy(argument_pointer, this_argument);

        if( needs_escaping )
            *(argument_pointer + argument_length) = _T('"');
    }

    argument_pointers.emplace_back(nullptr);

    // run the program
    int return_value = _tspawnvp(_P_WAIT, argv[1], argument_pointers.data());

    // refocus the foreground window
    HWND hWnd = GetForegroundWindow();

    if( hWnd != nullptr )
        ShowWindow(hWnd, SW_RESTORE);

    return return_value;
}
