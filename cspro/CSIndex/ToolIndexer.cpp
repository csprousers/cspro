#include "stdafx.h"
#include "ToolIndexer.h"
#include "DuplicateChooserDlg.h"


bool ToolIndexer::SupportsInteractiveMode() const
{
    // all run types are valid
    return true;
}


void ToolIndexer::DisplayInteractiveModeMessage(const NullTerminatedString message) const
{
    AfxMessageBox(message);
}


void ToolIndexer::ChooseDuplicate(std::vector<DuplicateInfo>& case_duplicates, const size_t duplicate_index, const size_t number_duplicates) const
{
    DuplicateChooserDlg duplicate_chooser_dlg(case_duplicates, duplicate_index, number_duplicates);

    if( duplicate_chooser_dlg.DoModal() != IDOK )
        throw UserCanceledException();
}


bool ToolIndexer::RethrowTerminatingException() const
{
    return true;
}
