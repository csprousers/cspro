//***************************************************************************
//  File name: DDRules.cpp
//
//  Description:
//       Data Dictionary rule manager implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              02 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "DictionaryValidator.h"
#include <zAppO/DictionaryDescription.h>
#include <zDesignerF/UWM.h>
#include <unordered_map>


namespace
{
    constexpr int MAX_NOTE_LEN = 32000;

    CString csMsg; // global for speed; holds string table lookups

    CString GetErrorName(const DictNamedBase& dict_element)
    {
        CString name = dict_element.GetName();

        if( !name.IsEmpty() )
            name.Append(_T(":  "));

        return name;
    }


    constexpr const TCHAR* IDS_DEF_DICT_LABEL  = _T("New Dictionary");
    constexpr const TCHAR* IDS_DEF_LEVEL_LABEL = _T("Level %d");
    constexpr const TCHAR* IDS_DEF_REC_LABEL   = _T("Record %d-%d");
    constexpr const TCHAR* IDS_DEF_ITEM_LABEL  = _T("Item %d-%d-%d");
    constexpr const TCHAR* IDS_DEF_VSET_LABEL  = _T("Value Set %d");

    constexpr const TCHAR* IDS_RULE_MSG000     = _T("***  DICTIONARY '%s' CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG002     = _T("Dictionary label cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG003     = _T("Dictionary label cannot contain spaces only.");
    constexpr const TCHAR* IDS_RULE_MSG004     = _T("Dictionary label is too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG006     = _T("Dictionary name cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG008     = _T("Dictionary name must contain A-Z, 0-9, or underline. It must start with a letter and end with either a letter or number.");
    constexpr const TCHAR* IDS_RULE_MSG009     = _T("Dictionary name is not unique in this dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG010     = _T("Dictionary name is a reserved word.");

    constexpr const TCHAR* IDS_RULE_MSG021     = _T("Record type start must be <= %d.");
    constexpr const TCHAR* IDS_RULE_MSG022     = _T("Record type start must be 0 if record type length is 0.");
    constexpr const TCHAR* IDS_RULE_MSG023     = _T("Record type start must be greater than 0 if there is more than one record type.");
    constexpr const TCHAR* IDS_RULE_MSG031     = _T("Record type length is too long (maximum is %d).");
    constexpr const TCHAR* IDS_RULE_MSG032     = _T("Record type length must be 0 if record type start is 0.");
    constexpr const TCHAR* IDS_RULE_MSG033     = _T("Record type length must be greater than 0 if there is more than one record type.");

    constexpr const TCHAR* IDS_RULE_MSG051     = _T("Too many levels.  Maximum is 3.");

    constexpr const TCHAR* IDS_RULE_MSG100     = _T("***  LEVEL CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG102     = _T("Level label cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG103     = _T("Level label cannot contain spaces only.");
    constexpr const TCHAR* IDS_RULE_MSG104     = _T("Level label is too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG106     = _T("Level name cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG108     = _T("Level name must contain A-Z, 0-9, or underline. It must start with a letter and end with either a letter or number.");
    constexpr const TCHAR* IDS_RULE_MSG109     = _T("Level name is not unique in this dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG110     = _T("Level name is a reserved word.");

    constexpr const TCHAR* IDS_RULE_MSG200     = _T("***  RECORD CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG202     = _T("Record label cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG203     = _T("Record label cannot contain spaces only.");
    constexpr const TCHAR* IDS_RULE_MSG204     = _T("Record label is too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG206     = _T("Record name cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG208     = _T("Record name must contain A-Z, 0-9, or underline. It must start with a letter and end with either a letter or number.");
    constexpr const TCHAR* IDS_RULE_MSG209     = _T("Record name is not unique in this dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG210     = _T("Record name is a reserved word.");
    constexpr const TCHAR* IDS_RULE_MSG221     = _T("Record type value is required when there is more than 1 record defined.");
    constexpr const TCHAR* IDS_RULE_MSG222     = _T("Record type value length must be equal to record type length = %d.");
    constexpr const TCHAR* IDS_RULE_MSG224     = _T("Record type value cannot be tilde ('~') in the first character position.");
    constexpr const TCHAR* IDS_RULE_MSG225     = _T("Record type value is not unique.");
    constexpr const TCHAR* IDS_RULE_MSG241     = _T("Max records must be from 1 to %d.");
    constexpr const TCHAR* IDS_RULE_MSG242     = _T("Max records must be 1 for records containing items with multiple occurrences.");

    constexpr const TCHAR* IDS_RULE_MSG300     = _T("***  ITEM CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG302     = _T("Item label cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG303     = _T("Item label cannot contain spaces only.");
    constexpr const TCHAR* IDS_RULE_MSG304     = _T("Item label is too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG306     = _T("Item name cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG308     = _T("Item name must contain A-Z, 0-9, or underline. It must start with a letter and end with either a letter or number.");
    constexpr const TCHAR* IDS_RULE_MSG309     = _T("Item name is not unique in this dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG310     = _T("Item name is a reserved word.");
    constexpr const TCHAR* IDS_RULE_MSG316     = _T("Item start must be from 1 to %d.");
    constexpr const TCHAR* IDS_RULE_MSG321     = _T("Item length for numeric items must be between 1 and %d.");
    constexpr const TCHAR* IDS_RULE_MSG322     = _T("Item length for alphanumeric items must be between 1 and %d.");
    constexpr const TCHAR* IDS_RULE_MSG323     = _T("Item length makes record too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG324     = _T("Item length for image, audio and document items must be 1.");
    constexpr const TCHAR* IDS_RULE_MSG336     = _T("Item item type must be ""Item"" or ""Subitem"".");
    constexpr const TCHAR* IDS_RULE_MSG337     = _T("Id items cannot have subitems.");
    constexpr const TCHAR* IDS_RULE_MSG341     = _T("Item occurrences must be between 1 and %d.");
    constexpr const TCHAR* IDS_RULE_MSG342     = _T("This item cannot have more than one occurrence, because subitem %s already does.");
    constexpr const TCHAR* IDS_RULE_MSG343     = _T("This subitem cannot have more than one occurrence, because its parent item %s already does.");
    constexpr const TCHAR* IDS_RULE_MSG344     = _T("Id items cannot have multiple occurrences.");
    constexpr const TCHAR* IDS_RULE_MSG345     = _T("Multiple records cannot have items with multiple occurrences.");
    constexpr const TCHAR* IDS_RULE_MSG351     = _T("Item decimal places must between 0 and %d.");
    constexpr const TCHAR* IDS_RULE_MSG352     = _T("Item decimal places must be 0 for non-numeric items.");
    constexpr const TCHAR* IDS_RULE_MSG353     = _T("Item decimal places must be less than or equal to item length.");
    constexpr const TCHAR* IDS_RULE_MSG354     = _T("Item decimal places must be 0 for Id Items.");
    constexpr const TCHAR* IDS_RULE_MSG361     = _T("Decimal character takes up entire item.");
    constexpr const TCHAR* IDS_RULE_MSG362     = _T("Decimal character does not allow for number of decimal places.");
    constexpr const TCHAR* IDS_RULE_MSG371     = _T("Subitem %s declared before any item.");
    constexpr const TCHAR* IDS_RULE_MSG372     = _T("Subitem %s must start on or after the preceeding item.");
    constexpr const TCHAR* IDS_RULE_MSG373     = _T("Subitem %s extends past preceeding item.");
    constexpr const TCHAR* IDS_RULE_MSG374     = _T("Subitem %s cannot be image, audio or document.");
    constexpr const TCHAR* IDS_RULE_MSG375     = _T("Parent of subitem %s cannot be image, audio or document.");
    constexpr const TCHAR* IDS_RULE_MSG382     = _T("Item overlaps with the record type indicator.");
    constexpr const TCHAR* IDS_RULE_MSG383     = _T("Item %s overlaps with %s.");

    constexpr const TCHAR* IDS_RULE_MSG400     = _T("***  VALUE SET CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG402     = _T("Value set label cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG403     = _T("Value set label cannot contain spaces only.");
    constexpr const TCHAR* IDS_RULE_MSG404     = _T("Value set label is too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG406     = _T("Value set name cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG408     = _T("Value set name must contain A-Z, 0-9, or underline. It must start with a letter and end with either a letter or number.");
    constexpr const TCHAR* IDS_RULE_MSG409     = _T("Value set name is not unique in this dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG410     = _T("Value set name is a reserved word.");

    constexpr const TCHAR* IDS_RULE_MSG500     = _T("***  VALUE CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG504     = _T("Value label is too long (maximum %d characters).");
    constexpr const TCHAR* IDS_RULE_MSG506     = _T("Missing From value.");
    constexpr const TCHAR* IDS_RULE_MSG511     = _T("Special values must have only a From value.");
    constexpr const TCHAR* IDS_RULE_MSG512     = _T("Special values cannot be defined for non-numeric items.");

    constexpr const TCHAR* IDS_RULE_MSG600     = _T("***  VALUE PAIR CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG601     = _T("From value must be numeric or can be blank only if using the special value NotAppl.");
    constexpr const TCHAR* IDS_RULE_MSG602     = _T("From value must be numeric.");
    constexpr const TCHAR* IDS_RULE_MSG603     = _T("To value must be numeric.");
    constexpr const TCHAR* IDS_RULE_MSG604     = _T("To value cannot be defined for alphanumeric items.");
    constexpr const TCHAR* IDS_RULE_MSG605     = _T("From value is too long.");
    constexpr const TCHAR* IDS_RULE_MSG606     = _T("To value is too long.");
    constexpr const TCHAR* IDS_RULE_MSG607     = _T("To value cannot be defined for special values.");
    constexpr const TCHAR* IDS_RULE_MSG611     = _T("To value must be greater than From value.");

    constexpr const TCHAR* IDS_RULE_MSG700     = _T("***  RELATION CONTAINS ERRORS  ***");
    constexpr const TCHAR* IDS_RULE_MSG701     = _T("Relation name cannot be empty.");
    constexpr const TCHAR* IDS_RULE_MSG703     = _T("Relation name must contain A-Z, 0-9, or underline. It must start with a letter and end with either a letter or number.");
    constexpr const TCHAR* IDS_RULE_MSG704     = _T("Relation name is not unique in this dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG705     = _T("Relation name is a reserved word.");
    constexpr const TCHAR* IDS_RULE_MSG706     = _T("Primary %s is not in the dictionary.");
    constexpr const TCHAR* IDS_RULE_MSG707     = _T("Primary %s is not a multiple item.");
    constexpr const TCHAR* IDS_RULE_MSG708     = _T("Primary %s is not a multiple record.");
    constexpr const TCHAR* IDS_RULE_MSG709     = _T("Primary %s is not a record or item.");
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::DictionaryValidator
//
/////////////////////////////////////////////////////////////////////////////

DictionaryValidator::DictionaryValidator()
{
    m_pDict = NULL;
    m_iLevelNum = NONE;
    m_iRecordNum = NONE;
    m_iItemNum = NONE;
    m_iVSetNum = NONE;
    m_iInvalidEdit = NONE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValidSave
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValidSave(const CDataDict& dictionary)
{
    bool bRetVal = true;
    if (dictionary.GetNumLevels() > MaxNumberLevels) {
        csMsg = IDS_RULE_MSG051;
        ErrorMessage::Display(csMsg);
        bRetVal = false;
    }
    UINT iIdLen = 0;
    for( const DictLevel& dict_level : dictionary.GetLevels() ) {
        if (dict_level.GetIdItemsRec()->GetNumItems() == 0) {
            csMsg.Format(_T("Level '%s' does not have any id items!\nAll levels must have at least one id item."), dict_level.GetLabel().GetString());
            ErrorMessage::Display(csMsg);
            bRetVal = false;
        }
        const CDictRecord* pRec = dict_level.GetIdItemsRec();
        for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
            iIdLen += pRec->GetItem(i)->GetLen();
        }
        if (dict_level.GetNumRecords() == 0) {
            csMsg.Format(_T("Level '%s' does not have any records!\nAll levels must have at least one record."), dict_level.GetLabel().GetString());
            ErrorMessage::Display(csMsg);
            bRetVal = false;
        }
    }
    if (iIdLen >= MAX_DICTKEYSIZE) {
        csMsg.Format(_T("Total length of all id items = %d\n Maximum length is %d."), iIdLen, MAX_DICTKEYSIZE-1);
        ErrorMessage::Display(csMsg);
        bRetVal = false;
    }
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Dict)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(CDataDict* pDict,
                          bool bSilent /*=false*/,
                          bool bAutoFixAndRecurse /*=false*/,
                          bool bQuick /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = NONE;
    m_iRecordNum = NONE;
    m_iItemNum = NONE;
    m_iVSetNum = NONE;
    m_iValueNum = NONE;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    m_iInvalidEdit = NONE;
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    if (!CheckNote(*pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckName(pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 2;
        }
    }
    if (!CheckAliases(*pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    if (!CheckRecTypeLen(pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 4;
        }
    }
    if (!CheckRecTypeStart(*pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 3;
        }
    }
    m_iRecordNum = COMMON;
    if (!CheckRecTypeStartLen(pDict)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 3;
        }
    }
    bValid = (m_iInvalidEdit == NONE);
    if (bAutoFixAndRecurse) {
        for( size_t level_number = 0; level_number < pDict->GetNumLevels(); ++level_number ) {
            if (level_number >= MaxNumberLevels) {
                bValid = false;
                csMsg = IDS_RULE_MSG051;
                m_csErrorReport += GetErrorName(pDict->GetLevel(level_number)) + csMsg + CRLF;
                pDict->RemoveLevel(level_number);
                --level_number;
            }
            else {
                bValid &= IsValid(pDict->GetLevel(level_number), (int)level_number, true, bAutoFixAndRecurse, bQuick);
            }
        }

        for( size_t r = 0; r < pDict->GetNumRelations(); ++r ) {
            if (!IsValid(pDict->GetRelation(r), !bSilent, bAutoFixAndRecurse)) {
                pDict->RemoveRelation(r);
                --r;
                bValid = false;
            }
        }
    }
    if (!bSilent && !bValid) {
        // *** Dictionary contains errors ***
        csMsg = IDS_RULE_MSG000;
        // RHF INIC Jul 10, 2003
        CString csFullMsg;

        csFullMsg.Format( csMsg, pDict->GetName().GetString() );
        csMsg = csFullMsg;
        // RHF INIC Jul 10, 2003

        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    ResetRecLens();
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLabel(Dict)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLabel(CDataDict* pDict)
{
    bool bValid = true;
    CString csLabel = pDict->GetLabel();

    // Dictionary label cannot be empty.
    if (csLabel.GetLength() == 0) {
        bValid = false;
        csMsg = IDS_RULE_MSG002;
        m_csErrorReport += GetErrorName(*pDict) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg = IDS_DEF_DICT_LABEL;
            pDict->SetLabel(csMsg);
        }
        return bValid;
    }
    // Dictionary label cannot contain spaces only.
    if (SO::IsBlank(csLabel)) {
        bValid = false;
        csMsg = IDS_RULE_MSG003;
        m_csErrorReport += GetErrorName(*pDict) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg = IDS_DEF_DICT_LABEL;
            pDict->SetLabel(csMsg);
        }
    }
    // Dictionary label is too long (maximum %1 characters).
    if (csLabel.GetLength() > MAX_LABEL_LEN) {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG004, MAX_LABEL_LEN);
        m_csErrorReport += GetErrorName(*pDict) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pDict->SetLabel(csLabel.Left(MAX_LABEL_LEN));
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckName(Dict)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckName(CDataDict* pDict)
{
    bool bValid = true;
    CIMSAString csName = pDict->GetName();

    // Dictionary name cannot be empty.
    if (csName.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG006;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pDict->SetName(GetDefaultName(pDict->GetLabel()));
            m_pDict->UpdateNameList(*pDict);
        }
        return bValid;
    }
    // Dictionary name must contain A-Z, 0-9, or underline and start with letter.
    if (!csName.IsName())  {
        bValid = false;
        csMsg = IDS_RULE_MSG008;
        m_csErrorReport += GetErrorName(*pDict) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            csName.MakeName();
            pDict->SetName(csName);
            m_pDict->UpdateNameList(*pDict);
        }
    }
    // Dictionary name is not unique in this dictionary.
    if (!pDict->IsNameUnique(csName))  {
        bValid = false;
        csMsg = IDS_RULE_MSG009;
        m_csErrorReport += GetErrorName(*pDict) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pDict->SetName(pDict->GetUniqueName(csName));
            m_pDict->UpdateNameList(*pDict);
        }
    }
    // Dictionary name is a reserved word.
    if (csName.IsReservedWord())  {
        bValid = false;
        csMsg = IDS_RULE_MSG010;
        m_csErrorReport += GetErrorName(*pDict) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pDict->SetName(pDict->GetUniqueName(csName));
            m_pDict->UpdateNameList(*pDict);
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckRecTypeStart(Dict)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckRecTypeStart(CDataDict& dictionary)
{
    bool bValid = true;
    UINT uRecTypeLen = dictionary.GetRecTypeLen();
    UINT uRecTypeStart = dictionary.GetRecTypeStart();

    // Record type start must be <= XXX.
    if (uRecTypeStart > MAX_ITEM_START) {
        bValid = false;
        csMsg = IDS_RULE_MSG021;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dictionary.SetRecTypeStart(MAX_ITEM_START);
        }
    }
    // Record type start must be 0 if record type length is 0.
    size_t num_records = dictionary.GetNumRecords();
    if (uRecTypeLen == 0 && uRecTypeStart != 0 && num_records < 2) {
        bValid = false;
        csMsg = IDS_RULE_MSG022;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dictionary.SetRecTypeStart(0);
        }
    }
    // Record type start must be greater than 0 if there is more than one record type.
    if (uRecTypeStart == 0 && num_records > 1) {
        bValid = false;
        csMsg = IDS_RULE_MSG023;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dictionary.SetRecTypeStart(1);
        }
    }
    // *** if no records are present, then set start to 0
    if (uRecTypeStart != 0 && num_records == 0) {
        if (m_bAutoFixAndRecurse)  {
            dictionary.SetRecTypeStart(0);
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckRecTypeLen(Dict)
//
/////////////////////////////////////////////////////////////////////////////


bool DictionaryValidator::CheckRecTypeLen(CDataDict* pDict)
{
    bool bValid = true;
    UINT uRecTypeLen = pDict->GetRecTypeLen();
    UINT uRecTypeStart = pDict->GetRecTypeStart();

    // Record type length is too long (maximum is %d).
    if (uRecTypeLen > MAX_RECTYPE_LEN)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG031, MAX_RECTYPE_LEN);
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pDict->SetRecTypeLen(MAX_RECTYPE_LEN);
        }
    }
    // Record type length must be 0 if record type start is 0.
    size_t num_records = pDict->GetNumRecords();
    if (num_records < 2 && uRecTypeLen != 0 && uRecTypeStart == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG032;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pDict->SetRecTypeLen(0);
        }
    }
    // Record type length must be greater than 0 if there is more than one record type.
    if (uRecTypeLen == 0 && num_records > 1)  {
        bValid = false;
        csMsg = IDS_RULE_MSG033;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            // impute length of longest record type value
            int iRTLen = 0;
            DictionaryIterator::Foreach<CDictRecord>(*pDict,
                [&](const CDictRecord& dict_record)
                {
                    iRTLen = std::max(iRTLen, dict_record.GetRecTypeVal().GetLength());
                });
            if (iRTLen == 0) {
                iRTLen = IntToString(num_records).GetLength();
            }
            pDict->SetRecTypeLen((UINT) iRTLen);
        }
    }
    // *** If no records are present, then set length to 0
    if (uRecTypeLen != 0 && num_records == 0)  {
        if (m_bAutoFixAndRecurse)  {
            pDict->SetRecTypeLen(0);
        }
    }
    // *** If length = 0, set start to zero
    if (pDict->GetRecTypeLen() == 0) {
        pDict->SetRecTypeStart(0);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckRecTypeStartLen(Dict)
//
/////////////////////////////////////////////////////////////////////////////


bool DictionaryValidator::CheckRecTypeStartLen(CDataDict* pDict)
{
    CDictItem item;
    item.SetStart(pDict->GetRecTypeStart());
    item.SetLen(pDict->GetRecTypeLen());
    return (CheckStartLen(&item));
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Level)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(DictLevel& dict_level,
                          int iLevelNum,
                          bool bSilent /*=false*/,
                          bool bAutoFixAndRecurse /*=false*/,
                          bool bQuick /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = NONE;
    m_iItemNum = NONE;
    m_iVSetNum = NONE;
    m_iValueNum = NONE;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    m_iInvalidEdit = NONE;
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    if (m_iLevelNum >= (int)MaxNumberLevels) {
        csMsg = IDS_RULE_MSG051;
        m_csErrorReport += csMsg + CRLF;
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 1;
        }
    }
    else {
#ifdef WIN_DESKTOP
        DictionaryType dictionary_type;

        if( ( m_iLevelNum > 0 ) &&
            ( AfxGetMainWnd() != nullptr && AfxGetMainWnd()->SendMessage(UWM::Designer::GetDictionaryType, reinterpret_cast<WPARAM>(m_pDict),
                                                                                                           reinterpret_cast<LPARAM>(&dictionary_type)) != 0 ) &&
            ( dictionary_type == DictionaryType::External || dictionary_type == DictionaryType::Working ) )
        {
            csMsg = _T("External and Working Storage dictionaries can have only 1 level.");
            m_csErrorReport += csMsg + CRLF;
            if (m_iInvalidEdit == NONE)  {
                m_iInvalidEdit = 1;
            }
        }
#endif
    }
    if (!CheckNote(dict_level))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(dict_level))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckName(dict_level))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 2;
        }
    }
    if (!CheckAliases(dict_level)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    bValid = (m_iInvalidEdit == NONE);
    if (bAutoFixAndRecurse)  {
        bValid &= IsValid(dict_level.GetIdItemsRec(), iLevelNum, COMMON, true, bAutoFixAndRecurse, bQuick);
        for (int i = 0 ; i < dict_level.GetNumRecords() ; i++)  {
            bValid &= IsValid(dict_level.GetRecord(i), iLevelNum, i, true, bAutoFixAndRecurse, bQuick);
        }
    }

    if (!bSilent && !bValid) {
        // *** Level xxx contains errors ***
        csMsg = IDS_RULE_MSG100;
        csMsg += _T("\r\n\r\n") + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLabel(Level)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLabel(DictLevel& dict_level)
{
    bool bValid = true;
    CString csLabel = dict_level.GetLabel();

    // Level label cannot be empty.
    if (csLabel.GetLength() == 0) {
        bValid = false;
        csMsg = IDS_RULE_MSG102;
        m_csErrorReport += GetErrorName(dict_level) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg.Format(IDS_DEF_LEVEL_LABEL, m_iLevelNum);
            dict_level.SetLabel(csMsg);
        }
        return bValid;
    }
    // Level label cannot contain spaces only.
    if (SO::IsBlank(csLabel)) {
        bValid = false;
        csMsg = IDS_RULE_MSG103;
        m_csErrorReport += GetErrorName(dict_level) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg.Format(IDS_DEF_LEVEL_LABEL, m_iLevelNum);
            dict_level.SetLabel(csMsg);
        }
    }
    // Level label is too long (maximum XXX characters).
    if (csLabel.GetLength() > MAX_LABEL_LEN) {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG104, MAX_LABEL_LEN);
        m_csErrorReport += GetErrorName(dict_level) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            dict_level.SetLabel(csLabel.Left(MAX_LABEL_LEN));
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckName(Level)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckName(DictLevel& dict_level)
{
    bool bValid = true;
    CIMSAString csName = dict_level.GetName();

    // Level name cannot be empty.
    if (csName.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG106;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            dict_level.SetName(GetDefaultName(dict_level.GetLabel()));
            m_pDict->UpdateNameList(dict_level);
        }
        return bValid;
    }
    // Level name must contain A-Z, 0-9, or underline and start with letter.
    if (!csName.IsName())  {
        bValid = false;
        csMsg = IDS_RULE_MSG108;
        m_csErrorReport += GetErrorName(dict_level) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            csName.MakeName();
            dict_level.SetName(csName);
            m_pDict->UpdateNameList(dict_level);
        }
    }
    // Level name is not unique in this dictionary.
    if (!m_pDict->IsNameUnique(csName, m_iLevelNum))  {
        bValid = false;
        csMsg = IDS_RULE_MSG109;
        m_csErrorReport += GetErrorName(dict_level) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dict_level.SetName(m_pDict->GetUniqueName(csName, m_iLevelNum));
            m_pDict->UpdateNameList(dict_level, m_iLevelNum);
        }
    }
    // Level name is a reserved word.
    if (csName.IsReservedWord())  {
        bValid = false;
        csMsg = IDS_RULE_MSG110;
        m_csErrorReport += GetErrorName(dict_level) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dict_level.SetName(m_pDict->GetUniqueName(csName, m_iLevelNum));
            m_pDict->UpdateNameList(dict_level, m_iLevelNum);
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Record)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(CDictRecord* pRec,
                          int iLevelNum,
                          int iRecordNum,
                          bool bSilent /*=false*/,
                          bool bAutoFixAndRecurse /*=false*/,
                          bool bQuick /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = iRecordNum;
    m_iItemNum = NONE;
    m_iVSetNum = NONE;
    m_iValueNum = NONE;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    // sort items in record by start position
    if (!IsSorted(iLevelNum, iRecordNum))  {
        Sort(iLevelNum, iRecordNum);
        m_pDict->UpdateNameList(m_iLevelNum, m_iRecordNum);
    }
    // Empty report string
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    m_iInvalidEdit = NONE;
    if (!CheckNote(*pRec))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(pRec))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckName(pRec))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 2;
        }
    }
    if (!CheckAliases(*pRec)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    if (!CheckRecTypeVal(pRec))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 3;
        }
    }
    if (!CheckRequired(pRec))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 4;
        }
    }
    if (!CheckMaxRecs(pRec))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 5;
        }
    }
    UINT uRecLen = GetRecordLength(m_iLevelNum, m_iRecordNum);
    pRec->SetRecLen(uRecLen);
    ASSERT(pRec->GetRecLen() >= 1 && pRec->GetRecLen() <= MAX_RECLEN);

    bValid = (m_iInvalidEdit == NONE);
    if (bAutoFixAndRecurse)  {
        for (int i = 0 ; i < pRec->GetNumItems() ; i++)  {
            bValid &= IsValid(pRec->GetItem(i), iLevelNum, iRecordNum, i, true, bAutoFixAndRecurse, bQuick);
        }
    }
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG200;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLabel(Record)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLabel(CDictRecord* pRec)
{
    bool bValid = true;
    CString csLabel = pRec->GetLabel();

    // A Level Id record (COMMON) does not have a label
    if (m_iRecordNum == COMMON)  {
        return true;
    }
    // Record label cannot be empty.
    if (csLabel.GetLength() == 0)  {
        csMsg = IDS_RULE_MSG202;
        m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg.Format(IDS_DEF_REC_LABEL, m_iLevelNum, m_iRecordNum);
            pRec->SetLabel(csMsg);
        }
        return false;
    }
    // Record label cannot contain spaces only.
    if (SO::IsBlank(csLabel))  {
        csMsg = IDS_RULE_MSG203;
        m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            csMsg.Format(IDS_DEF_REC_LABEL, m_iLevelNum, m_iRecordNum);
            pRec->SetLabel(csMsg);
        }
        return false;
    }
    // Record label is too long (maximum %d characters).
    if (csLabel.GetLength() > MAX_LABEL_LEN)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG204, MAX_LABEL_LEN);
        m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pRec->SetLabel(csLabel.Left(MAX_LABEL_LEN));
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckName(Record)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckName(CDictRecord* pRec)
{
    bool bValid = true;
    CIMSAString csName = pRec->GetName();

    // A Level Id record (COMMON) does not have a name
    if (m_iRecordNum == COMMON)  {
        return true;
    }
    // Record name cannot be empty.
    if (csName.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG206;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pRec->SetName(GetDefaultName(pRec->GetLabel()));
            m_pDict->UpdateNameList(*pRec, m_iLevelNum, m_iRecordNum);
        }
    }
    // Record name must contain A-Z, 0-9, or underline and start with letter.
    if (bValid)  {
        if (!csName.IsName())  {
            bValid = false;
            csMsg = IDS_RULE_MSG208;
            m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                csName.MakeName();
                pRec->SetName(csName);
                m_pDict->UpdateNameList(*pRec, m_iLevelNum, m_iRecordNum);
            }
        }
    }
    // Record name is not unique in this dictionary.
    if (bValid)  {
        if (!m_pDict->IsNameUnique(csName, m_iLevelNum, m_iRecordNum))  {
            bValid = false;
            csMsg = IDS_RULE_MSG209;
            m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pRec->SetName(m_pDict->GetUniqueName(csName, m_iLevelNum, m_iRecordNum));
                m_pDict->UpdateNameList(*pRec, m_iLevelNum, m_iRecordNum);
            }
        }
    }
    // Record name is reserved word.
    if (bValid)  {
        if (csName.IsReservedWord())  {
            bValid = false;
            csMsg = IDS_RULE_MSG210;
            m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pRec->SetName(m_pDict->GetUniqueName(csName, m_iLevelNum, m_iRecordNum));
                m_pDict->UpdateNameList(*pRec, m_iLevelNum, m_iRecordNum);
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckRecTypeVal(Record)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckRecTypeVal(CDictRecord* pRec)
{
    bool bValid = true;
    CIMSAString csRecTypeVal = pRec->GetRecTypeVal();
    UINT uRTLen = m_pDict->GetRecTypeLen();

    // A Level Id record (COMMON) does not have a value
    if (m_iRecordNum == COMMON)  {
        return true;
    }
    // Record type value is required when there is more than 1 record defined.
    size_t num_records = m_pDict->GetNumRecords();
    if (num_records > 1)  {
        if (csRecTypeVal.IsEmpty())  {
            bValid = false;
            csMsg = IDS_RULE_MSG221;
            m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pRec->SetRecTypeVal(GetDefaultRecTypeVal());
            }
        }
    }
    // Record type value length must be equal to record type length (%d).
    if ((UINT) csRecTypeVal.GetLength() != uRTLen)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG222, (int)uRTLen);
        m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            if (uRTLen == 0)  {
                // adjust the RTLen
                m_pDict->SetRecTypeLen((UINT)csRecTypeVal.GetLength());
                ASSERT(m_pDict->GetRecTypeStart() == 0);
                m_pDict->SetRecTypeStart(1);
            }
            else  {
                // adjust the RT value
                pRec->SetRecTypeVal(csRecTypeVal.AdjustLenLeft(m_pDict->GetRecTypeLen(), ZERO));
            }
        }
    }
    // Record type value cannot be '~' in the first character position.
    if (m_pDict->GetRecTypeStart() == 1) {
        if (!csRecTypeVal.IsEmpty() && csRecTypeVal[0] == '~') {
            bValid = false;
            csMsg = IDS_RULE_MSG224;
            m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pRec->SetRecTypeVal(GetDefaultRecTypeVal());
            }
        }
    }
    // Record type value is not unique.
    for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
        DictLevel& dict_level = m_pDict->GetLevel(level_number);
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            CString csTemp = dict_level.GetRecord(r)->GetRecTypeVal();
            if (csTemp == csRecTypeVal && ((int)level_number != m_iLevelNum || r != m_iRecordNum)) {
                bValid = false;
                csMsg = IDS_RULE_MSG225;
                m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
                if (m_bAutoFixAndRecurse)  {
                    pRec->SetRecTypeVal(GetDefaultRecTypeVal());
                }
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckRequired(Record)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckRequired(CDictRecord* /*pRec*/)
{
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckMaxRecs(Record)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckMaxRecs(CDictRecord* pRec)
{
    bool bValid = true;
    UINT uMaxRecs = pRec->GetMaxRecs();

    // A Level Id record (COMMON) does not have a max records value
    if (m_iRecordNum == COMMON)  {
        return true;
    }

    // Max records must be from 1 to %d.
    if (uMaxRecs < 1 || uMaxRecs > MAX_MAX_RECS)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG241, MAX_MAX_RECS);
        m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pRec->SetMaxRecs(DictionaryDefaults::MaxRecs);
        }
    }

    // Max records must be 1 for records containing items with multiple occurrences.
    if (uMaxRecs > 1) {
        BOOL bMultiple = false;
        for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
            if (pRec->GetItem(i)->GetOccurs() > 1) {
                bMultiple = true;
                break;
            }
        }
        /*if (bMultiple) {
            bValid = false;
            csMsg.Format(IDS_RULE_MSG242, MAX_MAX_RECS);
            m_csErrorReport += GetErrorName(*pRec) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pRec->SetMaxRecs(1);
            }
        }*/
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValidPre(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValidPre (CDictItem* pItem,
                             int iLevelNum,
                             int iRecordNum,
                             int iItemNum,
                             bool bSilent /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = iRecordNum;
    m_iItemNum = iItemNum;
    m_iVSetNum = NONE;
    m_iValueNum = NONE;
    m_bSilent = bSilent;

    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    m_iInvalidEdit = NONE;
    if (!CheckNote(*pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckName(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 2;
        }
    }
    if (!CheckAliases(*pItem)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    if (!CheckDataType(*pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 5;
        }
    }
    if (!CheckItemType(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 6;
        }
    }
    if (!CheckStart(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 3;
        }
    }
    if (!CheckLen(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 4;
        }
    }
    if (!CheckOccurs(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 7;
        }
    }
    if (!CheckDecimal(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 8;
        }
    }
    if (!CheckDecimalChar(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 9;
        }
    }
    if (!CheckZeroFill(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 10;
        }
    }
/*
    if (!CheckSubitem(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 3;
        }
    }
*/
    bValid = (m_iInvalidEdit == NONE);
    // ***  ITEM CONTAINS ERRORS ***
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG300;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(CDictItem* pItem,
                          int iLevelNum,
                          int iRecordNum,
                          int iItemNum,
                          bool bSilent /*=false*/,
                          bool bAutoFixAndRecurse /*=false*/,
                          bool bQuick /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = iRecordNum;
    m_iItemNum = iItemNum;
    m_iVSetNum = NONE;
    m_iValueNum = NONE;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;

    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    m_iInvalidEdit = NONE;
    if (!CheckNote(*pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckName(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 2;
        }
    }
    if (!CheckAliases(*pItem)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    if (!CheckDataType(*pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 5;
        }
    }
    if (!CheckItemType(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 6;
        }
    }
    if (!bQuick)  {
        if (!CheckStart(pItem))  {
            if (m_iInvalidEdit == NONE)  {
                m_iInvalidEdit = 3;
            }
        }
        if (!CheckLen(pItem))  {
            if (m_iInvalidEdit == NONE)  {
                m_iInvalidEdit = 4;
            }
        }
        if (!CheckOccurs(pItem))  {
            if (m_iInvalidEdit == NONE)  {
                m_iInvalidEdit = 7;
            }
        }
    }
    if (!CheckDecimal(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 8;
        }
    }
    if (!CheckDecimalChar(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 9;
        }
    }
    if (!CheckZeroFill(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 10;
        }
    }
    if (!CheckCaptureInfo(pItem)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    if (!CheckSubitem(pItem))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 3;
        }
    }
    if (pItem->GetItemType() == ItemType::Item) {
        bool subitems_valid = true;
        CDictRecord* record = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum);
        for (int isub = m_iItemNum + 1; isub < record->GetNumItems() && record->GetItem(isub)->GetItemType() == ItemType::Subitem; ++isub) {
            subitems_valid &= CheckSubitem(record->GetItem(isub));
        }
        if (!subitems_valid && m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 3;
        }
    }
    if (!bQuick)  {
        if (!CheckStartLen(pItem))  {
            if (m_iInvalidEdit == NONE)  {
                m_iInvalidEdit = 4;
            }
        }
    }
    bValid = (m_iInvalidEdit == NONE);
    if (bAutoFixAndRecurse)  {
        int v = 0;
        for( auto& dict_value_set : pItem->GetValueSets() ) {
            bValid &= IsValid(dict_value_set, iLevelNum, iRecordNum, iItemNum, v, true, bAutoFixAndRecurse);
            ++v;
        }
    }
    // ***  ITEM CONTAINS ERRORS ***
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG300;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLabel(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLabel(CDictItem* pItem)
{
    bool bValid = true;
    CString csLabel = pItem->GetLabel();

    // Item label cannot be empty.
    if (csLabel.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG302;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg.Format(IDS_DEF_ITEM_LABEL, m_iLevelNum, m_iRecordNum, m_iItemNum);
            pItem->SetLabel(csMsg);
        }
        return bValid;
    }
    // Item label cannot contain spaces only.
    if (SO::IsBlank(csLabel))  {
        bValid = false;
        csMsg = IDS_RULE_MSG303;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            csMsg.Format(IDS_DEF_ITEM_LABEL, m_iLevelNum, m_iRecordNum, m_iItemNum);
            pItem->SetLabel(csMsg);
        }
        return bValid;
    }
    // Item label is too long (maximum %d characters).
    if (csLabel.GetLength() > MAX_LABEL_LEN)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG304, MAX_LABEL_LEN);
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetLabel(csLabel.Left(MAX_LABEL_LEN));
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckName(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckName(CDictItem* pItem)
{
    bool bValid = true;
    CIMSAString csName = pItem->GetName();

    // Item name cannot be empty.
    if (csName.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG306;
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetName(GetDefaultName(pItem->GetLabel()));
            m_pDict->UpdateNameList(*pItem, m_iLevelNum, m_iRecordNum, m_iItemNum);
        }
    }
    // Item name must contain A-Z, 0-9, or underline and start with letter.
    if (bValid)  {
        if (!csName.IsName())  {
            bValid = false;
            csMsg = IDS_RULE_MSG308;
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                csName.MakeName();
                pItem->SetName(csName);
                m_pDict->UpdateNameList(*pItem, m_iLevelNum, m_iRecordNum, m_iItemNum);
            }
        }
    }
    // Item name is not unique in this dictionary.
    if (bValid)  {
        if (!m_pDict->IsNameUnique(csName, m_iLevelNum, m_iRecordNum, m_iItemNum))  {
            bValid = false;
            csMsg = IDS_RULE_MSG309;
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pItem->SetName(m_pDict->GetUniqueName(csName, m_iLevelNum, m_iRecordNum, m_iItemNum));
                m_pDict->UpdateNameList(*pItem, m_iLevelNum, m_iRecordNum, m_iItemNum);
            }
        }
    }
    // Item name is reserved word.
    if (bValid)  {
        if (csName.IsReservedWord())  {
            bValid = false;
            csMsg = IDS_RULE_MSG310;
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pItem->SetName(m_pDict->GetUniqueName(csName, m_iLevelNum, m_iRecordNum, m_iItemNum));
                m_pDict->UpdateNameList(*pItem, m_iLevelNum, m_iRecordNum, m_iItemNum);
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckStart(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckStart(CDictItem* pItem)
{
    bool bValid = true;
    UINT uStart = pItem->GetStart();

    // Item start must be from 1 to %d.
    if (uStart < 1 || uStart > MAX_ITEM_START)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG316, MAX_ITEM_START);
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetStart(GetDefaultItemStart(m_iLevelNum, m_iRecordNum, m_iItemNum));
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLen(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLen(CDictItem* pItem)
{
    bool bValid = true;
    int iLen = pItem->GetLen();

    switch(pItem->GetContentType())
    {
        case ContentType::Numeric:
            // Item length for numeric items must be between 1 and %d.
            if (iLen < 1 || iLen > MAX_NUMERIC_ITEM_LEN)  {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG321, MAX_NUMERIC_ITEM_LEN);
                m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
                if (m_bAutoFixAndRecurse) {
                    if (iLen < 1) {
                        pItem->SetLen(1);
                    }
                    else {
                        pItem->SetLen(MAX_NUMERIC_ITEM_LEN);
                    }
                }
            }
            break;

        case ContentType::Alpha:
            // Item length for alpha items must be between 1 and %d.
            if (iLen < 1 || iLen > MAX_ALPHA_ITEM_LEN)  {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG322, MAX_ALPHA_ITEM_LEN);
                m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
                if (m_bAutoFixAndRecurse) {
                    if (iLen < 1) {
                        pItem->SetLen(1);
                    }
                    else {
                        pItem->SetLen(MAX_ALPHA_ITEM_LEN);
                    }
                }
            }
            break;

        default:
            // Item length for binary items must be 1.
            if (iLen != 1) {
                bValid = false;
                csMsg = IDS_RULE_MSG324;
                m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
                if (m_bAutoFixAndRecurse) {
                    pItem->SetLen(1);
                }
            }
            break;
    }
    // Item length makes record too long (maximum %d characters).
    if (bValid)  {
        int iRecLen = GetRecordLength(m_iLevelNum, m_iRecordNum);
        ASSERT(iRecLen >= 1);
        if (iRecLen > MAX_RECLEN)  {
            bValid = false;
            csMsg.Format(IDS_RULE_MSG323, MAX_RECLEN);
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse) {
                int iNewLen = MAX_RECLEN - pItem->GetStart() + 1;
                if (iNewLen > 0) {
                    pItem->SetLen(iNewLen);
                }
                else {
                    pItem->SetLen(1);
                }
            }
        }
        else {
            DictLevel& dict_level = m_pDict->GetLevel(m_iLevelNum);

            dict_level.GetRecord(m_iRecordNum)->SetRecLen(iRecLen);

            // GHM 20111026 if the IDs were moved past all the items in a record, the record lengths of each of the records
            // weren't getting reset; this caused problems with the reformatting tool
            if( m_iRecordNum == COMMON )
            {
                for( int i = 0; i < dict_level.GetNumRecords(); i++ )
                {
                    if( dict_level.GetRecord(i)->GetDataDict() )
                        dict_level.GetRecord(i)->CalculateRecLen();
                }
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckDataType(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckDataType(CDictItem& dict_item)
{
    const std::vector<ContentType>& supported_content_types = GetContentTypesSupportedByDictionary();
    bool valid = ( std::find(supported_content_types.cbegin(), supported_content_types.cend(), dict_item.GetContentType()) != supported_content_types.cend() );

    if( valid )
    {
        if( m_iRecordNum == COMMON && !DictionaryRules::CanBeIdItem(dict_item) )
        {
            m_csErrorReport.AppendFormat(_T("%sID items cannot be of type %s.%s"), GetErrorName(dict_item).GetString(), ToString(dict_item.GetContentType()), CRLF);
            valid = false;
        }
    }

    else
    {
        m_csErrorReport += GetErrorName(dict_item) + _T("Item data type is invalid.") + CRLF;
    }

    if( !valid && m_bAutoFixAndRecurse )
        dict_item.SetContentType(DictionaryDefaults::ContentType);

    return valid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckItemType(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckItemType(CDictItem* pItem)
{
    bool bValid = true;

    // Item item type must be "Item" or "Subitem".
    ItemType it = pItem->GetItemType();
    if (it != ItemType::Item && it != ItemType::Subitem)  {
        bValid = false;
        csMsg = IDS_RULE_MSG336;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetItemType(ItemType::Item);
        }
    }
    if (m_iRecordNum < 0 && it == ItemType::Subitem) {
        bValid = false;
        csMsg = IDS_RULE_MSG337;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
//            pItem->SetItemType(ItemType::Item);
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckOccurs(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckOccurs(CDictItem* pItem)
{
    bool bValid = true;
    UINT uOccurs = pItem->GetOccurs();

    // Item occurrences must be between 1 and %d.
    if (uOccurs < 1 || uOccurs > MAX_OCCURS)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG341, MAX_OCCURS);    // BMD 11 Feb 2003
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pItem->SetOccurs(DictionaryDefaults::Occurs);
        }
    }

    // Id items cannot have multiple occurrences.
    if (uOccurs > 1) {
        if (m_iRecordNum == COMMON) {
            bValid = false;
            csMsg = IDS_RULE_MSG344;
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                pItem->SetOccurs(1);
            }
        }

        else {
            // Multiple records cannot have items with multiple occurrences.
            /*if (m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetMaxRecs() > 1) {
                bValid = false;
                csMsg = IDS_RULE_MSG345;
                m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
                if (m_bAutoFixAndRecurse)  {
                    pItem->SetOccurs(1);
                }
            }*/
        }
    }

    if (pItem->GetItemType() == ItemType::Item)  {
        // This item cannot have more than one occurrence, because one of its subitems already does.
        if (uOccurs > 1)  {
            int iNumSubitems = GetNumSubitems(m_iLevelNum, m_iRecordNum, m_iItemNum);
            CDictItem* pSItem;
            ASSERT(m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum));
            for (int iS = 0 ; iS < iNumSubitems ; iS++)  {
                pSItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum+1+iS);
                ASSERT(pSItem);
                ASSERT(pSItem->GetItemType() == ItemType::Subitem);
                if (pSItem->GetOccurs() > 1)  {
                    bValid = false;
                    csMsg.Format(IDS_RULE_MSG342, pSItem->GetName().GetString());   // BMD 10 Mar 2003
                    m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
                    if (m_bAutoFixAndRecurse)  {
                        pItem->SetOccurs(1);
                    }
                }
            }
        }
    }
    else  {
        // This subitem cannot have more than one occurrence, because its parent item %s already does.
        if (uOccurs > 1)  {
            const CDictItem* pParentItem = m_pDict->GetParentItem(m_iLevelNum, m_iRecordNum, m_iItemNum);
            if (pParentItem->GetOccurs() > 1)  {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG343, pParentItem->GetName().GetString());
                m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
                if (m_bAutoFixAndRecurse)  {
                    pItem->SetOccurs(1);
                }
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckDecimal(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckDecimal(CDictItem* pItem)
{
    bool bValid = true;
    UINT uDecimal = pItem->GetDecimal();

    // Item decimal places must between 0 and %d.
    if (uDecimal > MAX_DECIMALS)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG351, MAX_DECIMALS);
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pItem->SetDecimal(MAX_DECIMALS);
        }
    }
    // Item decimal places must be 0 for alphanumeric items.
    ContentType content_type = pItem->GetContentType();
    if (uDecimal > 0 && content_type != ContentType::Numeric)  {
        bValid = false;
        csMsg = IDS_RULE_MSG352;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetDecimal(0);
        }
        return bValid;
    }
    // Item decimal places must be 0 for id item.
    if (m_iRecordNum < 0 && uDecimal > 0) {
        bValid = false;
        csMsg = IDS_RULE_MSG354;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetDecimal(0);
        }
        return bValid;
    }
    // Item decimal places must be less than or equal to item length.
    if (uDecimal > pItem->GetLen()) {
        bValid = false;
        csMsg = IDS_RULE_MSG353;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetDecimal(pItem->GetLen());
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckDecimalChar(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckDecimalChar(CDictItem* pItem)
{
    bool bValid = true;
    // If decimal places zero, make decimal character NO
    if (pItem->GetDecimal() == 0) {
        pItem->SetDecChar(false);
    }
    bool bDecChar = pItem->GetDecChar();
    // Decimal character takes up entire item.
    if (bDecChar && pItem->GetLen() == 1)  {
        bValid = false;
        csMsg = IDS_RULE_MSG361;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pItem->SetDecChar(false);
        }
    }
    // Decimal character does not allow for number of decimal places.
    if (bDecChar && pItem->GetLen() <= pItem->GetDecimal())  {
        bValid = false;
        csMsg = IDS_RULE_MSG362;
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pItem->SetDecChar(false);
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckZeroFill(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckZeroFill(CDictItem* pItem)
{
    // If data type is not numeric, make ZeroFill NO.
    if (pItem->GetContentType() != ContentType::Numeric) {
        pItem->SetZeroFill(false);
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckCaptureInfo(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckCaptureInfo(CDictItem* pItem)
{
    // validate the capture info...
    try
    {
        pItem->GetCaptureInfo().Validate(*pItem);
        return true;
    }

    catch( const CaptureInfo::ValidationException& exception )
    {
        m_csErrorReport += GetErrorName(*pItem) + WS2CS(exception.GetErrorMessage()) + CRLF;

        // ...set it to unspecified if it is not valid
        if( m_bAutoFixAndRecurse )
            pItem->GetCaptureInfo().SetCaptureType(CaptureType::Unspecified);

        return false;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckSubitem(Item)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckSubitem (CDictItem* pItem)
{
    if (pItem->GetItemType() != ItemType::Subitem) {
        return true;
    }

    // Subitem %s declared before any item.
    int iParentItemNum = m_pDict->GetParentItemNum(m_iLevelNum, m_iRecordNum, m_iItemNum);
    if (iParentItemNum == NONE)  {
        csMsg.Format(IDS_RULE_MSG371, pItem->GetName().GetString());
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            pItem->SetContentType(ContentType::Numeric);
        }
        return false;
    }

    CDictRecord* dict_record = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum);
    bool bValid = true;

    if (!DictionaryRules::CanBeSubitem(*dict_record, *pItem)) {
        csMsg.Format(IDS_RULE_MSG374, pItem->GetName().GetString());
        m_csErrorReport += csMsg + CRLF;
        bValid = false;
        if (m_bAutoFixAndRecurse) {
            pItem->SetContentType(ContentType::Numeric);
        }
    }

    CDictItem* pParentItem = dict_record->GetItem(iParentItemNum);

    if (!DictionaryRules::CanHaveSubitems(*dict_record, *pParentItem)) {
        csMsg.Format(IDS_RULE_MSG375, pItem->GetName().GetString());
        m_csErrorReport += csMsg + CRLF;
        bValid = false;
        if (m_bAutoFixAndRecurse) {
            pItem->SetItemType(ItemType::Item);
        }
    }

    // Subitem %s must start within the preceeding item.
    if (pItem->GetStart() < pParentItem->GetStart() || pItem->GetStart() >= pParentItem->GetStart() + pParentItem->GetLen()) {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG372, pItem->GetName().GetString());
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pItem->SetStart(pParentItem->GetStart());
        }
    }
    // Subitem %s extends past preceeding item.
    if (pItem->GetStart() + pItem->GetLen()*pItem->GetOccurs() > pParentItem->GetStart() + pParentItem->GetLen()) {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG373, pItem->GetName().GetString());
        m_csErrorReport += csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            pParentItem->SetLen(pItem->GetStart()+pItem->GetLen()*pItem->GetOccurs()-pParentItem->GetStart());
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckStartLen(Item)
//
/////////////////////////////////////////////////////////////////////////////
//
//  Checks the an item start and length against all other items in the
//  which it may overlap.
//
//  This function does NOT assume that the items are sorted by start position.
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckStartLen(CDictItem* pItem)
{
    bool bValid = true;

    // Don't Check subitems
    if (pItem->GetItemType() != ItemType::Item) {
        return bValid;
    }

    // Initialize
    UINT uStart = pItem->GetStart();
    int iLen = pItem->GetLen();
    UINT uOccurs = pItem->GetOccurs();
    UINT uEnd = uStart + iLen * uOccurs;
    const CDictRecord* pRec;
    const CDictItem* pTestItem;
    int r, i;

    // Item overlaps with the record type indicator.
    if (m_iLevelNum != NONE) {
        UINT uRecTypeStart = m_pDict->GetRecTypeStart();
        UINT uRecTypeLen = m_pDict->GetRecTypeLen();
        if (uStart < uRecTypeStart + uRecTypeLen && uEnd > uRecTypeStart) {
            bValid = false;
            csMsg = IDS_RULE_MSG382;
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse) {
                if (m_iRecordNum == COMMON) {
                    m_pDict->SetRecTypeStart(GetMaxRecLen() + 1);
                    ResetRecLens();
                }
                else {
                    pRec = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum);
                    pItem->SetStart(pRec->GetRecLen() + 1);
                    GetRecordLength(m_iLevelNum, m_iRecordNum);
                }
            }
        }
    }

    // Check for overlap with id items
    for( const DictLevel& dict_level : m_pDict->GetLevels() ) {
        pRec = dict_level.GetIdItemsRec();
        for (i = 0 ; i < pRec->GetNumItems() ; i++)  {
            pTestItem = pRec->GetItem(i);
            if (pTestItem->GetItemType() != ItemType::Item)  {
                continue;
            }
            if (m_iRecordNum == COMMON && i == m_iItemNum)  {
                continue;
            }
            UINT uTestStart = pTestItem->GetStart();
            UINT uTestEnd = uTestStart + pTestItem->GetLen()*pTestItem->GetOccurs();
            if (uStart < uTestEnd && uEnd > uTestStart)  {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG383, pItem->GetName().GetString(), pTestItem->GetName().GetString());
                m_csErrorReport += csMsg + CRLF;
                if (m_bAutoFixAndRecurse && m_iLevelNum != NONE) {
                    if (m_iRecordNum == COMMON) {
                        pItem->SetStart(GetMaxRecLen() + 1);
                        ResetRecLens();
                    }
                    else {
                        pItem->SetStart(pRec->GetRecLen() + 1);
                        GetRecordLength(m_iLevelNum, m_iRecordNum);
                    }
                }
            }
        }
    }

    // Check for overlap with record items
    if (m_iRecordNum == COMMON) {
        for( const DictLevel& dict_level : m_pDict->GetLevels() ) {
            for (r = 0 ; r < dict_level.GetNumRecords() ; r++) {
                pRec = dict_level.GetRecord(r);
                for (i = 0 ; i < pRec->GetNumItems() ; i++)  {
                    pTestItem = pRec->GetItem(i);
                    if (pTestItem->GetItemType() != ItemType::Item)  {
                        continue;
                    }
                    UINT uTestStart = pTestItem->GetStart();
                    UINT uTestEnd = uTestStart + pTestItem->GetLen()*pTestItem->GetOccurs();
                    if (uStart < uTestEnd && uEnd > uTestStart)  {
                        bValid = false;
                        csMsg.Format(IDS_RULE_MSG383, pItem->GetName().GetString(), pTestItem->GetName().GetString());
                        m_csErrorReport += csMsg + CRLF;
                        if (m_bAutoFixAndRecurse && m_iLevelNum != NONE) {
                            pItem->SetStart(GetMaxRecLen() + 1);
                            ResetRecLens();
                        }
                    }
                }
            }
        }
    }
    else {
        pRec = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum);
        for (i = 0 ; i < pRec->GetNumItems() ; i++)  {
            pTestItem = pRec->GetItem(i);
            if (i == m_iItemNum || pTestItem->GetItemType() != ItemType::Item)  {
                continue;
            }
            UINT uTestStart = pTestItem->GetStart();
            UINT uTestEnd = uTestStart + pTestItem->GetLen()*pTestItem->GetOccurs();
            if (uStart < uTestEnd && uEnd > uTestStart)  {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG383, pItem->GetName().GetString(), pTestItem->GetName().GetString());
                m_csErrorReport += csMsg + CRLF;
                if (m_bAutoFixAndRecurse) {
                    pItem->SetStart(pRec->GetRecLen() + 1);
                    GetRecordLength(m_iLevelNum, m_iRecordNum);
                }
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Value Set)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(DictValueSet& dict_value_set,
                                  int iLevelNum,
                                  int iRecordNum,
                                  int iItemNum,
                                  int iVSetNum,
                                  bool bSilent,
                                  bool bAutoFixAndRecurse)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = iRecordNum;
    m_iItemNum = iItemNum;
    m_iVSetNum = iVSetNum;
    m_iValueNum = NONE;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    m_iInvalidEdit = NONE;
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    if (!CheckNote(dict_value_set))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(dict_value_set))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckName(dict_value_set))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 2;
        }
    }
    if (!CheckAliases(dict_value_set)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = INT_MAX;
        }
    }
    if (!CheckValueType(dict_value_set))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 2;
        }
    }

    bValid = (m_iInvalidEdit == NONE);

    const CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);

    if( !DictionaryRules::CanHaveValueSet(*pItem) ) {
        bValid = false;
        csMsg.Format(_T("Items of type %s cannot have value sets"), ToString(pItem->GetContentType()));
        m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;

        if (bAutoFixAndRecurse) {
            // the value set should be removed from the item, but that isn't possble within this method;
            // instead, by removing all values we ensure that it won't be saved
            dict_value_set.RemoveAllValues();
        }
    }

    else if (bAutoFixAndRecurse)  {
        int i = 0;
        for( auto& dict_value : dict_value_set.GetValues() ) {
            bValid &= IsValid(dict_value, iLevelNum, iRecordNum, iItemNum, iVSetNum, i, true, bAutoFixAndRecurse);
            ++i;
        }
        if (AdjustValues(dict_value_set)) {
            bValid = false;
            csMsg = _T("CSPro 3.x Change: Special values moved to end of value set.");
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
        }
    }
    // ***  VALUE SET CONTAINS ERRORS ***
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG400;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLabel(Value Set)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLabel(DictValueSet& dict_value_set)
{
    bool bValid = true;
    CString csLabel = dict_value_set.GetLabel();

    // Value set label cannot be empty.
    if (csLabel.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG402;
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse) {
            csMsg.Format(IDS_DEF_VSET_LABEL, m_iVSetNum);
            dict_value_set.SetLabel(csMsg);
        }
        return bValid;
    }
    // Value set label cannot contain spaces only.
    if (SO::IsBlank(csLabel))  {
        bValid = false;
        csMsg = IDS_RULE_MSG403;
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            csMsg.Format(IDS_DEF_VSET_LABEL, m_iVSetNum);
            dict_value_set.SetLabel(csMsg);
        }
        return bValid;
    }
    // Value set label is too long (maximum %d characters).
    if (csLabel.GetLength() > MAX_LABEL_LEN)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG404, MAX_LABEL_LEN);
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dict_value_set.SetLabel(csLabel.Left(MAX_LABEL_LEN));
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckName(Value Set)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckName(DictValueSet& dict_value_set)
{
    bool bValid = true;
    CIMSAString csName = dict_value_set.GetName();

    // Value set name cannot be empty.
    if (csName.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG406;
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dict_value_set.SetName(GetDefaultName(dict_value_set.GetLabel()));
            m_pDict->UpdateNameList(dict_value_set, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum);
        }
    }
    // Value set name must contain A-Z, 0-9, or underline and start with letter.
    if (bValid)  {
        if (!csName.IsName())  {
            bValid = false;
            csMsg = IDS_RULE_MSG408;
            CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                csName.MakeName();
                dict_value_set.SetName(csName);
                m_pDict->UpdateNameList(dict_value_set, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum);
            }
        }
    }
    // Value set name is not unique in this dictionary.
    if (bValid)  {
        if (!m_pDict->IsNameUnique(csName, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum))  {
            bValid = false;
            csMsg = IDS_RULE_MSG409;
            CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                dict_value_set.SetName(m_pDict->GetUniqueName(csName, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum));
                m_pDict->UpdateNameList(dict_value_set, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum);
            }
        }
    }
    // Value set name is reserved word.
    if (bValid)  {
        if (csName.IsReservedWord())  {
            bValid = false;
            csMsg = IDS_RULE_MSG410;
            CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                dict_value_set.SetName(m_pDict->GetUniqueName(csName, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum));
                m_pDict->UpdateNameList(dict_value_set, m_iLevelNum, m_iRecordNum, m_iItemNum, m_iVSetNum);
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckValueType(Value Set)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckValueType(DictValueSet& dict_value_set)
{
    bool valid = true;
    std::unordered_map<double, bool> defined_values;

    for( size_t i = 0; i < dict_value_set.GetNumValues(); ++i )
    {
        const DictValue& dict_value = dict_value_set.GetValue(i);

        if( !dict_value.IsSpecial() )
            continue;

        if( defined_values.find(dict_value.GetSpecialValue()) == defined_values.cend() )
        {
            defined_values.emplace(dict_value.GetSpecialValue(), true);
        }

        // a duplicate special type
        else
        {
            valid = false;
            csMsg.Format(_T("More than one %s value"), SpecialValues::ValueToString(dict_value.GetSpecialValue(), false));

            const CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(dict_value_set) + csMsg + CRLF;

            if( m_bAutoFixAndRecurse )
            {
                dict_value_set.RemoveValue(i);
                --i;
            }
        }
    }

    return valid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Value)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(DictValue& dict_value, int iLevelNum, int iRecordNum, int iItemNum, int iVSetNum, int iValueNum,
                                  bool bSilent /*=false*/, bool bAutoFixAndRecurse /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = iRecordNum;
    m_iItemNum = iItemNum;
    m_iVSetNum = iVSetNum;
    m_iValueNum = iValueNum;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    m_iInvalidEdit = NONE;
    if (!CheckNote(dict_value))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 0;
        }
    }
    if (!CheckLabel(dict_value))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 3;
        }
   }
    if (!CheckValueType(dict_value))  {
        if (m_iInvalidEdit == NONE)  {
            m_iInvalidEdit = 6;
        }
   }
    bValid = (m_iInvalidEdit == NONE);
    if (bAutoFixAndRecurse)  {
        for( auto& dict_value_pair : dict_value.GetValuePairs() ) {
            bValid &= IsValid(dict_value_pair, iLevelNum, iRecordNum, iItemNum, iVSetNum, iValueNum, true, bAutoFixAndRecurse);
        }
    }
    // ***  VALUE CONTAINS ERRORS ***
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG500;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLabel(Value)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLabel(DictValue& dict_value)
{
    bool bValid = true;

    // Value label is too long (maximum %d characters).
    if (dict_value.GetLabel().GetLength() > MAX_LABEL_LEN)  {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG504, MAX_LABEL_LEN);
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
        if (m_bAutoFixAndRecurse)  {
            dict_value.SetLabel(dict_value.GetLabel().Left(MAX_LABEL_LEN));
        }
    }
    // Missing From value
    if (!dict_value.GetLabel().IsEmpty()) {
        ASSERT(dict_value.HasValuePairs());
        if (dict_value.GetValuePair(0).GetFrom().IsEmpty() && !dict_value.IsSpecialValue(NOTAPPL)) {
            bValid = false;
            csMsg = IDS_RULE_MSG506;
            CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
            m_csErrorReport += GetErrorName(*pItem) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckValueType(Value)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckValueType(DictValue& dict_value)
{
    bool bValid = true;

    if (dict_value.IsSpecial()) {
        // Special value must have only a From value.
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        if (dict_value.GetNumValuePairs() > 1) {
            bValid = false;
            csMsg = IDS_RULE_MSG511;
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum)->GetValueSet(m_iVSetNum)) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse) {
                while (dict_value.GetNumValuePairs() > 1) {
                    dict_value.RemoveValuePair(1);
                }
            }
        }
        if (pItem->GetContentType() != ContentType::Numeric) {
            bValid = false;
            csMsg = IDS_RULE_MSG512;
            m_csErrorReport += GetErrorName(*pItem) + GetErrorName(pItem->GetValueSet(m_iVSetNum)) + csMsg + CRLF;
            if (m_bAutoFixAndRecurse) {
                dict_value.SetNotSpecial();
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Value Pair)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(DictValuePair& dict_value_pair, int iLevelNum, int iRecordNum, int iItemNum, int iVSetNum, int iValue,
                                  bool bSilent /*=false*/, bool bAutoFixAndRecurse /*=false*/)
{
    bool bValid = true;

    m_iLevelNum = iLevelNum;
    m_iRecordNum = iRecordNum;
    m_iItemNum = iItemNum;
    m_iVSetNum = iVSetNum;
    m_iValueNum = iValue;
    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    m_iInvalidEdit = NONE;

    // If both pairs empty, make From all spaces
    if (dict_value_pair.GetFrom().IsEmpty() && dict_value_pair.GetTo().IsEmpty()) {
        UINT uLen = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum)->GetLen();
        dict_value_pair.SetFrom(CString(SPACE, (int)uLen));
    }
    // If From = To, make To empty
    if (dict_value_pair.GetFrom() == dict_value_pair.GetTo()) {
        dict_value_pair.SetTo(_T(""));
    }

    bool bIsFromField = true;
    if (m_iInvalidEdit == NONE || m_bAutoFixAndRecurse)  {
        if (!CheckDataType(dict_value_pair, bIsFromField)) {
            m_iInvalidEdit = (bIsFromField ? 4 : 5);
        }
    }
    if (m_iInvalidEdit == NONE || m_bAutoFixAndRecurse)  {
        if (!CheckLen(dict_value_pair, bIsFromField))  {
            m_iInvalidEdit = (bIsFromField ? 4 : 5);
        }
    }
    if (m_iInvalidEdit == NONE || m_bAutoFixAndRecurse)  {
        if (!CheckFromTo(dict_value_pair))  {
            m_iInvalidEdit = 5;
        }
    }
    // ***  VALUE PAIR CONTAINS ERRORS ***
    bValid = (m_iInvalidEdit == NONE);
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG600;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckDataType(Value Pair)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckDataType(DictValuePair& dict_value_pair, bool& bIsFromField)
{
    bool bValid = true;
    CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
    const DictValue& dict_value = pItem->GetValueSet(m_iVSetNum).GetValue(m_iValueNum);
    ContentType content_type = pItem->GetContentType();

    switch (content_type)  {
        case ContentType::Numeric:
            // To value cannot be defined for Special values.
            if (dict_value.IsSpecial() && !dict_value_pair.GetTo().IsEmpty()) {
                bValid = false;
                bIsFromField = false;
                csMsg = IDS_RULE_MSG607;
                m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetTo() + _T(": ") + csMsg + CRLF;
                if (m_bAutoFixAndRecurse) {
                    dict_value_pair.SetTo(_T(""));
                }
            }

            if (dict_value_pair.GetTo().IsEmpty()) {
                // From value must be either numeric or blank (if the special value is notappl).
                if( !SO::IsWhitespace(dict_value_pair.GetFrom()) || !dict_value.IsSpecialValue(NOTAPPL) ) {
                    if (!CIMSAString::IsNumeric(dict_value_pair.GetFrom())) {
                        bValid = false;
                        bIsFromField = true;
                        csMsg = dict_value.IsSpecial() ? IDS_RULE_MSG601 : IDS_RULE_MSG602;
                        m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetFrom() + _T(": ") + csMsg + CRLF;
                        if (m_bAutoFixAndRecurse)  {
                            dict_value_pair.SetFrom(CIMSAString::MakeNumeric(dict_value_pair.GetFrom()));
                        }
                    }
                }
            }

            else {
                // From value must be numeric.
                if (!CIMSAString::IsNumeric(dict_value_pair.GetFrom())) {
                    bValid = false;
                    bIsFromField = true;
                    csMsg = IDS_RULE_MSG602;
                    m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetFrom() + _T(": ") + csMsg + CRLF;
                    if (m_bAutoFixAndRecurse) {
                        dict_value_pair.SetFrom(CIMSAString::MakeNumeric(dict_value_pair.GetFrom()));
                    }
                }
                // To value must be numeric.
                if (!CIMSAString::IsNumeric(dict_value_pair.GetTo())) {
                    bValid = false;
                    bIsFromField = false;
                    csMsg = IDS_RULE_MSG603;
                    m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetTo() + _T(": ") + csMsg + CRLF;
                    if (m_bAutoFixAndRecurse) {
                        dict_value_pair.SetTo(CIMSAString::MakeNumeric(dict_value_pair.GetTo()));
                    }
                }
            }
            break;

        case ContentType::Alpha:
            // To value cannot be defined for Alphanumeric items.
            if (!dict_value_pair.GetTo().IsEmpty())  {
                bValid = false;
                bIsFromField = false;
                csMsg = IDS_RULE_MSG604;
                m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetTo() + _T(": ") + csMsg + CRLF;
                if (m_bAutoFixAndRecurse) {
                    dict_value_pair.SetTo(_T(""));
                }
            }
            break;

        default:
            ASSERT(false);
            bValid = false;
            break;
    }

    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLen(Value Pair)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLen(DictValuePair& dict_value_pair, bool& bIsFromField)
{
    // bIsFromField signals whether the offending field was FROM or TO
    double dValue;
    TCHAR pszTemp[30];
    bool bValid = true;
    CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
    UINT uLen = pItem->GetLen();

    // If From is blank and To is empty, make From all blank.
    if (SO::IsBlank(dict_value_pair.GetFrom()) && dict_value_pair.GetTo().IsEmpty())  {
        dict_value_pair.SetFrom(CIMSAString(dict_value_pair.GetFrom()).AdjustLenLeft(uLen, SPACE));
        return bValid;
    }

    bool value_is_numeric = ( pItem->GetContentType() == ContentType::Numeric ) &&
                            ( !pItem->GetValueSet(m_iVSetNum).GetValue(m_iValueNum).IsSpecialValue(NOTAPPL) || !SO::IsWhitespace(dict_value_pair.GetFrom()) );

    if( value_is_numeric ) {
        TCHAR decimal_ch = CIMSAString::GetDecChar();
        uLen = pItem->GetCompleteLen();
        // From value is longer than the item length.
        CString csFrom = dict_value_pair.GetFrom();
        if ((UINT) csFrom.GetLength() > uLen) {
            bValid = false;
            bIsFromField= true;
            csMsg = IDS_RULE_MSG605;
            m_csErrorReport += GetErrorName(*pItem) + csFrom + _T(": ") + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                if (csFrom[0] == HYPHEN) {
                    UINT i = csFrom.GetLength() - pItem->GetDecimal() - 1;
                    ASSERT(i < (UINT) csFrom.GetLength());
                    if (csFrom.GetAt(i) == decimal_ch) {
                        dict_value_pair.SetFrom(_T("0"));
                    }
                    else {
                        dict_value_pair.SetFrom(HYPHEN + csFrom.Right(uLen - 1));
                    }
                }
                else  {
                    dict_value_pair.SetFrom(csFrom.Right(uLen));
                }
                dValue = atod(dict_value_pair.GetFrom());
                dict_value_pair.SetFrom(dtoa(dValue, pszTemp, pItem->GetDecimal(), decimal_ch, false));
            }
        }
        // To value is longer than the item length.
        if ((UINT) dict_value_pair.GetTo().GetLength() > uLen)  {
            CString csTo = dict_value_pair.GetTo();
            bValid = false;
            bIsFromField= false;
            csMsg = IDS_RULE_MSG606;
            m_csErrorReport += GetErrorName(*pItem) + csTo + _T(": ") + csMsg + _T("\r\n");
            if (m_bAutoFixAndRecurse)  {
                if (csTo[0] == HYPHEN) {
                    UINT i = csTo.GetLength() - pItem->GetDecimal() - 1;
                    ASSERT(i < (UINT) csTo.GetLength());
                    if (csTo.GetAt(i) == decimal_ch) {
                        dict_value_pair.SetTo(_T("0"));
                    }
                    else {
                        dict_value_pair.SetTo(HYPHEN + csTo.Right(uLen - 1));
                    }
                }
                else  {
                    dict_value_pair.SetTo(csTo.Right(uLen));
                }
                dValue = atod(dict_value_pair.GetTo());
                dict_value_pair.SetTo(dtoa(dValue, pszTemp, pItem->GetDecimal(), decimal_ch, false));
            }
        }
    }
    else {
        if ((UINT) dict_value_pair.GetFrom().GetLength() < uLen) {
            // auto fill without a message
            dict_value_pair.SetFrom(CIMSAString(dict_value_pair.GetFrom()).AdjustLenRight(uLen, SPACE));
        }
        else if ((UINT) dict_value_pair.GetFrom().GetLength() > uLen) {
            // From value too long
            bValid = false;
            bIsFromField= true;
            csMsg = IDS_RULE_MSG605;
            m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetFrom() + _T(": ") + csMsg + CRLF;
            if (m_bAutoFixAndRecurse)  {
                dict_value_pair.SetFrom(dict_value_pair.GetFrom().Left(uLen));
            }
        }
        if (!dict_value_pair.GetTo().IsEmpty()) {
            if ((UINT) dict_value_pair.GetTo().GetLength() < uLen) {
                // auto fill without a message
                dict_value_pair.SetTo(CIMSAString(dict_value_pair.GetTo()).AdjustLenRight(uLen, SPACE));
            }
            else if ((UINT) dict_value_pair.GetTo().GetLength() > uLen) {
                // To value is too long.
                bValid = false;
                bIsFromField= false;
                csMsg = IDS_RULE_MSG606;
                m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetTo() + _T(": ") + csMsg + CRLF;
                if (m_bAutoFixAndRecurse)  {
                    dict_value_pair.SetTo(dict_value_pair.GetTo().Left(uLen));
                }
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckFromTo(Value Pair)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckFromTo(DictValuePair& dict_value_pair)
{
    bool bRetVal = true;

    if( !dict_value_pair.GetTo().IsEmpty() )
    {
        CDictItem* pItem = m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum);
        ASSERT(pItem);
        if (pItem->GetContentType() == ContentType::Numeric) {
            ASSERT(CIMSAString::IsNumeric(dict_value_pair.GetFrom()));
            ASSERT(CIMSAString::IsNumeric(dict_value_pair.GetTo()));
            UINT uDec = pItem->GetDecimal();
            double dFrom = atod(dict_value_pair.GetFrom(), uDec);
            double dTo = atod(dict_value_pair.GetTo(), uDec);
            bRetVal = dFrom <= dTo;
            if (!bRetVal)  {
                // To value must be greater than From value.
                csMsg = IDS_RULE_MSG611;
                m_csErrorReport += GetErrorName(*pItem) + dict_value_pair.GetTo() + _T(": ") + csMsg + _T("\r\n");
                if (m_bAutoFixAndRecurse)  {
                    dict_value_pair.SetTo(_T(""));
                }
            }
        }
    }
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsValid(Relation)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsValid(const DictRelation& dict_relation,
                                  bool bSilent /*=false*/,
                                  bool bAutoFixAndRecurse /*=false*/)
{
    bool bValid = true;
    bool bBadPrimary = false;

    m_bSilent = bSilent;
    m_bAutoFixAndRecurse = bAutoFixAndRecurse;
    m_iInvalidEdit = NONE;
    if (!bSilent)  {
        m_csErrorReport.Empty();
    }
    if (!CheckName(dict_relation)) {
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 1;
        }
    }
    if (!CheckPrimaryName(dict_relation)) {
        bBadPrimary = true;
        if (m_iInvalidEdit == NONE) {
            m_iInvalidEdit = 2;
        }
    }
    for( const auto& dict_relation_part : dict_relation.GetRelationParts() ) {
        bool bValidPart = true;
        if (bBadPrimary) {
            bValidPart = false;
        }
        else {
            if (!CheckPrimaryLink(dict_relation, dict_relation_part)) {
                bValidPart = false;
                if (m_iInvalidEdit == NONE) {
                    m_iInvalidEdit = 3;
                }
            }
        }
        if (!CheckSecondaryName(dict_relation, dict_relation_part)) {
            bValidPart = false;
            if (m_iInvalidEdit == NONE) {
                m_iInvalidEdit = 4;
            }
        }
        else {
            if (!CheckSecondaryLink(dict_relation, dict_relation_part)) {
                bValidPart = false;
                if (m_iInvalidEdit == NONE) {
                    m_iInvalidEdit = 5;
                }
            }
        }
        if (bValidPart) {
            if (!CheckLinks(dict_relation, dict_relation_part)) {
                if (m_iInvalidEdit == NONE) {
                    m_iInvalidEdit = 2;
                }
            }
        }
    }
    // ***  RELATION CONTAINS ERRORS ***
    bValid = (m_iInvalidEdit == NONE);
    if (!bValid) {
        m_csErrorReport += GetErrorName(dict_relation) + _T("Relation removed!") + CRLF;
    }
    if (!bSilent && !bValid)  {
        csMsg = IDS_RULE_MSG700;
        csMsg = csMsg + CRLF + CRLF + m_csErrorReport;
        ErrorMessage::Display(csMsg);
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckName(Relation)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckName(const DictRelation& dict_relation)
{
    bool bValid = true;
    CIMSAString csName = dict_relation.GetName();

    // Relation name cannot be empty.
    if (csName.GetLength() == 0)  {
        bValid = false;
        csMsg = IDS_RULE_MSG701;
        m_csErrorReport += csMsg + CRLF;
    }
    // Relation name must contain A-Z, 0-9, or underline and start with letter.
    if (bValid)  {
        if (!csName.IsName())  {
            bValid = false;
            csMsg = IDS_RULE_MSG703;
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
    }
    // Relation name is not unique in this dictionary.
    if (bValid)  {
        if (m_pDict->LookupName(csName, nullptr)) {
            bValid = false;
            csMsg = IDS_RULE_MSG704;
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
        else {
            for( const auto& test_dict_relation : m_pDict->GetRelations() ) {
                if (&dict_relation != &test_dict_relation && csName == test_dict_relation.GetName() ) {
                    bValid = false;
                    csMsg = IDS_RULE_MSG704;
                    m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                }
            }
        }
    }
    // Relation name is reserved word.
    if (bValid)  {
        if (csName.IsReservedWord())  {
            bValid = false;
            csMsg = IDS_RULE_MSG705;
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckPrimaryName(Relation)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckPrimaryName(const DictRelation& dict_relation)
{
    bool bValid = true;
    const CDictRecord* pRec;
    const CDictItem* pItem;
    const DictValueSet* pVSet;
    if (!m_pDict->LookupName(dict_relation.GetPrimaryName(), nullptr, &pRec, &pItem, &pVSet)) {
        bValid = false;
        csMsg.Format(IDS_RULE_MSG706, dict_relation.GetPrimaryName().c_str());
        m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
    }
    else {
        if (pVSet == NULL && pItem != NULL) {
            if (pItem->GetOccurs() == 1) {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG707, dict_relation.GetPrimaryName().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
        else if (pVSet == NULL && pItem == NULL && pRec != NULL) {
            if (pRec->GetMaxRecs() == 1) {
                bValid = false;
                csMsg.Format(IDS_RULE_MSG708, dict_relation.GetPrimaryName().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
        else {
            bValid = false;
            csMsg.Format(IDS_RULE_MSG709, dict_relation.GetPrimaryName().c_str());
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckPrimaryLink(Relation)
//
/////////////////////////////////////////////////////////////////////////////
bool DictionaryValidator::CheckPrimaryLink(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part)
{
    bool bValid = true;

    if( dict_relation_part.IsPrimaryLinkedByOccurrence() )
        return bValid;

    int iPrimLevel;
    int iPrimRec;
    int iPrimItem;
    int iPrimVSet;
    m_pDict->LookupName(dict_relation.GetPrimaryName(),&iPrimLevel,&iPrimRec,&iPrimItem,&iPrimVSet);

    int iLevel;
    int iRec;
    int iItem;
    int iVSet;
    if (!m_pDict->LookupName(dict_relation_part.GetPrimaryLink(),&iLevel,&iRec,&iItem,&iVSet)) {
        bValid = false;
        csMsg.Format(_T("Primary link %s is not in the dictionary."), dict_relation_part.GetPrimaryLink().c_str());
        m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
    }
    else {
        if (iPrimVSet == NONE && iPrimItem == NONE) {
            // Primary is a multiple record
            if (iPrimLevel == iLevel && iPrimRec == iRec && iItem != NONE && iVSet == NONE) {
                CDictItem* pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                if (pItem->GetOccurs() > 1 || pItem->GetItemType() != ItemType::Item) {
                    bValid = false;
                    csMsg.Format(_T("Primary link %s is not an item or has multiple occurrences."), dict_relation_part.GetPrimaryLink().c_str());
                    m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                }
            }
            else {
                bValid = false;
                csMsg.Format(_T("Primary link %s is not an item in the primary record."), dict_relation_part.GetPrimaryLink().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
        else {
            // Primary is a multiple item
            if (iPrimLevel == iLevel && iPrimRec == iRec && iItem != NONE && iVSet == NONE) {
                CDictItem* pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                if (pItem->GetOccurs() > 1 || pItem->GetItemType() != ItemType::Subitem) {
                    bValid = false;
                    csMsg.Format(_T("Primary link %s is not a subitem or has multiple occurrences."), dict_relation_part.GetPrimaryLink().c_str());
                    m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                }
                else {
                    CDictItem* pPrimItem = m_pDict->GetLevel(iPrimLevel).GetRecord(iPrimRec)->GetItem(iPrimItem);
                    if (pItem->GetParentItem() != pPrimItem) {
                        bValid = false;
                        csMsg.Format(_T("Primary link %s is not a subitem within the primary item."), dict_relation_part.GetPrimaryLink().c_str());
                        m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                    }
                }
            }
            else {
                bValid = false;
                csMsg.Format(_T("Primary link %s is not within the primary item."), dict_relation_part.GetPrimaryLink().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckSecondaryName(Relation)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckSecondaryName(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part)
{
    bool bValid = true;
    const CDictRecord* pRec;
    const CDictItem* pItem;
    const DictValueSet* pVSet;
    if (!m_pDict->LookupName(dict_relation_part.GetSecondaryName(), nullptr, &pRec, &pItem, &pVSet)) {
        bValid = false;
        csMsg.Format(_T("Secondary %s is not in the dictionary."), dict_relation_part.GetSecondaryName().c_str());
        m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
    }
    else {
        if (pVSet == NULL && pItem != NULL) {
            if (pItem->GetOccurs() == 1) {
                bValid = false;
                csMsg.Format(_T("Secondary %s is not a multiple item."), dict_relation_part.GetSecondaryName().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
        else if (pVSet == NULL && pItem == NULL && pRec != NULL) {
            if (pRec->GetMaxRecs() == 1) {
                bValid = false;
                csMsg.Format(_T("Secondary %s is not a multiple record."), dict_relation_part.GetSecondaryName().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
        else {
            bValid = false;
            csMsg.Format(_T("Secondary %s is not a record or item."), dict_relation_part.GetSecondaryName().c_str());
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
        if (SO::EqualsNoCase(dict_relation_part.GetSecondaryName(), dict_relation.GetPrimaryName())) {
            bValid = false;
            csMsg.Format(_T("Secondary %s is the same as the primary."), dict_relation_part.GetSecondaryName().c_str());
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
        for( const auto& test_dict_relation_part : dict_relation.GetRelationParts() ) {
            if (&dict_relation_part != &test_dict_relation_part && SO::EqualsNoCase(dict_relation_part.GetSecondaryName(), test_dict_relation_part.GetSecondaryName())) {
                bValid = false;
                csMsg.Format(_T("Secondary %s is a duplicate."), dict_relation_part.GetSecondaryName().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckSecondaryLink(Relation)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckSecondaryLink(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part)
{
    bool bValid = true;

    if( dict_relation_part.IsSecondaryLinkedByOccurrence() )
        return bValid;

    int iSecLevel;
    int iSecRec;
    int iSecItem;
    int iSecVSet;
    m_pDict->LookupName(dict_relation_part.GetSecondaryName(),&iSecLevel,&iSecRec,&iSecItem,&iSecVSet);

    int iLevel;
    int iRec;
    int iItem;
    int iVSet;
    if (!m_pDict->LookupName(dict_relation_part.GetSecondaryLink(),&iLevel,&iRec,&iItem,&iVSet)) {
        bValid = false;
        csMsg.Format(_T("Secondary link %s is not in the dictionary."), dict_relation_part.GetSecondaryLink().c_str());
        m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
    }
    else {
        if (iSecVSet == NONE && iSecItem == NONE) {
            // Secondary is a multiple record
            if (iSecLevel == iLevel && iSecRec == iRec && iItem != NONE && iVSet == NONE) {
                CDictItem* pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                if (pItem->GetOccurs() > 1 || pItem->GetItemType() != ItemType::Item) {
                    bValid = false;
                    csMsg.Format(_T("Secondary link %s is not an item or has multiple occurrences."), dict_relation_part.GetSecondaryLink().c_str());
                    m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                }
            }
            else {
                bValid = false;
                csMsg.Format(_T("Secondary link %s is not an item in the secondary record."), dict_relation_part.GetSecondaryLink().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
        else {
            // Secondary is a multiple item
            if (iSecLevel == iLevel && iSecRec == iRec && iItem != NONE && iVSet == NONE) {
                CDictItem* pItem = m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
                if (pItem->GetOccurs() > 1 || pItem->GetItemType() != ItemType::Subitem) {
                    bValid = false;
                    csMsg.Format(_T("Secondary link %s is not a subitem or has multiple occurrences."), dict_relation_part.GetSecondaryLink().c_str());
                    m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                }
                else {
                    CDictItem* pSecItem = m_pDict->GetLevel(iSecLevel).GetRecord(iSecRec)->GetItem(iSecItem);
                    if (pItem->GetParentItem() != pSecItem) {
                        bValid = false;
                        csMsg.Format(_T("Secondary link %s is not a subitem within the secondary item."), dict_relation_part.GetSecondaryLink().c_str());
                        m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
                    }
                }
            }
            else {
                bValid = false;
                csMsg.Format(_T("Secondary link %s is not within the secondary item."), dict_relation_part.GetSecondaryLink().c_str());
                m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
            }
        }
    }
    return bValid;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::CheckLinks(Relation)
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::CheckLinks(const DictRelation& dict_relation, const DictRelationPart& dict_relation_part)
{
    bool bValid = true;
    if (!dict_relation_part.IsPrimaryLinkedByOccurrence() && dict_relation_part.IsSecondaryLinkedByOccurrence()) {
        const CDictItem* pPrimItem = m_pDict->LookupName<CDictItem>(WS2CS(dict_relation_part.GetPrimaryLink()));
        if (pPrimItem->GetContentType() != ContentType::Numeric) {
            bValid = false;
            csMsg.Format(_T("Primary link %s must be numeric."), dict_relation_part.GetPrimaryLink().c_str());
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
    }
    if (dict_relation_part.IsPrimaryLinkedByOccurrence() && !dict_relation_part.IsSecondaryLinkedByOccurrence()) {
        const CDictItem* pSecItem = m_pDict->LookupName<CDictItem>(WS2CS(dict_relation_part.GetSecondaryLink()));
        if (pSecItem->GetContentType() != ContentType::Numeric) {
            bValid = false;
            csMsg.Format(_T("Secondary link %s must be numeric."), dict_relation_part.GetSecondaryLink().c_str());
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
    }
    if (!dict_relation_part.IsPrimaryLinkedByOccurrence() && !dict_relation_part.IsSecondaryLinkedByOccurrence()) {
        const CDictItem* pPrimItem = m_pDict->LookupName<CDictItem>(WS2CS(dict_relation_part.GetPrimaryLink()));
        const CDictItem* pSecItem = m_pDict->LookupName<CDictItem>(WS2CS(dict_relation_part.GetSecondaryLink()));
        if (pPrimItem->GetContentType() != pSecItem->GetContentType()) {
            bValid = false;
            csMsg.Format(_T("Primary %s and secondary %s links must be the same data type."),
                         dict_relation_part.GetPrimaryLink().c_str(), dict_relation_part.GetSecondaryLink().c_str());
            m_csErrorReport += GetErrorName(dict_relation) + csMsg + CRLF;
        }
    }
    return bValid;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::FindNewPos
//
/////////////////////////////////////////////////////////////////////////////

int DictionaryValidator::FindNewPos(int iStart, int iItem, CDictRecord* pRec)
{
    int iReturn = pRec->GetNumItems();
    for (int i = 0 ; i < pRec->GetNumItems() ; i++) {
        if (i == iItem) {
            continue;
        }
        int iItemStart = pRec->GetItem(i)->GetStart();
        if (iItemStart >= iStart) {
            if (pRec->GetItem(iItem)->GetItemType() == ItemType::Subitem &&
                    iItemStart == iStart) {
                iReturn = i + 1;
            }
            else {
                iReturn = i;
            }
            break;
        }
    }
    return iReturn;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::AdjustStartPositions
//
/////////////////////////////////////////////////////////////////////////////

void DictionaryValidator::AdjustStartPositions(CDataDict& dictionary)
{
    // Adjust Record Type start
    bool bRecordTypePlaced = false;
    if (dictionary.GetRecTypeStart() == 0) {
        bRecordTypePlaced = true;
    }
    int iNextStart = 1;

    // Adjust Level Ids for all levels
    for( DictLevel& dict_level : dictionary.GetLevels() ) {
        CDictRecord* pRec = dict_level.GetIdItemsRec();
        int iOldItemStart = 0;
        int iNewItemStart = 0;
        int iOffset;
        for (int i = 0 ; i < pRec->GetNumItems(); i++) {
            if (!bRecordTypePlaced && (int)dictionary.GetRecTypeStart() <= iNextStart) {
                dictionary.SetRecTypeStart(iNextStart);
                iNextStart += dictionary.GetRecTypeLen();
                bRecordTypePlaced = true;
            }
            CDictItem* pItem = pRec->GetItem(i);
            if (pItem->GetItemType() == ItemType::Item) {
                iOldItemStart = pItem->GetStart();
                pItem->SetStart(iNextStart);
                iNewItemStart = iNextStart;
                iNextStart += (pItem->GetLen() * pItem->GetOccurs());
            }
            else {
                iOffset = pItem->GetStart() - iOldItemStart;
                pItem->SetStart(iNewItemStart + iOffset);
            }
        }
    }
    if (!bRecordTypePlaced) {
        dictionary.SetRecTypeStart(iNextStart);
        iNextStart += dictionary.GetRecTypeLen();
        bRecordTypePlaced = true;
    }

    // Adjust Record Items for all records
    CString csVal;
    int iSaveStart = iNextStart;
    for( DictLevel& dict_level : dictionary.GetLevels() ) {
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            iNextStart = iSaveStart;
            CDictRecord* pRec = dict_level.GetRecord(r);
            int iOldItemStart = 0;
            int iNewItemStart = 0;
            int iOffset;
            for (int i = 0 ; i < pRec->GetNumItems(); i++) {
                CDictItem* pItem = pRec->GetItem(i);
                if (pItem->GetItemType() == ItemType::Item) {
                    iOldItemStart = pItem->GetStart();
                    pItem->SetStart(iNextStart);
                    iNewItemStart = iNextStart;
                    iNextStart += (pItem->GetLen() * pItem->GetOccurs());
                }
                else {
                    iOffset = pItem->GetStart() - iOldItemStart;
                    pItem->SetStart(iNewItemStart + iOffset);
                }
            }
        }
    }

    ResetRecLens(dictionary);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetNumSubitems
//
/////////////////////////////////////////////////////////////////////////////

int DictionaryValidator::GetNumSubitems(int iLevel, int iRec, int iItem)
{
    CDictRecord* pRec = m_pDict->GetLevel(iLevel).GetRecord(iRec);
    int iRetVal = 0;

    if (pRec->GetItem(iItem)->GetItemType() == ItemType::Subitem)  {
        // find the parent item!
        iItem = m_pDict->GetParentItemNum(iLevel, iRec, iItem);
        ASSERT(iItem >= 0 && iItem < pRec->GetNumItems());
    }

    for (int i = iItem+1 ; i < pRec->GetNumItems() ; i++)  {
        if (pRec->GetItem(i)->GetItemType() == ItemType::Subitem)  {
            iRetVal++;
        }
        else  {
            break;
        }
    }
    return iRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::IsSorted
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::IsSorted(int iLevelNum, int iRecordNum)
{
    if (iRecordNum == NONE)  {
        // the record in question is in the process of being added; it doesn't have
        // any items yet, so it must be sorted!
        return true;
    }

    ASSERT(m_pDict);
    CDictRecord* pRec = m_pDict->GetLevel(iLevelNum).GetRecord(iRecordNum);
    ASSERT(pRec);
    CDictItem* pItem;

    bool bRetVal = true;
    UINT uStart = 0;
    UINT uSubStart = 0;
    for (int i = 0 ; i < pRec->GetNumItems() ; i++)  {
        pItem = pRec->GetItem(i);
        if (pItem->GetItemType() == ItemType::Item)  {
            if (pItem->GetStart() >= uStart)  {
                uStart = uSubStart = pItem->GetStart();
            }
            else  {
                bRetVal = false;
                break;
            }
        }
        else  {
            // subitems have to be sorted w/in order under their parent item, but they can overlap
            if (pItem->GetStart() >= uSubStart)  {
                uSubStart = pItem->GetStart();
            }
            else  {
                bRetVal = false;
                break;
            }
        }
    }
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::Sort
//
/////////////////////////////////////////////////////////////////////////////

void DictionaryValidator::Sort(int iLevelNum, int iRecordNum)
{
    // sort the RT by start position ...
    ASSERT(m_pDict);
    CDictRecord* pCurrRec = m_pDict->GetLevel(iLevelNum).GetRecord(iRecordNum);
    ASSERT(pCurrRec);

    CDictRecord temp_dict_record;

    UINT uStartMin;         // min starting position
    int iMinIndex = 0;      // item # in pCurrRec for the min starting position
    int iSubMinIndex = 0;   // item # in pCurrRec for the min substart position

    int iInitSize = pCurrRec->GetNumItems();
    while (pCurrRec->GetNumItems() > 0)  {
        uStartMin = std::numeric_limits<UINT>::max();

        for (int i = 0 ; i < pCurrRec->GetNumItems() ; i++)  {
            CDictItem* pItem = pCurrRec->GetItem(i);
            if (pItem->GetStart() < uStartMin && pItem->GetItemType()==ItemType::Item)  {
                uStartMin = pItem->GetStart();
                iMinIndex = i;
            }
        }

        // add the min item to the (sorted) record
        ASSERT(uStartMin < std::numeric_limits<UINT>::max());
        CDictItem* pItem = pCurrRec->GetItem(iMinIndex);

        temp_dict_record.AddItem(pItem);

        pCurrRec->RemoveItemAt(iMinIndex);

        // now add all of that item's subitems
        int iSubitemCount = 0;
        for (int i = iMinIndex ; i < pCurrRec->GetNumItems() ; i++)  {
            // count the number of subitems for this item
            if (pCurrRec->GetItem(i)->GetItemType() == ItemType::Subitem)  {
                iSubitemCount++;
            }
            else  {
                break;
            }
        }


        for (int i = 0 ; i < iSubitemCount ; i++)  {
            uStartMin = std::numeric_limits<UINT>::max();
            for (int j = 0 ; j < iSubitemCount - i ; j++)  {
                ASSERT(pCurrRec->GetItem(j+iMinIndex)->GetItemType() == ItemType::Subitem);
                ASSERT(j+iMinIndex < pCurrRec->GetNumItems());
                pItem = pCurrRec->GetItem(j+iMinIndex);
                if (pItem->GetStart() < uStartMin)  {
                    ASSERT(pItem->GetItemType()==ItemType::Subitem);
                    uStartMin = pItem->GetStart();
                    iSubMinIndex = j+iMinIndex;
                }
            }

            // add the min subitem to the (sorted record)
            ASSERT(uStartMin < std::numeric_limits<UINT>::max());
            pItem = pCurrRec->GetItem(iSubMinIndex);

            temp_dict_record.AddItem(pItem);

            pCurrRec->RemoveItemAt(iSubMinIndex);
        }
    }

    // now move the sorted rec back into the dictionary
    ASSERT(pCurrRec->GetNumItems() == 0);
    for (int i = 0 ; i < temp_dict_record.GetNumItems() ; i++)  {
        pCurrRec->AddItem(temp_dict_record.GetItem(i));
    }
    ASSERT(iInitSize == pCurrRec->GetNumItems());
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetDefaultItemStart
//
/////////////////////////////////////////////////////////////////////////////
//
//      This is not guaranteed to generate a non-overlapping start
//
/////////////////////////////////////////////////////////////////////////////

int DictionaryValidator::GetDefaultItemStart(int iLevel, int iRec, int iItem)
{
    ASSERT(m_pDict);
    ASSERT (iLevel >= 0 && iLevel < (int)m_pDict->GetNumLevels());
    ASSERT ((iRec >= 0 || iRec == COMMON) && iRec < m_pDict->GetLevel(iLevel).GetNumRecords());
    ASSERT (iItem >= 0 && iItem <= m_pDict->GetLevel(iLevel).GetRecord(iRec)->GetNumItems());

    int iRetVal = NONE;
    int iRTStart = m_pDict->GetRecTypeStart();
    int iRTLen = m_pDict->GetRecTypeLen();

    const CDictRecord* pIdRec = m_pDict->GetLevel(iLevel).GetRecord(iRec);
    if (iItem > 0 && iItem - 1 < pIdRec->GetNumItems())  {
        // If there is a previous item, start just after it
        const CDictItem* pItem = pIdRec->GetItem(iItem - 1);
        if (pItem->GetItemType() != ItemType::Item)  {
            pItem = m_pDict->GetParentItem(iLevel, iRec, iItem - 1);
        }
        iRetVal = pItem->GetStart() + pItem->GetLen()*pItem->GetOccurs();
    }
    else {
        for( size_t level_number = 0; level_number < m_pDict->GetNumLevels(); ++level_number ) {
            const DictLevel& dict_level = m_pDict->GetLevel(level_number);
            const CDictRecord* pRec = dict_level.GetIdItemsRec();
            if (pRec->GetNumItems() > 0)  {
                // If there are only id items, start just after after last one
                iItem = pRec->GetNumItems() - 1;
                const CDictItem* pItem = pRec->GetItem(iItem);
                if (pItem->GetItemType() != ItemType::Item)  {
                    pItem = m_pDict->GetParentItem(level_number, COMMON, iItem);
                }
                int iStart = pItem->GetStart() + pItem->GetLen() * pItem->GetOccurs();
                if (iStart > iRetVal) {
                    iRetVal = iStart;
                }
            }
        }
        if (iRetVal == NONE) {
            // If there is only a record type, start just after it
            if (iRTStart != 0 && iRTLen != 0)  {
                iRetVal = iRTStart + iRTLen;
            }
            else  {
            // Otherwise start = 1
                iRetVal = 1;
            }
        }
    }
    ASSERT(iRetVal != NONE);
    if (iRTStart != 0 && iRTLen != 0 && iRetVal >= iRTStart && iRetVal < iRTStart+iRTLen)  {
        // If the chosen start overlaps with the record type, start after the record type
        iRetVal = iRTStart + iRTLen;
    }
    return iRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetDefaultName
//
/////////////////////////////////////////////////////////////////////////////

CString DictionaryValidator::GetDefaultName(const CString& label)
{
    CIMSAString name = label;
    name.MakeName();
    name = m_pDict->GetUniqueName(name, m_iLevelNum, m_iRecordNum, m_iItemNum);
    return name;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetDefaultRecTypeVal
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::MakeRecordTypeUnique(const CDataDict& dictionary, CString& record_type, const std::set<CString>& additional_record_types)
{
    ASSERT(dictionary.GetRecTypeLen() > 0);

    record_type = CIMSAString::MakeExactLength(record_type, dictionary.GetRecTypeLen());

    auto is_record_type_unique =
        [&](const CString& record_type_to_check)
        {
            ASSERT(record_type_to_check.GetLength() == (int)dictionary.GetRecTypeLen());

            if( additional_record_types.find(record_type_to_check) != additional_record_types.cend() )
                return false;

            bool unique = true;

            DictionaryIterator::ForeachWhile<CDictRecord>(dictionary,
                [&](const CDictRecord& dict_record)
                {
                    if( !dict_record.IsIdRecord() && dict_record.GetRecTypeVal() == record_type_to_check )
                        unique = false;

                    return unique;
                });

            return unique;
        };

    if( !SO::IsWhitespace(record_type) && is_record_type_unique(record_type) )
        return true;

    // if the record type is not unique:

    // 1. try numbers (up to nine digits)
    int highest_valid_record_type = (int)Power10[std::min<unsigned>(dictionary.GetRecTypeLen(), 9)] - 1;

    for( int i = 1; i <= highest_valid_record_type; ++i )
    {
        record_type.Format(_T("%0*d"), (int)dictionary.GetRecTypeLen(), i);
        ASSERT(record_type.GetLength() == (int)dictionary.GetRecTypeLen());

        if( is_record_type_unique(record_type) )
            return true;
    }

    // 2. try letters
    constexpr uint64_t first_letter = 'A';
    constexpr uint64_t last_letter  = 'Z';
    constexpr uint64_t letter_range = last_letter - first_letter + 1;

    uint64_t number_permutations = (uint64_t)std::min(std::pow<double>(letter_range, dictionary.GetRecTypeLen()), (double)UINT64_MAX);

    for( uint64_t i = 0; i < number_permutations; ++i )
    {
        record_type.Empty();

        uint64_t permutation = i;

        do
        {
            uint64_t remainder = permutation % letter_range;
            record_type.Insert(0, static_cast<TCHAR>(first_letter + remainder));
            permutation /= letter_range;

        } while( permutation != 0 );

        for( int padding_needed = (int)dictionary.GetRecTypeLen() - record_type.GetLength(); padding_needed > 0; --padding_needed )
            record_type.Insert(0, first_letter);

        if( is_record_type_unique(record_type) )
            return true;
    }

    return false;
}


CString DictionaryValidator::GetDefaultRecTypeVal() const
{
    int iUnique = 0;
    CIMSAString csRetVal;
    int iMaxNumeric = 99999;
    int iLen;

    iLen = m_pDict->GetRecTypeLen();
    if (iLen == 0)  {
        ASSERT(m_pDict->GetRecTypeStart() == 0);
        if (m_pDict->GetNumRecords() == 0)  {
            return csRetVal;
        }
        // 2nd RT in a fresh DD, give it 2 a record type value
        ASSERT(m_pDict->GetLevel(m_iLevelNum).GetRecord(0)->GetRecTypeVal().IsEmpty());
        csRetVal = _T("2");
        return csRetVal;
    }

    if (m_pDict->GetRecTypeLen() < 6)  {
        iMaxNumeric = (int) pow(10.0, iLen) - 1L; // RHF Make compatible with Visual 2005
    }
    csRetVal.Str(iUnique + 1, iLen, ZERO);
    CString csAlphaUnique(TCHAR('A' - 1), 1);

    bool bDone = false;
    while (!bDone)  {
        bDone = true;
        for( const DictLevel& dict_level : m_pDict->GetLevels() ) {
            for (int r = 0 ; r < dict_level.GetNumRecords() ; r++)  {
                if (dict_level.GetRecord(r)->GetRecTypeVal() == csRetVal)  {
                    /*-------------------------------------------------------------------
                        the initial try default RT value already exists, so let's start
                        checking again, increment from 1 until a unique number is found
                    -------------------------------------------------------------------*/
                    bDone = false;  // we need to make 1 full pass w/o changes
                    iUnique++;
                    csRetVal.Empty();

                    if (iUnique > iMaxNumeric)  {
                        // switch to alpha
                        int iAlphaLen = csAlphaUnique.GetLength();
                        TCHAR c = csAlphaUnique[iAlphaLen-1];
                        c++;
                        if (c > _T('Z') && iAlphaLen < iLen)  {
                            csAlphaUnique += _T('A');
                            iAlphaLen++;
                        }
                        else  {
                            csAlphaUnique.SetAt(iAlphaLen-1, c);
                        }
                        csRetVal = csAlphaUnique;
                    }
                    else  {
                        // increment the number
                        csRetVal.Str(iUnique, iLen, ZERO);
                    }
                }
            }
        }
    }
    return csRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetRecordLength
//
/////////////////////////////////////////////////////////////////////////////

unsigned DictionaryValidator::GetRecordLength(const CDataDict& dictionary, const CDictRecord* dict_record)
{
    unsigned length = DictionaryDefaults::RecLen;

    auto adjust = [&](unsigned this_start, unsigned this_length)
    {
        length = std::max(length, this_start + this_length - 1);
    };

    auto process_record = [&](const CDictRecord& this_dict_record)
    {
        for( int i = 0; i < this_dict_record.GetNumItems(); ++i )
        {
            const CDictItem* dict_item = this_dict_record.GetItem(i);

            if( !dict_item->IsSubitem() )
                adjust(dict_item->GetStart(), dict_item->GetLen() * dict_item->GetOccurs());
        }
    };

    // record type
    if( dictionary.GetRecTypeStart() != 0 && dictionary.GetRecTypeLen() != 0 )
        adjust(dictionary.GetRecTypeStart(), dictionary.GetRecTypeLen());

    // ID items
    for( const DictLevel& dict_level : dictionary.GetLevels() )
        process_record(*dict_level.GetIdItemsRec());

    // a supplied record
    if( dict_record != nullptr )
        process_record(*dict_record);

    return length;
}

unsigned DictionaryValidator::GetRecordLength(int iLevel, int iRec) const
{
    const CDictRecord* dict_record = ( iRec >= 0 ) ? m_pDict->GetLevel(iLevel).GetRecord(iRec) : nullptr;
    return GetRecordLength(dict_record);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::GetMaxRecLen
//
/////////////////////////////////////////////////////////////////////////////

UINT DictionaryValidator::GetMaxRecLen() const
{
    UINT uMaxLen = 0;
    for( const DictLevel& dict_level : m_pDict->GetLevels() ) {
        for (int r = 0 ; r < dict_level.GetNumRecords() ; r++) {
            uMaxLen = std::max(uMaxLen, (UINT) dict_level.GetRecord(r)->GetRecLen());
        }
    }
    return uMaxLen;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::ResetRecLens
//
/////////////////////////////////////////////////////////////////////////////

void DictionaryValidator::ResetRecLens(CDataDict& dictionary)
{
    for( DictLevel& dict_level : dictionary.GetLevels() )
    {
        for( int r = 0; r < dict_level.GetNumRecords(); ++r )
        {
            CDictRecord* dict_record = dict_level.GetRecord(r);
            dict_record->SetRecLen(GetRecordLength(dictionary, dict_record));
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           DictionaryValidator::AdjustValues
//
/////////////////////////////////////////////////////////////////////////////

bool DictionaryValidator::AdjustValues(DictValueSet& dict_value_set)
{
    // this function sorts the special values so that they are in the order: missing, refused, default, notappl
    bool changed = false;
    bool has_special_values = false;

    size_t expected_position = dict_value_set.GetNumValues() - 1;

    auto adjust_position = [&](double special_value)
    {
        for( size_t i = 0; i < dict_value_set.GetNumValues(); ++i )
        {
            DictValue& dict_value = dict_value_set.GetValue(i);

            if( dict_value.IsSpecialValue(special_value) )
            {
                if( i != expected_position )
                {
                    changed = true;
                    DictValue dict_value_copy = std::move(dict_value);
                    dict_value_set.RemoveValue(i);
                    dict_value_set.InsertValue(expected_position, dict_value_copy);
                }

                --expected_position;
                return;
            }

            else if( dict_value.IsSpecial() )
            {
                has_special_values = true;
            }
        }
    };

    adjust_position(NOTAPPL);

    if( has_special_values )
    {
        adjust_position(DEFAULT);
        adjust_position(REFUSED);
        adjust_position(MISSING);
    }

    return changed;
}


bool DictionaryValidator::CheckAliases(DictNamedBase& dict_element, bool throw_error/* = false*/,
    const std::set<CString>* new_aliases_to_check/* = nullptr*/)
{
    const auto& current_aliases = dict_element.GetAliases();
    const auto& aliases_to_check = ( new_aliases_to_check != nullptr ) ? *new_aliases_to_check : current_aliases;
    bool valid = true;

    for( const CString& alias : aliases_to_check )
    {
        auto issue_error = [&](const TCHAR* error_message)
        {
            CString full_message = FormatText(_T("The alias '%s' %s."), alias.GetString(), error_message);

            if( throw_error )
                throw CSProException(full_message);

            m_csErrorReport += GetErrorName(dict_element) + full_message + CRLF;
            valid = false;
        };

        if( !CIMSAString::IsName(alias) )
            issue_error(_T("is not a valid CSPro name"));

        else if( CIMSAString::IsReservedWord(alias) )
            issue_error(_T("cannot be used because it is a reserved word"));

        else
        {
            int iLevel;
            int iRec;
            int iItem;
            int iVSet;
            m_pDict->LookupName(dict_element.GetName(), &iLevel, &iRec, &iItem, &iVSet);

            if( !m_pDict->IsNameUnique(alias, iLevel, iRec, iItem, iVSet) )
                issue_error(_T("cannot be used because the name is already in use"));
        }
    }

    // remove all aliases if they are invalid
    if( !valid && m_bAutoFixAndRecurse )
        dict_element.SetAliases({ });

    return valid;
}


bool DictionaryValidator::CheckNote(DictBase& dict_base)
{
    bool valid = true;

    // if the note is all blank, set to empty
    if( SO::IsWhitespace(dict_base.GetNote()) && !dict_base.GetNote().IsEmpty() )
    {
        dict_base.SetNote(CString());
    }

    // if the note is too long, truncate it
    else if( dict_base.GetNote().GetLength() > MAX_NOTE_LEN )
    {
        valid = false;

        const DictNamedBase* dict_element_for_name;

        if( dict_base.GetElementType() == DictElementType::Value )
            dict_element_for_name = &m_pDict->GetLevel(m_iLevelNum).GetRecord(m_iRecordNum)->GetItem(m_iItemNum)->GetValueSet(m_iVSetNum);

        else
            dict_element_for_name = assert_cast<const DictNamedBase*>(&dict_base);

        m_csErrorReport += GetErrorName(*dict_element_for_name) +
                           FormatText(_T("The note is too long (maximum %d characters)."), MAX_NOTE_LEN) +
                           CRLF;

        if( m_bAutoFixAndRecurse )
            dict_base.SetNote(dict_base.GetNote().Left(MAX_NOTE_LEN));
    }

    return valid;
}
