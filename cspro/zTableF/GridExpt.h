#pragma once
//***************************************************************************
//  File name: GridExpt.h
//
//  Description:
//       Interface for table grid exporter.  Table grid exproters are used
//  to write table grid out to HTML, RTF, ASCII for save tables and
//  copy/paste.
//
//***************************************************************************

//////////////////////////////
//  CTableGridExporter
//
// Abstract interface for all grid exporters.
// Grid exporter methods are generally called as follows:
//    StartFile
//    for each format
//      WriteFormat
//    for each table:
//      StartTable
//      WriteTitle
//      WriteSubTitle
//      for each row in table
//          StartRow
//          for each cell in row
//              WriteCell
//          EndRow
//      EndTable
//    EndFile
//
// Only selected/visible cells will have WriteCell called for them (same for title
// and subtitle if they are not present or hidden).  In the case of joined cells,
// all cells in the join will have WriteCell called for them.
//
// Row and col coords as well as join coords are relative to the exported, not
// to the original grid.  The exported grid does not include hidden rows and cols,
// unblocked rows and cols (if blocking is on), and stubs/spanners/captions if
// data cells only is on.  So if a cell is at col 5, row 8 in the original grid
// but col 3 and rows 2 and 6 are hidden, then the coords of this cell in
// the export grid are col 4, row 6.
////////////////////////////


struct CTableGridExporter
{
    virtual void StartFile(_tostream& os) = 0;
    virtual void EndFile(_tostream& os) = 0;

    virtual void StartFormats(_tostream& os) = 0;
    virtual void WriteFormat(_tostream& os, const CFmt& fmt) = 0;
    virtual void EndFormats(_tostream& os) = 0;

    virtual void StartTable(_tostream& os, int iNumCols) = 0;
    virtual void StartHeaderRows(_tostream& os) = 0;
    virtual void EndHeaderRows(_tostream& os) = 0;
    virtual void EndTable(_tostream& os) = 0;

    virtual void WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle) = 0;
    virtual void WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle) = 0;

    virtual void StartRow(_tostream& os, int iRow, const CDWordArray& aRowHeaders) = 0;
    virtual void EndRow(_tostream& os) = 0;

    virtual bool IgnoreFormatting() = 0; // GHM 20100818

    struct CJoinRegion
    {
        long iStartRow;
        int iStartCol;
        long iEndRow;
        int iEndCol;
    };

    virtual void WriteCell(_tostream& os,
                           int iCol,
                           int iRow,
                           const CFmt& fmt,
                           const CString& sCellData,
                           const CJoinRegion& join,
                           const CArray<CJoinRegion>& aColHeaders) = 0;
};
