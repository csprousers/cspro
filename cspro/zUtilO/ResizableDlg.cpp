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
            ( ( height = std::get<0>(*m_serializationSettings).Read<int*>(std::get<2>(*m_serializationSettings)) ) != nullptr ) &&
            ( *width >= m_minimumSize.cx && *width <= static_cast<int>(MaxProportionScreenAllowed * GetSystemMetrics(SM_CXSCREEN)) ) &&
            ( *height >= m_minimumSize.cy && *height <= static_cast<int>(MaxProportionScreenAllowed * GetSystemMetrics(SM_CYSCREEN)) ) )
        {
            SetWindowPos(nullptr, 0, 0, *width, *height, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
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



// --------------------------------------------------------------------------
// ResizableDlg
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(ResizableDlg, CDialog)
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
END_MESSAGE_MAP()



// --------------------------------------------------------------------------
// ResizableDlgEx
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(ResizableDlgEx, CDialogEx)
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
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
    m_dialogInitialized = true;
    return __super::OnInitDialog();
}


void DynamicLayoutResizableDlg::OnSize(const UINT nType, const int cx, const int cy)
{
    if( m_dynamicLayoutControlResizer == nullptr && m_dialogInitialized )
        m_dynamicLayoutControlResizer = new DynamicLayoutControlResizer(*this, GetDynamicLayoutControls());

    __super::OnSize(nType, cx, cy);

    if( m_dynamicLayoutControlResizer != nullptr )
        m_dynamicLayoutControlResizer->OnSize(cx, cy);
}
