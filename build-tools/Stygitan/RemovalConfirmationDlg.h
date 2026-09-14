#pragma once

#include <zUtilO/ResizableDlg.h>


class RemovalConfirmationDlg : public ResizableDlg
{
public:
    RemovalConfirmationDlg(const std::vector<std::string>& paths, CWnd* pParent = nullptr);

    bool GetRecycle() const noexcept { return m_recycle; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    void OnRecycle();
    void OnDelete();

private:
    std::string m_pathsText;
    bool m_recycle;
};
