#pragma once

#ifdef WIN_DESKTOP

#include <zFormO/FormFile.h>


class CRenameDlg : public CDialog
{
public:
    CRenameDlg(const CDEFormBase& form_base, const std::vector<const DictNamedBase*>& dict_candidates, CWnd* pParent = nullptr);

    enum { IDD = IDD_RENAMEDLG };

    bool DeleteAllItemsNotFound() const { return ( m_deleteAllItemsNotFound == TRUE ); }

    const DictNamedBase* GetSelectedDictCandidate() const { return m_selectedDictCandidate; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnRename();
    afx_msg void OnDelete();

private:
    const CDEFormBase& m_formBase;
    const std::vector<const DictNamedBase*>& m_dictCandidates;

    CString m_heading;
    CComboBox m_candidateList;
    int m_renameOrDelete;
    BOOL m_deleteAllItemsNotFound;

    const DictNamedBase* m_selectedDictCandidate;
};

#endif
