//***************************************************************************
//  File name: Roster.cpp
//
//  Description:
//       Data Entry application classes implementation
//
//  History:    Date       Author     Comment
//              -----------------------------
//              07 Jul 00   smg       Created
//
//***************************************************************************

#include "StdAfx.h"
#include "Roster.h"
#include "DragOptions.h"

namespace
{
    const CDEText DefaultStubTemplate(_T("@"));
}


IMPLEMENT_DYNAMIC(CDERoster, CDEGroup)


CDERoster::CDERoster()
    :   m_orientation(FormDefaults::RosterOrientation),
        m_useOccurrenceLabels(false),
        m_freeMovement(FreeMovement::Disabled)
{
    SetItemType(CDEFormBase::Roster);
    m_bRightToLeft = false;

    m_cTotalSize.SetRectEmpty();      // CSC 8/22/00
    m_iFieldRowHeight = m_iColWidth = m_iStubColWidth = 0;  // csc 8/24/00
    m_iHeadingRowHeight = 0;
}


CDERoster::CDERoster(const CDictRecord& dict_record)
    :   CDERoster()
{
    SetRequired(dict_record.GetRequired());
}


CDERoster::CDERoster(const CDERoster& rhs)
    :   CDEGroup(rhs),
        m_orientation(rhs.m_orientation),
        m_useOccurrenceLabels(rhs.m_useOccurrenceLabels),
        m_freeMovement(rhs.m_freeMovement),
        m_stubTextSet(rhs.m_stubTextSet),
        m_freeCells(rhs.m_freeCells)
{
    SetRightToLeft     (rhs.GetRightToLeft());
    SetTotalSize       (rhs.GetTotalSize());
    SetFieldRowHeight  (rhs.GetFieldRowHeight());
    SetColWidth        (rhs.GetColWidth());
    SetHeadingRowHeight(rhs.GetHeadingRowHeight());
    SetStubColWidth    (rhs.GetStubColWidth());

    for (int i=0; i < rhs.GetNumCols(); i++)
    {
        CDECol* pCol = rhs.GetCol(i);
        CDECol* pColCopy = new CDECol(*pCol);
        pColCopy->RemoveAllFields();
        // update the col fields to point to fields in new roster
        for (int j=0; j < pCol->GetNumFields(); j++)
        {
            // Field ptr in pCol is field in orig roster, do a name lookup to find corresponding field
            // in pColCopy.
            CDEField* pField = static_cast<CDEField*>(GetItem(FindItem(pCol->GetField(j)->GetName())));
            pColCopy->AddField(pField);
        }
        AddCol(pColCopy);
    }    
}


CDERoster::~CDERoster()
{
    RemoveAndDeleteAllCols();
}


void CDERoster::SetFieldRowHeight(CString cs)
{
    TCHAR*  pszArgs = cs.GetBuffer(cs.GetLength());
    TCHAR*  pszArg;

    pszArg = strtoken (pszArgs, SPACE_COMMA_STR, NULL);
    int x = _ttoi (pszArg);

    if (x >= 0 && x < 1000)         // changed to >=0 from >0    csc 1/31/01
        SetFieldRowHeight (x);
    else
        SetFieldRowHeight (25);     // this, of course, shld be a #def too
}

void CDERoster::SetHeadingRowHeight (CString cs)
{
    TCHAR*  pszArgs = cs.GetBuffer(cs.GetLength());
    TCHAR*  pszArg;

    pszArg = strtoken (pszArgs, SPACE_COMMA_STR, NULL);
    int x = _ttoi (pszArg);

    if (x >= 0 && x < 1000)         // changed to >=0 from >0    csc 1/31/01
        SetHeadingRowHeight (x);
    else
        SetHeadingRowHeight (25);       // this, of course, shld be a #def too
}


void CDERoster::SetTotalSize(CString cs)
{
    TCHAR* pszArgs = cs.GetBuffer(cs.GetLength());
    CRect rect;

    TCHAR* pszArg = strtoken(pszArgs, SPACE_COMMA_STR, NULL);
    rect.left = _ttoi(pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    rect.top = _ttoi(pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    rect.right = _ttoi(pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);
    rect.bottom = _ttoi(pszArg);

    SetTotalSize(rect);
}

int CDERoster::GetColIndex(const CDEField* pField) const
{
    for (int c = 0; c < GetNumCols(); c++)
    {
        auto pCol = GetCol(c);
        for (int f = 0; f < pCol->GetNumFields(); ++f)
        {
            if (pCol->GetField(f) == pField)
                return c;
        }
    }

    return -1;
}

int CDERoster::GetColIndex(const CDEBlock* pBlock) const
{
    int iGroupIndex = GetItemIndex(pBlock);
    for (int i = iGroupIndex; i < GetNumItems(); ++i) {
        if (GetItem(i)->GetItemType() == CDEFormBase::Field)
        {
            CDEField* pField = static_cast<CDEField*>(GetItem(i));
            return GetColIndex(pField);
        }
    }

    return GetNumCols();
}

// Overridden from CDEGroup to handle columns
void CDERoster::RemoveItem(int iIndex)
{
    if (GetItem(iIndex)->GetItemType() == CDEFormBase::Field)
    {
        auto pField = static_cast<CDEField*>(GetItem(iIndex));
        int iColIndex = GetColIndex(pField);
        if (iColIndex != NONE)
        {
            CDECol* pCol = GetCol(iColIndex);
            int iIndexInCol = pCol->GetFieldIndex(pField);
            if (iIndexInCol != NONE)
            {
                pCol->RemoveFieldAt(iIndexInCol);
                if (pCol->GetNumFields() == 0) {//GSF wanted this //SAVY 04/16/01
                    RemoveAndDeleteCol(GetColIndex(pCol));
                }
            }
        }
    }

    CDEGroup::RemoveItem(iIndex);
}

void CDERoster::RemoveAndDeleteCol(int i)
{
    auto pCol = m_aCol[i];
    while (pCol->GetNumFields() > 0)
    {
        CDEGroup::RemoveItem(GetItemIndex(pCol->GetField(0))); // Use the base class version as this one will delete col
        pCol->RemoveFieldAt(0);
    }

    delete m_aCol[i];       // remove array registry info
    m_aCol.erase(m_aCol.begin() + i);     // now remove the obj itself
}

void CDERoster::RemoveAndDeleteAllCols()
{
    for( CDECol* pCol : m_aCol )
        delete pCol;
}

int CDERoster::GetColIndex(const CDECol* pCol) const
{
    for (int i = 0; i < GetNumCols(); ++i) {
        if (pCol == m_aCol[i])
            return i;
    }
    return -1;
}

int CDERoster::AdjustColIndexForRightToLeft(int i) const
{
    if (GetRightToLeft())
    {
        return GetNumCols() - i;
    }
    else {
        return i;
    }
}

// Find the index in list of group items for a column.
// Should correspond to the index of the first field of the column but will
// also work if the column has no fields.
int CDERoster::GetGroupInsertIndexForCol(int iSearchCol) const
{
    int iGrpItem = 0;

    // Skip non fields (blocks) at start of group
    while (iGrpItem < GetNumItems() && GetItem(iGrpItem)->GetItemType() != CDEFormBase::Field)
        ++iGrpItem;

    // Run through all preceding columns and fields in those columns and count them to get
    // index
    for (int iCol = 0; iCol < GetNumCols() && iCol != iSearchCol && iGrpItem < GetNumItems(); ++iCol) {
        CDECol* pCol = GetCol(iCol);
        for (int iColFld = 0; iColFld < pCol->GetNumFields(); ++iColFld) {
            ASSERT(GetItem(iGrpItem) == pCol->GetField(iColFld));
            ++iGrpItem;
            if (iGrpItem == GetNumItems())
                break;
            while (iGrpItem < GetNumItems() && GetItem(iGrpItem)->GetItemType() != CDEFormBase::Field)
                ++iGrpItem;
        }
    }

    return iGrpItem;
}

    

void CDERoster::RecalcOffsets(int iCol /*=0*/)    // default to 1st col if nothing given
{
    int i;
    int width = 0;
    int max = GetNumCols();

    CPoint cpOffset(0,0);

    CDECol* pCol = NULL;

    if (iCol < max)
    {
        pCol = GetCol(iCol);
        width = pCol->GetWidth();
        cpOffset = pCol->GetOffset();
    }

    for (i=iCol+1; i < max; i++)    // i shld start one beyond start col
    {
        pCol  = GetCol(i);
        cpOffset.x += width;
        pCol->SetOffset (cpOffset);

        width = pCol->GetWidth();
    }
}


/////////////////////////////////////////////////////////////////////////////
/*  in CDERoster, the columns will keep the fields, since there is no
    longer a one-to-one correlation between the columns and fields
    (one column can have 0+ fields)

    ISSA, however, needs to see the ptrs in the group's array; so i'm
    stuffing them in
 */

void CDERoster::FillItemPtrs()
{
    int i, iMax = GetNumCols();
    int j, jMax;
    CDECol* pCol;

    RemoveAllItems();   // CDEGroup level func

    for (i=0; i < iMax; i++)
    {
        pCol = GetCol(i);
        jMax = pCol->GetNumFields();

        for (j = 0; j < jMax; j++)
        {
            AddItem (pCol->GetField (j));
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CDERoster::FillItemPtrs2()
//
/////////////////////////////////////////////////////////////////////////////////
void CDERoster::FillItemPtrs2()
{
    int iMax = GetNumCols();

    //save the blocks
    std::vector<CDEItemBase*> arrBlocks = m_aItem;

    m_aItem.clear();

    int iItem = 0;
    for (int i=0; i < iMax; i++)
    {
        CDECol* pCol = GetCol(i);
        int jMax = pCol->GetNumFields();

        for (int j = 0; j < jMax; j++)
        {
            //add the blocks back if they exist
            if (iItem < (int)arrBlocks.size()) {
                CDEBlock* pBlock = DYNAMIC_DOWNCAST(CDEBlock, arrBlocks[iItem]);
                if (pBlock) {
                    AddItem(pBlock);
                }
            }
            AddItem (pCol->GetField (j));
            iItem++;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// add a non-occurring dictionary item (i.e., does have occurrences) to a
// CDERoster object

void CDERoster::AddNonOccItem(const CDictRecord* pDR, const CDictItem* pDI, int iFormNum,
                              const DragOptions& drag_options, CDEFormFile* pFF, int& index)
{
    bool bOkToDropSubitems = pFF->TestSubitemDrop (pDR, pDI, drag_options);

    CString sDictName = pFF->GetDictionaryName();
    CDEField* pField;
    CDECol* pCol;
    CDEText text;

    if (bOkToDropSubitems)
    {
        // user wants to drop subitems, but they may overlap--check it out

        int max = pDR->GetNumItems();
        const CDictItem* pSubItem = pDR->GetItem(++index);

        while (index < max && pSubItem->GetItemType() == ItemType::Subitem)
        {
            pField = new CDEField (pSubItem, iFormNum, sDictName, drag_options);
            pField->SetParent (this);

            pCol = new CDECol();
            text.SetText (pField->GetText());
            pCol->SetHeaderText (text);
            pCol->AddField (pField);
            this->AddCol (pCol);
            pFF->AddUniqueName(pField->GetName());  // Field's uniq name must be added at FF level

            if (++index < max)
                pSubItem = pDR->GetItem(index);
        }
        --index; // back up so i don't miss the (possible) item that follows the last subitem
    }
    else
    {
        pField = new CDEField (pDI, iFormNum, sDictName, drag_options);
        pField->SetParent (this);
        pCol = new CDECol();
        text.SetText (pField->GetText());
        pCol->SetHeaderText (text);
        pCol->AddField (pField);
        this->AddCol (pCol);
        pFF->AddUniqueName(pField->GetName());  // new'ing Field can't add its uniq name
    }
}


// Check that fields in roster columns are consistent with fields in group
void CDERoster::CheckRosterIntegrity()
{
    std::vector<CDEField*> columnFields;
    for (int iCol = 0; iCol < GetNumCols(); ++iCol)
    {
        CDECol* pCol = GetCol(iCol);
        for (int iFld = 0; iFld < pCol->GetNumFields(); ++iFld)
        {
            columnFields.push_back(pCol->GetField(iFld));
        }
    }

    std::vector<CDEField*> groupFields;
    for (int iItem = 0; iItem < GetNumItems(); ++iItem)
    {
        CDEItemBase* pItem = GetItem(iItem);
        if (pItem->GetItemType() == CDEFormBase::Field)
            groupFields.push_back(static_cast<CDEField*>(pItem));
    }

    ASSERT(columnFields.size() == groupFields.size() &&
        std::equal(columnFields.begin(), columnFields.end(), groupFields.begin()));
}





/*  ********************************************************************* */
/*  after the roster is built, see what adjustments, if any, need to be
    made to the stub text array; there shld either be

    [1] a unique stub text entry for each row (occurrence) in the roster;
        i.e., "Corn", "Potatoes", "Rice", or
    [2] a single entry that will be used to populate the remaining fields;
        i.e., "Crop @", where '@' indicates a # should be placed
 */

void CDERoster::FinishStubTextInit()
{
    if( GetMaxLoopOccs() == (int)m_stubTextSet.GetNumTexts() )
    {
        // the # of stub txt items matches # of rows so we're hopefully ok
        return;
    }

    if( m_stubTextSet.GetNumTexts() == 0 )
    {
        // no stub entries found, default to #s in stubs
        SetAllStubs(&DefaultStubTemplate);
    }

    else if( m_stubTextSet.GetNumTexts() == 1 )
    {
        // then we have case #2 above, most likely
        CDEText stub_template = m_stubTextSet.GetText(0);

        if( stub_template.GetText() == _T("1") ) // '1' found, need to replace it with '@'
        {                                        // as we're assuming user wants rows #d 1,2,3 etc
            stub_template = DefaultStubTemplate;
        }

        SetAllStubs(&stub_template);
    }

    else if( GetMaxLoopOccs() < (int)m_stubTextSet.GetNumTexts() )   
    {
        // have too many stub text items, ditch the extra
        for( size_t i = m_stubTextSet.GetNumTexts() - 1; (int)i >= GetMaxLoopOccs(); --i )
            m_stubTextSet.RemoveText(i);
    }

    else if( GetMaxLoopOccs() > (int)m_stubTextSet.GetNumTexts() )
    {
        // not enough stub text items
        for( int i = (int)m_stubTextSet.GetNumTexts(); i < GetMaxLoopOccs(); ++i )
        {
            // default for now to use the index as the stub txt
            m_stubTextSet.AddText(std::make_shared<CDEText>(IntToString(i + 1)));
        }
    }
}

/*  ********************************************************************* */
/*  for each CDEField within every column, assign it the correct form #   */

void CDERoster::FinishFieldInit()
{
    int iFormNum = GetFormNum();

    if (iFormNum == NONE)   // a problem, it never got assigned during load; how2handle?
        return;

    int iCol, iField;
    int nCol = GetNumCols();
    CDECol* pCol = NULL;

    for (iCol=0; iCol < nCol; iCol++)
    {
        pCol = GetCol(iCol);
        int nField = pCol->GetNumFields();

        for (iField=0; iField < nField; iField++)
        {
            pCol->GetField (iField)->SetFormNum (iFormNum);
        }
    }
}



// --------------------------------------------------
// stub text
// --------------------------------------------------

template<typename CF>
void CDERoster::SetAllStubs(const CDEText& stub_template, CF callback_function)
{
    const CString& stub_text = stub_template.GetText();
    int pos = stub_text.Find('@');

    // '@' found, need to adjust the stubs accordingly
    if( pos >= 0 )   
    {
        CString left_text = stub_text.Left(pos);
        CString right_text = stub_text.Mid(pos + 1);

        for( int occurrence = 0; occurrence < GetMaxLoopOccs(); ++occurrence )
        {
            CString text = callback_function(occurrence);

            if( text.IsEmpty() )
                text = left_text + IntToString(occurrence + 1) + right_text;

            m_stubTextSet.GetText(occurrence).SetText(text);
        }
    }
}


void CDERoster::SetAllStubs(const CDEText* stub_template, const DictNamedBase* dict_element/* = nullptr*/)
{
    if( stub_template == nullptr )
        stub_template = &DefaultStubTemplate;

    // get rid of any extant stub text
    m_stubTextSet.RemoveAllTexts(); 

    // initialize the stubs
    for( int i = 0; i < GetMaxLoopOccs(); ++i )
        m_stubTextSet.AddText(std::make_shared<CDEText>(*stub_template));

    // record occurrence labels
    if( dict_element != nullptr && dict_element->GetElementType() == DictElementType::Record )
    {
        const CDictRecord& dict_record = assert_cast<const CDictRecord&>(*dict_element);
        SetAllStubs(*stub_template, [&](int occurrence) { return dict_record.GetOccurrenceLabels().GetLabel(occurrence); });
    }

    // item occurrence labels
    else if( dict_element != nullptr )
    {
        const CDictItem& dict_item = assert_cast<const CDictItem&>(*dict_element);
        SetAllStubs(*stub_template, [&](int occurrence) { return dict_item.GetOccurrenceLabels().GetLabel(occurrence); });
    }

    else
    {
        SetAllStubs(*stub_template, [&](int /*occurrence*/) { return CString();});
    }
}


void CDERoster::RefreshStubsFromOccurrenceLabels(const CDEFormFile& form_file)
{
    if( !m_useOccurrenceLabels )
        return;

    //Fix the occurrence labels for the current language
    const CDictRecord* dict_record;
    const CDictItem* dict_item;
    form_file.GetDictionary()->LookupName(GetTypeName(), nullptr, &dict_record, &dict_item, nullptr);

    if( dict_item != nullptr )
    {
        SetAllStubs(DefaultStubTemplate, [&](int occurrence) { return dict_item->GetOccurrenceLabels().GetLabel(occurrence); });
    }

    else if( dict_record != nullptr )
    {
        SetAllStubs(DefaultStubTemplate, [&](int occurrence) { return dict_record->GetOccurrenceLabels().GetLabel(occurrence); });
    }

    else
    {
        SetAllStubs(DefaultStubTemplate, [&](int /*occurrence*/) { return CString(); });
    }
}



// --------------------------------------------------
// free cells
// --------------------------------------------------

void CDERoster::InsertFreeCell(size_t index, std::shared_ptr<CDEFreeCell> free_cell)
{
    ASSERT(index <= m_freeCells.size() && free_cell != nullptr);
    m_freeCells.insert(m_freeCells.begin() + index, std::move(free_cell));
}


void CDERoster::RemoveFreeCell(size_t index)
{
    ASSERT(index < m_freeCells.size());
    m_freeCells.erase(m_freeCells.begin() + index);
}


CDEFreeCell& CDERoster::GetOrCreateFreeCell(int row, int column)
{
    for( CDEFreeCell& free_cell : GetFreeCells() )
    {
        if( free_cell.GetRow() == row && free_cell.GetColumn() == column )
            return free_cell;
    }

    auto new_free_cell = std::make_shared<CDEFreeCell>(row, column);
    AddFreeCell(new_free_cell);

    return *new_free_cell;
}



// --------------------------------------------------
// serialization
// --------------------------------------------------

bool CDERoster::Build(CSpecFile& frmFile, bool bSilent/* = false*/)
{
    CString csCmd, csArg;
    bool bDone = false, rtnVal = false;

    SetItemType (CDEFormBase::Roster);  // just in case it didn't get set yet

    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK) {

        ASSERT (csCmd.GetLength() > 0);

        if (csCmd.CompareNoCase(FRM_CMD_NAME) == 0)
            SetName (csArg);
        else if (csCmd.CompareNoCase(FRM_CMD_LABEL) == 0)
            SetLabel (csArg);
        else if (csCmd.CompareNoCase(FRM_CMD_FORMNUM) == 0)
            SetFormNum (csArg);
        else if (csCmd.CompareNoCase(_T("Required")) == 0)
            SetRequired (TEXT_TO_BOOL(csArg));
        else if (csCmd.CompareNoCase(_T("Type")) == 0)
            SetRIType (csArg);
        else if (csCmd.CompareNoCase(_T("TypeName")) == 0)
            SetTypeName (csArg);
        else if (csCmd.CompareNoCase(_T("Max")) == 0)
            SetMaxLoopOccs(csArg);
        else if (csCmd.CompareNoCase(_T("MaxField")) == 0)
            SetMaxField (csArg);

        else if (csCmd.CompareNoCase(FRM_CMD_DISPLAYSZ) == 0)
            SetDims (csArg);
        else if (csCmd.CompareNoCase(FRM_CMD_TOTALSZ) == 0)
            SetTotalSize(csArg);

        else if (csCmd.CompareNoCase(FRM_CMD_ORIENT) == 0) {
            m_orientation = ( csArg.CompareNoCase(ROSTER_ORIENT_VERT) == 0 ) ? RosterOrientation::Vertical :
                                                                               RosterOrientation::Horizontal;
        }

        else if (csCmd.CompareNoCase(FRM_CMD_USEOCCLABELS) == 0) {
            SetUseOccurrenceLabels(csArg.CompareNoCase(_T("Yes")) ==0);
        }

        else if (csCmd.CompareNoCase(FRM_CMD_FREEMOVEMENT) == 0) {
            if( TEXT_TO_BOOL(csArg) != UsingFreeMovement() ) {
                m_freeMovement = UsingFreeMovement() ? FreeMovement::Disabled : FormDefaults::FreeMovement;
            }
        }

        else if (csCmd.CompareNoCase(FRM_CMD_DEFAULTMOVEMENT) == 0) {
            m_freeMovement = ( csArg.CompareNoCase(ROSTER_ORIENT_HORZ) == 0 ) ? FreeMovement::Horizontal :
                             ( csArg.CompareNoCase(ROSTER_ORIENT_VERT) == 0 ) ? FreeMovement::Vertical : 
                                                                                FreeMovement::Disabled;
        }

        else if (csCmd.CompareNoCase(FRM_CMD_FLDROW_HGHT) == 0)
            SetFieldRowHeight (csArg);
        else if (csCmd.CompareNoCase(FRM_CMD_COL_WIDTH) == 0)
            SetColWidth(csArg);  // csc 8/22/00
        else if (csCmd.CompareNoCase(FRM_CMD_HDGROW_HGHT) == 0)
            SetHeadingRowHeight (csArg);
        else if (csCmd.CompareNoCase(FRM_CMD_STUBCOL_WIDTH) == 0)
            SetStubColWidth(csArg);

        else if (csCmd[0] == '[')
        {
            if (csCmd.CompareNoCase(HEAD_GRIDEND) == 0) // [EndGrid]
            {
                bDone = true;
                rtnVal = true;
            }
            else if (csCmd.CompareNoCase(HEAD_COLUMN) == 0) // [Column]
            {
                CDECol* pCol = new CDECol();

                if (pCol->Build(frmFile, this)) {   // if the build went ok
                    AddCol(pCol);
                    //  Add fields from the column to the group
                    for (int iColField = 0; iColField < pCol->GetNumFields(); ++iColField)
                    {
                        AddItem(pCol->GetField(iColField));
                    }
                }
                else
                {
                    delete pCol;
                    bDone = true;
                }
            }
            else if (csCmd.CompareNoCase(HEAD_CELL) == 0) // [Cell]
            {
                auto new_free_cell = std::make_shared<CDEFreeCell>();

                if (new_free_cell->Build(frmFile))     // if the build went ok
                {
                    AddFreeCell(std::move(new_free_cell));
                }
                else
                {
                    bDone = true;
                }
            }
            else if (csCmd.CompareNoCase(HEAD_TEXT) == 0) // [Text]
            {
                auto text = std::make_shared<CDEText>();

                if (text->Build (frmFile))     // if the build went ok
                {
                    m_stubTextSet.AddText(std::move(text));
                }

                else
                {
                    bDone = true;
                }
            }
            else if (csCmd.CompareNoCase(HEAD_BLOCK) == 0) // [Block]
            {
                auto pBlock = new CDEBlock();
                pBlock->SetParent(this);
                pBlock->SetFormNum(GetFormNum());

                if (pBlock->Build(frmFile, bSilent))    // if the build went ok
                    InsertItemAt(pBlock, pBlock->GetSerializedPositionInParentGroup());
                else
                {
                    delete pBlock;
                    bDone = true;
                }
            }
            else            // got some other kind of blk, not allowed, bail
                bDone = true;
        }
        else
        {                      // Incorrect attribute
            if (!bSilent)
            {
                ErrorMessage::Display(FormatText(_T("Incorrect [Grid] attribute\n\n%s"), (LPCTSTR)csCmd));
            }
            rtnVal = false;
        }
    }
    FinishStubTextInit();
    FinishFieldInit();
    return  rtnVal;
}


void CDERoster::Save(CSpecFile& frmFile) const
{
    CString csOutput;

    frmFile.PutLine (HEAD_GRID);

    frmFile.PutLine (FRM_CMD_NAME, GetName());
    frmFile.PutLine (FRM_CMD_LABEL, GetLabel());
    frmFile.PutLine (FRM_CMD_FORMNUM, GetFormNum()+1);

    frmFile.PutLine (_T("Required"),  BOOL_TO_TEXT(GetRequired()) );

    if (GetRIType() != CDEFormBase::UnknownRI)
    {
        frmFile.PutLine (_T("Type"), GetRIStr());
        frmFile.PutLine (_T("TypeName"), GetTypeName());
    }
    frmFile.PutLine (_T("Max"), GetMaxLoopOccs());

    if ( !SO::IsBlank(GetMaxField()) )         // ditto; if no val assigned,
        frmFile.PutLine (_T("MaxField"), GetMaxField());   // don't try2print!

    WriteDimsToStr (csOutput);
    frmFile.PutLine (FRM_CMD_DISPLAYSZ, csOutput);

    frmFile.PutLine (FRM_CMD_ORIENT, ToString(m_orientation));
    if (m_orientation == RosterOrientation::Horizontal)  {   // csc 8/22/00
        frmFile.PutLine (FRM_CMD_FLDROW_HGHT, GetFieldRowHeight());
    }
    else  {
        frmFile.PutLine(FRM_CMD_COL_WIDTH, GetColWidth());
        frmFile.PutLine(FRM_CMD_STUBCOL_WIDTH, GetStubColWidth());
    }

    frmFile.PutLine (FRM_CMD_HDGROW_HGHT, GetHeadingRowHeight());

    if(m_useOccurrenceLabels){
        frmFile.PutLine (FRM_CMD_USEOCCLABELS, _T("Yes"));
    }
    if(UsingFreeMovement()){
        frmFile.PutLine (FRM_CMD_FREEMOVEMENT, _T("Yes"));
        frmFile.PutLine (FRM_CMD_DEFAULTMOVEMENT, ( m_freeMovement == FreeMovement::Horizontal ) ? ROSTER_ORIENT_HORZ : ROSTER_ORIENT_VERT);
    }
    else {
        frmFile.PutLine (FRM_CMD_FREEMOVEMENT, _T("No"));
    }
    frmFile.PutLine (_T(" "));  // blank line to sep end of [grid] blk and beg of [text] blk for stubs

    for( const CDEText& text : m_stubTextSet.GetTexts() )
    {
        text.Save(frmFile);
        frmFile.PutLine (_T("  "));             // blank line to sep the stub text blks
    }

    for( const CDEFreeCell& free_cell : GetFreeCells() )
    {
        // only save the free cell if it contains content
        if( free_cell.HasContent() )
            free_cell.Save(frmFile);
    }

    for (int i = 0; i < GetNumCols(); i++)   {
        GetCol(i)->Save (frmFile);
    }

    for (int i = 0; i < GetNumItems(); ++i)
    {
        if (GetItem(i)->GetItemType() == CDEFormBase::Block) {
            CDEBlock* pBlock = static_cast<CDEBlock*>(GetItem(i));
            pBlock->Save(frmFile);
        }
    }

    frmFile.PutLine (HEAD_GRIDEND);
    frmFile.PutLine (_T(" "));
}


void CDERoster::serialize(Serializer& ar)
{
    CDEGroup::serialize(ar);

    if( ( ar.IsSaving() && !FormSerialization::isRepeated(this, ar) ) ||
        ( ar.IsLoading() && !FormSerialization::CheckRepeated(this, ar) ) )
    {
        ar & m_cTotalSize;

        ar.SerializeEnum(m_orientation);
        ar & m_useOccurrenceLabels;

        if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        {
            ar.SerializeEnum(m_freeMovement);
        }

        else
        {
            bool bFreeMovement = ar.Read<bool>();
            int iDefaultMovement = ar.Read<int>();
            m_freeMovement = ( !bFreeMovement )        ? FreeMovement::Disabled :
                             ( iDefaultMovement == 1 ) ? FreeMovement::Horizontal :
                                                         FreeMovement::Vertical;
        }
        
        ar & m_iFieldRowHeight
           & m_iColWidth
           & m_iHeadingRowHeight
           & m_iStubColWidth;

        int iNumCols = GetNumCols();
        ar & iNumCols;

        ar & m_bRightToLeft
           & m_freeCells
           & m_stubTextSet;

        for( int i = 0; i < iNumCols; i++ )
        {
            if( ar.IsSaving() )
            {
                GetCol(i)->serialize(ar, this);
            }

            else
            {
                CDECol* pCDECol = new CDECol;
                pCDECol->serialize(ar, this);
                m_aCol.emplace_back(pCDECol);
            }
        }
    }
}
