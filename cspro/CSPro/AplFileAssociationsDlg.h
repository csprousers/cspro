#pragma once

#include <CSPro/AplFileAssociationsGrid.h>
#include <CSPro/FileAssociation.h>


/////////////////////////////////////////////////////////////////////////////
//
//                                 CAplFileAssociationsDlg
//
/////////////////////////////////////////////////////////////////////////////

class CAplFileAssociationsDlg : public CDialog
{
public:
    CAplFileAssociationsDlg(CWnd* pParent = NULL);

    enum { IDD = IDD_APLFILEASSOCIATIONSDLG };

public:
    void SetGridData();

    const FileAssociation* FindFileAssoc(LPCTSTR sFilePathOrig) const;
    int FindFileAssocIndex(LPCTSTR sFilePathOrig) const;

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    BOOL PreTranslateMessage(MSG* pMsg) override;

    BOOL OnInitDialog() override;
    void OnOK() override;

private:
    void EstablishSuggestedDictionaries();

public:
    CAplFileAssociationsGrid m_aplFileAssociationsGrid;
    CRect m_Rect;

    enum class WorkingStorageType { Editable, ReadOnly, Hidden };
    WorkingStorageType m_workingStorageType;

    CString m_sTitle;
    CString m_sAppName;
    CString m_sWSDName;
    std::vector<FileAssociation> m_fileAssociations;
    BOOL m_bWorkingStorage;

private:
    std::vector<std::wstring> m_suggestedDictionaries;
    size_t m_nextSuggestedDictionaryIndex;
};
