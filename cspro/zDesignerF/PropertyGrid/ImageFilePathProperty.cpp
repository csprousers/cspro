#include "StdAfx.h"
#include "FilenamePropertyBase.h"
#include <zUtilO/WindowHelpers.h>
#include <zMultimediaO/ImageSelectorDlg.h>

namespace PropertyGrid { class ImageFilePathProperty; }


namespace
{
    constexpr int ThumbnailMargin = 1;
}


class PropertyGrid::ImageFilePathProperty : public FilenamePropertyBase<Type::ImageFilePath>
{
public:
    ImageFilePathProperty(std::shared_ptr<PropertyGridData<Type::ImageFilePath>> data);

protected:
    // CMFCPropertyGridColorProperty overrides
    void OnDrawValue(CDC* pDC, CRect rect) override;
    void AdjustInPlaceEditRect(CRect& rectEdit, CRect& rectSpin) override;

    // Property overrides
    void HandleButtonClick() override;

private:
    std::shared_ptr<const CImage> m_image;
    std::unique_ptr<WindowHelpers::ModelessDialogHolder> m_imageViewerDlgHolder;
};


CMFCPropertyGridProperty* PropertyGrid::PropertyBuilder<PropertyGrid::Type::ImageFilePath>::ToProperty()
{
    return new ImageFilePathProperty(m_data);
}


PropertyGrid::ImageFilePathProperty::ImageFilePathProperty(std::shared_ptr<PropertyGridData<Type::ImageFilePath>> data)
    :   FilenamePropertyBase<Type::ImageFilePath>(std::move(data))
{
}


void PropertyGrid::ImageFilePathProperty::OnDrawValue(CDC* const pDC, CRect rect)
{
    // if a file path is provided, create a thumbnail equal to the height of the row
    Type::ImageFilePath value = FromOleVariant(GetValue());

    if( !value.filename.IsEmpty() )
    {
        m_image = ImageManager::GetImage(UTF8_TODO::GetUtf8(value.filename));

        if( m_image != nullptr )
        {
            const int max_size = rect.Height() - 2 * ThumbnailMargin;

            m_image = ImageManager::GetResizedImageToSize(*m_image, max_size, GetSysColorBrush(COLOR_WINDOW));

            // draw the image
            HBITMAP hBitmap = (HBITMAP)*m_image;

            CDC memDC;
            memDC.CreateCompatibleDC(pDC);

            HANDLE hOldObject = memDC.SelectObject(hBitmap);

            pDC->BitBlt(rect.left + ThumbnailMargin,
                        rect.top + ThumbnailMargin + ( max_size - m_image->GetHeight() ) / 2,
                        m_image->GetWidth(), m_image->GetHeight(),
                        &memDC, 0, 0, SRCCOPY);

            memDC.SelectObject(hOldObject);

            rect.left += m_image->GetWidth() + 2 * ThumbnailMargin;
        }
    }

    FilenamePropertyBase<Type::ImageFilePath>::OnDrawValue(pDC, rect);
}


void PropertyGrid::ImageFilePathProperty::AdjustInPlaceEditRect(CRect& rectEdit, CRect& rectSpin)
{
    FilenamePropertyBase<Type::ImageFilePath>::AdjustInPlaceEditRect(rectEdit, rectSpin);

    // move the text editor to the right of the thumbnail
    if( m_image != nullptr )
        rectEdit.left += m_image->GetWidth() + 2 * ThumbnailMargin;
}


void PropertyGrid::ImageFilePathProperty::HandleButtonClick()
{
    std::optional<Type::ImageFilePath> new_value;

    if( m_data->button_click_callback )
    {
        new_value = m_data->button_click_callback();
    }

    else
    {
        CRect property_rect = m_Rect;
        m_pWndList->ClientToScreen(&property_rect);

        Type::ImageFilePath value = FromOleVariant(GetValue());

        if( PortableFunctions::FileIsRegular(value.filename) )
        {
            ImageSelectorDlg* const image_viewer_dlg = new ImageSelectorDlg(
                UTF8_TODO::GetUtf8(value.filename),
                [&](const std::string& file_path)
                {
                    SetValue(ToOleVariant(FromOleVariant(COleVariant(UTF8_TODO::GetCString(file_path)))));
                    m_pWndList->OnPropertyChanged(this);
                },
                property_rect,
                m_pWndList);

            if( !image_viewer_dlg->Create(ImageSelectorDlg::IDD, m_pWndList) )
            {
                ASSERT(false);
                delete image_viewer_dlg;
                return;
            }

            // show a modeless dialog that displays the image and allows it to be modified
            m_imageViewerDlgHolder = std::make_unique<WindowHelpers::ModelessDialogHolder>(image_viewer_dlg);
        }

        // if no file exists, simply show the image file selector dialog
        else
        {
            value.filename = UTF8_TODO::GetCString(ImageSelectorDlg::SelectFile());

            if( !value.filename.IsEmpty() )
                new_value = value;
        }
    }

    if( new_value.has_value() )
    {
        SetValue(ToOleVariant(*new_value));
        m_pWndList->OnPropertyChanged(this);
    }
}
