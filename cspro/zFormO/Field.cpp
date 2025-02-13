#include "StdAfx.h"
#include "Field.h"
#include "DragOptions.h"


IMPLEMENT_DYNAMIC(CDEField, CDEItemBase)


namespace CaptureInfoSaveTemp
{
    // with the expectation that this file will be JSON in the next version, this hack is here
    // to prevent CaptureType=Unspecified from being written to batch files
    // JSON_TODO static_assert(Serializer::GetCurrentVersion() == Serializer::Iteration_7_7_000_2);

    bool WriteCaptureInfo = true;
}


void CDEField::BaseConstructorInit()
{
    SetItemType (Field);

    m_pDictItem = NULL;

    m_bRequireEnter     = false;
    m_bProtected        = false;
    m_bHideInCaseTree   = false;
    m_alwaysVisualValue = false;
    m_bSequential       = false;
    m_bMirror           = false;
    m_bPersistent       = false;
    m_bAutoIncrement    = false;
    m_bUpperCase        = false;

    m_bHidden           = false; // RHF Nov 22, 2002

    m_sPlusTarget.Empty();

    m_bVerify = true;
    m_eValidationMethod = ValidationMethod::Validate;
    m_fieldLabelType = FormDefaults::FieldLabelType;

    m_bUseUnicodeTextBox = false;
    m_bAllowMultiLine = false;

    m_szUnicodeTextBox = CSize(0,0);

    m_KLID = 0; // 20120822

    m_runtimeOccurrence = NONE;

    m_font = PortableFont::FieldDefault;
}

// don't assign the name to the field here; the empty arg list constructor is called,
// usually, when building the field up from the, frm file, and if that's the case the
// name shld be unique (i'll be checking later anyway that it is);
//
// also, if the field will be keyed, it'll use the dictionary's item name; however,
// if the field will be a display/protected field, then it'll have to get a new
// unique name; it's the creator's responsibility to create a uniq name, if nec

CDEField::CDEField()
    :   CDEItemBase()
{
    BaseConstructorInit();
}


// this constructor will get called from the [Form] blk when loading a .frm file

CDEField::CDEField(const CString& sItemName, const CString& sDictName)
    :   CDEItemBase()
{
    BaseConstructorInit();

    SetName(sItemName);

    SetItemName(sItemName);
    SetItemDict(sDictName);
}


CDEField::CDEField(const CDEField& field)  // copy constructor
    :   CDEItemBase(field),
        m_font(field.m_font)
{
    SetDictItem (field.GetDictItem());  // CDEField level vars
    SetCDEText  (field.GetCDEText());

    SetItemName (field.GetItemName());   // csc 11/16/00
    SetItemDict (field.GetItemDict());   // csc 11/16/00

    IsEnterKeyRequired  (field.IsEnterKeyRequired());
    IsProtected         (field.IsProtected());
    IsSequential        (field.IsSequential());
    IsMirror            (field.IsMirror());
    IsPersistent        (field.IsPersistent());
    IsAutoIncrement     (field.IsAutoIncrement());
    IsUpperCase         (field.IsUpperCase());
    IsHiddenInCaseTree  (field.IsHiddenInCaseTree());
    SetVerifyFlag       (field.GetVerifyFlag());
    SetValidationMethod (field.GetValidationMethod());

    m_captureInfo = field.m_captureInfo;

    SetUseUnicodeTextBox(field.UseUnicodeTextBox());
    SetMultiLineOption(field.AllowMultiLine());

    SetKLID(field.GetKLID());

    SetFieldLabelType(field.GetFieldLabelType());
}


/////////////////////////////////////////////////////////////////////////////
// assign each field to a page, giving it a location, etc.
CDEField::CDEField(const CDictItem* pDictItem,      // dictItem we're pulling vals from
                   int             iFormNum,
                   const CString&  sDictName,      // on which form does this field appear?
                   const std::variant<CDC*, CSize>& pDC_or_single_character_text_extent,
                   const DragOptions& drag_options,
                   int             iFieldSpacing/*=0*/,// how much space to put bet boxes & text; will be 0 if a roster
                   CPoint          dropPt, /*=(0,0)*/
                   int             row /*=NONE*/)          // screen row coord
{
    BaseConstructorInit();

    CString csFieldLabelOrName = ( drag_options.GetTextUse() == TextUse::Label ) ?
        pDictItem->GetLabel() : pDictItem->GetName();

    CSize sizeCh = std::holds_alternative<CDC*>(pDC_or_single_character_text_extent) ? std::get<CDC*>(pDC_or_single_character_text_extent)->GetTextExtent(_T("9")) :
                                                                                       std::get<CSize>(pDC_or_single_character_text_extent);

    if (row == NONE)                // then we're adding a field to a roster
    {
        CRect box (0, 0, pDictItem->GetLen() * sizeCh.cx + 10, 0);

        SetDims (box);  // i need this later when determining the roster col width for this item
    }
    else
    {
        int temp;
        CRect box (50,row,50,row);

        if (dropPt.x != 0)
        {
            box.left = box.right = dropPt.x;
        }
        if (dropPt.y != 0)
        {
            box.top = box.bottom = dropPt.y;
        }

        CSize sizeText = std::holds_alternative<CDC*>(pDC_or_single_character_text_extent) ? std::get<CDC*>(pDC_or_single_character_text_extent)->GetTextExtent(csFieldLabelOrName) :
            CSize(std::get<CSize>(pDC_or_single_character_text_extent).cx * csFieldLabelOrName.GetLength(), std::get<CSize>(pDC_or_single_character_text_extent).cy);

        if (drag_options.GetTextLayout() == TextLayout::Left)    // left-justify the text, right-justify the box
        {
            box.right  += sizeText.cx;
            box.bottom += sizeText.cy;

            SetCDEText (box, csFieldLabelOrName);      // set the text dims

            temp = box.right;

            // determine the data entry box placement (right-justifying)

            box.right = box.left + iFieldSpacing;
            box.left = box.right - (pDictItem->GetLen() * sizeCh.cx + 10);

            if (box.left < temp)    // if the text would stomp over box location
            {
                int offset = temp - box.left;   // how much would be stomped on?

                box.OffsetRect(offset + FormDefaults::FormPadding, 0);  // make that our offset
            }
            box.top -= 3;
            box.bottom = box.top + sizeCh.cy + 6;

            SetDims (box);  // sets the data entry box dims

            box.top += 3;   // have the box end at orig position
        }
        else    // left-justify the box, right-justify the text
        {
            box.right += pDictItem->GetLen() * sizeCh.cx + 10;

            box.top -= 3;
            box.bottom = box.top + sizeCh.cy + 6;

            SetDims (box);          // sets the dims of the data entry box

            temp = box.right;

            // now, right-justify the text

            box.right = box.left + iFieldSpacing;
            box.left = box.right - sizeText.cx;

            if (box.left < temp)    // if the box would stomp over the text
            {
                int offset = temp - box.left;   // how much would be stomped on?

                box.OffsetRect(offset + FormDefaults::FormPadding, 0);
            }
            box.top += 3;   // reset it
            box.bottom = box.top + sizeText.cy;

            SetCDEText(box, csFieldLabelOrName);       // set the text dims
        }
    }

    FieldLabelType field_label_type = ( drag_options.GetTextUse() == TextUse::Label ) ? FieldLabelType::DictionaryLabel :
                                                                                        FieldLabelType::DictionaryName;

    SetCDEText(( field_label_type == FieldLabelType::DictionaryLabel ) ? pDictItem->GetLabel() : pDictItem->GetName());

    SetFieldLabelType(drag_options.LinkFieldTextToDictionary() ? field_label_type : FieldLabelType::Custom);

    SetName     (pDictItem->GetName());
    SetLabel    (pDictItem->GetLabel());
    SetFormNum  (iFormNum);

    SetItemType (Field);

    SetItemName(pDictItem->GetName());
    SetItemDict(sDictName);

    IsEnterKeyRequired(drag_options.UseEnterKey());

    SetupCaptureInfo(*pDictItem, drag_options);
}


/////////////////////////////////////////////////////////////////////////////
// this func is for initializing a CDERoster's column field member

CDEField::CDEField (const CDictItem* pDictItem,  // dictItem we're pulling vals from
                    int iFormNum,
                    const CString& sDictName,  // in which dict does this field appear?
                    const DragOptions& drag_options)
{
    BaseConstructorInit();

    SetLabel    (pDictItem->GetLabel());
    SetName     (pDictItem->GetName());
    SetFormNum  (iFormNum);
    SetItemName (pDictItem->GetName());
    SetItemDict (sDictName);

    FieldLabelType field_label_type = ( drag_options.GetTextUse() == TextUse::Label ) ? FieldLabelType::DictionaryLabel :
                                                                                        FieldLabelType::DictionaryName;

    SetCDEText(( field_label_type == FieldLabelType::DictionaryLabel ) ? pDictItem->GetLabel() : pDictItem->GetName());

    SetFieldLabelType(drag_options.LinkFieldTextToDictionary() ? field_label_type : FieldLabelType::Custom);

    IsEnterKeyRequired (drag_options.UseEnterKey());

    SetupCaptureInfo(*pDictItem, drag_options);
}


CDEField::~CDEField()
{
    // smg: do i nd to call destructor for m_cText? it'll get called automatically, yea?
}


void CDEField::operator=(const CDEField& field)
{
    const CDEItemBase* pBase = &field;

    m_pDictItem = field.m_pDictItem;

//  from CDEFormBase:

    SetName(pBase->GetName());
    SetLabel(pBase->GetLabel());

    SetDims (field.GetDims());

//  from CDEItemBase:

    SetFormNum (field.GetFormNum());
    SetItemType (Field);

//      from CDEField:

    m_sItemName     = field.m_sItemName;
    m_sDictName     = field.m_sDictName;
    m_cText         = field.m_cText;
    m_sData         = field.m_sData;

    m_bRequireEnter     = field.m_bRequireEnter;
    m_bProtected        = field.m_bProtected;
    m_bHideInCaseTree   = field.m_bHideInCaseTree;
    m_alwaysVisualValue = field.m_alwaysVisualValue;
    m_bSequential       = field.m_bSequential;
    m_bMirror           = field.m_bMirror;
    m_bPersistent       = field.m_bPersistent;
    m_bAutoIncrement    = field.m_bAutoIncrement;
    m_bUpperCase        = field.m_bUpperCase;

    m_sPlusTarget   = field.m_sPlusTarget;

    m_fieldLabelType = field.m_fieldLabelType;

    m_captureInfo = field.m_captureInfo;
    m_bUseUnicodeTextBox = field.m_bUseUnicodeTextBox;
    m_bAllowMultiLine = field.m_bAllowMultiLine;

    m_KLID = field.m_KLID;

    m_font = field.m_font;
}


void CDEField::SetCDEText(CRect cr, const CString& cs)
{
    m_cText.SetDims(cr);
    m_cText.SetText(cs);
}


const CaptureInfo& CDEField::GetEvaluatedCaptureInfo() const
{
    if( m_captureInfo.IsSpecified() || m_pDictItem == nullptr )
        return m_captureInfo;

    else
        return m_pDictItem->GetCaptureInfo();
}


// this func is used during Build to do the parsing of the string read in

void CDEField::SetItemInfo(CString cs)
{
    TCHAR* pszArgs = cs.GetBuffer(cs.GetLength());
    TCHAR* pszArg = strtoken(pszArgs, SPACE_COMMA_STR, NULL);

    SetItemName(pszArg);

    pszArg = strtoken(NULL, SPACE_COMMA_STR, NULL);

    if (pszArg != NULL) {   // then fmt was ItemName,DictShortName
        SetItemDict(pszArg);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      bool CDEField::Compare(CDEField* pField)
//
/////////////////////////////////////////////////////////////////////////////////
bool CDEField::Compare(CDEField* pField)
{
    bool bRet = false;
    if(GetName().CompareNoCase(pField->GetName()) !=0 )
        return bRet;
    const CDictItem* pDItem0 = GetDictItem();
    ASSERT(pDItem0); //Make sure u call UpdatePointers before you call this compare
    const CDictItem* pDItem1 = pField->GetDictItem();
    ASSERT(pDItem1);//Make sure u call UpdatePointers before you call this compare
    if(pDItem0 != pDItem1) //Since they come from the same dictionary they should be same
        return bRet;
    return true;
}


void CDEField::SetupCaptureInfo(const CDictItem& dictionary_item, const DragOptions& drag_options)
{
    if( !drag_options.UseExtendedControls() || IsMirror() )
    {
        // use the base capture type when not using extended controls or for mirror fields
        m_captureInfo = CaptureInfo::GetBaseCaptureType(dictionary_item);
    }

    else
    {
        // use the dictionary item's capture info if possible
        if( dictionary_item.GetCaptureInfo().IsSpecified() )
        {
            m_captureInfo = CaptureType::Unspecified;
        }

        // otherwise get the default capture type (which takes to account the item's primary value set)
        else
        {
            m_captureInfo = CaptureInfo::GetDefaultCaptureInfo(dictionary_item);
        }

        m_bUseUnicodeTextBox = ( dictionary_item.GetContentType() == ContentType::Alpha &&
                                 m_captureInfo.GetCaptureType() == CaptureType::TextBox );
    }

    ASSERT(m_captureInfo.IsSpecified() || dictionary_item.GetCaptureInfo().IsSpecified());
}


bool CDEField::Build (CSpecFile& frmFile, bool bSilent /* = false */) {

    CString csCmd, csArg;
    bool bDone = false;
    bool capture_type_specified = false;

    SetItemType (Field);

    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK)
    {
        ASSERT (csCmd.GetLength() > 0);

        if (csCmd[0] == '.')    // then it's a comment line, ignore
            continue;

        else if( csCmd[0] == '[')
        {
            if (csCmd.CompareNoCase(HEAD_TEXT) == 0 )  // [Text]
            {
                m_cText.Build (frmFile);  // the string is the value set's label...
            }
            else
            {
                frmFile.UngetLine();  // let the calling func figure out what we have
            }
            bDone = true;       // either way, done for now
        }

        else if( csCmd.CompareNoCase(FRM_CMD_NAME) == 0 )
            SetName(csArg);

        else if( csCmd.CompareNoCase(FRM_CMD_LABEL) == 0 )
            SetLabel(csArg);

        else if( csCmd.CompareNoCase(FRM_CMD_POSITION) == 0 )
            SetDims (csArg);

        else if( csCmd.CompareNoCase(FRM_CMD_ITEM) == 0 )
            SetItemInfo(csArg);

        else if( csCmd.CompareNoCase(FRM_CMD_LENGTH) == 0 )
            ; // no longer used

        else if( csCmd.CompareNoCase(FRM_CMD_ENTERKEY) == 0 )
            IsEnterKeyRequired(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_VERIFY) == 0 )
            SetVerifyFlag(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_FORCEORANGE) == 0 )
        {
            if( TEXT_TO_BOOL(csArg) )
                SetValidationMethod(ValidationMethod::ValidateWithConfirmation);
        }

        else if( csCmd.CompareNoCase(FRM_CMD_VALIDATION_METHOD) == 0 )
        {
            if( csArg.CompareNoCase(CSPRO_ARG_CONFIRM) == 0 )
                SetValidationMethod(ValidationMethod::ValidateWithConfirmation);

            else if( csArg.CompareNoCase(CSPRO_ARG_NOCONFIRM) == 0 )
                SetValidationMethod(ValidationMethod::ValidateWithoutConfirmation);
        }

        else if( csCmd.CompareNoCase(FRM_CMD_SKIPTO) == 0 )
        {
            if( !csArg.IsEmpty() )
                SetPlusTarget(csArg);
        }

        else if( csCmd.CompareNoCase(FRM_CMD_PROTECTED) == 0 )
            IsProtected(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_HIDE_IN_CASETREE) == 0 )
            IsHiddenInCaseTree(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_ALWAYS_VISUAL_VALUE) == 0 )
            SetAlwaysVisualValue(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_SEQUENTIAL) == 0 )
            IsSequential(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_MIRROR) == 0 )
            IsMirror(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_PERSISTENT) == 0 )
            IsPersistent(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_AUTOINCREMENT) == 0 )
            IsAutoIncrement(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_UPPERCASE) == 0 )
            IsUpperCase(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_USEUNICODETEXTBOX) == 0 )
            SetUseUnicodeTextBox(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_ALLOWMULTILINE) == 0 )
            SetMultiLineOption(TEXT_TO_BOOL(csArg));

        else if( csCmd.CompareNoCase(FRM_CMD_FIELD_LABEL_TYPE) == 0 )
        {
            if (csArg.CompareNoCase (DICTIONARY_NAME) == 0 )
                SetFieldLabelType(FieldLabelType::DictionaryName);

            else if(csArg.CompareNoCase (DICTIONARY_LABEL) == 0 )
                SetFieldLabelType(FieldLabelType::DictionaryLabel);

            else  //set it to custom if it is custom or anything else
                SetFieldLabelType(FieldLabelType::Custom);
        }

        else if( csCmd.CompareNoCase(CMD_CAPTURE_TYPE) == 0 ||
                 csCmd.CompareNoCase(_T("DataCaptureType")) == 0 ) // used prior to CSPro 7.7
        {
            capture_type_specified = true;

            // prior to 7.1, the date format was serialized on the capture type line
            bool use_capture_info_build = true;
            int iCommaPos = csArg.Find(_T(","));

            if( iCommaPos != -1 )
            {
                CString csPart1 = csArg.Left(iCommaPos).Trim();
                CString csPart2 = csArg.Mid(iCommaPos + 1).Trim();

                std::optional<CaptureInfo> capture_info = CaptureInfo::GetCaptureTypeFromSerializableName(csPart1);

                if( capture_info == CaptureType::Date )
                {
                    m_captureInfo.SetCaptureType(CaptureType::Date);
                    m_captureInfo.GetExtended<DateCaptureInfo>().SetFormat(csPart2);
                    use_capture_info_build = false;
                }
            }

            if( use_capture_info_build )
                m_captureInfo.Build(frmFile, csArg);
        }

        else if( csCmd.CompareNoCase(FRM_CMD_KEYBOARD_ID) == 0 ) // 20120817
        {
            SetKLID(_tcstoul(csArg,NULL,10));
        }

        else if( csCmd.CompareNoCase(_T("Occurrence")) == 0 )
        {
            ASSERT(false); // occurrences were never fully implemented
        }

        else if( csCmd.CompareNoCase(FRM_CMD_FORMNUM) == 0 )
        {
            SetFormNum(csArg);
        }

        else                       // Incorrect attribute
        {
            if (!bSilent)
            {
                ErrorMessage::Display(FormatText(_T("Incorrect [Field] attribute\n\n%s"), (LPCTSTR)csCmd));
            }

            ASSERT(false);
        }
    }

    // prior to CSPro 7.7, the capture type wasn't specified for text boxes
    if( !capture_type_specified )
        m_captureInfo.SetCaptureType(CaptureType::TextBox);

    // smg: nd to set dict ptrs too
    return bDone;
}


void CDEField::Save(CSpecFile& frmFile) const
{
    this->Save (frmFile, false);
}

void CDEField::Save(CSpecFile& frmFile, bool bGridField) const
{
    CaptureInfoSaveTemp::WriteCaptureInfo = ( PortableFunctions::PathGetFileExtension<CString>(frmFile.GetFileName()).CompareNoCase(FileExtensions::Order) != 0 );
    
    const CDictItem* pDictItem = GetDictItem();

    frmFile.PutLine(HEAD_FIELD);
    frmFile.PutLine(FRM_CMD_NAME, GetName());

    if (!GetDims().IsRectEmpty())   // when a field's w/in a roster or being used for a batch run,
    {                                                               // the dimensions won't apply
        CString csDims;
        WriteDimsToStr(csDims);
        frmFile.PutLine(FRM_CMD_POSITION, csDims);
    }

    // get the item/variable's name
    CString csItemName = GetItemName();

    if( !GetItemDict().IsEmpty() )
        csItemName += _T(",") + GetItemDict();

    frmFile.PutLine(FRM_CMD_ITEM, csItemName);

    // since the default settings for the following 4 bools is false, only output
    // the values if different

    if (IsEnterKeyRequired())
        frmFile.PutLine(FRM_CMD_ENTERKEY, CSPRO_ARG_YES);

    if (IsProtected())
        frmFile.PutLine(FRM_CMD_PROTECTED, CSPRO_ARG_YES);

    if (IsHiddenInCaseTree())
        frmFile.PutLine(FRM_CMD_HIDE_IN_CASETREE, CSPRO_ARG_YES);

    if (IsAlwaysVisualValue())
        frmFile.PutLine(FRM_CMD_ALWAYS_VISUAL_VALUE, CSPRO_ARG_YES);

    if (IsSequential())
        frmFile.PutLine(FRM_CMD_SEQUENTIAL, CSPRO_ARG_YES);

    if (IsMirror())
        frmFile.PutLine(FRM_CMD_MIRROR, CSPRO_ARG_YES);

    if (IsPersistent())
        frmFile.PutLine(FRM_CMD_PERSISTENT, CSPRO_ARG_YES);

    if( IsAutoIncrement() )
        frmFile.PutLine(FRM_CMD_AUTOINCREMENT,CSPRO_ARG_YES);

    if (!GetVerifyFlag())
        frmFile.PutLine(FRM_CMD_VERIFY, CSPRO_ARG_NO);

    if( GetValidationMethod() != ValidationMethod::Validate )
        frmFile.PutLine(FRM_CMD_VALIDATION_METHOD, GetValidationMethod() == ValidationMethod::ValidateWithConfirmation ? CSPRO_ARG_CONFIRM : CSPRO_ARG_NOCONFIRM);

    if( !GetPlusTarget().IsEmpty() )
        frmFile.PutLine(FRM_CMD_SKIPTO, GetPlusTarget());

    if (IsUpperCase())
        frmFile.PutLine(FRM_CMD_UPPERCASE, CSPRO_ARG_YES);

    switch(GetFieldLabelType()) {
         case FieldLabelType::DictionaryName:
             frmFile.PutLine(FRM_CMD_FIELD_LABEL_TYPE, DICTIONARY_NAME);
             break;
         case FieldLabelType::DictionaryLabel:
             frmFile.PutLine(FRM_CMD_FIELD_LABEL_TYPE, DICTIONARY_LABEL);
             break;
        /*case Custom:
        default:
             frmFile.PutLine(FRM_CMD_FIELD_LABEL_TYPE, CUSTOM_TEXT);
             break;*/
    }


    if( pDictItem != nullptr && pDictItem->GetContentType() == ContentType::Alpha )
    {
        if( UseUnicodeTextBox() )
            frmFile.PutLine(FRM_CMD_USEUNICODETEXTBOX,CSPRO_ARG_YES);

        if( AllowMultiLine() )
            frmFile.PutLine(FRM_CMD_ALLOWMULTILINE,CSPRO_ARG_YES);
    }

    // capture info
    if( CaptureInfoSaveTemp::WriteCaptureInfo )
        m_captureInfo.Save(frmFile, true);

    if( GetKLID() ) // 20120822
        frmFile.PutLine(FRM_CMD_KEYBOARD_ID,GetKLID());

    if (GetFormNum() != NONE && !bGridField)
        frmFile.PutLine(FRM_CMD_FORMNUM, GetFormNum()+1);

    frmFile.PutLine(_T("  "));         // blank line

    if ( !(GetText().IsEmpty()) )
        m_cText.Save(frmFile);
}


void CDEField::serialize(Serializer& ar) // 20121114
{
    CDEItemBase::serialize(ar);

    if( ( ar.IsSaving() && !FormSerialization::isRepeated(this, ar) ) ||
        ( ar.IsLoading() && !FormSerialization::CheckRepeated(this, ar) ) )
    {
        ar & m_sItemName
           & m_sDictName
           & m_bRequireEnter
           & m_bVerify;
        
        ar.SerializeEnum(m_eValidationMethod);

        ar & m_sPlusTarget
           & m_bProtected
           & m_bSequential
           & m_bMirror
           & m_bPersistent
           & m_bUpperCase;

        ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iOcc

        ar & m_cText
           & m_sData
           & m_bHidden;

        if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_1) )
        {
           ar & m_captureInfo;
        }

        else
        {
            CaptureType capture_type;
            ar.SerializeEnum(capture_type);

            m_captureInfo.SetCaptureType(capture_type);

            CString date_format = ar.Read<CString>();

            if( capture_type == CaptureType::Date )
                m_captureInfo.GetExtended<DateCaptureInfo>().SetFormat(date_format);
        }

        ar & m_bUseUnicodeTextBox
           & m_bAllowMultiLine
           & m_KLID;

        ar.SerializeEnum(m_fieldLabelType);

        ar & m_bHideInCaseTree
           & m_bAutoIncrement;

        if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
            ar & m_alwaysVisualValue;
    }
}
