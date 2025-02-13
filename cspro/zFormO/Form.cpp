#include "StdAfx.h"
#include "Form.h"


IMPLEMENT_DYNAMIC(CDEForm, CDEFormBase)


/////////////////////////////////////////////////////////////////////////////
// a page can contain 0+ display texts, fields, and rosters,

CDEForm::CDEForm(const CString& name/* = CString()*/, const CString& label/* = CString()*/)
    :   CDEFormBase(name, label),
        m_backgroundColor(FormDefaults::FormBackgoundColor),
        m_questionTextHeight(FormDefaults::QuestionTextHeightDefault),
        m_capturePos{ -1, -1 },
        m_iLevel(NONE),
        m_pGroup(nullptr)
{
    SetDims(0, 0, 800, 800);  // smg: this shld be the same as INIT_SCRSZ in zFormF
}


CDEForm::CDEForm(const CDEForm& rhs)
    :   CDEFormBase(rhs),
        m_backgroundColor(rhs.m_backgroundColor),
        m_questionTextHeight(rhs.m_questionTextHeight),
        m_capturePos(rhs.m_capturePos),
        m_iLevel(rhs.m_iLevel),
        m_pGroup(rhs.m_pGroup),
        m_bMultRecName(rhs.m_bMultRecName),
        m_boxSet(rhs.m_boxSet)
{
    for( int i = 0; i < rhs.GetNumItems(); i++ )
    {
        // CDEText items belongs to the form, e'body else belongs
        // to the CDEGroup, so only copy the CDEText guys

        const CDEItemBase* pItem = rhs.GetItem(i);

        if (pItem->GetItemType() == CDEFormBase::Text)
        {
            CDEText* pText = new CDEText(*(CDEText*)pItem);
            AddItem(pText);
        }
    }
}


CDEForm::~CDEForm()
{
    RemoveAllItems();
}


/////////////////////////////////////////////////////////////////////////////
// loop thru all items assoc w/this group and add them to the form

void CDEForm::AddGroupItems (CDEGroup* pGroup)
{
    for (int j=0; j < pGroup->GetNumItems(); j++)
    {
        //SAVY ADDED CODE FOR GROUP WITHIN GROUP
        CDEItemBase* pBase = pGroup->GetItem(j);

        if(dynamic_cast<CDEGroup*>(pBase) != nullptr){//I am not touching rosters stuff
            //this change is for orders
            if(pBase->GetItemType() != CDEFormBase::Roster){
                ((CDEGroup*)pBase)->SetFormName(this->GetName());
                AddGroupItems((CDEGroup*)pBase);
                continue;
            }
        }

        AddItem(pBase);
    }
}


// in this case, i have the ptr but need to know it's index within the form!

int CDEForm::GetItemIndex(const CDEItemBase* pItem) const
{
    for( int i = 0; i < GetNumItems(); ++i )
    {
        const CDEItemBase* pLocalItem = GetItem(i);

        if (pLocalItem == pItem)
            return i;

        if(pItem->GetItemType() == CDEItemBase::Text){
            if(pLocalItem->GetItemType() == CDEItemBase::Field){
                if(&assert_cast<const CDEField*>(pLocalItem)->GetCDEText() == pItem) {
                    return i;
                }
            }
        }        
    }

    return NONE;
}

CDEItemBase* CDEForm::GetItem(int i) const
{
    if (i < 0 || i >= (int)m_aItem.size()) // if we're asking for smthng beyond the last element
        return NULL;
    else
        return m_aItem[i];
}

// the CDEGroup will handle the actual delete of the item

void CDEForm::RemoveItem (int i)
{
    if (GetItem(i)->GetItemType() == Text)         // only forms point to the text blks, so the
        delete m_aItem [i];                             // form must remove the memory assoc w/text

    m_aItem.erase(m_aItem.begin() + i);                // remove array registry info
}

// when deleting an object from the group, first call in to the form and delete
// the item; but i won't know the index; so easiest to just search for it by its
// unique name

void CDEForm::RemoveItem(const CString& sName)
{
    for( int i = 0; i < GetNumItems(); ++i )
    {
        const CDEItemBase* pItem = GetItem(i);

        if( sName.CompareNoCase(pItem->GetName()) == 0 )
        {
            RemoveItem(i);
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CDEForm::RemoveItem (CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
void CDEForm::RemoveItem (CDEField* pField)
{
    CDEItemBase* pItem;
    bool bFound = false;

    for (int i=0; i < GetNumItems() && !bFound; i++){
        pItem = GetItem(i);
        if (pItem->GetItemType() == CDEFormBase::Field)
        {
            if(pField == (CDEField*) pItem) {
                RemoveItem (i);
                bFound = true;
                break;
            }
        }
    }
}


// just delete the text blks that belong to the form (somebody's gotta do it)
// the CDEGroup will handle the deletion of the field & roster items

void CDEForm::RemoveAllItems()
{
    for( const CDEItemBase* pItem : m_aItem )
    {
        if( pItem->GetItemType() == CDEFormBase::Text )
            delete pItem;
    }

    m_aItem.clear();
}


void CDEForm::UpdateDims()
{
    // go though all the items on the form and make the form's boundaries reflect the highest dimensions
    CRect form_rect;

    auto update_form_rect = [&](const CRect& item_rect)
    {
        form_rect.right = std::max(form_rect.right, item_rect.right);
        form_rect.bottom = std::max(form_rect.bottom, item_rect.bottom);
    };

    // items
    for( int i = 0; i < GetNumItems(); ++i )
    {
        const CDEItemBase* pItem = GetItem(i);
        update_form_rect(pItem->GetDims());

        // look at a field's text dimensions too
        if( pItem->GetItemType() == CDEFormBase::Field )
            update_form_rect(static_cast<const CDEField*>(pItem)->GetTextDims());
    }

    // boxes
    for( const CDEBox& box : m_boxSet.GetBoxes() )
        update_form_rect(box.GetDims());

    // add some padding
    form_rect.right += FormDefaults::FormPadding;
    form_rect.bottom += FormDefaults::FormPadding;

    SetDims(form_rect);
}


inline void UpdateRowCol(const CRect & rect,int & rowMax,int & colMax,int & colMin) // 20100421
{
    if( rect.right > colMax )
        colMax = rect.right;

    if( rect.bottom > rowMax )
        rowMax = rect.bottom;

    if( rect.left < colMin )
        colMin = rect.left;
}

bool CDEForm::UpdateDims(int windowWidth, int* newSpacingDiff) // 20100421 for centering data on forms
{
    int colMax(0), rowMax(0);
    int colMin(2147483647);

    if( newSpacingDiff )
        *newSpacingDiff = 0;

    CRect rect;

    CDEItemBase* pItem;

    for( int i = 0; i < GetNumItems(); i++ )
    {
        pItem = GetItem(i);
        rect = pItem->GetDims();
        UpdateRowCol(rect,rowMax,colMax,colMin);

        if( pItem->GetItemType() == CDEFormBase::Field )
        {
            if( !((CDEField*) pItem)->GetText().IsEmpty() ) // 20101206 empty text was messing things up
            {
                rect = ((CDEField*) pItem)->GetTextDims();
                UpdateRowCol(rect,rowMax,colMax,colMin);
            }
        }
    }

    for( const CDEBox& box : m_boxSet.GetBoxes() )
        UpdateRowCol(box.GetDims(), rowMax, colMax, colMin);

    // now adjust things to center the form
    int widthOfForm = colMax - colMin;
    int spacingDiff = ( windowWidth / 2 ) - ( colMin + widthOfForm / 2 );

    if( widthOfForm < 0 || // nothing on the form
        widthOfForm > windowWidth || // no way to resize a form that is too big for the screen
        !spacingDiff ) // nothing to do ... the form is already centered
    {
        SetDims(0, 0, colMax + FormDefaults::FormPadding, rowMax + FormDefaults::FormPadding);
        return false;
    }

    // otherwise we adjust the items on the form

    bool gridsOnForm = false;

    for( int i = 0; i < GetNumItems(); i++ )
    {
        pItem = GetItem(i);
        rect = pItem->GetDims();
        rect.left += spacingDiff;
        rect.right += spacingDiff;
        pItem->SetDims(rect);

        if( pItem->GetItemType() == CDEFormBase::Field )
        {
            rect = ((CDEField*) pItem)->GetTextDims();
            rect.left += spacingDiff;
            rect.right += spacingDiff;
            ((CDEField*) pItem)->SetTextDims(rect);
        }

        if( pItem->GetItemType() == CDEFormBase::Roster )
            gridsOnForm = true;
    }

    for( CDEBox& box : m_boxSet.GetBoxes() )
    {
        CRect& box_rect = box.GetDims();
        box_rect.left += spacingDiff;
        box_rect.right += spacingDiff;
    }

    SetDims(0, 0, colMax + FormDefaults::FormPadding + spacingDiff, rowMax + FormDefaults::FormPadding);

    if( newSpacingDiff )
        *newSpacingDiff = spacingDiff;

    return gridsOnForm; // true means to redraw the grids at the current new location
}




// i need this func to help me evaluate an item drop from a multiple record;
// if there are *no* keyed fields on the form, then i need to ask the user if
// they want the drop to result in the data being rostered or unrostered; if,
// however, there *are* keyed fields on the form, then i have to analyze the
// exact drop location, as they could be trying to add the item to an existing
// roster, or trying to create a new one (as nothing's underneath the drop location)

bool CDEForm::AnyKeyedFieldsOnForm() const
{
    for( int i = 0; i < GetNumItems(); ++i )
    {
        const CDEItemBase* pItem = GetItem(i);

        if( pItem->GetItemType() == CDEFormBase::Roster )
        {
            return true;
        }

        else if( pItem->GetItemType() == CDEFormBase::Field )
        {
            auto pField =(const CDEField*) pItem;

            if( pField->IsKeyed() )  // a keyed field was found, bail
                return true;
        }
    }

    return false;
}

// a corollary to the above; i need to find it anything is located at the drop point;
// so run thru all the items on the form and see if any contain the dropPoint

CDEItemBase* CDEForm::FindItem (CPoint dropPoint)
{
    int i, max = GetNumItems();

    CRect itemRect;

    for (i = 0; i < max; i++)
    {
        itemRect = GetItem(i)->GetDims();

        if (itemRect.PtInRect (dropPoint))
        {
            return GetItem(i);
        }
    }
    return NULL;
}


CDEField* CDEForm::GetField(int iSym)
{
    //Since it is a field . I can process all the forms and get the field
    for( int i = 0; i < GetNumItems(); ++i )
    {
        CDEItemBase* pItem = GetItem(i);

        switch( pItem->GetItemType() )
        {
            case CDEFormBase::Field :
            {
                CDEField* pField = (CDEField*)pItem;

                if( pField->GetSymbol() == iSym )
                    return pField;

                break;
            }

            case CDEFormBase::Roster:
            {
                CDERoster* pRoster = (CDERoster*)pItem;

                //Set the dict item or vset for the roster
                for( int j = 0; j < pRoster->GetNumCols(); ++j )
                {
                    CDECol* pCol = pRoster->GetCol(j);

                    for( int k = 0; k < pCol->GetNumFields(); ++k )
                    {
                        CDEField* pField = pCol->GetField(k);

                        if( pField->GetSymbol() == iSym )
                            return pField;
                    }

                }

                break;
            }
        }
    }

    return nullptr;
}



// --------------------------------------------------
// serialization
// --------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// no [Group]s on a form, only [Text]s, [Field]s, and [Roster]s or [Grid]s

bool CDEForm::Build (CSpecFile& frmFile, const CString& sDictName, bool bSilent /* = FALSE */)
{
    CString csCmd, csArg;
    bool bDone = false, rtnVal = false;

    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK)
    {
        ASSERT (csCmd.GetLength() > 0);

        if (csCmd[0] == '.')    // then it's a comment line, ignore
            continue;

        if (csCmd[0] == '[')
        {
            if (csCmd.CompareNoCase(HEAD_ENDFORM) == 0 ) // "[EndForm]", bail
            {
                bDone = true;
                rtnVal = true;
            }
            else if( csCmd.CompareNoCase(HEAD_TEXT) == 0 )
            {
                CDEText* pText = new CDEText();

                if (pText->Build(frmFile))      // if the build went ok
                    AddItem(pText);            // add the item to the page
                else
                    delete pText;
            }
            else    // err condition, let somebody else figure out :)
            {
                frmFile.UngetLine();
                bDone = true;
            }
        }
        else if( csCmd.CompareNoCase(FRM_CMD_NAME) == 0 )
            SetName(csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_LABEL) == 0 )
            SetLabel(csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_LEVEL) == 0 )
            SetLevel ( _ttoi(csArg)-1 );                 // deduct 1 to make 0-based
        else if( csCmd.CompareNoCase(FRM_CMD_REPEAT) == 0 )
            SetRecordRepeatName(csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_SIZE) == 0 )
            SetUpperDims (csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_COLOR) == 0 ) {
            SetBackgroundColor(PortableColor::FromCOLORREF(_ttoi(csArg)));
        }
        else if( csCmd.CompareNoCase(FRM_CAPTUREPOS) == 0 ) { // 20120405
            int commaPos = csArg.Find(_T(','));

            if( commaPos ) // there must be a comma separating the X and Y values
            {
                int iXpos = _ttoi(csArg.Left(commaPos));
                int iYpos = _ttoi(csArg.Mid(commaPos + 1));
                SetCapturePos(iXpos,iYpos);
            }
        }
        else if( csCmd.CompareNoCase(FRM_CMD_WHOLEFORM ) == 0 ) {
            // whole form is no longer used
        }

        else if( csCmd.CompareNoCase(FRM_CMD_BOX) == 0 )
        {
            m_boxSet.AddBox(std::make_shared<CDEBox>(csArg));
        }

        // an item's build will be finished off in CDEGroup's build; for now, default to Field

        else if( csCmd.CompareNoCase(FRM_CMD_ITEM) == 0 )
        {
            TCHAR*   pszArgs = csArg.GetBuffer(csArg.GetLength());
            TCHAR*   pszArg;

            pszArg = strtoken(pszArgs, SPACE_COMMA_STR, NULL);
            CString csItemName = pszArg;

            CDEField* pField = new CDEField(csItemName, sDictName);
            AddItem(pField);               // add the item to the form
        }

        else if( csCmd.CompareNoCase(FRM_CMD_QSTXT_HGT) == 0 )
        {
            SetQuestionTextHeight(_ttoi(csArg));
        }

        else                       // Incorrect attribute
        {
            if (!bSilent)
            {
                ErrorMessage::Display(FormatText(_T("Incorrect [Form] attribute\n\n%s"), (LPCTSTR)csCmd));
            }
        }
    }
    return  rtnVal;
}


/*  ////////////////////////////////////////////////////////////////////////////

[1] first, write out all the header info related to the [Form] blk
[2] next, write out all the [Field] and [Roster] (or [Grid]) blks from the
form in "Item=<Name>" format
[3] write out all the boxes!
[4] and finally, write out all the [Text] blks using full notation

//////////////////////////////////////////////////////////////////////////// */

void CDEForm::Save(CSpecFile& frmFile) const 
{
    CString sOutput;
    CIMSAString sTemp;

    //      [1] header stuff

    frmFile.PutLine(HEAD_FORM);
    frmFile.PutLine(FRM_CMD_NAME, GetName());
    frmFile.PutLine(FRM_CMD_LABEL, GetLabel());

    if( m_questionTextHeight != FormDefaults::QuestionTextHeightDefault ) {
        frmFile.PutLine(FRM_CMD_QSTXT_HGT, m_questionTextHeight);
    }

    frmFile.PutLine(FRM_CMD_LEVEL, GetLevel()+1);

    if (isFormMultiple())  {
        frmFile.PutLine(FRM_CMD_REPEAT, GetRecordRepeatName());
    }

    frmFile.PutLine(FRM_CMD_SIZE, GetUpperDimsStr());
    if (m_backgroundColor != FormDefaults::FormBackgoundColor)
        frmFile.PutLine(FRM_CMD_COLOR, (int)m_backgroundColor.ToCOLORREF());

    if( GetCapturePosX() >= 0 ) // 20120405
    {
        sTemp.Format(_T("%d,%d"),GetCapturePosX(),GetCapturePosY());
        frmFile.PutLine(FRM_CAPTUREPOS,sTemp);
    }


    frmFile.PutLine(_T("  "));             // blank line to sep stuff
    CDEItemBase* pItem;
    CDEField* pField;
    int max = GetNumItems();

    //      [2] [field] and [roster] blks

    CDEFormBase::eItemType eIT;
    for (int i = 0 ; i < max; i++)          {
        pItem = GetItem(i);
        eIT = pItem->GetItemType();
        if (eIT == CDEFormBase::Roster) {
            frmFile.PutLine(FRM_CMD_ITEM, pItem->GetName());
        }
        else if(eIT == CDEFormBase::Field)  {
            pField = (CDEField*) pItem;
            frmFile.PutLine(FRM_CMD_ITEM, pField->GetName());
        }
        // i'll do [text] all together at end
    }

    //      [3] save the boxes

    frmFile.PutLine(_T("  "));             // blank line to sep stuff

    for( const CDEBox& box : m_boxSet.GetBoxes() )
        frmFile.PutLine(FRM_CMD_BOX, box.GetSerializedText());

    //      [4] now do the full [text] blks

    max = GetNumItems();
    for (int i = 0 ; i < max; i++) {
        pItem = GetItem(i);
        if (pItem->GetItemType() == CDEFormBase::Text)  {
            // only save the full citation for [text] blks
            frmFile.PutLine(_T("  "));             // blank line to sep the groups
            ((CDEText*) pItem)->Save(frmFile);
        }
    }
    frmFile.PutLine(HEAD_ENDFORM);
    frmFile.PutLine(_T("  "));
}


void CDEForm::serialize(Serializer& ar)
{
    CDEFormBase::serialize(ar);

    ar & m_iLevel
       & m_bMultRecName;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar & m_backgroundColor;
    }

    else
    {
        m_backgroundColor = PortableColor::FromCOLORREF(ar.Read<COLORREF>());
    }

    ar & m_questionTextHeight
       & m_capturePos
       & m_boxSet;

    if( ar.PredatesVersionIteration(Serializer::Iteration_7_7_000_1) && m_questionTextHeight == 0 )
        m_questionTextHeight = 60;

    int iNumItems = GetNumItems();
    ar & iNumItems;

    for( int i = 0; i < iNumItems; i++ )
    {
        if( ar.IsSaving() )
        {
            CDEFormBase::eItemType itemType = GetItem(i)->GetItemType();
            ar.SerializeEnum(itemType);
            FormSerialization::SerializeItem(ar,GetItem(i));
        }

        else
        {
            CDEItemBase* pItem = FormSerialization::getType_createItem(ar);

            try
            {
                FormSerialization::SerializeItem(ar,pItem);
            }

            catch( const FormSerialization::RepeatedItemException& e )
            {
                // Temporary hack to fix form serialization until I can come up with a better solution.
                // Deleting 2nd copy of roster deletes the fields that are pointed to by 1st copy
                // so remove fields first before deleting roster.
                if (pItem->GetItemType() == CDEFormBase::Roster)
                {
                    CDERoster* pRoster = assert_cast<CDERoster*>(pItem);

                    for( int j = pRoster->GetNumItems() - 1; j >= 0; --j )
                    {
                        const CDEItemBase* pRepeatedItem = pRoster->GetItem(j);

                        // the second copy of the block must be deleted
                        if( pRepeatedItem->isA(CDEFormBase::Block) )
                            delete pRepeatedItem;

                        pRoster->RemoveItemAt(j);
                    }
                }
                delete pItem;
                pItem = assert_cast<CDEItemBase*>(FormSerialization::getItem(e.GetPosition()));
            }

            AddItem(pItem);
        }
    }

    int iPosition = -1;

    if( ar.IsSaving() )
        FormSerialization::isPresent(m_pGroup,&iPosition);

    ar & iPosition;

    if( ar.IsLoading() && iPosition >= 0 )
    {
        CDEItemBase* pItem = FormSerialization::getItem(iPosition);

        if( pItem->isA(Group) )
            m_pGroup = assert_cast<CDEGroup*>(pItem);
    }
}
