#include "StdAfx.h"
#include "RosterColumn.h"


IMPLEMENT_DYNAMIC(CDECol, CObject)


CDECol::CDECol()
    :   m_iWidth(0),
        m_headerText(std::make_shared<CDEText>())
{
}


CDECol::CDECol(const CDECol& rhs)
    :   m_iWidth(rhs.m_iWidth),
        m_Offset(rhs.m_Offset),
        m_columnCell(rhs.m_columnCell),
        m_headerText(std::make_shared<CDEText>(*rhs.m_headerText))        
{
    for( int i = 0; i < rhs.GetNumFields(); ++i )
        AddField(rhs.GetField(i));
}


CDECol::~CDECol()
{
    RemoveAllFields();
}


void CDECol::RemoveFieldAt(int i)
{
    m_aField.erase(m_aField.begin() + i);
}

void CDECol::RemoveAllFields()
{
    m_aField.clear();
}

void CDECol::ResetSizeAndHeader()
{
    CString newLabel;
    for (int i = 0; i < GetNumFields(); ++i)
    {
        CDEField* pColFld = GetField(i);
        if (pColFld->UseUnicodeTextBox()) {
            pColFld->SetUnicodeTextBoxSize(pColFld->GetDims().Size());
        }
        pColFld->SetDims(0, 0, 0, 0);
        newLabel += pColFld->GetLabel();
        if (i < GetNumFields() - 1)
            newLabel += _T(" + ");
    }

    m_headerText->SetDims(0, 0, 0, 0);  // force a recalc
    m_headerText->SetText(newLabel);
}

int CDECol::GetFieldIndex(CDEField* pField) const
{
    for (int i = 0; i < GetNumFields(); ++i)
    {
        if (GetField(i) == pField)
            return i;
    }

    return -1;
}

void CDECol::SetOffset(CString cs)
{
    TCHAR*  pszArgs = cs.GetBuffer(cs.GetLength());
    TCHAR*  pszArg;

    pszArg = strtoken(pszArgs, SPACE_COMMA_STR, NULL);
    int x = _ttoi (pszArg);

    pszArg = strtoken(NULL,    SPACE_COMMA_STR, NULL);
    int y = _ttoi (pszArg);

    SetOffset (CPoint (x,y));

    cs = strtoken(NULL, _T("\""), NULL);
}


bool CDECol::Build(CSpecFile& frmFile, CDEGroup* pParentGroup, bool bSilent/* = false*/)
{
    CString csCmd, csArg;
    bool bDone = false, rtnVal = false;

    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK)
    {
        ASSERT (csCmd.GetLength() > 0);

        if (csCmd.CompareNoCase(_T("Width")) == 0)
        {
            SetWidth ( _ttoi (csArg) );
        }

        else if (csCmd.CompareNoCase(_T("Offset")) == 0)
        {
            SetOffset (csArg);
        }

        else if (csCmd.CompareNoCase(FRM_CMD_BOX) == 0)
        {
            m_columnCell.GetBoxSet().AddBox(std::make_shared<CDEBox>(csArg));
        }

        else if (csCmd[0] == '[')
        {
            if (csCmd.CompareNoCase(HEAD_COLUMNEND) == 0) // [EndColumn]
            {
                bDone = true;
                rtnVal = true;
            }

            else if (csCmd.CompareNoCase(HEAD_HDRTEXT) == 0)  // [HeaderText]
            {
                m_headerText->Build(frmFile);
            }

            else if (csCmd.CompareNoCase(HEAD_TEXT) == 0) // [Text]
            {
                auto text = std::make_shared<CDEText>();

                if (text->Build(frmFile))                   // if the build went ok
                    m_columnCell.GetTextSet().AddText(std::move(text));  // add the text item to the cell
            }

            else if (csCmd.CompareNoCase(HEAD_FIELD) == 0) // [Field]
            {
                CDEField* pField = new CDEField();      // trash it during CDEGroup::Build()

                if (pField->Build(frmFile))     // if the build went ok
                {
                    pField->SetParent (pParentGroup);

                    AddField (pField);      // 1st add the item to the roster
                }

                else
                {
                    delete pField;
                    bDone = true;
                }
            }

            else // got some other kind of blk, not allowed, bail
            {
                bDone = true;
            }
        }

        else
        {                      // Incorrect attribute
            if (!bSilent)
                ErrorMessage::Display(FormatText(_T("Incorrect [Column] attribute\n\n%s"), (LPCTSTR)csCmd));

            rtnVal = false;
        }
    }

    return rtnVal;
}


void CDECol::Save(CSpecFile& frmFile) const
{
    frmFile.PutLine (HEAD_COLUMN);

    if (m_iWidth>0)  {
        frmFile.PutLine (_T("Width"), GetWidth());          // csc 12/21/00
    }

    // save col's boxes, found w/in member m_Cell
    m_columnCell.Save(frmFile);

    if ( !m_headerText->GetText().IsEmpty())  {
        frmFile.PutLine (_T("  "));
        frmFile.PutLine (HEAD_HDRTEXT);
        m_headerText->Save(frmFile, false); // don't want [Text] written out
    }

    // finally, save the [CDEField]s w/in the cell
    for (int i = 0; i < GetNumFields(); i++)  {
        GetField(i)->Save(frmFile, true);
    }

    frmFile.PutLine (HEAD_COLUMNEND);
    frmFile.PutLine (_T("  "));
}


void CDECol::serialize(Serializer& ar, CDEGroup* pParentGroup)
{
    ar & m_iWidth
       & m_Offset
       & *m_headerText
       & m_columnCell;

    if( ar.IsSaving() )
    {
        ar.Write<int>(GetNumFields());

        for( int i = 0; i < GetNumFields(); ++i )
            ar << *GetField(i);
    }

    else
    {
        for( int i = ar.Read<int>(); i > 0; --i )
        {
            CDEField* pField = NULL;

            try
            {
                pField = new CDEField;
                ar >> *pField;
                pField->SetParent(pParentGroup);
            }

            catch( const FormSerialization::RepeatedItemException& e )
            {
                delete pField;
                pField = assert_cast<CDEField*>(FormSerialization::getItem(e.GetPosition()));
            }

            AddField(pField);
        }
    }
}
