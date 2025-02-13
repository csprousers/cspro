#pragma once

#include <zUtilF/SrtLstCt.h>


class CCSConcatDlg : public CDialog
{
public:
    CCSConcatDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_CSCONCAT_DIALOG };

    void RunBatch(const std::wstring& pff_filename);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
    afx_msg void OnFileSaveAs();

    afx_msg void OnAddFiles();
    afx_msg void OnOutputOpen();
    afx_msg void OnRemove();
    afx_msg void OnClear();
    virtual void OnOK();
    afx_msg void OnBeginDragFileList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnEndDragFileList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeOutput();
    afx_msg void OnBnClickedDictBrowse();
    afx_msg void OnEnChangeDictionary();
    afx_msg void OnBnClickedConcatMethodCase();
    afx_msg void OnBnClickedConcatMethodFile();

    LRESULT OnUpdateDialogUI(WPARAM wParam, LPARAM lParam);    

private:
    void DropItemOnList(CListCtrl* pDragList, CListCtrl* pDropList);

    void SetDefaultPffSettings();
    bool UIToPff(bool show_errors);

    void AddConnectionStrings(const std::vector<ConnectionString>& connection_strings);

    void OnDropFiles(const std::vector<std::wstring>& filenames);

private:
    CMenu m_menu;
    const HICON m_hIcon;

    CSortListCtrl m_fileList;

    CImageList* m_pDragImage; // For creating and managing the drag-image
    BOOL m_bDragging;         // T during a drag operation
    int m_nDragIndex;         // Index of selected item in the List we are dragging FROM
    int m_nDropIndex;         // Index at which to drop item in the List we are dropping ON
    CWnd* m_pDropWnd;         // Pointer to window we are dropping on (will be cast to CListCtrl* type)

    PFF m_pff;
    ConnectionString m_outputConnectionString;
    std::wstring m_dictionaryFilename;
};
