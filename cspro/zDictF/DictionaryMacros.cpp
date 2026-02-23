#include "StdAfx.h"
#include "DictionaryMacros.h"
#include "ItemGrid.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/NameShortener.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilF/SystemIcon.h>
#include <zUtilF/ThreadedProgressDlg.h>
#include <zAppO/PFF.h>
#include <zBridgeO/DataFileDlg.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/TextRepositoryNotesFile.h>
#include <random>


BEGIN_MESSAGE_MAP(DictionaryMacrosDlg, CDialog)
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


DictionaryMacrosDlg::DictionaryMacrosDlg(CDDDoc* const pDDDoc, CWnd* const pParent/* nullptr */)
    :   CDialog(IDD_DICTIONARY_MACROS, pParent),
        m_pDictDoc(pDDDoc),
        m_pDict(m_pDictDoc->GetDict())
{
}


void DictionaryMacrosDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    if( m_pDict->GetNumLevels() > 1 )
    {
        for( const int id : { IDC_GENERATE_DATA_FILE, IDC_NUMBER_CASES, IDC_NOTAPPL_PERCENT, IDC_INVALID_PERCENT } )
            GetDlgItem(id)->EnableWindow(FALSE);
    }

    CheckDlgButton(IDC_RANDOM_FILE,BST_CHECKED);
    GetDlgItem(IDC_START_POS)->SetWindowText(L"1");

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

    CComboBox* const pComboBox = static_cast<CComboBox*>(GetDlgItem(IDC_RECORD_LISTING));

    for( const DictLevel& dict_level : m_pDict->GetLevels() )
    {
        for( int record = 0; record < dict_level.GetNumRecords(); ++record )
        {
            const CDictRecord* const dict_record = dict_level.GetRecord(record);
            const int pos = pComboBox->AddString(TC::ToWide(dict_record->GetName()).c_str());
            pComboBox->SetItemDataPtr(pos, const_cast<CDictRecord*>(dict_record));
        }
    }

    CString csDialogTitleText;
    GetWindowText(csDialogTitleText);
    csDialogTitleText.AppendFormat(L" - %s", UTF8_TODO::GetWide(m_pDict->GetName()).c_str());
    SetWindowText(csDialogTitleText);
}


void DictionaryMacrosDlg::OnBnClickedDeleteValueSets() // 20101108
{
    // 20120608 a Jordanian in the workshop asked for this confirmation
    if( AfxMessageBox(L"Are you sure you want to delete all the value sets?", MB_YESNO | MB_ICONSTOP) != IDYES )
        return;

    size_t num_value_sets = 0;

    DictionaryIterator::Foreach<CDictItem>(*m_pDict,
        [&](CDictItem& dict_item)
        {
            num_value_sets += dict_item.GetNumValueSets();
            dict_item.RemoveAllValueSets();
        });

    m_pDictDoc->SetModified(true);

    AfxMessageBox(FormatText("%d value set%s have been deleted", static_cast<int>(num_value_sets), PluralizeWord(num_value_sets)));
}


void DictionaryMacrosDlg::SetRequireRecords(bool required) // 20101108
{
    DictionaryIterator::Foreach<CDictRecord>(*m_pDict,
        [&](CDictRecord& dict_record)
        {
            dict_record.SetRequired(required);
        });

    m_pDictDoc->SetModified(true);
}


void DictionaryMacrosDlg::OnBnClickedRequireRecordsYes() // 20101108
{
    SetRequireRecords(true);
    AfxMessageBox(L"All records are now required");
}


void DictionaryMacrosDlg::OnBnClickedRequireRecordsNo() // 20101108
{
    SetRequireRecords(false);
    AfxMessageBox(L"All records are not required");
}


CString DictionaryMacrosDlg::GetLabelWithLanguages(const LabelSet& label, bool copy_all_languages) const
{
    if( !copy_all_languages )
        return label.GetLabel();

    CString labels;

    for( size_t i = 0; i < m_pDict->GetLanguages().size(); ++i )
        labels.AppendFormat(L"%s%s", ( i == 0 ) ? L"" : L"\t", label.GetLabel(i).GetString());

    return labels;
}


void DictionaryMacrosDlg::SetLabelWithLanguages(LabelSet& label, const CStringArray& csaLabels, bool paste_all_languages)
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


void DictionaryMacrosDlg::OnBnClickedCopyDictionaryNames() // 20101108
{
    struct NameDictionaryIterator : public DictionaryIterator::Iterator
    {
        CString text;
        bool copy_all_languages;
        bool copy_item_lengths;
        CString indentation;
        const DictionaryMacrosDlg* dlg;

        void ProcessLevel(DictLevel& dict_level) override
        {
            text.AppendFormat(L"%s\t%s\r\n",
                              UTF8_TODO::GetWide(dict_level.GetName()).c_str(),
                              dlg->GetLabelWithLanguages(dict_level.GetLabelSet(), copy_all_languages).GetString());
        }

        void ProcessRecord(CDictRecord& dict_record) override
        {
            text.AppendFormat(L"\t%s%s\t%s\r\n",
                              indentation.GetString(),
                              UTF8_TODO::GetWide(dict_record.GetName()).c_str(),
                              dlg->GetLabelWithLanguages(dict_record.GetLabelSet(), copy_all_languages).GetString());
        }

        void ProcessItem(CDictItem& dict_item) override
        {
            text.AppendFormat(L"\t%s\t%s%s\t%s",
                              indentation.GetString(), indentation.GetString(),
                              UTF8_TODO::GetWide(dict_item.GetName()).c_str(),
                              dlg->GetLabelWithLanguages(dict_item.GetLabelSet(), copy_all_languages).GetString());

            if( copy_item_lengths )
                text.AppendFormat(L"\t%d", dict_item.GetLen());

            text.Append(L"\r\n");
        }
    };

    NameDictionaryIterator iterator;
    iterator.copy_all_languages = ((CButton*)GetDlgItem(IDC_ITEM_ALL_LANGUAGES))->GetCheck();
    iterator.copy_item_lengths = ((CButton*)GetDlgItem(IDC_ITEM_LENGTHS))->GetCheck();
    iterator.indentation = CString('\t', iterator.copy_all_languages ? m_pDict->GetLanguages().size() : 1);
    iterator.dlg = this;

    iterator.Iterate(*m_pDict);

    WinClipboard::PutText(this, iterator.text);
    AfxMessageBox(L"Names and labels copied to the clipboard");
}


int DictionaryMacrosDlg::readEntry(CString text,int startPos,CString& readWord)
{
    int endPos;

    for( endPos = startPos; endPos < text.GetLength(); endPos++ )
    {
        if( text[endPos] == '\t' || text[endPos] == '\n' || text[endPos] == '\r' )
            break;
    }

    readWord = text.Mid(startPos,endPos - startPos);

    return endPos;
}

int DictionaryMacrosDlg::readLabelsEntry(CString text,int startPos,CStringArray& csaLabels,bool paste_all_languages, bool& bSuccessfulRead)
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

            if( i < ( m_pDict->GetLanguages().size() - 1 ) && ( text[endPos++] != '\t' ) )
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


void DictionaryMacrosDlg::makeNewNameWork(DictNamedBase& dict_element, const CString& oldName) // 20101109
{
    int level = -1,record = -1,item = -1,vset = -1;
    m_pDict->LookupName(UTF8_TODO::GetUtf8(oldName), &level, &record, &item, &vset);
    m_pDict->UpdateNameList(dict_element,level,record,item,vset);
    m_pDict->SetOldName(UTF8_TODO::GetUtf8(oldName));
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


void DictionaryMacrosDlg::OnBnClickedPasteDictionaryNames() // 20101108
{
    CString text = WS2CS(WinClipboard::GetText(this));
    text.Append(L"\n               "); // this should ensure that none of my text[textPtr++] codes will cause an out of bounds error

    // we need to check two things:
    // 1) that the text on the clipboard matches what is already in the dictionary (i.e., same number of records, items, etc.)
    // 2) that no duplicate names are used

    bool successfulRead = true;
    bool firstPass = true;
    CArray<CString> newNames;

    bool paste_all_languages = ((CButton*)GetDlgItem(IDC_ITEM_ALL_LANGUAGES))->GetCheck();
    bool bPasteItemLengths = ((CButton*)GetDlgItem(IDC_ITEM_LENGTHS))->GetCheck();
    CString csErrorMessage = L"Error: Clipboard contents do not match dictionary";

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

                successfulRead = text[textPtr++] == '\n';
            }

            if( !successfulRead )
                break;

            textPtr = readEntry(text,textPtr,name);
            name.MakeUpper();
            successfulRead = text[textPtr++] == '\t';

            if( successfulRead )
                textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);

            if( firstPass )
            {
                newNames.Add(name);
            }

            else
            {
                SetLabelWithLanguages(dict_level.GetLabelSet(), csaLabels, paste_all_languages);

                oldName = UTF8_TODO::GetCString(dict_level.GetName());

                if( name != oldName )
                {
                    dict_level.SetName(UTF8_TODO::GetUtf8(name));
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

                successfulRead = text[textPtr++] == '\n';

                for( int iTab = 0; successfulRead && iTab < ( 1 + iExpectedLabelIndentationTabs ); iTab++ )
                    successfulRead = text[textPtr++] == '\t';

                if( successfulRead )
                {
                    textPtr = readEntry(text,textPtr,name);
                    name.MakeUpper();
                    successfulRead = text[textPtr++] == '\t';

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

                            oldName = UTF8_TODO::GetCString(pRecord->GetName());

                            if( name != oldName )
                            {
                                pRecord->SetName(UTF8_TODO::GetUtf8(name));
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

                            successfulRead = text[textPtr++] == '\n';

                            for( int iTab = 0; successfulRead && iTab < ( 2 * ( 1 + iExpectedLabelIndentationTabs ) ) ; iTab++ )
                                successfulRead = text[textPtr++] == '\t';

                            if( successfulRead )
                            {
                                textPtr = readEntry(text,textPtr,name);
                                name.MakeUpper();
                                successfulRead = text[textPtr++] == '\t';

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

                                        oldName = UTF8_TODO::GetCString(pItem->GetName());

                                        if( name != oldName )
                                        {
                                            pItem->SetName(UTF8_TODO::GetUtf8(name));
                                            m_pDict->SetChangedObject(pItem);
                                            makeNewNameWork(*pItem,oldName);
                                        }
                                    }

                                    if( bPasteItemLengths ) // 20140309
                                    {
                                        successfulRead = text[textPtr++] == '\t';

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
                                                    csErrorMessage.Format(L"The length of item %s must be at least 1", name.GetString());
                                                }

                                                else if( pItem->GetContentType() == ContentType::Numeric )
                                                {
                                                    if( iItemLength > 15 )
                                                    {
                                                        successfulRead = false;
                                                        csErrorMessage.Format(L"The length of numeric item %s cannot be greater than 15", name.GetString());
                                                    }

                                                    else if( iItemLength < ( pItem->GetDecimal() + pItem->GetDecChar() ) )
                                                    {
                                                        successfulRead = false;
                                                        csErrorMessage.Format(L"The length of numeric item %s with %d decimal characters cannot be %d", name.GetString(), pItem->GetDecimal(), iItemLength);
                                                    }
                                                }

                                                if( iItemLength != 1 && !DictionaryRules::CanModifyLength(*pItem) )
                                                {
                                                    // binary type must be length 1
                                                    successfulRead = false;
                                                    csErrorMessage.Format(L"The length of %s item %s must be 1", UTF8_TODO::GetWide(ToString(pItem->GetContentType())).c_str(), name.GetString());
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
                                                        csErrorMessage.Format(L"The length of an item (%s) with overlapping subitems cannot be reduced in size", name.GetString());
                                                    }
                                                }

                                                else
                                                {
                                                    if( bHasOverlappingSubitems )
                                                    {
                                                        if( iItemLength != pItem->GetLen() )
                                                        {
                                                            successfulRead = false;
                                                            csErrorMessage.Format(L"The lengths of overlapping subitems (including %s) cannot be modified", name.GetString());
                                                        }
                                                    }

                                                    else
                                                    {
                                                        iCumulativeSubitemLength += iItemLength * pItem->GetOccurs();

                                                        if( iCumulativeSubitemLength > iLastItemLength )
                                                        {
                                                            successfulRead = false;
                                                            csErrorMessage.Format(L"The length of subitem %s exceeds the length of its parent item", name.GetString());
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
                    AfxMessageBox(L"Error: Clipboard contents contain a name that is empty");
                    return;
                }

                // see if there are any any errors in the name
                bool invalidName = false;

                TCHAR firstChar = newNames[i2].GetAt(0);

                if( ( firstChar < 'A' || firstChar > 'Z' ) && newNames[i2].Find(L"_IDS") < 0 ) // IDs has the _ at the beginning exception
                {
                    invalidName = true;
                }

                else
                {
                    for( int j = 1; j < newNames[i2].GetLength() && !invalidName; j++ )
                    {
                        TCHAR thisChar = newNames[i2].GetAt(j);

                        if( !( thisChar >= 'A' && thisChar <= 'Z' ) && !( thisChar >= '0' && thisChar <= '9' ) && thisChar != '_' )
                            invalidName = true;
                    }
                }

                if( invalidName )
                {
                    text.Format(L"Error: Clipboard contents contain an invalid name: %s", newNames[i2].GetString());
                    AfxMessageBox(text);
                    return;
                }


                // see if any of the desired names is a duplicate
                for( int j = i2 + 1; j < newNames.GetSize(); j++ )
                {
                    if( newNames[i2] == newNames[j] )
                    {
                        text.Format(L"Error: Clipboard contents contain multiple entries with the same name: %s", newNames[i2].GetString());
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
        AfxMessageBox(L"Names and labels pasted from the clipboard");
    }

    else
    {
        AfxMessageBox(csErrorMessage);
    }
}


void DictionaryMacrosDlg::OnBnClickedCopyValueSets()
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
                text.Append(L"\r\n\r\n");

            text.AppendFormat(L"%s\t%s",
                              UTF8_TODO::GetWide(dict_value_set.GetName()).c_str(),
                              GetLabelWithLanguages(dict_value_set.GetLabelSet(), copy_all_languages).GetString());

            for( const DictValue& dict_value : dict_value_set.GetValues() )
            {
                bool first_pair = true;

                for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
                {
                    text.AppendFormat(L"\r\n\t%s%s\t%s\t%s\t%s",
                                      label_indentation.GetString(),
                                      first_pair ? GetLabelWithLanguages(dict_value.GetLabelSet(), copy_all_languages).GetString() : label_indentation.Left(label_indentation.GetLength() - 1).GetString(),
                                      dict_value_pair.GetFrom().GetString(),
                                      dict_value_pair.GetTo().GetString(),
                                      dict_value.IsSpecial() ? UTF8_TODO::GetWide(SpecialValues::ValueToString(dict_value.GetSpecialValue(), false)).c_str() : L"");

                    if( copy_value_set_images && first_pair )
                        text.AppendFormat(L"\t%s", UTF8_TODO::GetCString(dict_value.GetImageFilePath()).GetString());

                    first_pair = false;
                }
            }
        });

    WinClipboard::PutText(this, text);
    AfxMessageBox(FormatText(L"%d value sets copied to the clipboard", num_value_sets));
}


CString DictionaryMacrosDlg::makeValueValid(CString value,CDictItem* pItem,int & numValsModified) // 20101113
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
            return value + CString(' ', pItem->GetLen() - value.GetLength());
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
            if( value[i] == '.' && !oneDecimal )
                oneDecimal = true;

            else if( value[i] == '0' )
                ; // fine

            else
                notZero = true;
        }

        if( notZero )
            numValsModified++;

        return L"0";
    }

    else
    {
        if( pItem->GetDecimal() == 0 ) // no decimal part
        {
            if( value.Find('.') >= 0 ) // the pasted value has a decimal point
                numValsModified++;

            else if( value.Trim().GetLength() > pItem->GetLen() )
                numValsModified++;

            double maxSize = pow(10.0, static_cast<int>(pItem->GetLen())) - 1;

            while( dVal > maxSize )
                dVal /= 10;

            value.Format(L"%.0Lf", dVal);

            return value;
        }

        else // the value has a decimal part
        {
            int decLen = pItem->GetDecimal();
            int intLen = pItem->GetLen() - decLen - pItem->GetDecChar();

            CString decimalFormatted,formatStyle;
            formatStyle.Format(L"%%.%dLf", decLen);

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


void DictionaryMacrosDlg::OnBnClickedPasteValueSets()
{
    CString text = WS2CS(WinClipboard::GetText(this));
    text.Append(L"\r\n\r\n\r\n\r\n\r\n"); // this should ensure that none of my text[textPtr++] codes will cause an out of bounds error

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
                successfulRead = text[textPtr++] == '\t';

                if( successfulRead )
                    textPtr = readLabelsEntry(text, textPtr, csaLabels, paste_all_languages, successfulRead);

                if( successfulRead )
                {
                    while( text[textPtr] == '\t' ) // there will be extra tabs if the data is copied from Excel
                        textPtr++;

                    if( text[textPtr] == '\r' )
                        textPtr++;

                    bool blankLine = false;

                    successfulRead = text[textPtr++] == '\n';

                    CDictItem* dict_item = nullptr;
                    DictValueSet* dict_value_set = nullptr;

                    if( secondPass ) // see if this value set exists in the current dictionary
                    {
                        if( m_pDict->LookupName<DictValueSet>(UTF8_TODO::GetUtf8(name), nullptr, nullptr, &dict_item, &dict_value_set) )
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
                            successfulRead = text[textPtr++] == '\t';

                        if( successfulRead )
                        {
                            bool bNoLabels = true;

                            for( int iTab = 0; bNoLabels && iTab < iExpectedLabelIndentationTabs; iTab++ )
                                bNoLabels = text[textPtr + iTab] == '\t';

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
                                successfulRead = text[textPtr++] == '\t';

                            if( successfulRead )
                            {
                                bool restOfLineIsBlank = false;
                                to.Empty();
                                special.Empty();
                                image.Empty();

                                // reading the from value
                                textPtr = readEntry(text,textPtr,from);

                                if( text[textPtr] == '\r' || text[textPtr] == '\n' )
                                    restOfLineIsBlank = true;

                                else
                                    successfulRead = text[textPtr++] == '\t';

                                // reading the to value
                                if( successfulRead && !restOfLineIsBlank )
                                {
                                    textPtr = readEntry(text,textPtr,to);

                                    if( text[textPtr] == '\r' || text[textPtr] == '\n' )
                                        restOfLineIsBlank = true;

                                    else
                                        successfulRead = text[textPtr++] == '\t';
                                }

                                // reading the special value
                                if( successfulRead && !restOfLineIsBlank )
                                {
                                    textPtr = readEntry(text,textPtr,special);

                                    if( text[textPtr] == '\r' || text[textPtr] == '\n' )
                                        restOfLineIsBlank = true;

                                    else
                                        successfulRead = text[textPtr++] == '\t';
                                }

                                // reading the value set image
                                if( successfulRead && !restOfLineIsBlank && paste_value_set_images )
                                {
                                    textPtr = readEntry(text,textPtr,image);

                                    if( text[textPtr] == '\r' || text[textPtr] == '\n' )
                                        restOfLineIsBlank = true;

                                    else
                                        successfulRead = text[textPtr++] == '\t';
                                }

                                if( successfulRead )
                                {
                                    while( text[textPtr] == '\t' ) // there could be extra tabs if the data is copied from Excel
                                        textPtr++;

                                    if( text[textPtr] == '\r' )
                                        textPtr++;

                                    successfulRead = text[textPtr++] == '\n';

                                    blankLine = bNoLabels && from.IsEmpty() && to.IsEmpty() && special.IsEmpty();

                                    if( successfulRead && !blankLine ) // process the data
                                    {
                                        if( dict_item != nullptr && dict_item->GetContentType() == ContentType::Alpha )
                                        {
                                            to.Empty();
                                            special.Empty();
                                        }

                                        std::optional<double> special_value;

                                        if( !special.IsEmpty() )
                                        {
                                            special_value = SpecialValues::StringIsSpecial<std::optional<double>>(UTF8_TODO::GetUtf8(special));

                                            if( !special_value.has_value() )
                                            {
                                                if( special.CompareNoCase(L"Not Applicable") == 0 ) // pre-8.0
                                                {
                                                    special_value = NOTAPPL;
                                                }

                                                else
                                                {
                                                    successfulRead = false;
                                                    name.Format(L"Special value '%s' in an invalid entry", special.GetString());
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

                                                newValue.SetImageFilePath(UTF8_TODO::GetUtf8(image));
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

                while( text[textPtr] == '\r' || text[textPtr] == '\n' ) // read until the next value set (or the end of the pasted text)
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

        std::string message = FormatText("%d value set%s pasted from the clipboard", numValueSetsCopied, PluralizeWord(numValueSetsCopied));

        if( numValuesModified )
            message.append(FormatText(" though %d invalid value%s modified", numValuesModified, numValuesModified == 1 ? " was" : "s were"));

        AfxMessageBox(message);
    }

    else
    {
        AfxMessageBox(L"Error: Clipboard contents do not match value sets format");
    }
}


void DictionaryMacrosDlg::OnBnClickedGenerateDataFile() // 20101114 this now currently only works for one-level applications
{
    // check the parameters
    CString strNumCases;
    GetDlgItem(IDC_NUMBER_CASES)->GetWindowText(strNumCases);

    if( strNumCases.IsEmpty() )
    {
        AfxMessageBox(L"Specify the number of cases desired");
        return;
    }

    int numCases = _ttoi(strNumCases);

    if( numCases <= 0 )
    {
        AfxMessageBox(L"You must select a positive number of cases");
        return;
    }

    CString strNotapplPercent;
    GetDlgItem(IDC_NOTAPPL_PERCENT)->GetWindowText(strNotapplPercent);
    notapplPercent = _ttoi(strNotapplPercent);

    CString strInvalidPercent;
    GetDlgItem(IDC_INVALID_PERCENT)->GetWindowText(strInvalidPercent);
    invalidPercent = _ttoi(strInvalidPercent);

    if( ( notapplPercent < 0 || notapplPercent > 100 ) || ( notapplPercent == 0 && !strNotapplPercent.IsEmpty() && strNotapplPercent != L"0" )  )
    {
        AfxMessageBox(L"Enter a valid number for the percent of not applicable values");
        return;
    }

    if( invalidPercent < 0 || invalidPercent > 100 || ( invalidPercent == 0 && !strInvalidPercent.IsEmpty() && strInvalidPercent != L"0" )  )
    {
        AfxMessageBox(L"Enter a valid number for the percent of invalid values");
        return;
    }

    regularPercent = 100 - notapplPercent - invalidPercent;

    if( regularPercent <= 0 )
    {
        AfxMessageBox(L"The percentages of not applicable and invalid values exceed or equal 100%");
        return;
    }

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false);
    data_file_dlg.SetDictionaryFilePath(m_pDict->GetFilePath());

    if( data_file_dlg.DoModal() != IDOK )
        return;

    std::string post_operation_message;

    try
    {
        constexpr const wchar_t* ProgressDlgMessage = L"Generating Data";
        ThreadedProgressDlg progress_dlg;
        progress_dlg.SetTitle(ProgressDlgMessage);
        progress_dlg.SetStatus(ProgressDlgMessage);
        progress_dlg.Show();

        const std::shared_ptr<CaseAccess> case_access = CreateCaseAccess();
        const std::unique_ptr<Case> data_case = case_access->CreateCase();

        // open the repository
        const std::unique_ptr<DataRepository> output_repository = DataRepository::CreateAndOpen(case_access,
                                                                                                data_file_dlg.GetConnectionString(),
                                                                                                DataRepositoryAccess::BatchOutput,
                                                                                                DataRepositoryOpenFlag::CreateNew);

        // seed the random number generator
        srand(static_cast<unsigned int>(time(nullptr)));

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
                for( const CaseItem* const case_item : id_record.GetCaseItems() )
                    AddRandomValue(*case_item, id_index);

                if( previous_keys.find(UTF8_TODO::GetCString(data_case->GetKey())) == previous_keys.end() )
                {
                    previous_keys.insert(UTF8_TODO::GetCString(data_case->GetKey()));
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
                    const CDictRecord& dict_record = case_record.GetCaseRecordMetadata().GetDictRecord();

                    size_t number_records_to_write = dict_record.GetMaxRecs();

                    // for singly occurring records that are not required, write them out only 75% of the time (an arbitrary value)
                    if( number_records_to_write == 1 )
                    {
                        if( !dict_record.GetRequired() && ( rand() % 4 ) == 0 )
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

                    if( dict_record.GetRequired() && number_records_to_write == 0 )
                        number_records_to_write = 1;

                    case_record.SetNumberOccurrences(number_records_to_write);

                    for( size_t k = 0; k < number_records_to_write; ++k )
                    {
                        CaseItemIndex index = case_record.GetCaseItemIndex(k);

                        for( const CaseItem* const case_item : case_record.GetCaseItems() )
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
                progress_dlg.SetPos(static_cast<int>(100.0 * numWritten / numCases));

            if( progress_dlg.IsCanceled() )
                throw std::exception();
        }

        output_repository->Close();

        if( numWritten != numCases )
        {
            post_operation_message = FormatText("Could only write out %d case%s due to ID size limitations", numWritten, PluralizeWord(numCases));
        }

        else
        {
            post_operation_message = FormatText("Successfully wrote out %d case%s", numCases, PluralizeWord(numCases));
        }
    }

    catch( const DataRepositoryException::Error& exception )
    {
        post_operation_message = exception.what();
    }

    catch(...)
    {
        post_operation_message = "Operation canceled";
    }

    AfxMessageBox(post_operation_message);
}


void DictionaryMacrosDlg::AddRandomValue(const CaseItem& case_item, CaseItemIndex& index)
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
    if( IsBinary(case_item.GetDataType()) )
    {
        AddRandomBinaryValue(case_item, index);
        return;
    }


    // for numeric and string case items...
    const CDictItem& dict_item = case_item.GetDictItem();

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
        assert_cast<const StringCaseItem&>(case_item).SetValue(index, UTF8_TODO::GetUtf8(string_value));
    }

    else
    {
        ASSERT(false);
    }
}


double DictionaryMacrosDlg::GenerateRandomNumeric(const CDictItem& dict_item)
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


CString DictionaryMacrosDlg::GenerateRandomAlpha(const CDictItem& dict_item)
{
    CString value;

    // we'll create a string of random length
    int string_length = rand() % ( dict_item.GetLen() + 1 );

    TCHAR* buffer = value.GetBufferSetLength(string_length);

    // random characters will alphanumeric, with commas and frequent spaces
    for( int i = 0; i < string_length; ++i )
    {
        TCHAR randChar = rand() % 70;
        TCHAR output_ch = ' ';

        if( randChar < 26 )
            output_ch = 'a' + randChar;

        else if( randChar < 52 )
            output_ch = 'A' + randChar - 26;

        else if( randChar < 62 )
            output_ch = '0' + randChar - 52;

        else if( randChar == 63 )
            output_ch = '.';

        buffer[i] = output_ch;
    }

    value.ReleaseBuffer();

    return CIMSAString::MakeExactLength(value, dict_item.GetLen());
}


int DictionaryMacrosDlg::CountAlphaValues(const DictValueSet& dict_value_set) // 20101114
{
    const auto& lookup = alphaValueSetValueCounts.find(&dict_value_set);

    if( lookup != alphaValueSetValueCounts.cend() )
        return lookup->second;

    size_t values = 0;

    for( const DictValue& dict_value : dict_value_set.GetValues() )
        values += dict_value.GetNumValuePairs();

    // saves the values so that we're not constantly recounting the number of values
    alphaValueSetValueCounts.try_emplace(&dict_value_set, static_cast<int>(values));

    return values;
}


void DictionaryMacrosDlg::AddRandomBinaryValue(const CaseItem& case_item, CaseItemIndex& index)
{
    ASSERT(IsBinary(case_item.GetDataType()));
    const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(case_item);
    const CDictItem& dict_item = case_item.GetDictItem();
    const ContentType content_type = dict_item.GetContentType();

    auto create_from_html_media = [&](const char* const filename)
    {
        try
        {
            const std::string file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Media), filename);

            BinaryDataMetadata binary_data_metadata;
            binary_data_metadata.SetFilename(filename);
            binary_case_item.SetValue(index, BinaryData(FileIO::Read(file_path), std::move(binary_data_metadata)));
        }
        catch(...) { ASSERT(false); }
    };

    // for the Audio type, use a sound recording of "CSPro" taken from Google Translate
    if( content_type == ContentType::Audio )
    {
        create_from_html_media("cspro-pronunciation.m4a");
    }

    // for the Document type, create a text file with a simple message
    else if( content_type == ContentType::Document )
    {
        const std::string message = FormatText("Document for %s created using Dictionary Macros.", dict_item.GetName().c_str());

        BinaryDataMetadata binary_data_metadata;
        binary_data_metadata.SetFilename("dictionary-macros-document.txt");

        binary_case_item.SetValue(index, BinaryData(SO::CreateByteVector(message), std::move(binary_data_metadata)));
    }

    // for the Geometry type, use a GeoJSON file with the Census Bureau's coordinates
    else if( content_type == ContentType::Geometry )
    {
        constexpr std::string_view CensusBureauGeoJson_sv = R"!({"type":"Feature","geometry":{"type":"Point","coordinates":[-76.931098,38.84839]},"properties":{"name":"United States Census Bureau"}})!";

        BinaryDataMetadata binary_data_metadata;
        binary_data_metadata.SetFilename("us-census-bureau.geojson");

        binary_case_item.SetValue(index, BinaryData(SO::CreateByteVector(CensusBureauGeoJson_sv), std::move(binary_data_metadata)));
    }

    // for the Image type, create a PNG of the CSPro logo
    else if( content_type == ContentType::Image )
    {
        std::shared_ptr<const std::vector<std::byte>> content = SystemIcon::GetPngForCSProLogo();

        if( content == nullptr )
            return;

        BinaryDataMetadata binary_data_metadata;
        binary_data_metadata.SetFilename("cspro-logo.png");
        binary_case_item.SetValue(index, BinaryData(std::move(content), std::move(binary_data_metadata)));
    }

    // for the Video type, use a video showing the transition of the CSPro logo from the 2.0 version to the modern one
    else if( content_type == ContentType::Video )
    {
        create_from_html_media("cspro-logo-transition.webm");
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
        pos = std::max(pos, static_cast<int>(pItem->GetStart() + pItem->GetLen()));
    }

    return pos;
}

void DictionaryMacrosDlg::OnBnClickedAddItemsToRecord() // 20140308
{
    CString csNumItems;
    GetDlgItem(IDC_NUM_ITEMS)->GetWindowText(csNumItems);

    if( csNumItems.IsEmpty() )
    {
        AfxMessageBox(L"Specify the number of items to add");
        return;
    }

    int iNumItems = _ttoi(csNumItems);

    if( iNumItems < 1 || iNumItems > 500 )
    {
        AfxMessageBox(L"Enter a valid number of items (1 - 500) to add");
        return;
    }

    CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_RECORD_LISTING);
    int iSelection = pComboBox->GetCurSel();

    if( iSelection < 0 )
    {
        AfxMessageBox(L"Select the record to which you want to add the items");
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
    iStartingPos = std::max(iStartingPos, static_cast<int>(m_pDict->GetRecTypeStart() + m_pDict->GetRecTypeLen()));

    for( int i = 0; i < iNumItems; i++ )
    {
        CDictItem* pItem = new CDictItem();
        pItem->GetLabelSet().SetCurrentLanguage(m_pDict->GetCurrentLanguageIndex());
        pItem->SetRecord(pRecord);
        pItem->SetZeroFill(bZeroFill);
        pItem->SetStart(iStartingPos++);

        CString csTemp;
        csTemp.Format(L"%s (Item %d)", pRecord->GetLabel().GetString(), pRecord->GetNumItems() + 1);
        pItem->SetLabel(csTemp);

        csTemp.Format(L"%s_ITEM%03d", UTF8_TODO::GetWide(pRecord->GetName()).c_str(), pRecord->GetNumItems() + 1);
        csTemp = UTF8_TODO::GetCString(m_pDictDoc->GetDict()->GetUniqueName(UTF8_TODO::GetUtf8(csTemp)));
        pItem->SetName(UTF8_TODO::GetUtf8(csTemp));

        m_pDict->AddToNameList(*pItem,iRecord, level_number, pRecord->GetNumItems() - 1,-1);
        pRecord->AddItem(pItem);
    }

    m_pDictDoc->SetModified(true);

    AfxMessageBox(FormatText("%d items have been added to %s", iNumItems, pRecord->GetName().c_str()));
}


CString DictionaryMacrosDlg::GetTempDataFileName(CString csFilename)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(csFilename);
    CString csBaseFilename = PortableFunctions::PathRemoveFileExtensionCS(csFilename);
    CString csTempFileName;

    do
    {
        csBaseFilename.AppendFormat(L".tmp");
        csTempFileName.Format(L"%s%s%s", csBaseFilename.GetString(), extension.empty() ? L"" : L".", extension.c_str());

    } while( PortableFunctions::FileExists(csTempFileName) );

    return csTempFileName;
}


ConnectionString DictionaryMacrosDlg::GetTempDataFileConnectionString(const ConnectionString& connection_string)
{
    return ConnectionString(connection_string.ToString(UTF8_TODO::GetUtf8(GetTempDataFileName(UTF8_TODO::GetCString(connection_string.GetFilePath())))));
}


std::unique_ptr<CaseAccess> DictionaryMacrosDlg::CreateCaseAccess()
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


void DictionaryMacrosDlg::RunCaseIteratorRoutine(const CaseIteratorRoutine& case_iterator_routine, const char* const action_verb_base)
{
    const size_t ProgressUpdateFrequency = 100;

    std::unique_ptr<DataRepository> output_repository;
    std::string post_operation_message;
    bool success = false;

    try
    {
        if( case_iterator_routine.output_connection_string.SharesResource(case_iterator_routine.input_connection_strings) )
            throw CSProException("You cannot output to the same data source as the input: " + case_iterator_routine.output_connection_string.ToDisplayString());

        const std::string progress_title_and_status = FormatText("%sing...", action_verb_base);

        ThreadedProgressDlg progress_dlg;
        progress_dlg.SetTitle(progress_title_and_status);
        progress_dlg.SetStatus(progress_title_and_status);
        progress_dlg.Show();

        const std::shared_ptr<CaseAccess> case_access = CreateCaseAccess();
        const std::unique_ptr<Case> data_case = case_access->CreateCase();

        size_t cases_written = 0;
        size_t cases_until_progress_update = ProgressUpdateFrequency;

        // open the repositories
        output_repository = DataRepository::CreateAndOpen(case_access,
                                                          case_iterator_routine.output_connection_string,
                                                          DataRepositoryAccess::BatchOutput,
                                                          DataRepositoryOpenFlag::CreateNew);

        for( const ConnectionString& input_connection_string : case_iterator_routine.input_connection_strings )
        {
            const std::unique_ptr<DataRepository> input_repository = DataRepository::CreateAndOpen(case_access,
                                                                                                   input_connection_string,
                                                                                                   case_iterator_routine.input_access_type,
                                                                                                   DataRepositoryOpenFlag::OpenMustExist);

            // start the iterator
            const CaseIterationMethod case_iteration_method = ( case_iterator_routine.input_access_type == DataRepositoryAccess::BatchInput ) ?
                                                              CaseIterationMethod::SequentialOrder : CaseIterationMethod::KeyOrder;
            std::unique_ptr<CaseIterator> case_iterator = input_repository->CreateCaseIterator(case_iteration_method, CaseIterationOrder::Ascending);

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

        post_operation_message = FormatText("%d case%s in %s%s %c%sed",
                                            static_cast<int>(cases_written), PluralizeWord(cases_written),
                                            case_iterator_routine.input_connection_strings.front().ToDisplayString(true).c_str(),
                                            ( case_iterator_routine.input_connection_strings.size() == 1 ) ? "" : " and other data sources",
                                            tolower(action_verb_base[0]), action_verb_base + 1);

        success = true;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        post_operation_message = exception.what();
    }

    catch(...)
    {
        post_operation_message = "Operation canceled";
    }

    // on error, delete the output repository
    if( !success && output_repository != nullptr )
    {
        try
        {
            output_repository->DeleteRepository();
        }
        catch(...) { ASSERT(false); }
    }

    AfxMessageBox(post_operation_message);
}


void DictionaryMacrosDlg::OnBnClickedCompactDataFile()
{
    RunCompactSortDataFile(true);
}


void DictionaryMacrosDlg::OnBnClickedSortDataFile()
{
    RunCompactSortDataFile(false);
}


void DictionaryMacrosDlg::RunCompactSortDataFile(const bool compact_data)
{
    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    data_file_dlg.SetDictionaryFilePath(m_pDict->GetFilePath());

    if( data_file_dlg.DoModal() != IDOK )
        return;

    if( !data_file_dlg.GetConnectionString().HasFilePath() )
    {
        AfxMessageBox(L"You must select a file-based data source.");
        return;
    }

    const CaseIteratorRoutine case_iterator_routine
    {
        data_file_dlg.GetConnectionStrings(),
        compact_data ? DataRepositoryAccess::BatchInput : DataRepositoryAccess::ReadOnly,
        GetTempDataFileConnectionString(data_file_dlg.GetConnectionString()),
        true,
        nullptr
    };

    RunCaseIteratorRoutine(case_iterator_routine, compact_data ? "Compact" : "Sort");
}


void DictionaryMacrosDlg::OnBnClickedCreateSample() // 20110222
{
    // check the parameters
    bool randomFile = IsDlgButtonChecked(IDC_RANDOM_FILE) == BST_CHECKED;

    CString strPercent;
    GetDlgItem(IDC_PERCENT)->GetWindowText(strPercent);

    if( strPercent.IsEmpty() )
    {
        AfxMessageBox(L"Specify the percentage of cases to output");
        return;
    }

    int percent = _ttoi(strPercent);

    if( percent < 1 || percent > 99 )
    {
        AfxMessageBox(L"Enter a valid number for the percentage of cases to output");
        return;
    }

    if( !randomFile && ( 100 % percent ) != 0 )
    {
        AfxMessageBox(L"When specifying sequential the percentage must divide evenly by 100");
        return;
    }

    CString strStartPos;
    GetDlgItem(IDC_START_POS)->GetWindowText(strStartPos);

    if( strStartPos.IsEmpty() )
    {
        AfxMessageBox(L"Specify the position to start outputting cases");
        return;
    }

    int startPos = _ttoi(strStartPos);

    if( startPos <= 0 )
    {
        AfxMessageBox(L"Enter a valid number for the position to start outputting cases");
        return;
    }


    DataFileDlg source_data_file_dlg(DataFileDlg::Type::OpenExisting, true);
    source_data_file_dlg.SetTitle(L"Select Original Data Source(s)")
                        .SetDictionaryFilePath(m_pDict->GetFilePath())
                        .AllowMultipleSelections();

    if( source_data_file_dlg.DoModal() != IDOK )
        return;

    // suggest a filename based on the input filename
    ConnectionString suggested_sample_connection_string;

    if( source_data_file_dlg.GetConnectionStrings().size() == 1 )
        suggested_sample_connection_string = PathHelpers::AppendToConnectionStringFilename(source_data_file_dlg.GetConnectionString(), "_sample");

    DataFileDlg sample_data_file_dlg(DataFileDlg::Type::CreateNew, false, suggested_sample_connection_string);
    sample_data_file_dlg.SetTitle(L"Select Sample Data File")
                        .SetDictionaryFilePath(m_pDict->GetFilePath())
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
        {
            return ( random_number_generator(random_engine) <= percent );
        }

        else
        {
            if( --down_counter == 0 )
            {
                down_counter = 100 / percent;
                return true;
            }

            else
            {
                return false;
            }
        }
    };

    // simply read cases from one repository and write them to another (for text repositories this will remove erased records)
    const CaseIteratorRoutine case_iterator_routine
    {
        source_data_file_dlg.GetConnectionStrings(),
        DataRepositoryAccess::BatchInput,
        sample_data_file_dlg.GetConnectionString(),
        false,
        &should_write_case_callback
    };

    RunCaseIteratorRoutine(case_iterator_routine, "Sampl");
}


void DictionaryMacrosDlg::OnBnClickedCreateNotesDictionary()
{
    constexpr std::string_view NamePrefix_sv = "NOTES_";

    SaveFileDlg save_file_dlg(0, FileExtensions::Dictionary, nullptr, L"Notes Dictionary File (*.dcf)|*.dcf||", this);

    if( save_file_dlg.DoModal() != IDOK )
        return;

    CDataDict notes_dictionary;
    notes_dictionary.SetName(SO::Concatenate(NamePrefix_sv, m_pDict->GetName()));
    notes_dictionary.SetLabel(m_pDict->GetLabel() + L" (Notes Dictionary)");
    notes_dictionary.SetPosRelative(true);
    notes_dictionary.SetRecTypeLen(0);
    notes_dictionary.SetRecTypeStart(0);

    const DictLevel& source_dict_level = m_pDict->GetLevel(0);

    DictLevel notes_dict_level;
    notes_dict_level.SetName(SO::Concatenate(NamePrefix_sv, source_dict_level.GetName()));
    notes_dict_level.SetLabel(source_dict_level.GetLabel() + L" (Notes Level)");

    CDictRecord notes_dict_record;
    notes_dict_record.SetName("NOTES_REC");
    notes_dict_record.SetLabel(m_pDict->GetLabel() + L" (Notes Record)");

    int iItemPos = 1;

    // add the ID items
    std::vector<const CDictItem*> id_items = m_pDict->GetIdItems();

    CDictRecord& id_dict_record = *notes_dict_level.GetIdItemsRec();

    for( size_t i = 0; i < id_items.size(); ++i )
    {
        id_dict_record.AddItem(id_items[i]);
        id_dict_record.GetItem(i)->SetStart(iItemPos);
        iItemPos += id_items[i]->GetLen();
    }

    // create a value set with the names of all of the fields
    DictValueSet field_name_value_set;

    for( const DictLevel& dict_level : m_pDict->GetLevels() )
    {
        for( int r = -1; r < dict_level.GetNumRecords(); ++r )
        {
            const CDictRecord& dict_record = ( r == -1 ) ? *dict_level.GetIdItemsRec() :
                                                           *dict_level.GetRecord(r);

            for( int i = 0; i < dict_record.GetNumItems(); ++i )
            {
                const CDictItem& dict_item = *dict_record.GetItem(i);
                std::string field_name = NameShortener::Shorten(dict_item.GetName(), TextRepositoryNotesFile::FieldLength);
                SO::WideMakeExactLength(field_name, TextRepositoryNotesFile::FieldLength);

                DictValue dict_value;
                dict_value.SetLabel(UTF8_TODO::GetCString(dict_item.GetName()));
                dict_value.AddValuePair(DictValuePair(std::move(field_name)));

                field_name_value_set.AddValue(std::move(dict_value));
            }
        }
    }

    // add the note items, all but the note itself as ID items
    CDictItem notes_dict_item;

    notes_dict_item.SetName("NOTES_FIELD");
    notes_dict_item.SetLabel(L"Note Field Name");
    notes_dict_item.SetContentType(ContentType::Alpha);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::FieldLength);

    field_name_value_set.SetName(notes_dict_item.GetName() + "_VS");
    field_name_value_set.SetLabel(notes_dict_item.GetLabel());
    notes_dict_item.AddValueSet(std::move(field_name_value_set));

    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.RemoveAllValueSets();

    notes_dict_item.SetName("NOTES_OPERATOR_ID");
    notes_dict_item.SetLabel(L"Note Operator ID");
    notes_dict_item.SetContentType(ContentType::Alpha);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::OperatorIdLength);
    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName("NOTES_MODIFIED_DATE");
    notes_dict_item.SetLabel(L"Note Modified Date");
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::ModifiedDateLength);
    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName("NOTES_MODIFIED_TIME");
    notes_dict_item.SetLabel(L"Note Modified Time");
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::ModifiedTimeLength);
    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName("NOTES_RECORD_OCC");
    notes_dict_item.SetLabel(L"Note Record Occurrence");
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::OccurrenceLength);
    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName("NOTES_ITEM_OCC");
    notes_dict_item.SetLabel(L"Note Item Occurrence");
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::OccurrenceLength);
    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName("NOTES_SUBITEM_OCC");
    notes_dict_item.SetLabel(L"Note Subitem Occurrence");
    notes_dict_item.SetContentType(ContentType::Numeric);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(TextRepositoryNotesFile::OccurrenceLength);
    id_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_item.SetName("NOTES_NOTE");
    notes_dict_item.SetLabel(L"Note");
    notes_dict_item.SetContentType(ContentType::Alpha);
    notes_dict_item.SetStart(iItemPos);
    notes_dict_item.SetLen(MAX_ALPHA_ITEM_LEN);
    notes_dict_record.AddItem(&notes_dict_item);
    iItemPos += notes_dict_item.GetLen();

    notes_dict_record.SetRecLen(iItemPos - 1);

    notes_dict_level.AddRecord(&notes_dict_record);
    notes_dictionary.AddLevel(std::move(notes_dict_level));

    try
    {
        notes_dictionary.Save(save_file_dlg.GetFilePath());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
