#pragma once

#include <zUtilF/SortListCtrl.h>


class ManageLibrariesView : public CFormView
{
    DECLARE_DYNCREATE(ManageLibrariesView)

protected:
    ManageLibrariesView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    LRESULT OnUpdateLibraryIds(WPARAM wParam, LPARAM lParam);

    void OnRefreshLibraryIds();

    void OnCreateBuiltLibrary()  { OnBuiltLibraryAction(true); }
    void OnPreviewBuiltLibrary() { OnBuiltLibraryAction(false); }

    void OnViewBuiltLibraryInputs();

private:
    void OnBuiltLibraryAction(bool create);

private:
    Controller& m_controller;

    CSortListCtrl m_libraryIdsListCtrl;
    std::string m_commit;
};
