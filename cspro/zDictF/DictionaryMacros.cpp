#include "StdAfx.h"
#include "DictionaryMacros.h"
#include "Itemgrid.h"
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/NameShortener.h>
#include <zUtilO/Filedlg.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilF/SystemIcon.h>
#include <zUtilF/ThreadedProgressDlg.h>
#include <zAppO/PFF.h>
#include <ZBRIDGEO/DataFileDlg.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <random>


// CDictionaryMacros dialog

IMPLEMENT_DYNAMIC(CDictionaryMacros, CDialog)

BEGIN_MESSAGE_MAP(CDictionaryMacros, CDialog)
    ON_BN_CLICKED(IDC_DELETE_VALUE_SETS, OnBnClickedDeleteValueSets)
    ON_BN_CLICKED(IDC_REQUIRE_RECORDS_YES, OnBnClickedRequireRecordsYes)
    ON_BN_CLICKED(IDC_REQUIRE_RECORDS_NO, OnBnClickedRequireRecordsNo)
    ON_BN_CLICKED(IDC_COPY_DICTIONARY_NAMES, OnBnClickedCopyDictionaryNames)
    ON_BN_CLICKED(IDC_PASTE_DICTIONARY_NAMES, OnBnClickedPasteDictionaryNames)
    ON_BN_CLICKED(IDC_COPY_VALUE_SETS, OnBnClickedCopyValueSets)
    ON_BN_CLICKED(IDC_PASTE_VALUE_SETS, OnBnClickedPasteValueSets)
    ON_BN_CLICKED(IDC_GENERATE_DATA_FILE, OnBnClickedGenerateDataFile)
    ON_BN_CLICKED(IDC_CREATE_SAMPLE, OnBnClickedCreateSample)
    ON_BN_CLICKED(IDC_ADD_ITEMS, OnBnClickedAddItemsToRecord)
    ON_BN_CLICKED(IDC_COMPACT_DATA_FILE, OnBnClickedCompactDataFile)
    ON_BN_CLICKED(IDC_SORT_DATA_FILE, OnBnClickedSortDataFile)
    ON_BN_CLICKED(IDC_CREATE_NOTES_DICT, OnBnClickedCreateNotesDictionary)
END_MESSAGE_MAP()


CDictionaryMacros::CDictionaryMacros(CDDDoc* pDDDoc,CWnd* pParent /*=NULL*/)
    :   CDialog(CDictionaryMacros::IDD, pParent),
        m_pDictDoc(pDDDoc),
        m_pDict(m_pDictDoc->GetDict())
{
}

void CDictionaryMacros::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    if( m_pDict->GetNumLevels() > 1 )
    {
        for( int id : { IDC_GENERATE_DATA_FILE, IDC_NUMBER_CASES, IDC_NOTAPPL_PERCENT, IDC_INVALID_PERCENT } )
            GetDlgItem(id)->EnableWindow(FALSE);
    }

    CheckDlgButton(IDC_RANDOM_FILE,BST_CHECKED);
    GetDlgItem(IDC_START_POS)->SetWindowText(_T("1"));

    if( m_pDict->GetLanguages().size() > 1 )
    {
        CheckDlgButton(IDC_ITEM_ALL_LANGUAGES,BST_CHECKED);
        CheckDlgButton(IDC_VS_ALL_LANGUAGES,BST_CHECKED);
    }

    else
    {
        GetDlgItem(IDC_ITEM_ALL_LANGUAGES)->EnableWindow(FALSE);
        GetDlgItem(IDC_VS_ALL_LANGUAGES)->EnableWindow(FALSE);
    }

    CheckDlgButton(IDC_VS_IMAGES,BST_CHECKED);

    if( !m_pDict->IsPosRelative() )
        GetDlgItem(IDC_ITEM_LENGTHS)->EnableWindow(FALSE);

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_RECORD_LISTING);
    for( const DictLevel& dict_level : m_pDict->GetLevels() )
    {
        for( int record = 0; record < dict_level.GetNumRecords(); record++ )
        {
            const CDictRecord* pRecord = dict_level.GetRecord(record);
            int pos = pComboBox->AddString(pRecord->GetName());
            pComboBox->SetItemDataPtr(pos, (void*)pRecord);
        }
    }

    CString csDialogTitleText;
    GetWindowText(csDialogTitleText);
    csDialogTitleText.AppendFormat(_T(" - %s"), (LPCTSTR)m_pDict->GetName());
    SetWindowText(csDialogTitleText);
}


// CDictionaryMacros message handlers

void CDictionaryMacros::OnBnClickedDeleteValueSets() // 20101108
{
    // 20120608 a Jordanian in the workshop asked for this confirmation
    if( AfxMessageBox(_T("Are you sure you want to delete all the value sets?"), MB_YESNO | MB_ICONSTOP) != IDYES ) 
        return;

    size_t num_value_sets = 0;

    DictionaryIterator::Foreach<CDictItem>(*m_pDict,
        [&](CDictItem& dict_item)
        {
            num_value_sets += dict_item.GetNumValueSets();
            dict_item.RemoveAllValueSets();
        });

    m_pDictDoc->SetModified(true);

    AfxMessageBox(FormatText(_T("%d value set%s have been deleted"), (int)num_value_sets, PluralizeWord(num_value_sets)));
}


void CDictionaryMacros::SetRequireRecords(bool required) // 20101108
{
    DictionaryIterator::Foreach<CDictRecord>(*m_pDict,
        [&](CDictRecord& dict_record)
        {
            dict_record.SetRequired(required);
        });

    m_pDictDoc->SetModified(true);
}


void CDictionaryMacros::OnBnClickedRequireRecordsYes() // 20101108
{
    SetRequireRecords(true);
    AfxMessageBox(_T("All records are now required"));
}


void CDictionaryMacros::OnBnClickedRequireRecordsNo() // 20101108
{
    SetRequireRecords(false);
    AfxMessageBox(_T("All records are not required"));
}


CString CDictionaryMacros::GetLabelWithLanguages(const LabelSet& label, bool copy_all_languages) const
{
    if( !copy_all_languages )
        return label.GetLabel();

    CString labels;

    for( size_t i = 0; i < m_pDict->GetLanguages().size(); ++i )
        labels.AppendFormat(_T("%s%s"), ( i == 0 ) ? _T("") : _T("\t"), (LPCTSTR)label.GetLabel(i));

    return labels;
}


void CDictionaryMacros::SetLabelWithLanguages(LabelSet& label, const CStringArray& csaLabels, bool paste_all_languages)
{
    if( !paste_all_languages )
    {
        label.SetLabel(csaLabels[0]);
        return;
    }

    for( size_t i = m_pDict->GetLanguages().size() - 1; i < m_pDict->GetLanguages().size(); --i )
    {
        // prevent unmodified labels that are equal to the primary language label from being set
        bool modify_label =
            ( i == 0 ) ||
            ( csaLabels[i].Compare(csaLabels[0]) != 0 ) ||
            ( i < label.GetLabels().size() && !label.GetLabel(i).IsEmpty() );

        if( modify_label )
            label.SetLabel(csaLabels[i], i);
    }
}


void CDictionaryMacros::OnBnClickedCopyDictionaryNames() // 20101108
{
    struct NameDictionaryIterator : public DictionaryIterator::Iterator
    {
        CString text;
        bool copy_all_languages;
        bool copy_item_lengths;
        CString indentation;
        const CDictionaryMacros* dlg;

        void ProcessLevel(DictLevel& dict_level) override
        {
            text.AppendFormat(_T("%s\t%s\r\n"),
                              (LPCTSTR)dict_level.GetName(),
                              (LPCTSTR)dlg->GetLabelWithLanguages(dict_level.GetLabelSet(), copy_all_languages));
        }

        void ProcessRecord(CDictRecord& dict_record) override
        {
            text.AppendFormat(_T("\t%s%s\t%s\r\n"),
                              (LPCTSTR)indentation,
                              (LPCTSTR)dict_record.GetName(),
                              (LPCTSTR)dlg->GetLabelWithLanguages(dict_record.GetLabelSet(), copy_all_languages));
        }

        void ProcessItem(CDictItem& dict_item) override
        {
            text.AppendFormat(_T("\t%s\t%s%s\t%s"),
                              (LPCTSTR)indentation, (LPCTSTR)indentation,
                              (LPCTSTR)dict_item.GetName(),
                              (LPCTSTR)dlg->GetLabelWithLanguages(dict_item.GetLabelSet(), copy_all_languages));

            if( copy_item_lengths )
                text.AppendFormat(_T("\t%d"), dict_item.GetLen());

            text.Append(_T("\r\n"));
        }
    };

    NameDictionaryIterator iterator;
    iterator.copy_all_languages = ((CButton*)GetDlgItem(IDC_ITEM_ALL_LANGUAGES))->GetCheck();
    iterator.copy_item_lengths = ((CButton*)GetDlgItem(IDC_ITEM_LENGTHS))->GetCheck();
    iterator.indentation = CString('\t', iterator.copy_all_languages ? m_pDict->GetLanguages().size() : 1);
    iterator.dlg = this;

    iterator.Iterate(*m_pDict);

    WinClipboard::PutText(this, iterator.text);
    AfxMessageBox(_T("Names and labels copied to the clipboard"));
}


int CDictionaryMacros::readEntry(CString text,int startPos,CString& readWord)
{
    int endPos;

    for( endPos = startPos; endPos < text.GetLength(); endPos++ )
    {
        if( text[endPos] == _T('\t') || text[endPos] == _T('\n') || text[endPos] == '\r' )
            break;
    }

    readWord = text.Mid(startPos,endPos - startPos);

    return endPos;
}

int CDictionaryMacros::readLabelsEntry(CString text,int startPos,CStringArray& csaLabels,bool paste_all_languages, bool& bSuccessfulRead)
{
    CString csLabel;
    int endPos;

    csaLabels.RemoveAll();

    if( paste_all_languages )
    {
        endPos = startPos;

        for( size_t i = 0; bSuccessfulRead && i < m_pDict->GetLanguages().size(); ++i )
        {
            endPos = readEntry(text,endPos,csLabel);

            if( i < ( m_pDict->GetLanguages().size() - 1 ) && ( text[endPos++] != _T('\t') ) )
                bSuccessfulRead = false;

            else if( csLabel.IsEmpty() )
                bSuccessfulRead = false;

            else
                csaLabels.Add(csLabel);
        }
    }

    else
    {
        endPos = readEntry(text,startPos,csLabel);

        if( csLabel.IsEmpty() )
            bSuccessfulRead = false;

        else
            csaLabels.Add(csLabel);
    }

    return endPos;
}


void CDictionaryMacros::makeNewNameWork(DictNamedBase& dict_element, const CString& oldName) // 20101109
{
    int level = -1,record = -1,item = -1,vset = -1;
    m_pDict->LookupName(oldName,&level,&record,&item,&vset);
    m_pDict->UpdateNameList(dict_element,level,record,item,vset);
    m_pDict->SetOldName(oldName);
    AfxGetMainWnd()->SendMessage(UWM::Dictionary::NameChange, (WPARAM)m_pDictDoc);
}


void AdjustItemStartPositions(CDictRecord* pRecord,CDictItem* pResizedItem,int startAdjustmentFactor) // 20140309
{
    CDictItem* pLastItem = NULL;

    for( int i = 0; i < pRecord->GetNumItems(); i++ )
    {
        CDictItem* pItem = pRecord->GetItem(i);

        if( pItem->GetItemType() == ItemType::Item )
            pLastItem = pItem;

        if( pItem->GetStart() > pResizedItem->GetStart() )
        {
            // don't resize subitems of the item that is being resized
            if( pItem->GetItemType() != ItemType::Subitem || pLastItem != pResizedItem )
                pItem->SetStart(pItem->GetStart() + startAdjustmentFactor);
        }
    }
}


void CDictionaryMacros::OnBnClickedPasteDictionaryNames() // 20101108
{
    CString text = WS2CS(WinClipboard::GetText(this));
    text.Append(_T("\n               ")); // this should ensure that none of my text[textPtr++] codes will cause an out of bounds error

    // we need to check two things:
    // 1) that the text on the clipboard matches what is already in the dictionary (i.e., same number of records, items, etc.)
    // 2) that no duplicate names are used

    bool successfulRead = true;
    bool firstPass = true;
    CArray<CString> newNames;

    bool paste_all_languages = ((CButton*)GetDlgItem(IDC_ITEM_ALL_LANGUAGES))->GetCheck();
    bool bPasteItemLengths = ((CButton*)GetDlgItem(IDC_ITEM_LENGTHS))->GetCheck();
    CString csErrorMessage = _T("Error: Clipboard contents do not match dictionary");

    int iExpectedLabelIndentationTabs = paste_all_languages ? m_pDict->GetLanguages().size() : 1;

    for( int i1 = 0; i1 < 2 && successfulRead; i1++ ) // first pass is the above checks; second pass actually changes the values
    {
        int textPtr = 0;
        CString name;
        CStringArray csaLabels;
        CString oldName;

        for( DictLevel& dict_level : m_pDict->GetLevels() )
        {
            // read the newline separating multiple levels 
            if( successfulRead && dict_level.GetLevelNumber() > 0 )
            {
                if( text[textPtr] == '\r' )
                    textPtr++;

                successfulRead = text[textPtr++] == _T('\n');
            }

            if( !successfulRead )
                break;

            textPtr = readEntry(text,textPtr,name);
            name.MakeUpper();
            successfulRead = text[textPtr++] == _T('\t');

            if( successfulRead )
                textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);

            if( firstPass )
            {
                newNames.Add(name);
            }

            else
            {
                SetLabelWithLanguages(dict_level.GetLabelSet(), csaLabels, paste_all_languages);

                oldName = dict_level.GetName();

                if( name != oldName )
                {
                    dict_level.SetName(name);
                    m_pDict->SetChangedObject(&dict_level);
                    makeNewNameWork(dict_level, oldName);
                }
            }

            for( int record = -1; record < dict_level.GetNumRecords() && successfulRead; record++ )
            {
                CDictRecord* pRecord = record == -1 ? dict_level.GetIdItemsRec() : dict_level.GetRecord(record);

                while( text[textPtr] == '\t' ) // there will be extra tabs if the data is copied from Excel
                    textPtr++;

                if( text[textPtr] == '\r' )
                    textPtr++;

                successfulRead = text[textPtr++] == _T('\n');

                for( int iTab = 0; successfulRead && iTab < ( 1 + iExpectedLabelIndentationTabs ); iTab++ )
                    successfulRead = text[textPtr++] == _T('\t');

                if( successfulRead )
                {
                    textPtr = readEntry(text,textPtr,name);
                    name.MakeUpper();
                    successfulRead = text[textPtr++] == _T('\t');

                    if( successfulRead )
                    {
                        textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);

                        if( firstPass )
                        {
                            newNames.Add(name);
                        }

                        else
                        {
                            SetLabelWithLanguages(pRecord->GetLabelSet(), csaLabels, paste_all_languages);

                            oldName = pRecord->GetName();

                            if( name != oldName )
                            {
                                pRecord->SetName(name);
                                m_pDict->SetChangedObject(pRecord);
                                makeNewNameWork(*pRecord,oldName);
                            }
                        }

                        int iLastItemLength = 0; // 20140309
                        int iCumulativeSubitemLength = 0;
                        bool bHasOverlappingSubitems = false;

                        for( int item = 0; item < pRecord->GetNumItems() && successfulRead; item++ )
                        {
                            CDictItem* pItem = pRecord->GetItem(item);

                            while( text[textPtr] == '\t' ) // there will be extra tabs if the data is copied from Excel
                                textPtr++;

                            if( text[textPtr] == '\r' )
                                textPtr++;

                            successfulRead = text[textPtr++] == _T('\n');

                            for( int iTab = 0; successfulRead && iTab < ( 2 * ( 1 + iExpectedLabelIndentationTabs ) ) ; iTab++ )
                                successfulRead = text[textPtr++] == _T('\t');

                            if( successfulRead )
                            {
                                textPtr = readEntry(text,textPtr,name);
                                name.MakeUpper();
                                successfulRead = text[textPtr++] == _T('\t');

                                if( successfulRead )
                                {
                                    textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);

                                    if( firstPass )
                                    {
                                        newNames.Add(name);
                                    }

                                    else
                                    {
                                        // if the value set's label was equal to the old label, change it to the new label
                                        if( pItem->HasValueSets() )
                                        {
                                            DictValueSet& dict_value_set = pItem->GetValueSet(0);

                                            if( paste_all_languages )
                                            {
                                                for( size_t lang = 0; lang < m_pDict->GetLanguages().size(); ++lang )
                                                {
                                                    // the bItemLabelChanged flag is so that we don't add a label for a language
                                                    // that had its label undefined (and was thus using the first language's label)
                                                    bool bItemLabelChanged = ( csaLabels[lang].Compare(pItem->GetLabelSet().GetLabel(lang)) != 0 );

                                                    if( bItemLabelChanged && dict_value_set.GetLabelSet().GetLabel(lang).Compare(pItem->GetLabelSet().GetLabel(lang)) == 0 )
                                                        dict_value_set.GetLabelSet().SetLabel(csaLabels[lang], lang);
                                                }
                                            }

                                            else if( dict_value_set.GetLabel().Compare(pItem->GetLabel()) == 0 )
                                            {
                                                dict_value_set.SetLabel(csaLabels[0]);
                                            }
                                        }

                                        SetLabelWithLanguages(pItem->GetLabelSet(), csaLabels, paste_all_languages);

                                        oldName = pItem->GetName();

                                        if( name != oldName )
                                        {
                                            pItem->SetName(name);
                                            m_pDict->SetChangedObject(pItem);
                                            makeNewNameWork(*pItem,oldName);
                                        }
                                    }

                                    if( bPasteItemLengths ) // 20140309
                                    {
                                        successfulRead = text[textPtr++] == _T('\t');

                                        if( successfulRead )
                                        {
                                            CString csItemLength;
                                            textPtr = readEntry(text,textPtr,csItemLength);
                                            int iItemLength = _ttoi(csItemLength);

                                            if( firstPass ) // validate the item length
                                            {
                                                if( iItemLength < 1 )
                                                {
                                                    successfulRead = false;
                                                    csErrorMessage.Format(_T("The length of item %s must be at least 1"),(LPCTSTR)name);
                                                }

                                                else if( pItem->GetContentType() == ContentType::Numeric )
                                                {
                                                    if( iItemLength > 15 )
                                                    {
                                                        successfulRead = false;
                                                        csErrorMessage.Format(_T("The length of numeric item %s cannot be greater than 15"),(LPCTSTR)name);
                                                    }

                                                    else if( iItemLength < ( pItem->GetDecimal() + pItem->GetDecChar() ) )
                                                    {
                                                        successfulRead = false;
                                                        csErrorMessage.Format(_T("The length of numeric item %s with %d decimal characters cannot be %d"),(LPCTSTR)name,pItem->GetDecimal(),iItemLength);
                                                    }
                                                }

                                                if( iItemLength != 1 && !DictionaryRules::CanModifyLength(*pItem) )
                                                {
                                                    // binary type must be length 1
                                                    successfulRead = false;
                                                    csErrorMessage.Format(_T("The length of %s item %s must be 1"), ToString(pItem->GetContentType()), (LPCTSTR)name);
                                                }

                                                if( pItem->GetItemType() == ItemType::Item )
                                                {
                                                    iLastItemLength = iItemLength;
                                                    iCumulativeSubitemLength = 0;

                                                    int iNextStartPosition = 0;
                                                    bHasOverlappingSubitems = false;

                                                    for( int subitem = item + 1; subitem < pRecord->GetNumItems(); subitem++ )
                                                    {
                                                        CDictItem* pSubitem = pRecord->GetItem(subitem);

                                                        if( pSubitem->GetItemType() != ItemType::Subitem )
                                                            break;

                                                        if( iNextStartPosition > pSubitem->GetStart() )
                                                        {
                                                            bHasOverlappingSubitems = true;
                                                            break;
                                                        }

                                                        iNextStartPosition = pSubitem->GetStart() + pSubitem->GetLen() * pSubitem->GetOccurs();
                                                    }

                                                    if( bHasOverlappingSubitems && iItemLength < pItem->GetLen() )
                                                    {
                                                        successfulRead = false;
                                                        csErrorMessage.Format(_T("The length of an item (%s) with overlapping subitems cannot be reduced in size"),(LPCTSTR)name);
                                                    }
                                                }

                                                else
                                                {
                                                    if( bHasOverlappingSubitems )
                                                    {
                                                        if( iItemLength != pItem->GetLen() )
                                                        {
                                                            successfulRead = false;
                                                            csErrorMessage.Format(_T("The lengths of overlapping subitems (including %s) cannot be modified"),(LPCTSTR)name);
                                                        }
                                                    }

                                                    else
                                                    {
                                                        iCumulativeSubitemLength += iItemLength * pItem->GetOccurs();

                                                        if( iCumulativeSubitemLength > iLastItemLength )
                                                        {
                                                            successfulRead = false;
                                                            csErrorMessage.Format(_T("The length of subitem %s exceeds the length of its parent item"),(LPCTSTR)name);
                                                        }
                                                    }
                                                }
                                            }

                                            else // second pass, adjust the lengths
                                            {
                                                if( pItem->GetLen() != iItemLength )
                                                {
                                                    int startAdjustmentFactor = ( iItemLength - pItem->GetLen() ) * pItem->GetOccurs();

                                                    pItem->SetLen(iItemLength);

                                                    if( pItem->GetItemType() == ItemType::Subitem )
                                                    {
                                                        // this will be a subitem that doesn't overlap other subitems
                                                        for( int subitem = item + 1; subitem < pRecord->GetNumItems(); subitem++ )
                                                        {
                                                            CDictItem* pSubitem = pRecord->GetItem(subitem);

                                                            if( pSubitem->GetItemType() != ItemType::Subitem )
                                                                break;

                                                            pSubitem->SetStart(pSubitem->GetStart() + startAdjustmentFactor);
                                                        }
                                                    }

                                                    // if we're on the ID record, we have to adjust all the records, and maybe the record type
                                                    else if( record == -1 )
                                                    {
                                                        if( m_pDict->GetRecTypeStart() > pItem->GetStart() )
                                                            m_pDict->SetRecTypeStart(m_pDict->GetRecTypeStart() + startAdjustmentFactor);

                                                        for( DictLevel& adj_dict_level : m_pDict->GetLevels() )
                                                        {
                                                            for( int adjRecord = -1; adjRecord < adj_dict_level.GetNumRecords(); adjRecord++ )
                                                            {
                                                                AdjustItemStartPositions(adjRecord == -1 ? adj_dict_level.GetIdItemsRec() : adj_dict_level.GetRecord(adjRecord),
                                                                    pItem,startAdjustmentFactor);
                                                            }
                                                        }
                                                    }

                                                    else
                                                    {
                                                        AdjustItemStartPositions(pRecord,pItem,startAdjustmentFactor);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if( firstPass )
        {
            firstPass = false;

            for( int i2 = 0; i2 < newNames.GetSize(); i2++ )
            {
                if( newNames[i2].IsEmpty() )
                {
                    AfxMessageBox(_T("Error: Clipboard contents contain a name that is empty"));
                    return;
                }

                // see if there are any any errors in the name
                bool invalidName = false;

                TCHAR firstChar = newNames[i2].GetAt(0);

                if( ( firstChar < _T('A') || firstChar > 'Z' ) && newNames[i2].Find(_T("_IDS")) < 0 ) // IDs has the _ at the beginning exception
                {
                    invalidName = true;
                }

                else
                {
                    for( int j = 1; j < newNames[i2].GetLength() && !invalidName; j++ )
                    {
                        TCHAR thisChar = newNames[i2].GetAt(j);

                        if( !( thisChar >= _T('A') && thisChar <= 'Z' ) && !( thisChar >= _T('0') && thisChar <= '9' ) && thisChar != '_' )
                            invalidName = true;
                    }
                }

                if( invalidName )
                {
                    text.Format(_T("Error: Clipboard contents contain an invalid name: %s"), (LPCTSTR)newNames[i2]);
                    AfxMessageBox(text);
                    return;
                }


                // see if any of the desired names is a duplicate
                for( int j = i2 + 1; j < newNames.GetSize(); j++ )
                {
                    if( newNames[i2] == newNames[j] )
                    {
                        text.Format(_T("Error: Clipboard contents contain multiple entries with the same name: %s"), (LPCTSTR)newNames[i2]);
                        AfxMessageBox(text);
                        return;
                    }
                }
            }
        }
    }

    if( successfulRead )
    {
        m_pDictDoc->SetModified(true);
        AfxMessageBox(_T("Names and labels pasted from the clipboard"));
    }

    else
    {
        AfxMessageBox(csErrorMessage);
    }
}


void CDictionaryMacros::OnBnClickedCopyValueSets()
{
    CString text;
    int num_value_sets = 0;
    bool copy_all_languages = ((CButton*)GetDlgItem(IDC_VS_ALL_LANGUAGES))->GetCheck();
    bool copy_value_set_images = ((CButton*)GetDlgItem(IDC_VS_IMAGES))->GetCheck();

    CString label_indentation('\t', copy_all_languages ? m_pDict->GetLanguages().size() : 1);

    DictionaryIterator::Foreach<DictValueSet>(*m_pDict,
        [&](const DictValueSet& dict_value_set)
        {
            ++num_value_sets;

            if( num_value_sets > 1 )
                text.Append(_T("\r\n\r\n"));

            text.AppendFormat(_T("%s\t%s"),
                                (LPCTSTR)dict_value_set.GetName(),
                                (LPCTSTR)GetLabelWithLanguages(dict_value_set.GetLabelSet(), copy_all_languages));

            for( const DictValue& dict_value : dict_value_set.GetValues() )
            {
                bool first_pair = true;

                for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
                {
                    text.AppendFormat(_T("\r\n\t%s%s\t%s\t%s\t%s"),
                                        (LPCTSTR)label_indentation,
                                        first_pair ? (LPCTSTR)GetLabelWithLanguages(dict_value.GetLabelSet(), copy_all_languages) : (LPCTSTR)label_indentation.Left(label_indentation.GetLength() - 1),
                                        (LPCTSTR)dict_value_pair.GetFrom(),
                                        (LPCTSTR)dict_value_pair.GetTo(),
                                        dict_value.IsSpecial() ? SpecialValues::ValueToString(dict_value.GetSpecialValue(), false) : _T(""));

                    if( copy_value_set_images && first_pair )
                        text.AppendFormat(_T("\t%s"), (LPCTSTR)dict_value.GetImageFilename());

                    first_pair = false;
                }
            }
        });

    WinClipboard::PutText(this, text);
    AfxMessageBox(FormatText(_T("%d value sets copied to the clipboard"), num_value_sets));
}


CString CDictionaryMacros::makeValueValid(CString value,CDictItem* pItem,int & numValsModified) // 20101113
{
    if( pItem->GetContentType() == ContentType::Alpha )
    {
        if( value.GetLength() > pItem->GetLen() )
        {
            numValsModified++;
            return value.Left(pItem->GetLen());
        }

        else if( value.GetLength() < pItem->GetLen() )
        {
            return value + CString(_T(' '),pItem->GetLen() - value.GetLength());
        }

        else
        {
            return value;
        }
    }

    // if there is an invalid value, we'll just set the value to 0
    double dVal = _tstof(value);

    if( dVal == 0 ) // check if the input was actually 0 or if atof couldn't translate the value
    {
        bool notZero = false;
        bool oneDecimal = false;

        for( int i = 0; i < value.GetLength(); i++ )
        {
            if( value[i] == _T('.') && !oneDecimal )
                oneDecimal = true;

            else if( value[i] == '0' )
                ; // fine

            else
                notZero = true;
        }

        if( notZero )
            numValsModified++;

        return _T("0");
    }

    else
    {
        if( pItem->GetDecimal() == 0 ) // no decimal part
        {
            if( value.Find('.') >= 0 ) // the pasted value has a decimal point
                numValsModified++;

            else if( value.Trim().GetLength() > pItem->GetLen() )
                numValsModified++;

            double maxSize = pow((double)10,(int)pItem->GetLen()) - 1;

            while( dVal > maxSize )
                dVal /= 10;

            value.Format(_T("%.0Lf"),dVal);

            return value;
        }

        else // the value has a decimal part
        {
            int decLen = pItem->GetDecimal();
            int intLen = pItem->GetLen() - decLen - pItem->GetDecChar();

            CString decimalFormatted,formatStyle;
            formatStyle.Format(_T("%%.%dLf"),decLen);

            decimalFormatted.Format(formatStyle,dVal);

            double formattedVal = _tstof(decimalFormatted);

            double maxSize = pow((double)10,intLen) - 1;

            while( formattedVal > maxSize )
                formattedVal /= 10;

            if( formattedVal != dVal )
                numValsModified++;

            value.Format(formatStyle,formattedVal);

            return value;
        }
    }
}


void CDictionaryMacros::OnBnClickedPasteValueSets()
{
    CString text = WS2CS(WinClipboard::GetText(this));
    text.Append(_T("\r\n\r\n\r\n\r\n\r\n")); // this should ensure that none of my text[textPtr++] codes will cause an out of bounds error

    // we first need to check that the text on the clipboard constitutes one or more valid value sets

    bool successfulRead = true;
    int numValueSetsCopied = 0;
    int numValuesModified = 0;
    bool secondPass = false;

    std::vector<DictValueSet*> linked_value_sets_modified;

    bool paste_all_languages = ((CButton*)GetDlgItem(IDC_VS_ALL_LANGUAGES))->GetCheck();
    int iExpectedLabelIndentationTabs = paste_all_languages ? m_pDict->GetLanguages().size() : 1;

    bool paste_value_set_images = ((CButton*)GetDlgItem(IDC_VS_IMAGES))->GetCheck();


    for( int i = 0; i < 2 && successfulRead; i++ ) // first pass is the above checks; second pass actually changes the values
    {
        int textPtr = 0;
        CString name,from,to,special,image;
        CStringArray csaLabels;

        bool moreEntries = true;

        while( moreEntries && successfulRead )
        {
            textPtr = readEntry(text,textPtr,name);

            if( name.IsEmpty() )
            {
                moreEntries = false;
            }

            else
            {
                successfulRead = text[textPtr++] == _T('\t');

                if( successfulRead )
                    textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);

                if( successfulRead )
                {
                    while( text[textPtr] == '\t' ) // there will be extra tabs if the data is copied from Excel
                        textPtr++;

                    if( text[textPtr] == '\r' )
                        textPtr++;

                    bool blankLine = false;

                    successfulRead = text[textPtr++] == _T('\n');

                    CDictItem* dict_item = nullptr;
                    DictValueSet* dict_value_set = nullptr;

                    if( secondPass ) // see if this value set exists in the current dictionary
                    {
                        if( m_pDict->LookupName<DictValueSet>(name, nullptr, nullptr, &dict_item, &dict_value_set) )
                        {
                            if( dict_value_set->IsLinkedValueSet() )
                                linked_value_sets_modified.emplace_back(dict_value_set);

                            dict_value_set->RemoveAllValues();
                            numValueSetsCopied++;
                            m_pDictDoc->SetModified(true);

                            SetLabelWithLanguages(dict_value_set->GetLabelSet(), csaLabels, paste_all_languages);
                        }
                    }


                    while( successfulRead && ( text[textPtr] == '\t' ) && !blankLine ) // keep reading in new values
                    {
                        for( int iTab = 0; successfulRead && iTab < ( 1 + iExpectedLabelIndentationTabs ); iTab++ )
                            successfulRead = text[textPtr++] == _T('\t');

                        if( successfulRead )
                        {
                            bool bNoLabels = true;

                            for( int iTab = 0; bNoLabels && iTab < iExpectedLabelIndentationTabs; iTab++ )
                                bNoLabels = text[textPtr + iTab] == _T('\t');

                            if( bNoLabels )
                            {
                                textPtr += ( iExpectedLabelIndentationTabs - 1 );
                                csaLabels.RemoveAll();
                            }

                            else
                            {
                                textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);
                            }

                            if( successfulRead )
                                successfulRead = text[textPtr++] == _T('\t');

                            if( successfulRead )
                            {
                                bool restOfLineIsBlank = false;
                                to = _T("");
                                special = _T("");
                                image = _T("");

                                // reading the from value
                                textPtr = readEntry(text,textPtr,from);

                                if( text[textPtr] == _T('\r') || text[textPtr] == '\n' )
                                    restOfLineIsBlank = true;

                                else
                                    successfulRead = text[textPtr++] == _T('\t');

                                // reading the to value
                                if( successfulRead && !restOfLineIsBlank )
                                {
                                    textPtr = readEntry(text,textPtr,to);

                                    if( text[textPtr] == _T('\r') || text[textPtr] == '\n' )
                                        restOfLineIsBlank = true;

                                    else
                                        successfulRead = text[textPtr++] == _T('\t');
                                }

                                // reading the special value
                                if( successfulRead && !restOfLineIsBlank )
                                {
                                    textPtr = readEntry(text,textPtr,special);

                                    if( text[textPtr] == _T('\r') || text[textPtr] == '\n' )
                                        restOfLineIsBlank = true;

                                    else
                                        successfulRead = text[textPtr++] == _T('\t');
                                }

                                // reading the value set image
                                if( successfulRead && !restOfLineIsBlank && paste_value_set_images )
                                {
                                    textPtr = readEntry(text,textPtr,image);

                                    if( text[textPtr] == _T('\r') || text[textPtr] == '\n' )
                                        restOfLineIsBlank = true;

                                    else
                                        successfulRead = text[textPtr++] == _T('\t');
                                }

                                if( successfulRead )
                                {
                                    while( text[textPtr] == '\t' ) // there could be extra tabs if the data is copied from Excel
                                        textPtr++;

                                    if( text[textPtr] == '\r' )
                                        textPtr++;

                                    successfulRead = text[textPtr++] == _T('\n');

                                    blankLine = bNoLabels && from.IsEmpty() && to.IsEmpty() && special.IsEmpty();

                                    if( successfulRead && !blankLine ) // process the data
                                    {
                                        if( dict_item != nullptr && dict_item->GetContentType() == ContentType::Alpha )
                                        {
                                            to = _T("");
                                            special = _T("");
                                        }

                                        std::optional<double> special_value;

                                        if( !special.IsEmpty() )
                                        {
                                            special_value = SpecialValues::StringIsSpecial<std::optional<double>>(special);

                                            if( !special_value.has_value() )
                                            {
                                                if( special.CompareNoCase(_T("Not Applicable")) == 0 ) // pre-8.0
                                                {
                                                    special_value = NOTAPPL;
                                                }

                                                else
                                                {
                                                    successfulRead = false;
                                                    name.Format(_T("Special value '%s' in an invalid entry"),(LPCTSTR)special);
                                                    AfxMessageBox(name);
                                                    return;
                                                }
                                            }
                                        }

                                        if( successfulRead && dict_value_set != nullptr )
                                        {
                                            if( !dict_value_set->HasValues() || !bNoLabels ) // if it's empty this is a second, third, etc. value of the previous label
                                            {
                                                DictValue newValue;

                                                if( bNoLabels )
                                                    newValue.GetLabelSet().SetLabels(LabelSet());

                                                else
                                                    SetLabelWithLanguages(newValue.GetLabelSet(), csaLabels, paste_all_languages);

                                                newValue.SetImageFilename(image);
                                                newValue.SetSpecialValue(special_value);

                                                dict_value_set->AddValue(std::move(newValue));
                                            }

                                            DictValuePair dict_value_pair;

                                            from.Trim(); // 20110901 fixes a problem when pasting in special values with no from

                                            if( !special_value.has_value() || !from.IsEmpty() )
                                                dict_value_pair.SetFrom(makeValueValid(from, dict_item, numValuesModified));

                                            if( !to.IsEmpty() )
                                                dict_value_pair.SetTo(makeValueValid(to, dict_item, numValuesModified));

                                            dict_value_set->GetValues().back().AddValuePair(std::move(dict_value_pair));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                while( text[textPtr] == _T('\r') || text[textPtr] == '\n' ) // read until the next value set (or the end of the pasted text)
                    textPtr++;

                if( textPtr == text.GetLength() )
                    moreEntries = false;
            }
        }

        secondPass = true;
    }


    if( successfulRead )
    {
        // sync any modified linked value sets
        for( DictValueSet* linked_value_set : linked_value_sets_modified )
            m_pDict->SyncLinkedValueSets(linked_value_set);

        CString message = FormatText(_T("%d value set%s pasted from the clipboard"), numValueSetsCopied, PluralizeWord(numValueSetsCopied));

        if( numValuesModified )
            message.AppendFormat(_T(" though %d invalid value%s modified"), numValuesModified, numValuesModified == 1 ? _T(" was") : _T("s were"));

        AfxMessageBox(message);
    }

    else
    {
        AfxMessageBox(_T("Error: Clipboard contents do not match value sets format"));
    }
}


void CDictionaryMacros::OnBnClickedGenerateDataFile() // 20101114 this now currently only works for one-level applications
{
    // check the parameters
    CString strNumCases;
    GetDlgItem(IDC_NUMBER_CASES)->GetWindowText(strNumCases);

    if( strNumCases.IsEmpty() )
    {
        AfxMessageBox(_T("Specify the number of cases desired"));
        return;
    }

    int numCases = _ttoi(strNumCases);

    if( numCases <= 0 )
    {
        AfxMessageBox(_T("You must select a positive number of cases"));
        return;
    }

    CString strNotapplPercent;
    GetDlgItem(IDC_NOTAPPL_PERCENT)->GetWindowText(strNotapplPercent);
    notapplPercent = _ttoi(strNotapplPercent);

    CString strInvalidPercent;
    GetDlgItem(IDC_INVALID_PERCENT)->GetWindowText(strInvalidPercent);
    invalidPercent = _ttoi(strInvalidPercent);

    if( ( notapplPercent < 0 || notapplPercent > 100 ) || ( notapplPercent == 0 && !strNotapplPercent.IsEmpty() && strNotapplPercent != _T("0") )  )
    {
        AfxMessageBox(_T("Enter a valid number for the percent of not applicable values"));
        return;
    }

    if( invalidPercent < 0 || invalidPercent > 100 || ( invalidPercent == 0 && !strInvalidPercent.IsEmpty() && strInvalidPercent != _T("0") )  )
    {
        AfxMessageBox(_T("Enter a valid number for the percent of invalid values"));
        return;
    }

    regularPercent = 100 - notapplPercent - invalidPercent;

    if( regularPercent <= 0 )
    {
        AfxMessageBox(_T("The percentages of not applicable and invalid values exceed or equal 100%"));
        return;
    }

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false);
    data_file_dlg.SetDictionaryFilename(m_pDict->GetFullFileName());

    if( data_file_dlg.DoModal() != IDOK )
        return;

    CString post_operation_message;

    try
    {
        const TCHAR* ProgressDlgMessage = _T("Generating Data");
        ThreadedProgressDlg progress_dlg;
        progress_dlg.SetTitle(ProgressDlgMessage);
        progress_dlg.SetStatus(ProgressDlgMessage);
        progress_dlg.Show();

        const std::shared_ptr<CaseAccess> case_access = CreateCaseAccess();
        const std::unique_ptr<Case> data_case = case_access->CreateCase();

        // open the repository
        std::unique_ptr<DataRepository> output_repository = DataRepository::CreateAndOpen(case_access,
            data_file_dlg.GetConnectionString(), DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew);

        // seed the random number generator
        srand((unsigned int)time(NULL));

        alphaValueSetValueCounts.clear();

        CaseLevel& root_case_level = data_case->GetRootCaseLevel();
        CaseRecord& id_record = root_case_level.GetIdCaseRecord();

        std::set<CString> previous_keys;
        int numWritten = 0;

        for( int i = 0; i < numCases; ++i )
        {
            data_case->Reset();

            // first generate the ID
            bool successful_id = false;
            size_t attempts = 0;
            const size_t MaxNumIDAttempts = 2000;
            CaseItemIndex id_index = id_record.GetCaseItemIndex();

            for( ; !successful_id && attempts < MaxNumIDAttempts; ++attempts )
            {
                for( const CaseItem* case_item : id_record.GetCaseItems() )
                    AddRandomValue(*case_item, id_index);

                if( previous_keys.find(data_case->GetKey()) == previous_keys.end() )
                {
                    previous_keys.insert(data_case->GetKey());
                    successful_id = true;
                }
            }

            if( !successful_id )
                break; // no longer attempt to create new cases

            bool at_least_one_record_generated = false;

            while( !at_least_one_record_generated )
            {
                for( size_t record_number = 0; record_number < root_case_level.GetNumberCaseRecords(); ++record_number )
                {
                    CaseRecord& case_record = root_case_level.GetCaseRecord(record_number);
                    const CDictRecord& dictionary_record = case_record.GetCaseRecordMetadata().GetDictionaryRecord();

                    size_t number_records_to_write = dictionary_record.GetMaxRecs();

                    // for singly occurring records that are not required, write them out only 75% of the time (an arbitrary value)
                    if( number_records_to_write == 1 )
                    {
                        if( !dictionary_record.GetRequired() && ( rand() % 4 ) == 0 )
                            number_records_to_write = 0;
                    }

                    // some dictionaries will have a huge number of possible records specified (just in case), so we'll perform
                    // a routine so that most of the time we're not outputting a massive number of records
                    else
                    {
                        // 75% of the time (when more than 10 records are specified) we'll output a much smaller number
                        if( ( rand() % 4 ) != 0 && number_records_to_write > 10 )
                            number_records_to_write = (size_t)sqrt((double)number_records_to_write) + ( rand() % 3 );

                        number_records_to_write = rand() % ( number_records_to_write + 1 );
                    }

                    if( dictionary_record.GetRequired() && number_records_to_write == 0 )
                        number_records_to_write = 1;

                    case_record.SetNumberOccurrences(number_records_to_write);

                    for( size_t k = 0; k < number_records_to_write; ++k )
                    {
                        CaseItemIndex index = case_record.GetCaseItemIndex(k);

                        for( const CaseItem* case_item : case_record.GetCaseItems() )
                        {
                            for( index.SetItemSubitemOccurrence(*case_item, 0); index.GetItemSubitemOccurrence(*case_item) < case_item->GetTotalNumberItemSubitemOccurrences(); index.IncrementItemSubitemOccurrence(*case_item) )
                                AddRandomValue(*case_item, index);
                        }

                        at_least_one_record_generated = true;
                    }
                }
            }

            output_repository->WriteCase(*data_case);

            ++numWritten;

            // update the progress bar every 10 cases
            if( numWritten % 10 == 0 )
                progress_dlg.SetPos((int)( 100.0 * numWritten / numCases ));

            if( progress_dlg.IsCanceled() )
                throw std::exception();
        }

        output_repository->Close();

        if( numWritten != numCases )
            post_operation_message.Format(_T("Could only write out %d case%s due to ID size limitations"), numWritten, PluralizeWord(numCases));

        else
            post_operation_message.Format(_T("Successfully wrote out %d case%s"), numCases, PluralizeWord(numCases));
    }

    catch( const DataRepositoryException::Error& exception )
    {
        post_operation_message = WS2CS(exception.GetErrorMessage());
    }

    catch(...)
    {
        post_operation_message = _T("Operation canceled");
    }

    AfxMessageBox(post_operation_message);
}


void CDictionaryMacros::AddRandomValue(const CaseItem& case_item, CaseItemIndex& index)
{
    // subitems will overwrite their parent item
    int randomNum = rand() % 100;

    const int VALUE_RANDOM = 0;
    const int VALUE_INVALID = 1;

    int type = VALUE_RANDOM;

    if( randomNum < invalidPercent )
        type = VALUE_INVALID;

    else if( randomNum < ( invalidPercent + notapplPercent ) )
        return; // the value is notappl so all we have to do is return without adding


    // binary case items will be handled separately
    if( case_item.IsTypeBinary() )
    {
        AddRandomBinaryValue(case_item, index);
        return;
    }


    // for numeric and string case items...
    const CDictItem& dict_item = case_item.GetDictionaryItem();

    double numeric_value = 0;
    CString string_value;

    // first handle the easiest case, that for values without a value set
    if( !dict_item.HasValueSets() || !dict_item.GetValueSet(0).HasValues() )
    {
        if( IsNumeric(dict_item) )
            numeric_value = GenerateRandomNumeric(dict_item);

        else if( IsString(dict_item) )
            string_value = GenerateRandomAlpha(dict_item);

        // unknown content type
        else
            ASSERT(false);
    }

    // we have to create a value in the value set, or an invalid value not in the value set
    else 
    {
        const DictValueSet& dict_value_set = dict_item.GetValueSet(0);

        const int InvalidAttemptMax = 1000;

        int values = CountAlphaValues(dict_value_set);

        // 20101201 if values is less then 0, then it means that it was too difficult to find an
        // invalid value within the data set (which can occur, for instance, if the values in the value set
        // occupy all possible values, like 0-9 for a one digit number); if that's the case, just assign
        // a valid value
        if( values < 0 )
        {
            values *= -1;
            type = VALUE_RANDOM;
        }


        // numeric values
        if( IsNumeric(dict_item) )
        {
            if( type == VALUE_INVALID ) // generate a random value and see if it's in the value set
            {
                bool invalidValueFound = false;

                for( int i = 0; !invalidValueFound && i < InvalidAttemptMax; i++ )
                {
                    numeric_value = GenerateRandomNumeric(dict_item);

                    invalidValueFound = true; // assume it's not in the value set

                    for( size_t j = 0; invalidValueFound && j < dict_value_set.GetNumValues(); j++ )
                    {
                        const DictValue& dict_value = dict_value_set.GetValue(j);

                        for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
                        {
                            double dFrom = _tstof(dict_value_pair.GetFrom());

                            if( dict_value_pair.GetTo().IsEmpty() )
                            {
                                if( dFrom == numeric_value )
                                {
                                    invalidValueFound = false;
                                    break;
                                }
                            }

                            else
                            {
                                if( numeric_value >= dFrom && numeric_value <= _tstof(dict_value_pair.GetTo()) )
                                {
                                    invalidValueFound = false;
                                    break;
                                }
                            }
                        }
                    }
                }

                // we couldn't successfully find an invalid value so mark this value set as not having an invalid
                // value so we don't pointlessly search each time through the loop
                if( !invalidValueFound )
                    alphaValueSetValueCounts[&dict_value_set] = -1 * values;
            }

            else // get a value from the value set
            {
                // first count the number of of values in the value set
                // ideally we would have a different function, countNumberValues, that would optimize
                // ranges, but that seems like too much work for a debugging tool

                // the problem with this as is is if you have two values, 1, 5-10, then
                // half the resulting values will be 1, instead of 1/7 of the values being 1
                //int values = countAlphaValues(dict_value_set);

                int desiredValue = rand() % values;

                values = 0;

                for( const DictValue& dict_value : dict_value_set.GetValues() )
                {
                    if( desiredValue >= ( values + dict_value.GetNumValuePairs() ) )
                    {
                        values += dict_value.GetNumValuePairs();
                    }

                    else
                    {
                        const DictValuePair& dict_value_pair = dict_value.GetValuePair(desiredValue - values);

                        if( dict_value_pair.GetTo().IsEmpty() )
                        {
                            numeric_value = _tstof(dict_value_pair.GetFrom());
                        }

                        else // if the value here is a range then we need to select a value from within the range
                        {
                            double dFrom = _tstof(dict_value_pair.GetFrom());
                            double dTo = _tstof(dict_value_pair.GetTo());
                            numeric_value = dFrom + ( dTo - dFrom ) * ( rand() / (double)RAND_MAX );

                            // format the number to match the specified decimals
                            double decimal_multiplier = std::pow(10, dict_item.GetDecimal());

                            double integer;
                            double fraction = std::modf(numeric_value, &integer);

                            double fraction_as_integer = std::round(fraction * decimal_multiplier);

                            numeric_value = integer + ( fraction_as_integer / decimal_multiplier );
                        }

                        break;
                    }
                }
            }
        }


        // alpha values
        else if( IsString(dict_item) )
        {
            if( type == VALUE_INVALID ) // generate a random value and see if it's in the value set
            {
                bool invalidValueFound = false;

                for( int i = 0; !invalidValueFound && i < InvalidAttemptMax; i++ )
                {
                    string_value = GenerateRandomAlpha(dict_item);

                    invalidValueFound = true; // assume it's not in the value set

                    for( const DictValue& dict_value : dict_value_set.GetValues() )
                    {
                        for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
                        {
                            if( dict_value_pair.GetFrom() == string_value )
                            {
                                invalidValueFound = false;
                                break;
                            }
                        }

                        if( !invalidValueFound )
                            break;
                    }
                }

                if( !invalidValueFound )
                    alphaValueSetValueCounts[&dict_value_set] = -1 * values;
            }

            else // get a value from the value set
            {
                // first count the number of of values in the value set
                //int values = countAlphaValues(dict_value_set);

                int desiredValue = rand() % values;

                values = 0;

                for( const DictValue& dict_value : dict_value_set.GetValues() )
                {
                    if( desiredValue >= ( values + dict_value.GetNumValuePairs() ) )
                    {
                        values += dict_value.GetNumValuePairs();
                    }

                    else
                    {
                        string_value = dict_value.GetValuePair(desiredValue - values).GetFrom();
                        break;
                    }
                }
            }
        }


        // unknown content type
        else
        {
            ASSERT(false);
        }
    }


    // set the value
    if( dict_item.GetContentType() == ContentType::Numeric )
    {
        assert_cast<const NumericCaseItem&>(case_item).SetValue(index, numeric_value);
    }

    else if( dict_item.GetContentType() == ContentType::Alpha )
    {
        assert_cast<const StringCaseItem&>(case_item).SetValue(index, string_value);
    }

    else
    {
        ASSERT(false);
    }
}


double CDictionaryMacros::GenerateRandomNumeric(const CDictItem& dict_item)
{
    double value = 0;

    UINT remaining_digits = dict_item.GetLen();
    bool make_negative = false;

    if( dict_item.GetDecChar() && dict_item.GetDecimal() > 0 )
        --remaining_digits;

    // 10% of the time make the value negative
    if( remaining_digits > 1 && rand() % 10 == 0 )
    {
        make_negative = true;
        --remaining_digits;
    }

    for( ; remaining_digits > 0; --remaining_digits )
        value = value * 10 + rand() % 10;

    if( make_negative )
        value /= -1;

    if( dict_item.GetDecimal() > 0 )
        value /= std::pow(10, dict_item.GetDecimal());

    return value;
}


CString CDictionaryMacros::GenerateRandomAlpha(const CDictItem& dict_item)
{
    CString value;

    // we'll create a string of random length
    int string_length = rand() % ( dict_item.GetLen() + 1 );

    TCHAR* buffer = value.GetBufferSetLength(string_length);

    // random characters will alphanumeric, with commas and frequent spaces
    for( int i = 0; i < string_length; ++i )
    {
        TCHAR randChar = rand() % 70;
        TCHAR output_ch = _T(' ');

        if( randChar < 26 )
            output_ch = _T('a') + randChar;

        else if( randChar < 52 )
            output_ch = _T('A') + randChar - 26;

        else if( randChar < 62 )
            output_ch = _T('0') + randChar - 52;

        else if( randChar == 63 )
            output_ch = '.';

        buffer[i] = output_ch;
    }

    value.ReleaseBuffer();

    return CIMSAString::MakeExactLength(value, dict_item.GetLen());
}


int CDictionaryMacros::CountAlphaValues(const DictValueSet& dict_value_set) // 20101114
{
    const auto& lookup = alphaValueSetValueCounts.find(&dict_value_set);

    if( lookup != alphaValueSetValueCounts.cend() )
        return lookup->second;

    size_t values = 0;

    for( const DictValue& dict_value : dict_value_set.GetValues() )
        values += dict_value.GetNumValuePairs();

    // saves the values so that we're not constantly recounting the number of values
    alphaValueSetValueCounts.try_emplace(&dict_value_set, (int)values);

    return values;
}


void CDictionaryMacros::AddRandomBinaryValue(const CaseItem& case_item, CaseItemIndex& index)
{
    ASSERT(case_item.IsTypeBinary());
    const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(case_item);
    const CDictItem& dict_item = case_item.GetDictionaryItem();

    if( dict_item.GetContentType() == ContentType::Audio )
    {
        // no routine exists for the Audio type
        return;
    }

    // for the Document type, create a UTF-8 text file (with BOM) with a simple message
    else if( dict_item.GetContentType() == ContentType::Document )
    {
        const std::wstring message = FormatTextCS2WS(_T("Document for %s created using Dictionary Macros.\n"), dict_item.GetName().GetString());
        std::vector<std::byte> utf8_message = UTF8Convert::WideToUTF8Buffer(message);

        utf8_message.insert(utf8_message.begin(), reinterpret_cast<const std::byte*>(Utf8BOM_sv.data()),
                                                  reinterpret_cast<const std::byte*>(Utf8BOM_sv.data()) + Utf8BOM_sv.length());        

        BinaryDataMetadata metadata;
        metadata.SetFilename(_T("Dictionary Macros Document.txt"));
        binary_case_item.GetBinaryDataAccessor(index).SetBinaryData(std::move(utf8_message), std::move(metadata));
    }

    // for the Geometry type, use a GeoJSON file with the Census Bureau's coordinates
    else if( dict_item.GetContentType() == ContentType::Geometry )
    {
        constexpr std::string_view CensusBureauGeoJson_sv = R"!({"type":"Feature","geometry":{"type":"Point","coordinates":[-76.931098,38.84839]},"properties":{"name":"United States Census Bureau"}})!";
        std::vector<std::byte> content(reinterpret_cast<const std::byte*>(CensusBureauGeoJson_sv.data()),
                                       reinterpret_cast<const std::byte*>(CensusBureauGeoJson_sv.data()) + CensusBureauGeoJson_sv.length());

        BinaryDataMetadata metadata;
        metadata.SetFilename(_T("U.S. Census Bureau.geojson"));
        binary_case_item.GetBinaryDataAccessor(index).SetBinaryData(std::move(content), std::move(metadata));
    }

    // for the Image type, create a PNG of the CSPro logo
    else if( dict_item.GetContentType() == ContentType::Image )
    {
        std::shared_ptr<const std::vector<std::byte>> content = SystemIcon::GetPngForCSProLogo();

        if( content == nullptr )
            return;

        BinaryDataMetadata metadata;
        metadata.SetFilename(_T("CSPro Logo.png"));
        binary_case_item.GetBinaryDataAccessor(index).SetBinaryData(std::move(content), std::move(metadata));
    }

    else
    {
        throw ProgrammingErrorException();
    }
}


int GetRecordEndPos(CDictRecord* pRecord) // 20140308
{
    int pos = 0;

    for( int i = 0; i < pRecord->GetNumItems(); i++ )
    {
        CDictItem* pItem = pRecord->GetItem(i);
        pos = std::max(pos,(int) (pItem->GetStart() + pItem->GetLen()));
    }

    return pos;
}

void CDictionaryMacros::OnBnClickedAddItemsToRecord() // 20140308
{
    CString csNumItems;
    GetDlgItem(IDC_NUM_ITEMS)->GetWindowText(csNumItems);

    if( csNumItems.IsEmpty() )
    {
        AfxMessageBox(_T("Specify the number of items to add"));
        return;
    }

    int iNumItems = _ttoi(csNumItems);

    if( iNumItems < 1 || iNumItems > 500 )
    {
        AfxMessageBox(_T("Enter a valid number of items (1 - 500) to add"));
        return;
    }

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_RECORD_LISTING);
    int iSelection = pComboBox->GetCurSel();

    if( iSelection < 0 )
    {
        AfxMessageBox(_T("Select the record to which you want to add the items"));
        return;
    }

    CDictRecord* pRecord = (CDictRecord*)pComboBox->GetItemDataPtr(iSelection);
    CDictRecord* pIDRecord = NULL;

    // we will eventually need the level and record numbers
    size_t level_number = 0;
    int iRecord = 0;
    bool bFound = false;

    for( ; level_number < m_pDict->GetNumLevels(); ++level_number )
    {
        DictLevel& dict_level = m_pDict->GetLevel(level_number);

        for( iRecord = 0; iRecord < dict_level.GetNumRecords(); iRecord++ )
        {
            if( dict_level.GetRecord(iRecord) == pRecord )
            {
                pIDRecord = dict_level.GetIdItemsRec();
                bFound = true;
                break;
            }
        }

        if( bFound )
            break;
    }

    bool bZeroFill = m_pDict->IsZeroFill();

    int iStartingPos = std::max(GetRecordEndPos(pIDRecord),GetRecordEndPos(pRecord));
    iStartingPos = std::max(iStartingPos,(int) (m_pDict->GetRecTypeStart() + m_pDict->GetRecTypeLen()));

    for( int i = 0; i < iNumItems; i++ )
    {
        CDictItem* pItem = new CDictItem();
        pItem->GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
        pItem->SetRecord(pRecord);
        pItem->SetZeroFill(bZeroFill);
        pItem->SetStart(iStartingPos++);

        CString csTemp;
        csTemp.Format(_T("%s (Item %d)"),(LPCTSTR)pRecord->GetLabel(),pRecord->GetNumItems() + 1);
        pItem->SetLabel(csTemp);

        csTemp.Format(_T("%s_ITEM%03d"),(LPCTSTR)pRecord->GetName(),pRecord->GetNumItems() + 1);
        csTemp = m_pDictDoc->GetDict()->GetUniqueName(csTemp);
        pItem->SetName(csTemp);

        m_pDict->AddToNameList(*pItem,iRecord, level_number, pRecord->GetNumItems() - 1,-1);
        pRecord->AddItem(pItem);
    }

    m_pDictDoc->SetModified(true);

    CString csMessage;
    csMessage.Format(_T("%d items have been added to %s"),iNumItems,(LPCTSTR)pRecord->GetName());
    AfxMessageBox(csMessage);
}


namespace
{
    CString GetTempDataFileName(CString csFilename)
    {
        CString csExtension = PortableFunctions::PathGetFileExtension<CString>(csFilename);
        CString csBaseFilename = PortableFunctions::PathRemoveFileExtension<CString>(csFilename);
        CString csTempFileName;

        do
        {
            csBaseFilename.AppendFormat(_T(".tmp"));
            csTempFileName.Format(_T("%s%s%s"), (LPCTSTR)csBaseFilename, csExtension.IsEmpty() ? _T("") : _T("."), (LPCTSTR)csExtension);

        } while( PortableFunctions::FileExists(csTempFileName) );

        return csTempFileName;
    }

    ConnectionString GetTempDataFileConnectionString(const ConnectionString& connection_string)
    {
        return ConnectionString(connection_string.ToString(CS2WS(GetTempDataFileName(WS2CS(connection_string.GetFilename())))));
    }
}


std::unique_ptr<CaseAccess> CDictionaryMacros::CreateCaseAccess()
{
    m_pDict->UpdatePointers();

    return CaseAccess::CreateAndInitializeFullCaseAccess(*m_pDict);
}


struct CaseIteratorRoutine
{
    const std::vector<ConnectionString> input_connection_strings;
    const DataRepositoryAccess input_access_type;
    const ConnectionString  output_connection_string;
    const bool rename_output_to_input_on_success;
    const std::function<bool()>* should_write_case_callback;
};


void CDictionaryMacros::RunCaseIteratorRoutine(const CaseIteratorRoutine& case_iterator_routine, const TCHAR* action_verb_base)
{
    const size_t ProgressUpdateFrequency = 100;

    CString post_operation_message;
    bool success = false;

    try
    {
        CString progress_title_and_status;
        progress_title_and_status.Format(_T("%sing..."), action_verb_base);

        ThreadedProgressDlg progress_dlg;
        progress_dlg.SetTitle(progress_title_and_status);
        progress_dlg.SetStatus(progress_title_and_status);
        progress_dlg.Show();

        const std::shared_ptr<CaseAccess> case_access = CreateCaseAccess();
        const std::unique_ptr<Case> data_case = case_access->CreateCase();

        size_t cases_written = 0;
        size_t cases_until_progress_update = ProgressUpdateFrequency;

        // open the repositories
        std::unique_ptr<DataRepository> output_repository = DataRepository::CreateAndOpen(case_access,
            case_iterator_routine.output_connection_string, DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew);

        for( const ConnectionString& input_connection_string : case_iterator_routine.input_connection_strings )
        {
            std::unique_ptr<DataRepository> input_repository = DataRepository::CreateAndOpen(case_access,
                input_connection_string, case_iterator_routine.input_access_type, DataRepositoryOpenFlag::OpenMustExist);

            // start the iterator
            std::unique_ptr<CaseIterator> case_iterator = input_repository->CreateCaseIterator(
                ( case_iterator_routine.input_access_type == DataRepositoryAccess::BatchInput ) ?
                CaseIterationMethod::SequentialOrder : CaseIterationMethod::KeyOrder, CaseIterationOrder::Ascending);

            // read and write the case
            while( case_iterator->NextCase(*data_case) )
            {
                if( case_iterator_routine.should_write_case_callback == nullptr || (*case_iterator_routine.should_write_case_callback)() )
                {
                    output_repository->WriteCase(*data_case);
                    ++cases_written;
                }

                if( --cases_until_progress_update == 0 )
                {
                    // update progress bar
                    progress_dlg.SetPos(case_iterator->GetPercentRead());
                    cases_until_progress_update = ProgressUpdateFrequency;
                }

                if( progress_dlg.IsCanceled() )
                    throw std::exception();
            }

            case_iterator.reset();

            input_repository->Close();
        }

        output_repository->Close();

        // if successful, potentially recycle the original file and rename the new one to the original file's name
        if( case_iterator_routine.rename_output_to_input_on_success )
        {
            ASSERT(case_iterator_routine.input_connection_strings.size() == 1);
            DataRepositoryHelpers::RenameRepository(case_iterator_routine.output_connection_string, case_iterator_routine.input_connection_strings.front());
        }

        post_operation_message.Format(_T("%d case%s in %s%s %c%sed"), (int)cases_written, PluralizeWord(cases_written),
            (LPCTSTR)PortableFunctions::PathGetFilename(case_iterator_routine.input_connection_strings.front().GetFilename()),
            ( case_iterator_routine.input_connection_strings.size() == 1 ) ? _T("") : _T(" and other files"),
            tolower(action_verb_base[0]), action_verb_base + 1);

        success = true;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        post_operation_message = WS2CS(exception.GetErrorMessage());
    }

    catch(...)
    {
        post_operation_message = _T("Operation canceled");
    }

    // on error, delete the output repository
    if( !success )
        PortableFunctions::FileDelete(case_iterator_routine.output_connection_string.GetFilename());

    AfxMessageBox(post_operation_message);
}


void CDictionaryMacros::OnBnClickedCompactDataFile()
{
    RunCompactSortDataFile(true);
}

void CDictionaryMacros::OnBnClickedSortDataFile()
{
    RunCompactSortDataFile(false);
}

void CDictionaryMacros::RunCompactSortDataFile(bool compact_data)
{
    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    data_file_dlg.SetDictionaryFilename(m_pDict->GetFullFileName());

    if( data_file_dlg.DoModal() != IDOK )
        return;

    if( !data_file_dlg.GetConnectionString().IsFilenamePresent() )
    {
        AfxMessageBox(_T("You must select an actual data file"));
        return;
    }

    CaseIteratorRoutine case_iterator_routine
    {
        data_file_dlg.GetConnectionStrings(),
        compact_data ? DataRepositoryAccess::BatchInput : DataRepositoryAccess::ReadOnly,
        GetTempDataFileConnectionString(data_file_dlg.GetConnectionString()),
        true,
        nullptr
    };

    RunCaseIteratorRoutine(case_iterator_routine, compact_data ? _T("Compact") : _T("Sort"));
}


void CDictionaryMacros::OnBnClickedCreateSample() // 20110222
{
    // check the parameters
    bool randomFile = IsDlgButtonChecked(IDC_RANDOM_FILE) == BST_CHECKED;

    CString strPercent;
    GetDlgItem(IDC_PERCENT)->GetWindowText(strPercent);

    if( strPercent.IsEmpty() )
    {
        AfxMessageBox(_T("Specify the percentage of cases to output"));
        return;
    }

    int percent = _ttoi(strPercent);

    if( percent < 1 || percent > 99 )
    {
        AfxMessageBox(_T("Enter a valid number for the percentage of cases to output"));
        return;
    }

    if( !randomFile && ( 100 % percent ) != 0 )
    {
        AfxMessageBox(_T("When specifying sequential the percentage must divide evenly by 100"));
        return;
    }

    CString strStartPos;
    GetDlgItem(IDC_START_POS)->GetWindowText(strStartPos);

    if( strStartPos.IsEmpty() )
    {
        AfxMessageBox(_T("Specify the position to start outputting cases"));
        return;
    }

    int startPos = _ttoi(strStartPos);

    if( startPos <= 0 )
    {
        AfxMessageBox(_T("Enter a valid number for the position to start outputting cases"));
        return;
    }


    DataFileDlg source_data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    source_data_file_dlg.SetTitle(_T("Select Original Data File(s)"))
                        .SetDictionaryFilename(m_pDict->GetFullFileName())
                        .AllowMultipleSelections();

    if( source_data_file_dlg.DoModal() != IDOK )
        return;

    // suggest a filename based on the input filename
    ConnectionString suggested_sample_connection_string;

    if( source_data_file_dlg.GetConnectionStrings().size() == 1 )
        suggested_sample_connection_string = PathHelpers::AppendToConnectionStringFilename(source_data_file_dlg.GetConnectionString(), _T("_sample"));

    DataFileDlg sample_data_file_dlg(DataFileDlg::Type::CreateNew, false, suggested_sample_connection_string);
    sample_data_file_dlg.SetTitle(_T("Select Sample Data File"))
                        .SetDictionaryFilename(m_pDict->GetFullFileName())
                        .SuggestMatchingDataRepositoryType(source_data_file_dlg.GetConnectionStrings());

    if( sample_data_file_dlg.DoModal() != IDOK )
        return;


    // a routine for determining which cases to write out
    std::random_device random_device;
    std::mt19937 random_engine(random_device());
    std::uniform_int_distribution<> random_number_generator(1, 100);
    size_t down_counter = startPos;

    std::function<bool()> should_write_case_callback = [&]() -> bool
    {
        if( randomFile )
            return ( random_number_generator(random_engine) <= percent );

        else
        {
            if( --down_counter == 0 )
            {
                down_counter = 100 / percent;
                return true;
            }

            else
                return false;
        }
    };

    // simply read cases from one repository and write them to another (for text repositories this will remove erased records)
    CaseIteratorRoutine case_iterator_routine
    {
        source_data_file_dlg.GetConnectionStrings(),
        DataRepositoryAccess::BatchInput,
        sample_data_file_dlg.GetConnectionString(),
        false,
        &should_write_case_callback
    };

    RunCaseIteratorRoutine(case_iterator_routine, _T("Sampl"));
}


void CDictionaryMacros::OnBnClickedCreateNotesDictionary()
{
    const TCHAR* const NamePrefix = _T("NOTES_");

    CIMSAFileDialog dcfNameDlg(FALSE,FileExtensions::Dictionary,NULL,OFN_OVERWRITEPROMPT,_T("Notes Dictionary File (*.dcf)|*.dcf||"));

    if( dcfNameDlg.DoModal() != IDOK )
        return;

    CDataDict notes_dictionary;
    notes_dictionary.SetName(NamePrefix + m_pDict->GetName());
    notes_dictionary.SetLabel(m_pDict->GetLabel() + _T(" (Notes Dictionary)"));
    notes_dictionary.SetPosRelative(true);
    notes_dictionary.SetRecTypeLen(0);
    notes_dictionary.SetRecTypeStart(0);

    const DictLevel& source_dict_level = m_pDict->GetLevel(0);

    DictLevel notes_dict_level;
    notes_dict_level.SetName(NamePrefix + source_dict_level.GetName());
    notes_dict_level.SetLabel(source_dict_level.GetLabel() + _T(" (Notes Level)"));

    CDictRecord notes_dict_record;
    notes_dict_record.SetName(_T("NOTES_REC"));
    notes_dict_record.SetLabel(m_pDict->GetLabel() + _T(" (Notes Record)"));

    int iItemPos = 1;

    // add the ID items
    std::vector<const CDictItem*> id_items = m_pDict->GetIdItems();

    CDictRecord* pNotesIdItemsRec = notes_dict_level.GetIdItemsRec();

    for( size_t i = 0; i < id_items.size(); ++i )
    {
        pNotesIdItemsRec->AddItem(id_items[i]);
        pNotesIdItemsRec->GetItem(i)->SetStart(iItemPos);
        iItemPos += id_items[i]->GetLen();
    }

    // create a value set with the names of all of the fields
    const int FieldNameLength = 32;
    DictValueSet field_name_value_set;

    for( const DictLevel& dict_level : m_pDict->GetLevels() )
    {
        for( int record_index = -1; record_index < dict_level.GetNumRecords(); record_index++ )
        {
            const CDictRecord* pRecord = ( record_index == -1 ) ? dict_level.GetIdItemsRec() : dict_level.GetRecord(record_index);

            for( int item_index = 0; item_index < pRecord->GetNumItems(); item_index++ )
            {
                const CDictItem* pItem = pRecord->GetItem(item_index);
                CIMSAString field_name = CSProNameShortener::CSProToUnicode(pItem->GetName(), FieldNameLength);
                field_name.MakeExactLength(FieldNameLength);

                DictValue dict_value;
                dict_value.SetLabel(pItem->GetName());
                dict_value.AddValuePair(DictValuePair(field_name));

                field_name_value_set.AddValue(std::move(dict_value));
            }
        }
    }

    // add the note items, all but the note itself as ID items
    CDictItem notes_dict_item;

    notes_dict_item.SetName(_T("NOTES_FIELD"));
    notes_dict_item.SetLabel(_T("Note Field Name"));
    notes_dict_item.SetContentType(ContentType::Alpha);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(FieldNameLength);

    field_name_value_set.SetName(notes_dict_item.GetName() + _T("_VS"));
    field_name_value_set.SetLabel(notes_dict_item.GetLabel());
    notes_dict_item.AddValueSet(std::move(field_name_value_set));

    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.RemoveAllValueSets();

    notes_dict_item.SetName(_T("NOTES_OPERATOR_ID"));
    notes_dict_item.SetLabel(_T("Note Operator ID"));
    notes_dict_item.SetContentType(ContentType::Alpha);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(32);
    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName(_T("NOTES_MODIFIED_DATE"));
    notes_dict_item.SetLabel(_T("Note Modified Date"));
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(8);
    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName(_T("NOTES_MODIFIED_TIME"));
    notes_dict_item.SetLabel(_T("Note Modified Time"));
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(6);
    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName(_T("NOTES_RECORD_OCC"));
    notes_dict_item.SetLabel(_T("Note Record Occurrence"));
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(5);
    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName(_T("NOTES_ITEM_OCC"));
    notes_dict_item.SetLabel(_T("Note Item Occurrence"));
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(5);
    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName(_T("NOTES_SUBITEM_OCC"));
    notes_dict_item.SetLabel(_T("Note Subitem Occurrence"));
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(5);
    pNotesIdItemsRec->AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName(_T("NOTES_NOTE"));
    notes_dict_item.SetLabel(_T("Note"));
    notes_dict_item.SetContentType(ContentType::Alpha);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(999);
    notes_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_record.SetRecLen(iItemPos - 1);

    notes_dict_level.AddRecord(&notes_dict_record);
    notes_dictionary.AddLevel(std::move(notes_dict_level));

    try
    {
        notes_dictionary.Save(dcfNameDlg.GetPathName());
    }

    catch( const CSProException& exception )
    {
		ErrorMessage::Display(exception);
    }
}
