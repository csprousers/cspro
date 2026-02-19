#include "StdAfx.h"
#include "ResizableDlg.h"
#include "DynamicLayoutControlResizer.h"
#include "SettingsDb.h"


// --------------------------------------------------------------------------
// ResizableDlgBase
// --------------------------------------------------------------------------

template<typename DialogT>
void ResizableDlgBase<DialogT>::SerializeDialogSize(std::string serialize_key)
{
    ASSERT(m_serializationSettings == nullptr && !serialize_key.empty());

    std::string serialize_key_y = serialize_key + "-y";
    serialize_key.append("-x");

    m_serializationSettings = std::make_unique<std::tuple<SettingsDb, std::string, std::string>>(
        SettingsDb("States.db", "dialogs"),
        std::move(serialize_key),
        std::move(serialize_key_y));
}

template CLASS_DECL_ZUTILO void ResizableDlgBase<CDialog>::SerializeDialogSize(std::string serialize_key);
template CLASS_DECL_ZUTILO void ResizableDlgBase<CDialogEx>::SerializeDialogSize(std::string serialize_key);


template<typename DialogT>
BOOL ResizableDlgBase<DialogT>::OnInitDialog()
{
    if( !DialogT::OnInitDialog() )
        return FALSE;

    CRect rect;
    GetWindowRect(rect);
    m_minimumSize = rect.Size();

    // potentially restore the size
    if( m_serializationSettings != nullptr )
    {
        constexpr double MaxProportionScreenAllowed = 0.9;
        const int* width;
        const int* height;

        if( ( ( width = std::get<0>(*m_serializationSettings).Read<int*>(std::get<1>(*m_serializationSettings)) ) != nullptr ) &&
            ( ( height = std::get<0>(*m_serializationSettings).Read<int*>(std::get<2>(*m_serializationSettings)) ) != nullptr ) )
        {
            const int safe_width = std::min(*width, static_cast<int>(MaxProportionScreenAllowed * GetSystemMetrics(SM_CXSCREEN)));
            const int safe_height = std::min(*height, static_cast<int>(MaxProportionScreenAllowed * GetSystemMetrics(SM_CYSCREEN)));

            // only resize if the width or height is different from the default sizes
            if( safe_width > m_minimumSize.cx || safe_height > m_minimumSize.cy )
                PostMessage(UWM::UtilO::ResizableDlgRestoreSize, *width, *height);
        }
    }

    return TRUE;
}


template<typename DialogT>
void ResizableDlgBase<DialogT>::OnGetMinMaxInfo(MINMAXINFO FAR* const lpMMI)
{
    DialogT::OnGetMinMaxInfo(lpMMI);

    lpMMI->ptMinTrackSize.x = std::max(lpMMI->ptMinTrackSize.x, m_minimumSize.cx);
    lpMMI->ptMinTrackSize.y = std::max(lpMMI->ptMinTrackSize.y, m_minimumSize.cy);
}


template<typename DialogT>
void ResizableDlgBase<DialogT>::OnDestroy()
{
    // potentially save the size
    if( m_serializationSettings != nullptr )
    {
        CRect rect;
        GetWindowRect(rect);

        std::get<0>(*m_serializationSettings).Write(std::get<1>(*m_serializationSettings), rect.Width());
        std::get<0>(*m_serializationSettings).Write(std::get<2>(*m_serializationSettings), rect.Height());
    }

    DialogT::OnDestroy();
}


template<typename DialogT>
LRESULT ResizableDlgBase<DialogT>::OnRestoreSize(const WPARAM wParam, const LPARAM lParam)
{
    // a sizing-only call to SetWindowPos was initially done in OnInitDialog,
    // with the controls properly resized, but then when the dialog was resized, at
    // least while using DynamicLayoutResizableDlg, the controls did not size properly;
    // this only happened when the dialog was resized in OnInitDialog, but posting a message
    // to resize the dialog here seems to work, even though it leads to a flicker as the
    // dialog is resized
    const int new_width = static_cast<int>(wParam);
    const int new_height = static_cast<int>(lParam);

    // the sizing-only only call has been changed to a sizing and moving call so as to center
    // the dialog based on where it initially was displayed (which was not an issue when this
    // was done in OnInitDialog)
    CRect rect;
    GetWindowRect(&rect);
    const int diff_x = new_width - rect.Width();
    const int diff_y = new_height - rect.Height();

    SetWindowPos(
        nullptr,
        std::max<int>(0, rect.left - ( diff_x / 2 )),
        std::max<int>(0, rect.top - ( diff_y / 2 )),
        new_width,
        new_height,
        SWP_NOACTIVATE | SWP_NOZORDER
    );

    return 1;
}



// --------------------------------------------------------------------------
// ResizableDlg
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(ResizableDlg, CDialog)
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::UtilO::ResizableDlgRestoreSize, OnRestoreSize)
END_MESSAGE_MAP()



// --------------------------------------------------------------------------
// ResizableDlgEx
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(ResizableDlgEx, CDialogEx)
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::UtilO::ResizableDlgRestoreSize, OnRestoreSize)
END_MESSAGE_MAP()



// --------------------------------------------------------------------------
// DynamicLayoutResizableDlg
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(DynamicLayoutResizableDlg, ResizableDlg)
    ON_WM_SIZE()
END_MESSAGE_MAP()


DynamicLayoutResizableDlg::~DynamicLayoutResizableDlg()
{
    delete m_dynamicLayoutControlResizer;
}


BOOL DynamicLayoutResizableDlg::OnInitDialog()
{
    CRect rect;
    GetClientRect(rect);
    m_initialClientSize = rect.Size();

    return __super::OnInitDialog();
}


void DynamicLayoutResizableDlg::OnSize(const UINT nType, const int cx, const int cy)
{
    if( m_dynamicLayoutControlResizer == nullptr && m_initialClientSize.has_value() )
        m_dynamicLayoutControlResizer = new DynamicLayoutControlResizer(*this, GetDynamicLayoutControls(), &*m_initialClientSize);

    __super::OnSize(nType, cx, cy);

    if( m_dynamicLayoutControlResizer != nullptr )
        m_dynamicLayoutControlResizer->OnSize(cx, cy);
}
