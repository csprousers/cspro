#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zMultimediaO/ImageManager.h>
#include <zMultimediaO/resource.h>


// --------------------------------------------------------------------------
// ImageSelectorDlg
//
// A modeless dialog that displays an image and allows the user to
// modify or remove the image.
//
// The dialog closes automatically when it becomes inactive.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API ImageSelectorDlg : public CDialog
{
    DECLARE_DYNAMIC(ImageSelectorDlg)

public:
    ImageSelectorDlg(const std::string& file_path, std::function<void(const std::string&)> modification_callback,
                     std::optional<CRect> rect_above_desired_window, CWnd* pParent = nullptr);

    enum { IDD = IDD_IMAGE_SELECTOR };

    // returns a blank string if no file was selected
    static std::string SelectFile(const std::string& file_path = SO::Empty_string);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    afx_msg BOOL OnNcActivate(BOOL bActive);

    afx_msg void OnBnClickedModify();
    afx_msg void OnBnClickedRemove();

private:
    std::string m_filePath;
    std::function<void(const std::string&)> m_modificationCallback;
    std::optional<CRect> m_rectAboveDesiredWindow;

    std::shared_ptr<const CImage> m_image;
    CStatic m_imageStatic;
};
