#pragma once

//---------------------------------------------------------------------------
//  File name: Form2.h
//
//  Description:
//          Header for engine-Form class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//
//---------------------------------------------------------------------------

class CSymbolForm : public Symbol
{
// --- Data members --------------------------------------------------------
private:
    // --- associated CDEForm
    CDEForm* m_pCDEForm;

// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
public:
    CSymbolForm(std::wstring name, CDEForm* pCDEForm);

    // --- associated CDEForm
public:
    CDEForm* GetForm() const { return m_pCDEForm; }

    // only maked as virtual so that it is accessible by zEngineO
    virtual int GetSymGroup() const;
};
