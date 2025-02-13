#include "StdAfx.h"
#include "RosterCells.h"


IMPLEMENT_DYNAMIC(CDECell, CObject)
IMPLEMENT_DYNAMIC(CDEFreeCell, CDECell)


// --------------------------------------------------------------------------
//
// CDECell
//
// --------------------------------------------------------------------------

CDECell::CDECell() // FORM_TODO remove if no longer derived from CObject
{
}


CDECell::CDECell(const CDECell& rhs) // FORM_TODO remove if no longer derived from CObject
    :   m_boxSet(rhs.m_boxSet),
        m_textSet(rhs.m_textSet)
{
}


CDECell& CDECell::operator=(const CDECell& rhs) // FORM_TODO remove if no longer derived from CObject
{
    m_boxSet = rhs.m_boxSet;
    m_textSet = rhs.m_textSet;

    return *this;
}


void CDECell::MoveContent(CDECell& rhs)
{
    if( this == &rhs )
    {
        ASSERT(false);
        return;
    }

    // boxes
    for( const auto& box : rhs.m_boxSet.GetBoxes().GetSharedPointerVector() )
        m_boxSet.AddBox(box);

    rhs.m_boxSet.RemoveAllBoxes();

    // texts
    for( const auto& text : rhs.m_textSet.GetTexts().GetSharedPointerVector() )
        m_textSet.AddText(text);

    rhs.m_textSet.RemoveAllTexts();
}


void CDECell::Save(CSpecFile& frmFile) const
{
    // boxes
    if( m_boxSet.GetNumBoxes() > 0 )
    {
        for( const CDEBox& box : m_boxSet.GetBoxes() )
            frmFile.PutLine(FRM_CMD_BOX, box.GetSerializedText());

        // only give a blank line if there were boxes written
        frmFile.PutLine(_T("  "));
    }


    // texts
    for( const CDEText& text : m_textSet.GetTexts() )
    {
        // blank line to sep the groups
        frmFile.PutLine(_T("  "));

        text.Save(frmFile);
    }
}


void CDECell::serialize(Serializer& ar)
{
    ar & m_boxSet
       & m_textSet;
}



// --------------------------------------------------------------------------
//
// CDEFreeCell
//
// --------------------------------------------------------------------------

CDEFreeCell::CDEFreeCell(int row/* = 0*/, int column/* = 0*/)
    :   m_row(row),
        m_column(column)
{
}


bool CDEFreeCell::Build(CSpecFile& frmFile, bool bSilent/* = false */)
{
    CString csCmd, csArg;
    bool bDone = false, rtnVal = false;

    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK)
    {
        ASSERT (csCmd.GetLength() > 0);

        if (csCmd.CompareNoCase(FRM_CMD_ROW) == 0)
        {
            SetRow( _ttoi (csArg) );
        }

        else if (csCmd.CompareNoCase(FRM_CMD_COL) == 0)
        {
            SetColumn( _ttoi (csArg) );
        }

        else if (csCmd.CompareNoCase(FRM_CMD_BOX) == 0)
        {
            GetBoxSet().AddBox(std::make_shared<CDEBox>(csArg));
        }

        else if (csCmd[0] == '[')
        {
            if (csCmd.CompareNoCase(HEAD_CELLEND) == 0) // [EndCell]
            {
                bDone = true;
                rtnVal = true;
            }

            else if (csCmd.CompareNoCase(HEAD_TEXT) == 0) // [Text]
            {
                auto text = std::make_shared<CDEText>();

                if (text->Build(frmFile))                   // if the build went ok
                    GetTextSet().AddText(std::move(text));  // add the item to the page
            }

            else // got some other kind of blk, not allowed, bail
            {
                bDone = true;
            }
        }

        else
        {                      // Incorrect attribute
            if (!bSilent)
                ErrorMessage::Display(FormatText(_T("Incorrect [Cell] attribute\n\n%s"), (LPCTSTR) csCmd));

            rtnVal = false;
        }
    }

    return rtnVal;
}


void CDEFreeCell::Save(CSpecFile& frmFile, bool bWriteHeadingInfo /* = true*/) const
{
    if (bWriteHeadingInfo) {
        frmFile.PutLine(HEAD_CELL);
        frmFile.PutLine(FRM_CMD_ROW, GetRow());
        frmFile.PutLine(FRM_CMD_COL, GetColumn());
    }

    CDECell::Save(frmFile);

    if (bWriteHeadingInfo) {
        frmFile.PutLine(HEAD_CELLEND);
        frmFile.PutLine(_T("  "));
    }
}


void CDEFreeCell::serialize(Serializer& ar)
{
    CDECell::serialize(ar);

    ar & m_row
       & m_column;
}
