#pragma once
//***************************************************************************
//  File name: TbHdrClc.h
//
//  Description:
//       Helper classes to calculate table row and column headers when exporting
//       tables.  Row and col headers are needed for making exported tables
//       useable with a screen reader (sec 508 compliance).
//
//***************************************************************************

#include <zTableF/GridExpt.h>


class CRowHeaderCalculator
{
public:

    // constructor - give it total number of rows in table incl. hidden rows
    CRowHeaderCalculator(int numRows);

    // add next to row table and compute headers
    void AddRow(int iRow,  // row number counting hidden rows (row number in original table)
                bool bRowHidden, // if this row is hidden
                int iStubLevel, // level of stub where 1 is top level, 2 is child of 1, etc..
                int iVisRowsSoFar); // row number not including hidden rows (row number in exported table)

    // get headers for a given row (iRow is row number including hidden rows).
    const CDWordArray& GetHeaders(int iRow) const;

protected:

    // find parent row for headers of this row
    int FindParent(int iRow, int iStubLevel) const;

private:

    CArray<CDWordArray, CDWordArray&> m_aHeaders;
    CMap<int, int, int, int> m_aOrigToVisibleRowMap;
    CMap<int, int, int, int> m_aLastRowAtLevel;
    CByteArray m_aRowVisibleFlags;
};


class CColHeaderCalculator
{
public:

    // constructor - give it total number of cols in table incl. hidden rows
    CColHeaderCalculator(int numCols);


    // add next col to table and compute headers
    void AddColHeader(
                int iRow,  // row num including hidden rows
                int iCol,  // col number counting hidden cols (col number in original table)
                const CTableGridExporter::CJoinRegion& actualJoinCoords); // actual (exported) coords of joined cell (or of cell if not joined)

    // get headers for a given row (iCol is col number including hidden cols).
    const CArray<CTableGridExporter::CJoinRegion>& GetHeaders(int iCol) const;

private:

    CArray<CArray<CTableGridExporter::CJoinRegion> > m_aHeaders;
};
