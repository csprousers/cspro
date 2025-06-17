#pragma once

//---------------------------------------------------------------------------
//  File name: CtStatV.h
//
//  Description:
//          Header for CtStatVar class
//          This class keep a crosstab Stat Var
//
//  History:    Date       Author   Comment
//              ---------------------------
//              01 Jul 02   RHF     Created
//
//---------------------------------------------------------------------------


class CtStatVar
{
public:
    CtStatVar() = default;
    CtStatVar(const CtStatVar&) = default;
    CtStatVar& operator=(const CtStatVar&) = default;

    int GetSubtableNumber() const               { return m_iSubtableNumber; }
    void SetSubtableNumber(int iSubTableNumber) { m_iSubtableNumber = iSubTableNumber; }

    int GetCoordNumber() const            { return m_iCoordNumber; }
    void SetCoordNumber(int iCoordNumber) { m_iCoordNumber = iCoordNumber; }

    int GetSymVar() const       { return m_iSymVar; }
    void SetSymVar(int iSymVar) { m_iSymVar = iSymVar; }

    bool GetHasOverlappedCat() const           { return m_bOverlappedCat; }
    void SetHasOverlappedCat(bool bOverlapped) { m_bOverlappedCat = bOverlapped; }

private:
    int m_iSubtableNumber = -1;
    int m_iCoordNumber = -1;
    int m_iSymVar = -1;
    bool m_bOverlappedCat = false;
};
