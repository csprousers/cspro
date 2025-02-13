#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilF/resource.h>
#include <zUtilF/ImageManager.h>


// a modeless dialog that displays an image and allows the user to modify or remove the image;
// the dialog closes automatically when it becomes inactive

class CLASS_DECL_ZUTILF ImageSelectorDlg : public CDialog
{
    DECLARE_DYNAMIC(ImageSelectorDlg)

public:
    ImageSelectorDlg(const CString& filename, std::function<void(const CString&)> modification_callback,
                     std::optional<CRect> rect_above_desired_window, CWnd* pParent = nullptr);

    enum { IDD = IDD_IMAGE_SELECTOR };

    // returns a blank string if no file was selected
    static CString SelectFile(const CString& filename = CString());

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    afx_msg BOOL OnNcActivate(BOOL bActive);

    afx_msg void OnBnClickedModify();
    afx_msg void OnBnClickedRemove();

private:
    CString m_filename;
    std::function<void(const CString&)> m_modificationCallback;
    std::optional<CRect> m_rectAboveDesiredWindow;

    std::shared_ptr<CImage> m_image;
    CStatic m_imageStatic;
};
