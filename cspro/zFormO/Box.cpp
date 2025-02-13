#include "StdAfx.h"
#include "Box.h"


// --------------------------------------------------------------------------
// CDEBox
// --------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CDEBox, CObject)


CDEBox::CDEBox(CRect rect/* = CRect()*/, BoxType box_type/* = BoxType::Etched*/)
    :   m_box(rect),
        m_type(box_type)
{
}


CDEBox::CDEBox(const CDEBox& rhs) // FORM_TODO remove if no longer derived from CObject
    :   m_box(rhs.m_box),
        m_type(rhs.m_type)
{
}


CDEBox::CDEBox(wstring_view serialized_text)
{
    // if a string's being passed in, then it's because we're calling the constructor
    // from the Build func and it's in the fmt "x1,y1,x2,y2,BoxType"
    std::vector<std::wstring> components = SO::SplitString(serialized_text, SPACE_COMMA_STR);

    if( components.size() < 5 )
    {
        m_type = BoxType::Etched;
    }

    else
    {
        m_box.SetRect(_ttoi(components[0].c_str()), _ttoi(components[1].c_str()), _ttoi(components[2].c_str()), _ttoi(components[3].c_str()));

        const std::wstring& box_type_string = components[4];
        m_type = SO::EqualsNoCase(box_type_string, FRM_CMD_ETCHEDBOX) ? BoxType::Etched :
                 SO::EqualsNoCase(box_type_string, FRM_CMD_RAISEDBOX) ? BoxType::Raised :
                 SO::EqualsNoCase(box_type_string, FRM_CMD_THINBOX)   ? BoxType::Thin :
                 SO::EqualsNoCase(box_type_string, FRM_CMD_THICKBOX)  ? BoxType::Thick :
                                                                        ReturnProgrammingError(BoxType::Etched);
    }
}


CDEBox& CDEBox::operator=(const CDEBox& rhs) // FORM_TODO remove if no longer derived from CObject
{
    m_box = rhs.m_box;
    m_type = rhs.m_type;

    return *this;
}


CString CDEBox::GetSerializedText() const
{
    const TCHAR* box_string = ( m_type == BoxType::Etched ) ? FRM_CMD_ETCHEDBOX :
                              ( m_type == BoxType::Raised ) ? FRM_CMD_RAISEDBOX : 
                              ( m_type == BoxType::Thin )   ? FRM_CMD_THINBOX :
                              ( m_type == BoxType::Thick )  ? FRM_CMD_THICKBOX :
                                                              ReturnProgrammingError(FRM_CMD_ETCHEDBOX);

    return FormatText(_T("%d,%d,%d,%d,%s"), (int)m_box.left, (int)m_box.top, (int)m_box.right, (int)m_box.bottom, box_string);
}


void CDEBox::serialize(Serializer& ar)
{
    ar & m_box;
    ar.SerializeEnum(m_type);
}



void CDEBox::Draw(CDC* pDC, const CPoint* offset/* = nullptr*/) const
{
    CRect rect = m_box;

    if( offset != nullptr )
        rect.OffsetRect(*offset);

    if( m_type == BoxType::Etched )
    {        
        pDC->DrawEdge(&rect, EDGE_ETCHED, BF_RECT);
    }

    else if( m_type == BoxType::Raised )
    {
        pDC->DrawEdge(&rect, EDGE_BUMP, BF_RECT);
    }

    else if( m_type == BoxType::Thin )
    {
        // the box is really a line (FrameRect won't work if height or width = 0)
        if( rect.Height() == 0 || rect.Width() == 0 )
        {            
            pDC->SelectStockObject(BLACK_PEN);
            pDC->MoveTo(rect.TopLeft());
            pDC->LineTo(rect.BottomRight());
        }

        else
        {
            pDC->SelectStockObject(BLACK_BRUSH);
            pDC->FrameRect(rect, pDC->GetCurrentBrush());
        }
    }

    else 
    {
        // because i'm inflating the rect, don't have prob as w/thin line
        ASSERT(m_type == BoxType::Thick);

        pDC->SelectStockObject(BLACK_BRUSH);
        pDC->FrameRect(rect, pDC->GetCurrentBrush());
        rect.InflateRect(1, 1);
        pDC->FrameRect(rect, pDC->GetCurrentBrush());
    }
}



// --------------------------------------------------------------------------
// CDEBoxSet
// --------------------------------------------------------------------------

void CDEBoxSet::InsertBox(size_t index, std::shared_ptr<CDEBox> box)
{
    ASSERT(index <= m_boxes.size() && box != nullptr);
    m_boxes.insert(m_boxes.begin() + index, std::move(box));
}


std::shared_ptr<CDEBox> CDEBoxSet::RemoveBox(size_t index)
{
    auto box_index = m_boxes.begin() + index;
    ASSERT(box_index < m_boxes.end());

    std::shared_ptr<CDEBox> removed_box = *box_index;

    m_boxes.erase(box_index);

    return removed_box;
}


std::shared_ptr<CDEBox> CDEBoxSet::RemoveBox(const CDEBox& box)
{
    const auto& search = std::find_if(m_boxes.begin(), m_boxes.end(),
                                      [&](const auto& this_box) { return &box == this_box.get(); });

    return ( search != m_boxes.end() ) ? RemoveBox(std::distance(m_boxes.begin(), search)) :
                                         nullptr;
}


void CDEBoxSet::RemoveAllBoxes()
{
    m_boxes.clear();
}


void CDEBoxSet::serialize(Serializer& ar)
{
    ar & m_boxes;
}


void CDEBoxSet::Draw(CDC* pDC, const CPoint* offset/* = nullptr*/) const
{
    for( const CDEBox& box : GetBoxes() )
        box.Draw(pDC, offset);
}
