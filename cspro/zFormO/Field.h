#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/ItemBase.h>
#include <zFormO/Text.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/PortableFont.h>
#include <zDictO/CaptureInfo.h>

class CDictItem;
class DictValueSet;
class DragOptions;


/***************************************************************************
*
*                       CDEField : public CDEItemBase
*
***************************************************************************/

enum class ValidationMethod { Validate, ValidateWithConfirmation, ValidateWithoutConfirmation };

const CString DICTIONARY_NAME =     _T("DictionaryName");
const CString DICTIONARY_LABEL =    _T("DictionaryLabel");
const CString CUSTOM_TEXT =         _T("Custom");


class CLASS_DECL_ZFORMO CDEField : public CDEItemBase
{
    DECLARE_DYNAMIC (CDEField)

public:
    // construction/destruction
    CDEField();
    CDEField(const CDEField& field);         // copy constructor
    CDEField(const CString& sItemName, const CString& sDictName);

    CDEField(const CDictItem* pDictItem,
              int iFormNum,
              const CString& sDictName,
              const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
              const DragOptions& drag_options,
              int iFieldSpacing = 0,        // don't care about this is it's a roster
              CPoint dropPoint = CPoint(0, 0),
              int row = NONE);

    CDEField(const CDictItem* pDictItem,
              int iFormNum,
              const CString& sDictName,
              const DragOptions& drag_options);
    ~CDEField();

    void operator=(const CDEField& field);

    std::unique_ptr<CDEItemBase> Clone() const override { return std::make_unique<CDEField>(*this); }

    eItemType GetFormItemType() const override { return Field; }


    // access methods

    const CString&          GetItemName     () const { return m_sItemName; }
    const CString&          GetItemDict     () const { return m_sDictName; }
    const CString&          GetData         () const { return m_sData; }

    const CDEText&          GetCDEText      () const { return m_cText; }
    CDEText&                GetCDEText      ()       { return m_cText; }

    bool IsEnterKeyRequired                 () const { return m_bRequireEnter; }
    bool IsProtected                        () const { return m_bProtected; }
    bool IsSequential                       () const { return m_bSequential; }
    bool IsMirror                           () const { return m_bMirror; }
    bool IsKeyed                            () const { return !m_bMirror; }
    bool IsPersistent                       () const { return m_bPersistent; }
    bool IsAutoIncrement                    () const { return m_bAutoIncrement; }
    bool IsUpperCase                        () const { return m_bUpperCase; }
    bool IsHidden                           () const { return m_bHidden; }
    bool IsHiddenInCaseTree                 () const { return m_bHideInCaseTree; }
    bool IsAlwaysVisualValue                () const { return m_alwaysVisualValue; }

    const CString& GetText() const   { return m_cText.GetText(); }

    const CRect& GetTextDims() const { return m_cText.GetDims(); }
    CRect& GetTextDims()             { return m_cText.GetDims(); }

    UINT GetKLID() const { return m_KLID; }
    void SetKLID(UINT klid) { m_KLID = klid; }

    const CaptureInfo& GetCaptureInfo() const            { return m_captureInfo; }
    void SetCaptureInfo(const CaptureInfo& capture_info) { m_captureInfo = capture_info; }

    void SetupCaptureInfo(const CDictItem& dictionary_item, const DragOptions& drag_options);

    // returns the field's capture info, or if unspecified, the dictionary item's capture info
    const CaptureInfo&  GetEvaluatedCaptureInfo() const;

    void SetUnicodeTextBoxSize(CSize size) { m_szUnicodeTextBox = size; }
    CSize GetUnicodeTextBoxSize() const    { return m_szUnicodeTextBox; }

    bool UseUnicodeTextBox() const       { return m_bUseUnicodeTextBox; }
    void SetUseUnicodeTextBox(bool flag) { m_bUseUnicodeTextBox = flag; }

    bool AllowMultiLine() const        { return m_bAllowMultiLine; }
    void SetMultiLineOption(bool flag) { m_bAllowMultiLine = flag; }

    FieldLabelType GetFieldLabelType() const                { return m_fieldLabelType; }
    void SetFieldLabelType(FieldLabelType field_label_type) { m_fieldLabelType = field_label_type; }

    // set methods
    void SetCDEText         (const CDEText& ct)     { m_cText = ct; }
    void SetCDEText         (const CString& cTxt)   { m_cText.SetText(cTxt); }
    void SetCDEText         (CRect cr, const CString& cs);

    void SetTextDims        (CRect cr)                      { m_cText.SetDims (cr); }
    void SetTextDims        (int x1, int y1, int x2, int y2) { m_cText.SetDims (x1,y1,x2,y2); }

    void SetItemInfo        (CString cs);               // used to parse the itemVar & possible itemDict
    void SetItemName        (const CString& cs)         { m_sItemName = cs; }
    void SetItemDict        (const CString& cs)         { m_sDictName = cs; }

    void IsEnterKeyRequired         (bool b)            { m_bRequireEnter = b; }
    void IsProtected                (bool b)            { m_bProtected = b; }
    void IsSequential               (bool b)            { m_bSequential = b; }
    void IsMirror                   (bool b)            { m_bMirror = b; }
    void IsPersistent               (bool b)            { m_bPersistent = b; }
    void IsAutoIncrement            (bool b)            { m_bAutoIncrement = b; }

    void IsUpperCase                (bool b)            { m_bUpperCase = b; }

    void IsHidden                   (bool b)            { m_bHidden = b; }
    void IsHiddenInCaseTree         (bool b)            { m_bHideInCaseTree = b; }

    void SetAlwaysVisualValue       (bool b)            { m_alwaysVisualValue = b; }

    void SetData                    (const CString& cs) { m_sData = cs; ASSERT(m_sData.Find('\r') == -1); }

    /*********************************************************************************
            RUNTIME Functions
    **********************************************************************************/
    //Set && Get DictItem
    void SetDictItem(const CDictItem* pDictItem) { m_pDictItem = pDictItem;}
    const CDictItem* GetDictItem() const         { return m_pDictItem; }

    const CString& GetPlusTarget() const                 { return m_sPlusTarget; }
    void           SetPlusTarget(const CString& sString) { m_sPlusTarget = sString; }

    bool        GetVerifyFlag() const { return m_bVerify;}
    void        SetVerifyFlag(bool bFlag) { m_bVerify = bFlag;}

    ValidationMethod GetValidationMethod() const                             { return m_eValidationMethod; }
    void             SetValidationMethod(ValidationMethod validation_method) { m_eValidationMethod = validation_method; }

    int GetRuntimeOccurrence() const  { return m_runtimeOccurrence; }
    void SetRuntimeOccurrence(int occ) { m_runtimeOccurrence = occ; }

    bool Compare(CDEField* pField);

    const PortableFont& GetFont() const { return m_font; }
    void SetFont(PortableFont font)     { m_font = std::move(font); }


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, bool bSilent = false) override;
    void Save(CSpecFile& frmFile) const override;
    void Save(CSpecFile& frmFile, bool bGridField = false) const override; // to handle CDECol's field saves

    void serialize(Serializer& ar);


private:
    void BaseConstructorInit();


private:
    const CDictItem* m_pDictItem;

    CString m_sItemName; // the unique .dcf name of the item; empty if we have a wkg field
    CString m_sDictName; // the dictionary which contains the item; empty if wkg field

    CDEText m_cText; // text accompanying the field for display on the page

    bool m_bRequireEnter;   // if T, the keyer must press 'enter' to advance to next field
    bool m_bProtected;      // if T, the keyer can not access/modify the field
    bool m_bSequential;     // if T, each successive value will be incremented from the one before
    bool m_bMirror;         // if T, the field displays a previously-entered value for the field; the field must have already been dropped as keyed to allow this
    bool m_bPersistent;     // active for batch IDs only; if T, repeats val from prev case
    bool m_bAutoIncrement;
    bool m_bHideInCaseTree;
    bool m_alwaysVisualValue;

    bool m_bVerify;
    ValidationMethod m_eValidationMethod;
    int m_runtimeOccurrence;

    // when the user is presented w/a field's properties, the following behaviour
    // would be present for the above four flags:

    // - if protected is T, requireEnter disabled
    // - if mirror T, remaining three disabled
    // - mirror is always disabled, user can not change from a mirror to non-mirror var or vice versa
    // ********************************************************************************

    // on entry!

    CString m_sData;     // the info they typed
    bool m_bHidden;      // shld the field's data, on entry, be hidden? for passwords

    bool m_bUpperCase;   // gsf added 7-mar-01

    FieldLabelType m_fieldLabelType;

    CaptureInfo m_captureInfo;

    bool m_bUseUnicodeTextBox;
    bool m_bAllowMultiLine;

    CSize m_szUnicodeTextBox;

    UINT m_KLID; // 20120822

    CString m_sPlusTarget;  //"+"Key skip to target

    PortableFont m_font;
};
