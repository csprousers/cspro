#include "StdAfx.h"
#include "TableToolbarDropdown.h"
#include "TableToolbarButton.h"

namespace {
    const int num_rows = 5;
    const int num_cols = 5;
    const int box_margin = 3;
}

TableToolbarDropdown::TableToolbarDropdown()
    : m_parent(nullptr),
      m_selected_rows(0),
      m_selected_columns(0)
{
}

BEGIN_MESSAGE_MAP(TableToolbarDropdown, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void TableToolbarDropdown::OnPaint()
{
    CPaintDC paintDC(this);

    CRect client_rect;
    GetClientRect(client_rect);
    CDC memDC;
    memDC.CreateCompatibleDC(&paintDC);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&paintDC, client_rect.Width(), client_rect.Height());
    CBitmap* old_bitmap = memDC.SelectObject(&bitmap);
    memDC.FillSolidRect(client_rect, GetSysColor(COLOR_BTNFACE));

    DrawDimensionText(memDC);
    DrawBoxes(memDC);

    paintDC.BitBlt(client_rect.left, client_rect.top, client_rect.Width(), client_rect.Height(), &memDC, client_rect.left, client_rect.top, SRCCOPY);
    memDC.SelectObject(old_bitmap);
}

void TableToolbarDropdown::DrawBoxes(CDC& dc)
{
    CPen unselected_pen(PS_SOLID, 1, RGB(125, 125, 125));
    CPen selected_pen(PS_SOLID, 1, RGB(195, 217, 255));
    CBrush background_brush(GetSysColor(COLOR_BTNFACE));
    CBrush selected_brush(RGB(221, 234, 251));

    CBrush* old_brush = dc.SelectObject(&background_brush);
    CPen* old_pen = dc.SelectObject(&unselected_pen);

    const CSize box_size = GetBoxSize();
    const int margin = GetMargin();
    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            int left = margin + c * box_size.cx;
            int top = m_text_height + margin + r * box_size.cy;
            CRect rect(left, top, left + box_size.cx, top + box_size.cy);
            if (r < m_selected_rows && c < m_selected_columns) {
                dc.SelectObject(&selected_brush);
                dc.SelectObject(&selected_pen);
            }
            else {
                dc.SelectObject(&background_brush);
                dc.SelectObject(&unselected_pen);
            }
            rect.DeflateRect(box_margin, box_margin);
            dc.Rectangle(rect);
        }
    }

    dc.SelectObject(old_brush);
    dc.SelectObject(old_pen);
}

void TableToolbarDropdown::DrawDimensionText(CDC& dc)
{
    CString text = FormatText(L"%d x %d", m_selected_columns, m_selected_rows);
    CFont* old_font = dc.SelectObject(&GetGlobalData()->fontRegular);
    dc.TextOut(GetMargin(), GetMargin(), text);
    dc.SelectObject(old_font);
}

int TableToolbarDropdown::GetMargin() const
{
    return CMFCVisualManager::GetInstance()->GetMenuImageMargin();
}

CSize TableToolbarDropdown::GetBoxSize() const
{
    return CMFCToolBar::GetMenuImageSize();
}

BOOL TableToolbarDropdown::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

void TableToolbarDropdown::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
    ReleaseCapture();
    ASSERT(m_parent);

    CRect client_rect;
    GetClientRect(client_rect);
    client_rect.top += m_text_height;
    if (client_rect.PtInRect(point))
        m_parent->OnTableDimensionsSelected(CSize(m_selected_columns, m_selected_rows));
    m_parent->CloseDropdown();
}

void TableToolbarDropdown::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
    const int margin = GetMargin();
    const CSize box_size = GetBoxSize();
    m_selected_rows = point.y < margin ? 0 : (point.y - margin - box_margin - m_text_height) / box_size.cy + 1;
    m_selected_columns = point.x < margin ? 0 : (point.x - margin - box_margin) / box_size.cx + 1;
    if (m_selected_rows == 0 || m_selected_columns == 0)
        m_selected_rows = m_selected_columns = 1;
    
    Invalidate();
}

bool TableToolbarDropdown::Create(TableToolbarButton* parent)
{
    m_parent = parent;

    WNDCLASS wndClass;
    wndClass.style = CS_DBLCLKS;
    wndClass.lpfnWndProc = AfxWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = AfxGetInstanceHandle();
    wndClass.hIcon = 0;
    wndClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = _T("CSProTableToolbarDropdown");

    if (!AfxRegisterClass(&wndClass))
    {
        return false;
    }
    
    CRect button_rect = parent->Rect();
    CWnd* parent_wnd = parent->GetParentWnd();
    parent_wnd->ClientToScreen(button_rect);

    CDC dc_screen;
    dc_screen.Attach(::GetDC(NULL));
    CFont* old_font = dc_screen.SelectObject(&GetGlobalData()->fontRegular);
    TEXTMETRIC tm;
    dc_screen.GetTextMetrics(&tm);
    dc_screen.SelectObject(old_font);
    m_text_height = tm.tmHeight + GetMargin();

    CSize size = CalcBoxesSize(num_rows, num_cols);
    size.cy += m_text_height;
    CRect dropdown_rect(button_rect.left, button_rect.bottom, button_rect.left + size.cx, button_rect.bottom + size.cy);

    if (!CWnd::CreateEx(WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE, wndClass.lpszClassName,
                        L"", WS_BORDER| WS_VISIBLE | WS_POPUP, dropdown_rect, parent_wnd, 0, NULL))
    {
        return false;
    }

    SetCapture();

    return true;
}

CSize TableToolbarDropdown::CalcBoxesSize(int rows, int columns) const
{
    const CSize image_size = GetBoxSize();
    return CSize(GetMargin() * 2 + rows * image_size.cx, GetMargin() * 2 + columns * image_size.cy);
}
