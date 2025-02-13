#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/Group.h>
#include <zFormO/RosterCells.h>
#include <zFormO/RosterColumn.h>


#define FRM_CMD_ORIENT            _T("Orientation") // this blk used by CDERoster or its constituent parts
#define FRM_CMD_TOTALSZ           _T("TotalSize")
#define FRM_CMD_DISPLAYSZ         _T("DisplaySize")
#define FRM_CMD_FLDROW_HGHT       _T("FieldRowHeight")
#define FRM_CMD_COL_WIDTH         _T("ColumnWidth")
#define FRM_CMD_HDGROW_HGHT       _T("HeadingRowHeight")
#define FRM_CMD_STUBCOL_WIDTH     _T("StubColumnWidth")       // csc 12/20/00
#define FRM_CMD_HEIGHT            _T("Height")
#define FRM_CMD_WIDTH             _T("Width")
#define FRM_CMD_FREEMOVEMENT      _T("FreeMovement")
#define FRM_CMD_DEFAULTMOVEMENT   _T("DefaultMovement")
#define FRM_CMD_USEOCCLABELS      _T("UseOccurrenceLabels")


//////////////////////////////////////////////////////////
//      default section headings

#define HEAD_GRID               _T("[Grid]")                // this blk of defines all used w/in Roster serialization
#define HEAD_GRIDEND            _T("[EndGrid]")
#define HEAD_COLUMN             _T("[Column]")
#define HEAD_COLUMNEND          _T("[EndColumn]")
#define HEAD_CELL               _T("[Cell]")
#define HEAD_CELLEND            _T("[EndCell]")
#define HEAD_HDRTEXT            _T("[HeaderText]")


// --------------------------------------------------------------------------
//
// CDERoster
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDERoster : public CDEGroup
{
    DECLARE_DYNAMIC(CDERoster)

public:
    CDERoster();
    CDERoster(const CDERoster& rhs);
    CDERoster(const CDictRecord& dict_record);
    ~CDERoster();

    std::unique_ptr<CDEItemBase> Clone() const override { return std::make_unique<CDERoster>(*this); }

    void FinishStubTextInit();
    void FinishFieldInit();

    int  GetFieldRowHeight() const { return m_iFieldRowHeight; }
    void SetFieldRowHeight(int i)  { m_iFieldRowHeight = i; }
    void SetFieldRowHeight(CString cs);

    int GetHeadingRowHeight() const { return m_iHeadingRowHeight; }
    void SetHeadingRowHeight(int i) { m_iHeadingRowHeight = i; }
    void SetHeadingRowHeight(CString cs);

    int GetStubColWidth() const             { return m_iStubColWidth; }
    void SetStubColWidth(int iStubColWidth) { m_iStubColWidth = iStubColWidth; }
    void SetStubColWidth(const TCHAR* cs)   { m_iStubColWidth = (int)CIMSAString::Val(cs); }  // csc 12/20/00

    int GetColWidth() const           { return m_iColWidth; }
    void SetColWidth(int i)           { m_iColWidth = i; }
    void SetColWidth(const TCHAR* cs) { m_iColWidth = (int)CIMSAString::Val(cs); }  // csc 8/22/00

    RosterOrientation GetOrientation() const           { return m_orientation; }
    void SetOrientation(RosterOrientation orientation) { m_orientation = orientation; }

    bool GetUseOccurrenceLabels() const                     { return m_useOccurrenceLabels; }
    void SetUseOccurrenceLabels(bool use_occurrence_labels) { m_useOccurrenceLabels = use_occurrence_labels; }

    bool GetRightToLeft() const { return m_bRightToLeft; }
    void SetRightToLeft(bool b) { m_bRightToLeft = b; }

    FreeMovement GetFreeMovement() const             { return m_freeMovement; }
    void SetFreeMovement(FreeMovement free_movement) { m_freeMovement = free_movement; }
    bool UsingFreeMovement() const                   { return ( m_freeMovement != FreeMovement::Disabled ); }

    CRect GetTotalSize() const           { return m_cTotalSize; }
    void SetTotalSize(const CRect& rect) { m_cTotalSize = rect; }
    void SetTotalSize(CString cs);

    // methods for adding dict items (and hence, columns) to the roster

    void AddNonOccItem(const CDictRecord* pDR, const CDictItem* pDI, int iFormNum, const DragOptions& drag_options,
                       CDEFormFile* pFF, int& index);

    // methods for m_aCol

    void AddCol(CDECol* pCol)             { m_aCol.emplace_back(pCol); }
    int  GetNumCols()  const              { return (int)m_aCol.size(); }
    void InsertColAt(CDECol* pCol, int i) { m_aCol.insert(m_aCol.begin() + i, pCol); }
    void RemoveCol(int iCol)              { m_aCol.erase(m_aCol.begin() + iCol); }        // don't kill the col ptr
    void RemoveCol(CDECol* pCol)          { RemoveCol(GetColIndex(pCol)); } // don't kill the col ptr
    void RemoveAndDeleteCol(int iCol);

    CDECol* GetCol(int i) const { return m_aCol[i]; }
    int GetColIndex(const CDECol* pCol) const;
    int GetColIndex(const CDEField* pField) const;
    int GetColIndex(const CDEBlock* pBlock) const;
    int AdjustColIndexForRightToLeft(int i) const;
    int GetGroupInsertIndexForCol(int iSearchCol) const;

    // misc

    void RemoveItem(int iIndex) override;
    void RecalcOffsets (int iCol=0);     // default to 1st col if nothing passed

    // "accessory" methods for m_aItems in CDEGroup :)

    void FillItemPtrs();
    void FillItemPtrs2(); //the above one deletes the items which it should not.

    void CheckRosterIntegrity();


    // stub text (one entry for each occurrence)
    // --------------------------------------------------
    const CDETextSet& GetStubTextSet() const { return m_stubTextSet; }
    CDETextSet& GetStubTextSet()             { return m_stubTextSet; }

    void SetAllStubs(const CDEText* stub_template, const DictNamedBase* dict_element = nullptr);
    void RefreshStubsFromOccurrenceLabels(const CDEFormFile& form_file);


    // free cells
    // --------------------------------------------------
    size_t GetNumFreeCells() const { return m_freeCells.size(); }

    const SharedPointerVectorWrapper<CDEFreeCell> GetFreeCells() const { return SharedPointerVectorWrapper<CDEFreeCell>(m_freeCells); }
    SharedPointerVectorWrapper<CDEFreeCell, true> GetFreeCells()       { return SharedPointerVectorWrapper<CDEFreeCell, true>(m_freeCells); }

    const CDEFreeCell& GetFreeCell(size_t index) const { ASSERT(index < m_freeCells.size()); return *m_freeCells[index]; }
    CDEFreeCell& GetFreeCell(size_t index)             { ASSERT(index < m_freeCells.size()); return *m_freeCells[index]; }
    CDEFreeCell& GetOrCreateFreeCell(int row, int column);

    void AddFreeCell(std::shared_ptr<CDEFreeCell> free_cell) { ASSERT(free_cell != nullptr); m_freeCells.emplace_back(std::move(free_cell)); }
    void InsertFreeCell(size_t index, std::shared_ptr<CDEFreeCell> free_cell);
    void RemoveFreeCell(size_t index);


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, bool bSilent = false);
    void Save(CSpecFile& frmFile) const override;

    void serialize(Serializer& ar);


private:
    void RemoveAndDeleteAllCols();

    template<typename CF>
    void SetAllStubs(const CDEText& stub_template, CF callback_function);


private:
    // inheriting array of items from CDEGroup, though rosters will only use CDEFields (both disp & keyed)
    bool m_bRightToLeft;

    std::vector<CDECol*> m_aCol;

    CRect m_cTotalSize; // display sz inherited from CDEFormBase

    int m_iFieldRowHeight;   // all FieldRows have the same height when HORIZONTAL
    int m_iColWidth;         // all columns have the same width when VERTICAL
    int m_iHeadingRowHeight; // heading row has unique height
    int m_iStubColWidth;     // width of stub column, for use with VERTICAL

    RosterOrientation m_orientation;
    bool m_useOccurrenceLabels;
    FreeMovement m_freeMovement;

    CDETextSet m_stubTextSet;

    SharedPointerAsValueVector<CDEFreeCell> m_freeCells;
};
