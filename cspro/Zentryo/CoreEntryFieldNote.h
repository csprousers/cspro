#pragma once

struct CoreEntryFieldNote
{
    size_t index;
    size_t case_notes_index;
    CString note;
    CString operator_id;
    bool is_field_note;
    int group_symbol_index;
    CString group_label;
    CString label;
    CString sort_index;
};
