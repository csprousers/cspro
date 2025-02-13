#pragma once
//***************************************************************************
//  File name: FormID.h
//
//  Description:
//       Forms tree control node definitions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//              23 Feb 99   smg     modified refs from DD->Form
//
//***************************************************************************

#include <zformf/zFormF.h>
#include <zAppO/Application.h>

class CFormDoc;


enum eNodeType
{
    eINVALID_TYPE = -1,    // enum of Form Tree Item Type
    eFTT_FORMFILE,         // 0
    eFTT_LEVEL,            // 1
    eFTT_FORM,             // 2, underlying form group
    eFTT_BLOCK,            // 3,
    eFTT_FIELD,            // 5
    eFTT_GRID,             // 7
    eFTT_GRIDFIELD,        // 8
    eFFT_EXTERNALCODE,     // 9
    eFFT_REPORT,           // 10
};


class CLASS_DECL_ZFORMF CFormID : public CObject
{
    DECLARE_DYNAMIC(CFormID)

private:
    eNodeType       m_eNodeType;     // what type of node is it? see list above
                                     
    CFormDoc*       m_pFormDoc;      // ptr to the document that owns this tree ctrl
                                     
    HTREEITEM       m_hItem;         // hItem to which this item corresponds to
                                     
    int             m_iForm;         // form index; it it's a level will be NONE
    int             m_iCol;          // column index (for new grid);
    int             m_iRosterField;  // if the item is a roster, then the index of its field

    CDEFormBase*    m_pItem;

// the level index refers of course to the level, since those are top-level under the CDEFormFile;
// the group index, however, only refers to the top-level group beneath the level

public:
    CFormID();
    virtual ~CFormID();

    //Set and  get the document
    void SetFormDoc (CFormDoc* pDoc) { m_pFormDoc = pDoc;}
    CFormDoc* GetFormDoc() const { return m_pFormDoc;}

    void SetNodeType    (int ePos)               { m_eNodeType = (eNodeType) ePos; }
    void SetNodeType    (const eNodeType& eType) { m_eNodeType = eType; }

    void SetFormIndex   (int iForm)              { m_iForm = iForm; }
    void SetColumnIndex (int iCol)               { m_iCol = iCol; }
    void SetRosterField (int iRF)                { m_iRosterField = iRF; }

    void SetHItem       (const HTREEITEM& hItem) { m_hItem = hItem; }
    void SetItemPtr     (CDEFormBase* pFB)       { m_pItem = pFB; }
                                                 
    const eNodeType&    GetItemType()   const    { return m_eNodeType; }
    int                 GetFormIndex()  const    { return m_iForm; }
    int                 GetColumnIndex()const    { return m_iCol; }
    int                 GetRosterField()const    { return m_iRosterField; }
    const HTREEITEM&    GetHItem()      const    { return m_hItem; }
    CDEFormBase*        GetItemPtr()             { return m_pItem; }


    virtual std::optional<CString> GetName() const  { return std::nullopt; }
    virtual std::optional<CString> GetLabel() const { return std::nullopt; }

    virtual TextSource* GetTextSource()     { return nullptr; }
    const TextSource* GetTextSource() const { return const_cast<CFormID*>(this)->GetTextSource(); }
};


// *********************************************************************

class CFormNodeID : public CFormID {

    DECLARE_DYNAMIC(CFormNodeID)

private:
    CIMSAString     m_sFormFileName;
    CIMSAString     m_sFormFileLabel;

    int             m_iCount;       // smg: track the # of references?

public:
    CFormNodeID();
    virtual ~CFormNodeID();

    //Set and get the dict document name
    void            SetFFName   (const CString& sFileName) { m_sFormFileName = sFileName; }
    const CString&  GetFFName()  const { return m_sFormFileName; }

    void            SetFFLabel  (const CString& sFileLabel) { m_sFormFileLabel = sFileLabel; }
    const CString&  GetFFLabel() const { return m_sFormFileLabel; }

    int AddRef (void)   { m_iCount++; return m_iCount; }
    int Release (void)  { m_iCount--; return m_iCount; }

    int GetRefCount() const { return m_iCount; }
};


// *********************************************************************

class CLASS_DECL_ZFORMF FormExternalCodeID : public CFormID
{
    DECLARE_DYNAMIC(FormExternalCodeID)

public:
    FormExternalCodeID(CFormDoc* pDoc, std::optional<CodeFile> code_file);

    std::optional<CString> GetName() const override;
    std::optional<CString> GetLabel() const override;
    TextSource* GetTextSource() override;

    const std::optional<CodeFile>& GetCodeFile() const { return m_codeFile; }

private:
    std::optional<CodeFile> m_codeFile;
};



// *********************************************************************

class CLASS_DECL_ZFORMF FormReportID : public CFormID
{
    DECLARE_DYNAMIC(FormReportID)

public:
    FormReportID(CFormDoc* pDoc, std::shared_ptr<NamedTextSource> named_text_source);

    std::optional<CString> GetName() const override;
    std::optional<CString> GetLabel() const override;

    TextSource* GetTextSource() override;

    NamedTextSource* GetNamedTextSource() { return m_namedTextSource.get(); }

private:
    std::shared_ptr<NamedTextSource> m_namedTextSource;
};
