#include "StdAfx.h"
#include <zUtilO/BCMenu.h>

namespace
{
    struct CSEntryLanguageOverride
    {
        int resource_id;
        const TCHAR* const text;
    };

    const CSEntryLanguageOverride CSEntryOverrides[] =
    {
        // File
        ID_FILE_OPEN,                   _T("File_Open"),
        ID_OPENDAT_FILE,                _T("File_OpenDat"),
        ID_SAVE,                        _T("File_Save"),
        ID_APP_EXIT,                    _T("File_Exit"),

        // Mode
        ID_ADD,                         _T("Mode_Add"),
        ID_MODIFY,                      _T("Mode_Modify"),
        ID_VERIFY,                      _T("Mode_Verify"),
        ID_PAUSE,                       _T("Mode_Pause"),
        ID_STOP,                        _T("Mode_Stop"),

        // Edit
        ID_INSERT_CASE,                 _T("Edit_InsertCase"),
        ID_DELETECASE,                  _T("Edit_DeleteCase"),
        ID_FINDCASE,                    _T("Edit_FindCase"),
        ID_INSERTNODE,                  _T("Edit_InsertLevel"),
        ID_ADDNODE,                     _T("Edit_AddLevel"),
        ID_DELETENODE,                  _T("Edit_DeleteLevel"),
        ID_INSERT_GROUPOCC,             _T("Edit_InsertGroup"),
        ID_INSERT_GRPOCC_AFTER,         _T("Edit_InsertGroupAfter"),
        ID_DELETE_GRPOCC,               _T("Edit_DeleteGroup"),
        ID_SORTGRPOCC,                  _T("Edit_SortGroup"),
        ID_EDIT_FIELD_NOTE,             _T("Edit_EditNote"),
        ID_EDIT_CASE_NOTE,              _T("Edit_EditCaseNote"),
        ID_EDIT_REVIEW_NOTES,           _T("Edit_ReviewNotes"),
        ID_INTEDIT,                     _T("Edit_InteractiveEdit"),

        // Navigation
        ID_PREV_SCREEN,                 _T("Navigation_PreviousScreen"),
        ID_NEXT_SCREEN,                 _T("Navigation_NextScreen"),
        ID_FIRST_CASE,                  _T("Navigation_FirstCase"),
        ID_PREV_CASE,                   _T("Navigation_PreviousCase"),
        ID_NEXT_CASE,                   _T("Navigation_NextCase"),
        ID_LAST_CASE,                   _T("Navigation_LastCase"),
        ID_NEXT_GROUP_OCC,              _T("Navigation_EndGroupOcc"),
        ID_NEXT_GROUP,                  _T("Navigation_EndGroup"),
        ID_NEXT_LEVEL_OCC,              _T("Navigation_EndLevelOcc"),
        ID_NEXT_LEVEL,                  _T("Navigation_EndLevel"),
        ID_ADVTOEND,                    _T("Navigation_AdvanceEnd"),
        ID_GOTO,                        _T("Navigation_Goto"),
        ID_PREVIOUS_PERSISTENT,         _T("Navigation_PreviousPersistent"),

        // View
        ID_FULLSCREEN,                  _T("View_FullScreen"),
        ID_SORTORDER,                   _T("View_SortOrder"),
        ID_VIEW_ALL_CASES,              _T("View_AllCases"),
        ID_VIEW_NOT_DELETED_CASES_ONLY, _T("View_NotDeletedOnly"),
        ID_VIEW_DUPLICATE_CASES_ONLY,   _T("View_DuplicatesOnly"),
        ID_VIEW_PARTIAL_CASES_ONLY,     _T("View_PartialsOnly"),
        ID_TOGGLE_CASE_TREE,            _T("View_CaseTree"),
        ID_TOGGLE_NAMES,                _T("View_NamesInTree"),
        ID_VIEW_REFUSALS,               _T("View_ShowRefusals"),
        ID_VIEW_CHEAT,                  _T("View_ShowValues"),
        ID_VIEW_QUESTIONNAIRE,          _T("Edit_ViewQuestionnaire"),
        ID_STATS,                       _T("View_OperatorStatistics"),

        // Options
        ID_INTEDITOPTIONS,              _T("Options_InteractiveEdit"),
        ID_LANGUAGE,                    _T("Options_Language"),
        ID_CAPITOGGLE,                  _T("Options_ToggleResponseThis"),
        ID_CAPITOGGLEALLVARS,           _T("Options_ToggleResponseAll"),

        // Help
        ID_HELP_FINDER,                 _T("Help_Help"),
        ID_APP_ABOUT,                   _T("Help_About"),
    };


    void ReplaceMenuText(BCMenu& menu, const CString& command, const CString& argument)
    {
        // search the override array for the text
        for( size_t i = 0; i < _countof(CSEntryOverrides); ++i )
        {
            if( command.CompareNoCase(CSEntryOverrides[i].text) == 0 )
            {
                BCMenuData* menu_data = menu.FindMenuItem(CSEntryOverrides[i].resource_id);

                if( menu_data != nullptr )
                {
                    // we want to keep the shortcut text (that comes after a tab), so we'll parse the strings
                    CString default_text = menu_data->GetString();
                    CString new_menu_text = argument;
                    int tab_pos = default_text.Find('\t');

                    if( tab_pos >= 0 )
                        new_menu_text.Append(default_text.Mid(tab_pos));

                    menu_data->SetWideString(new_menu_text);
                    return;
                }
            }
        }

        // if it hasn't been found yet, it could be one of the top-level menus
        for( int i = 0; i < menu.GetMenuItemCount(); ++i )
        {
            CString this_menu_string;
            menu.GetMenuString(i, this_menu_string.GetBuffer(MAX_PATH), MAX_PATH, MF_BYPOSITION);
            this_menu_string.ReleaseBuffer();

            this_menu_string.Remove('&'); // remove any & characters in the string

            if( command.CompareNoCase(this_menu_string) == 0 )
            {
                menu.ModifyMenu(i, MF_BYPOSITION | MF_STRING, 0, argument);
                return;
            }
        }
    }
}


void ActivateDynamicMenus(const CString& override_filename, BCMenu& menu) // 20111125
{
    // open the file and process the overrides
    CSpecFile override_file;

    if( override_file.Open(override_filename, CFile::modeRead) )
    {
        CString command;
        CString argument;

        while( override_file.GetLine(command, argument) == SF_OK )
            ReplaceMenuText(menu, command, argument);

        override_file.Close();
    }
}
