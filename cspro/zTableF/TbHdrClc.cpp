//***************************************************************************
//  File name: TbHdrClc.cpp
//
//  Description:
//       Helper classes to calculate table row and column headers when exporting
//       tables.  Row and col headers are needed for making exported tables
//       useable with a screen reader (sec 508 compliance).
//
//***************************************************************************
#pragma once

#include "StdAfx.h"
#include "TbHdrClc.h"

/*********
**
** CRowHeaderCalculator
**
**********/

// constructor
CRowHeaderCalculator::CRowHeaderCalculator(int iNumRows)
{
    m_aRowVisibleFlags.SetSize(iNumRows);
    m_aHeaders.SetSize(iNumRows);
}

// add next to row table and compute headers
void CRowHeaderCalculator::AddRow(
            int iRow,  // row number counting hidden rows (row number in original table)
            bool bRowHidden, // if this row is hidden
            int iStubLevel, // level of stub where 1 is top level, 2 is child of 1, etc..
            int iVisRowsSoFar) // row number not including hidden rows (row number in exported table)
{
    // track last row at each stub level so we can use to find parents later on
    m_aLastRowAtLevel[iStubLevel] = iRow;

    // build map of rows to visible row indexes so that headers can be expressed only using
    // visible row indices
    m_aOrigToVisibleRowMap[iRow] = iVisRowsSoFar;

    // track visibility of rows
    m_aRowVisibleFlags.SetAt(iRow, !bRowHidden);

    // update headers

    // get parent row (last row at one higher stub level)
    int iParent = FindParent(iRow, iStubLevel);
    if (iParent != NONE) {

        CDWordArray& aHeaders = m_aHeaders[iRow];

        // add parent headers
        const CDWordArray& aParentHeaders = GetHeaders(iParent);
        aHeaders.Append(aParentHeaders);

        // add parent to list of headers (if parent is visible)
        if (m_aRowVisibleFlags.GetAt(iParent)) {
            int iVisParentRow;
            VERIFY(m_aOrigToVisibleRowMap.Lookup(iParent, iVisParentRow));
            aHeaders.Add(iVisParentRow);
        }
    }
}

// get headers for a given row (iRow is row number including hidden rows).
const CDWordArray& CRowHeaderCalculator::GetHeaders(int iRow) const
{
    return m_aHeaders.GetAt(iRow);
}

// find parent row for headers of this row
int CRowHeaderCalculator::FindParent(int iRow, int iStubLevel) const
{
    int iParent = NONE;
    int i = iStubLevel - 1;
    // look for last row one level up, if that doesn't exist then look one level up from that, etc..
    while (i > 0 && !m_aLastRowAtLevel.Lookup(i, iParent)) {
        --i;
    }
    return iParent;
}

/*********
**
** CColHeaderCalculator
**
**********/

// constructor
CColHeaderCalculator::CColHeaderCalculator(int iNumCols)
{
    m_aHeaders.SetSize(iNumCols);
}

// add next col to table and compute headers
void CColHeaderCalculator::AddColHeader(
            int iRow,  // row num including hidden rows
            int iCol,  // col number counting hidden cols (col number in original table)
            const CTableGridExporter::CJoinRegion& actualJoinCoords) // actual (exported) coords of joined cell (or of cell if not joined)
{
    m_aHeaders.GetAt(iCol).Add(actualJoinCoords);
}

const CArray<CTableGridExporter::CJoinRegion>&
CColHeaderCalculator::GetHeaders(int iCol) const
{
    return m_aHeaders.GetAt(iCol);
}

