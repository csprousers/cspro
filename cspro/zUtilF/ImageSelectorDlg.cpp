#include "StdAfx.h"
#include "ImageSelectorDlg.h"
#include <zToolsO/Screen.h>
#include <zUtilF/ImageFileDialog.h>


IMPLEMENT_DYNAMIC(ImageSelectorDlg, CDialog)

BEGIN_MESSAGE_MAP(ImageSelectorDlg, CDialog)
    ON_WM_NCACTIVATE()
    ON_BN_CLICKED(IDC_MODIFY_BUTTON, OnBnClickedModify)
    ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnBnClickedRemove)
END_MESSAGE_MAP()


ImageSelectorDlg::ImageSelectorDlg(const CString& filename, std::function<void(const CString&)> modification_callback,
                               std::optional<CRect> rect_above_desired_window, CWnd* pParent/*= nullptr*/)
    :   CDialog(ImageSelectorDlg::IDD, pParent),
        m_filename(filename),
        m_modificationCallback(std::move(modification_callback)),
        m_rectAboveDesiredWindow(std::move(rect_above_desired_window))
{
    ASSERT(m_modificationCallback);
}


BOOL ImageSelectorDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CRect rect_dialog;
    GetWindowRect(&rect_dialog);

    // the minimum width of the dialog content will be the designed dialog width from the Modify to Remove buttons;
    // the height will be the height of the buttons
    CWnd* modify_button = GetDlgItem(IDC_MODIFY_BUTTON);
    CRect rect_modify_button;
    modify_button->GetWindowRect(&rect_modify_button);

    CWnd* remove_button = GetDlgItem(IDC_REMOVE_BUTTON);
    CRect rect_remove_button;
    remove_button->GetWindowRect(&rect_remove_button);

    int margin = rect_modify_button.left - rect_dialog.left;
    int button_offset_from_center = ( rect_dialog.left + rect_dialog.right ) / 2 - rect_modify_button.right;
    int button_offset_from_bottom = rect_dialog.bottom - rect_modify_button.bottom;

    int initial_dialog_content_width = rect_remove_button.right - rect_modify_button.left;
    int initial_dialog_content_height = rect_remove_button.Height();

    int dialog_content_width = initial_dialog_content_width;
    int dialog_content_height = initial_dialog_content_height;

    // load the image and then adjust the dialog size
    m_image = ImageManager::GetImage(m_filename);

    if( m_image != nullptr )
    {
        double image_resample_scale = 1;

        if( m_image->GetWidth() > dialog_content_width )
        {
            // if the window is being anchored based on a parent window, use that window
            // width to inform the maximum width of the image
            int parent_width = m_rectAboveDesiredWindow.has_value() ? m_rectAboveDesiredWindow->Width() :
                                                                      (int)Screen::GetMaxDisplayWidth();
            dialog_content_width = std::min(m_image->GetWidth(), parent_width);

            image_resample_scale = std::min((double)dialog_content_width / m_image->GetWidth(), 1.0);
        }

        // the height cannot exceed the screen height
        dialog_content_height = std::min(initial_dialog_content_height + margin + (int)( m_image->GetHeight() * image_resample_scale ),
                                         (int)Screen::GetMaxDisplayHeight()) - initial_dialog_content_height;

        double image_resample_height_scale = (double)( dialog_content_height - margin - initial_dialog_content_height ) / m_image->GetHeight();

        // now adjust the width if necessary
        if( image_resample_height_scale < image_resample_scale )
        {
            image_resample_scale = image_resample_height_scale;
            dialog_content_width = std::min(dialog_content_width, (int)( m_image->GetWidth() * image_resample_scale ));
        }

        dialog_content_width = std::max(dialog_content_width, initial_dialog_content_width);

        // scale the image
        int scaled_image_width = (int)( m_image->GetWidth() * image_resample_scale );
        int scaled_image_height = (int)( m_image->GetHeight() * image_resample_scale );

        m_image = ImageManager::GetResizedImage(m_image, scaled_image_width, scaled_image_height, GetSysColorBrush(COLOR_3DFACE));

        // draw the image
        int offset_x = ( dialog_content_width - m_image->GetWidth() ) / 2;

        CRect image_rect(margin + offset_x, margin, margin + offset_x + scaled_image_width, margin + scaled_image_height);

        if( m_imageStatic.Create(nullptr, WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | SS_REALSIZECONTROL, image_rect, this) )
            m_imageStatic.SetBitmap((HBITMAP)*m_image);
    }


    // adjust the full dialog size
    rect_dialog.right += dialog_content_width - initial_dialog_content_width;
    rect_dialog.bottom += dialog_content_height - initial_dialog_content_height;


    // move the buttons
    int dialog_center = rect_dialog.Width() / 2;
    int button_top = rect_dialog.Height() - button_offset_from_bottom - rect_modify_button.Height();

    modify_button->MoveWindow(dialog_center - button_offset_from_center - rect_modify_button.Width(), button_top, rect_modify_button.Width(), rect_modify_button.Height(), FALSE);
    remove_button->MoveWindow(dialog_center + button_offset_from_center, button_top, rect_remove_button.Width(), rect_remove_button.Height(), FALSE);


    int dialog_x;
    int dialog_y;

    // show the dialog either underneath the desired window...
    if( m_rectAboveDesiredWindow.has_value() )
    {
        dialog_x = m_rectAboveDesiredWindow->left + ( m_rectAboveDesiredWindow->Width() - rect_dialog.Width() ) / 2;
        dialog_y = m_rectAboveDesiredWindow->bottom;

        // show the dialog above the desired position if necessary to show the entire dialog
        if( ( dialog_y + rect_dialog.Height() ) > Screen::GetMaxViewableHeight() )
            dialog_y = Screen::GetMaxViewableHeight() - rect_dialog.Height();
    }

    // ...or in the center of the screen
    else
    {
        dialog_x = Screen::GetMaxDisplayCenterPoint().x - ( rect_dialog.Width() / 2 );
        dialog_y = Screen::GetMaxDisplayCenterPoint().y - ( rect_dialog.Height() / 2 );
    }

    MoveWindow(dialog_x, dialog_y, rect_dialog.Width(), rect_dialog.Height(), FALSE);

    return TRUE;
}


BOOL ImageSelectorDlg::OnNcActivate(BOOL bActive)
{
    // close the dialog as soon as the user moves off of it
    if( !bActive )
        PostMessage(WM_CLOSE);

    return CDialog::OnNcActivate(bActive);    
}


CString ImageSelectorDlg::SelectFile(const CString& filename/* = CString()*/)
{
    ImageFileDialog image_file_dlg(filename);
    return ( image_file_dlg.DoModal() == IDOK ) ? image_file_dlg.GetPathName() : CString();
}


void ImageSelectorDlg::OnBnClickedModify()
{
    CString new_filename = SelectFile(m_filename);

    if( !new_filename.IsEmpty() && m_filename != new_filename )
        m_modificationCallback(new_filename);

    PostMessage(WM_CLOSE);
}


void ImageSelectorDlg::OnBnClickedRemove()
{
    m_modificationCallback(CString());
    PostMessage(WM_CLOSE);
}
