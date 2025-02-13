#include "StdAfx.h"
#include "CapiMacrosDlg.h"
#include <zUtilF/TextReportDlg.h>
#include <zFormO/FormFile.h>
#include <zFormO/FormFileIterator.h>


IMPLEMENT_DYNAMIC(CapiMacrosDlg, CDialog)

CapiMacrosDlg::CapiMacrosDlg(CAplDoc* pAplDoc, CWnd* pParent /*=NULL*/)
    :   CDialog(CapiMacrosDlg::IDD, pParent),
        m_pAplDoc(pAplDoc)
{
}


BEGIN_MESSAGE_MAP(CapiMacrosDlg, CDialog)
    ON_BN_CLICKED(IDC_CAPI_AUDIT_UNDEFINED_TEXT, &OnBnClickedAuditUndefinedText)
    ON_BN_CLICKED(IDC_CAPI_REMOVE_UNUSED_TEXT, &OnBnClickedRemoveUnusedText)
    ON_BN_CLICKED(IDC_CAPI_INITIALIZE_FROM_DICTIONARY_LABEL, &OnBnClickedInitializeFromDictionaryLabel)
    ON_BN_CLICKED(IDC_PASTE_FROM_CLIPBOARD, &OnBnClickedPasteFromClipboard)
END_MESSAGE_MAP()


namespace
{
    CString GetCapiFieldName(const CDEItemBase* pItemBase)
    {
        if( pItemBase->isA(CDEFormBase::eItemType::Block) )
            return pItemBase->GetName();

        else if( pItemBase->isA(CDEFormBase::eItemType::Field) )
        {
            // field names are preceeded by the dictionary name
            const CDEField* pField = (const CDEField*)pItemBase;
            return FormatText(_T("%s.%s"), (LPCTSTR)pField->GetItemDict(), (LPCTSTR)pField->GetName());
        }

        ASSERT(false);
        throw ProgrammingErrorException();
    }
}


CString CapiMacrosDlg::ConstructHtmlFromText(wstring_view text)
{
    return WS2CS(_T("<p>") + Encoders::ToHtml(text) + _T("</p>"));
}


int CapiMacrosDlg::IterateThroughBlocksAndFields(std::function<void(CDEItemBase*, const CDataDict*)>& callback_function,
    bool include_blocks, bool include_protected_fields, bool only_include_undefined_text_entities)
{
    int number_entities = 0;

    std::function<void(CDEItemBase*, const CDataDict*)> find_text_function =
        [this, &number_entities, callback_function, include_blocks, include_protected_fields,
        only_include_undefined_text_entities](CDEItemBase* pItemBase, const CDataDict* pDataDict) -> void
    {
        if( only_include_undefined_text_entities && m_pAplDoc->IsQHAvailable(pItemBase) )
            return;

        if( !include_blocks && pItemBase->isA(CDEFormBase::eItemType::Block) )
            return;

        if( !include_protected_fields && pItemBase->isA(CDEFormBase::eItemType::Field) && ((CDEField*)pItemBase)->IsProtected() )
            return;

        number_entities++;

        callback_function(pItemBase, pDataDict);
    };

    for( const auto& form_file : m_pAplDoc->GetAppObject().GetRuntimeFormFiles() )
        FormFileIterator::Iterator(FormFileIterator::Iterator::IterateOverType::BlockField, form_file.get(), find_text_function).Iterate();

    return number_entities;
}


void CapiMacrosDlg::OnBnClickedAuditUndefinedText()
{
    bool include_blocks = ( ((CButton*)GetDlgItem(IDC_CAPI_INCLUDE_BLOCKS))->GetCheck() == BST_CHECKED );
    bool include_protected_fields = ( ((CButton*)GetDlgItem(IDC_CAPI_INCLUDE_PROTECTED_FIELDS))->GetCheck() == BST_CHECKED );
    CString fields_with_undefined_text;

    std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [&fields_with_undefined_text](CDEItemBase* pItemBase, const CDataDict*) -> void
    {
        fields_with_undefined_text.AppendFormat(_T("%s\r\n"), (LPCTSTR)GetCapiFieldName(pItemBase));
    };

    int number_fields_with_undefined_text = IterateThroughBlocksAndFields(callback_function, include_blocks, include_protected_fields, true);

    if( number_fields_with_undefined_text == 0 )
    {
        CString message;
        message.Format(_T("All %sfields have question text in at least one language."), include_blocks ? _T("blocks and ") : _T(""));
        AfxMessageBox(message);
    }

    else
    {
        CString block_heading;

        if( include_blocks )
            block_heading.Format(_T("block%s or "), PluralizeWord(number_fields_with_undefined_text));

        CString heading;
        heading.Format(_T("There are %d %sfield%s with undefined question text:"),
            number_fields_with_undefined_text, (LPCTSTR)block_heading, PluralizeWord(number_fields_with_undefined_text));

        TextReportDlg text_report_dialog(heading, fields_with_undefined_text);
        text_report_dialog.DoModal();
    }
}


void CapiMacrosDlg::OnBnClickedRemoveUnusedText()
{
    std::set<CString> all_blocks_fields;

    std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [this, &all_blocks_fields](CDEItemBase* pItemBase, const CDataDict*) -> void
    {
        all_blocks_fields.insert(GetCapiFieldName(pItemBase));
    };

    IterateThroughBlocksAndFields(callback_function, true, true, false);

    int number_unused_fields = 0;
    CString unused_fields_text;

    std::vector<CapiQuestion> used_capi_questions;

    for( const CapiQuestion& question : m_pAplDoc->m_pQuestMgr->GetQuestions() )
    {
        if( all_blocks_fields.find(question.GetItemName()) == all_blocks_fields.end() )
        {
            unused_fields_text.AppendFormat(_T("%s\r\n"), (LPCTSTR)question.GetItemName());
            number_unused_fields++;
            m_pAplDoc->m_pQuestMgr->RemoveQuestion(question.GetItemName());
        }
    }

    if( number_unused_fields == 0 )
        AfxMessageBox(_T("All question text is associated with a block or field."));

    else
    {

        CString heading;
        heading.Format(_T("Unused question text was removed for the following %d nonexistent block%s or field%s:"),
            number_unused_fields, PluralizeWord(number_unused_fields), PluralizeWord(number_unused_fields));

        TextReportDlg text_report_dialog(heading, unused_fields_text);
        text_report_dialog.DoModal();
    }
}

void CapiMacrosDlg::OnBnClickedInitializeFromDictionaryLabel()
{
    CString fields_with_added_text;

    std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [this, &fields_with_added_text](CDEItemBase* pItemBase, const CDataDict* pDataDict) -> void
    {
        fields_with_added_text.AppendFormat(_T("%s\r\n"), (LPCTSTR)GetCapiFieldName(pItemBase));

        // first set the text for the main language
        const CDictItem* pDictItem = ((CDEField*)pItemBase)->GetDictItem();
        const auto& labels = pDictItem->GetLabelSet().GetLabels();

        m_pAplDoc->SetCapiTextForAllConditions(pItemBase, ConstructHtmlFromText(labels[0]));

        // then override the text for any additional languages
        for( size_t i = 1; i < labels.size(); ++i )
        {
            // don't set the text (an expensive operation) unless the language is different
            if( labels[i] != labels[0] )
            {
                m_pAplDoc->SetCapiTextForAllConditions(pItemBase, ConstructHtmlFromText(labels[i]),
                    pDataDict->GetLanguages()[i].GetName());
            }
        }
    };

    int number_fields_with_added_text = IterateThroughBlocksAndFields(callback_function, false, true, true);

    if( number_fields_with_added_text == 0 )
        AfxMessageBox(_T("All fields have question text in at least one language."));

    else
    {

        CString heading;
        heading.Format(_T("Dictionary labels were added as the question text for the following %d field%s:"),
            number_fields_with_added_text, PluralizeWord(number_fields_with_added_text));

        TextReportDlg text_report_dialog(heading, fields_with_added_text);
        text_report_dialog.DoModal();
    }
}


void CapiMacrosDlg::OnBnClickedPasteFromClipboard()
{
    // get the list of all the possible blocks and fields
    std::map<CString, CDEItemBase*> all_blocks_fields;

    std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [this, &all_blocks_fields](CDEItemBase* pItemBase, const CDataDict*) -> void
    {
        all_blocks_fields.insert(std::make_pair(GetCapiFieldName(pItemBase), pItemBase));
    };

    IterateThroughBlocksAndFields(callback_function, true, true, false);

    // get the possible prefixes to the field names
    std::vector<CString> dictionary_prefixes;
    dictionary_prefixes.push_back(_T(""));

    for( const auto& form_file : m_pAplDoc->GetAppObject().GetRuntimeFormFiles() )
        dictionary_prefixes.push_back(form_file->GetDictionary()->GetName() + _T("."));

    // process the clipboard contents
    enum class ProcessingStep { InvalidLine, InvalidField, InvalidLanguage, Success };
    ProcessingStep processing_step = ProcessingStep::InvalidLine;
    CString processing_buffers[4];
    int lines_processed = 0;

    CString clipboard_text = WS2CS(WinClipboard::GetText(this));
    clipboard_text.Trim();

    while( !clipboard_text.IsEmpty() )
    {
        // get the next line
        CString line;
        int line_end_position = clipboard_text.FindOneOf(_T("\r\n"));

        // there are additional lines
        if( line_end_position >= 0 )
        {
            line = clipboard_text.Left(line_end_position);
            line.TrimRight();

            clipboard_text = clipboard_text.Mid(line_end_position + 1);
            clipboard_text.TrimLeft();
        }

        // or we're on the last line
        else
        {
            line = clipboard_text;
            clipboard_text.Empty();
        }

        // skip blank lines
        if( line.IsEmpty() )
            continue;

        // now parse the line, throwing errors if necessary
        CString capi_field_name;
        CString language_name;
        CString question_text;
        CString message_for_processing_buffer = line;

        try
        {
            processing_step = ProcessingStep::InvalidLine;

            int tab_position1 = line.Find(_T('\t'));

            if( tab_position1 < 0 )
                throw std::exception();

            capi_field_name = line.Left(tab_position1);
            capi_field_name.TrimRight();
            capi_field_name.MakeUpper();

            message_for_processing_buffer = capi_field_name;

            int tab_position2 = line.Find(_T('\t'), tab_position1 + 1);

            if( tab_position2 >= 0 )
            {
                language_name = line.Mid(tab_position1 + 1, tab_position2 - tab_position1 - 1);
                language_name.Trim();
                language_name.MakeUpper();

                message_for_processing_buffer.AppendFormat(_T("(%s)"), (LPCTSTR)language_name);

                tab_position1 = tab_position2;
            }

            question_text = line.Mid(tab_position1 + 1);
            question_text.TrimLeft();


            // check that the name is a block or field
            processing_step = ProcessingStep::InvalidField;

            CDEItemBase* pItemBase = nullptr;

            // if the name isn't valid, preface it with each dictionary name and check
            for( const CString& dictionary_prefix : dictionary_prefixes )
            {
                CString capi_field_name_for_testing = dictionary_prefix + capi_field_name;

                auto map_lookup = all_blocks_fields.find(capi_field_name_for_testing);

                if( map_lookup != all_blocks_fields.end() )
                {
                    pItemBase = map_lookup->second;
                    break;
                }
            }

            if( pItemBase == nullptr )
                throw std::exception();


            // if a language is specified, check that it is valid
            processing_step = ProcessingStep::InvalidLanguage;

            if( !language_name.IsEmpty() )
            {
                if(std::find_if(m_pAplDoc->m_pQuestMgr->GetLanguages().cbegin(),
                                m_pAplDoc->m_pQuestMgr->GetLanguages().cend(),
                                [&](const Language& l) { return SO::EqualsNoCase(language_name, l.GetName()); } ) == m_pAplDoc->m_pQuestMgr->GetLanguages().cend())
                {
                    throw std::exception();
                }
            }


            // add or modify the question text
            processing_step = ProcessingStep::Success;

            m_pAplDoc->SetCapiTextForAllConditions(pItemBase, ConstructHtmlFromText(question_text), language_name);
        }

        catch(...)
        {
        }

        // add the message to the appropriate buffer
        lines_processed++;
        processing_buffers[(int)processing_step].AppendFormat(_T("    %s\r\n"), (LPCTSTR)message_for_processing_buffer);
    }


    if( lines_processed == 0 )
        AfxMessageBox(_T("No suitable content found on the clipboard"));

    else
    {
        CString heading;
        heading.Format(_T("%d line%s of text from the clipboard processed:"),
            lines_processed, PluralizeWord(lines_processed));

        CString clipboard_paste_report;

        for( int i = 0; i < _countof(processing_buffers); i++ )
        {
            if( !processing_buffers[i].IsEmpty() )
            {
                CString processing_buffer_header =
                    ( i == 0 ) ? _T("Lines that could not be processed") :
                    ( i == 1 ) ? _T("Invalid block or field names") :
                    ( i == 2 ) ? _T("Invalid language names") :
                    _T("Blocks or fields whose question text was successfully added or modified");

                clipboard_paste_report.AppendFormat(_T("%s%s:\r\n%s"),
                    clipboard_paste_report.IsEmpty() ? _T("") : _T("\r\n"),
                    (LPCTSTR)processing_buffer_header,
                    (LPCTSTR)processing_buffers[i]);
            }
        }

        TextReportDlg text_report_dialog(heading, clipboard_paste_report);
        text_report_dialog.DoModal();
    }
}
