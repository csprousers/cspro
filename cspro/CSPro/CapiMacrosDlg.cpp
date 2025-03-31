#include "StdAfx.h"
#include "CapiMacrosDlg.h"
#include <zUtilF/TextReportDlg.h>
#include <zFormO/FormFile.h>
#include <zFormO/FormFileIterator.h>
#include <zCapiO/CapiName.h>


BEGIN_MESSAGE_MAP(CapiMacrosDlg, CDialog)
    ON_BN_CLICKED(IDC_CAPI_AUDIT_UNDEFINED_TEXT, &OnBnClickedAuditUndefinedText)
    ON_BN_CLICKED(IDC_CAPI_REMOVE_UNUSED_TEXT, &OnBnClickedRemoveUnusedText)
    ON_BN_CLICKED(IDC_CAPI_INITIALIZE_FROM_DICTIONARY_LABEL, &OnBnClickedInitializeFromDictionaryLabel)
    ON_BN_CLICKED(IDC_PASTE_FROM_CLIPBOARD, &OnBnClickedPasteFromClipboard)
END_MESSAGE_MAP()


CapiMacrosDlg::CapiMacrosDlg(CAplDoc* const pAplDoc, CWnd* const pParent /* = nullptr*/)
    :   CDialog(IDD_CAPI_MACROS, pParent),
        m_pAplDoc(pAplDoc)
{
}


std::string CapiMacrosDlg::ConstructHtmlFromText(const std::string_view text_sv)
{
    return "<p>" + Encoders::ToHtml(text_sv) + "</p>";
}


int CapiMacrosDlg::IterateThroughBlocksAndFields(const std::function<void(CDEItemBase*, const CDataDict*)>& callback_function,
                                                 const bool include_blocks,
                                                 const bool include_protected_fields,
                                                 const bool only_include_undefined_text_entities)
{
    int number_entities = 0;

    const std::function<void(CDEItemBase*, const CDataDict*)> find_text_function =
        [&](CDEItemBase* const item_base, const CDataDict* const dictionary)
    {
        if( only_include_undefined_text_entities && m_pAplDoc->IsQHAvailable(item_base) )
            return;

        if( !include_blocks && item_base->isA(CDEFormBase::eItemType::Block) )
            return;

        if( !include_protected_fields && item_base->isA(CDEFormBase::eItemType::Field) && assert_cast<const CDEField*>(item_base)->IsProtected() )
            return;

        ++number_entities;

        callback_function(item_base, dictionary);
    };

    for( const std::shared_ptr<CDEFormFile>& form_file : m_pAplDoc->GetAppObject().GetRuntimeFormFiles() )
        FormFileIterator::Iterator(FormFileIterator::Iterator::IterateOverType::BlockField, form_file.get(), find_text_function).Iterate();

    return number_entities;
}


void CapiMacrosDlg::OnBnClickedAuditUndefinedText()
{
    bool include_blocks = ( static_cast<CButton*>(GetDlgItem(IDC_CAPI_INCLUDE_BLOCKS))->GetCheck() == BST_CHECKED );
    bool include_protected_fields = ( static_cast<CButton*>(GetDlgItem(IDC_CAPI_INCLUDE_PROTECTED_FIELDS))->GetCheck() == BST_CHECKED );
    std::string fields_with_undefined_text;

    const std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [&](CDEItemBase* const item_base, const CDataDict* /*dictionary*/)
        {
            SO::AppendWithSeparator(fields_with_undefined_text, CapiName::Create(item_base), SO::Newline_crlf_sv);
        };

    const int number_fields_with_undefined_text = IterateThroughBlocksAndFields(callback_function, include_blocks, include_protected_fields, true);

    if( number_fields_with_undefined_text == 0 )
    {
        AfxMessageBox(FormatText("All %sfields have question text in at least one language.", include_blocks ? "blocks and " : ""));
        return;
    }

    std::string heading = FormatText("There are %d %sfield%s with undefined question text:",
                                     number_fields_with_undefined_text,
                                     include_blocks ? FormatText("block%s or ", PluralizeWord(number_fields_with_undefined_text)).c_str() : "",
                                     PluralizeWord(number_fields_with_undefined_text));

    TextReportDlg text_report_dialog(std::move(heading), std::move(fields_with_undefined_text));
    text_report_dialog.DoModal();
}


void CapiMacrosDlg::OnBnClickedRemoveUnusedText()
{
    std::set<std::string> all_blocks_fields;

    const std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [&](CDEItemBase* const item_base, const CDataDict* /*dictionary*/)
        {
            all_blocks_fields.insert(CapiName::Create(item_base));
        };

    IterateThroughBlocksAndFields(callback_function, true, true, false);

    int number_unused_fields = 0;
    std::string unused_fields_text;

    std::vector<CapiQuestion> used_capi_questions;

    for( const CapiQuestion& question : m_pAplDoc->m_questionManager->GetQuestions() )
    {
        if( all_blocks_fields.find(question.GetItemName()) == all_blocks_fields.end() )
        {
            SO::AppendWithSeparator(unused_fields_text, question.GetItemName(), SO::Newline_crlf_sv);
            ++number_unused_fields;
            m_pAplDoc->m_questionManager->RemoveQuestion(question.GetItemName());
        }
    }

    if( number_unused_fields == 0 )
    {
        AfxMessageBox(L"All question text is associated with a block or field.");
        return;
    }

    std::string heading = FormatText("Unused question text was removed for the following %d nonexistent block%s or field%s:",
                                     number_unused_fields, PluralizeWord(number_unused_fields), PluralizeWord(number_unused_fields));

    TextReportDlg text_report_dialog(std::move(heading), std::move(unused_fields_text));
    text_report_dialog.DoModal();
}


void CapiMacrosDlg::OnBnClickedInitializeFromDictionaryLabel()
{
    std::string fields_with_added_text;

    const std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [&](CDEItemBase* const item_base, const CDataDict* const dictionary)
        {
            SO::AppendWithSeparator(fields_with_added_text, CapiName::Create(item_base), SO::Newline_crlf_sv);

            // first set the text for the main language
            const CDictItem* const dict_item = assert_cast<const CDEField*>(item_base)->GetDictItem();
            const std::vector<CString>& labels = dict_item ->GetLabelSet().GetLabels();

            m_pAplDoc->SetCapiTextForAllConditions(item_base, ConstructHtmlFromText(UTF8_TODO::GetUtf8(labels.front())));

            // then override the text for any additional languages
            for( size_t i = 1; i < labels.size(); ++i )
            {
                // don't set the text (an expensive operation) unless the language is different
                if( labels[i] != labels[0] )
                {
                    m_pAplDoc->SetCapiTextForAllConditions(item_base, ConstructHtmlFromText(UTF8_TODO::GetUtf8(labels[i])),
                                                           dictionary->GetLanguages()[i].GetName());
                }
            }
        };

    const int number_fields_with_added_text = IterateThroughBlocksAndFields(callback_function, false, true, true);

    if( number_fields_with_added_text == 0 )
    {
        AfxMessageBox(L"All fields have question text in at least one language.");
        return;
    }

    std::string heading = FormatText("Dictionary labels were added as the question text for the following %d field%s:",
                                     number_fields_with_added_text, PluralizeWord(number_fields_with_added_text));

    TextReportDlg text_report_dialog(std::move(heading), std::move(fields_with_added_text));
    text_report_dialog.DoModal();
}


void CapiMacrosDlg::OnBnClickedPasteFromClipboard()
{
    // get the list of all the possible blocks and fields
    std::map<std::string, CDEItemBase*> all_blocks_fields;

    const std::function<void(CDEItemBase*, const CDataDict*)> callback_function =
        [&](CDEItemBase* const item_base, const CDataDict* /*dictionary*/)
        {
            all_blocks_fields.try_emplace(CapiName::Create(item_base), item_base);
        };

    IterateThroughBlocksAndFields(callback_function, true, true, false);

    // get the possible prefixes to the field names
    std::vector<std::string> dictionary_prefixes = { std::string() };

    for( const std::shared_ptr<CDEFormFile>& form_file : m_pAplDoc->GetAppObject().GetRuntimeFormFiles() )
        dictionary_prefixes.emplace_back(form_file->GetDictionary()->GetName() + ".");

    // process the clipboard contents
    enum class ProcessingStep { InvalidLine, InvalidField, InvalidLanguage, Success };
    ProcessingStep processing_step;
    std::string processing_buffers[4];
    int lines_processed = 0;

    std::string clipboard_text = WinClipboard::GetText<std::string>(this);
    SO::MakeTrim(clipboard_text);

    SO::ForeachLine<SharableString>(clipboard_text, false,
        [&](const SharableString line)
        {
            // now parse the line, throwing errors if necessary
            std::vector<std::string> components = SO::SplitString(*line, '\t', true);

            SharableString message_for_processing_buffer = line;

            try
            {
                if( components.size() < 2 )
                    throw ProcessingStep::InvalidLine;

                std::string capi_field_name = std::move(components.front());
                SO::MakeTrim(capi_field_name);
                SO::MakeUpper(capi_field_name);

                message_for_processing_buffer = capi_field_name;

                std::optional<std::string> language_name;

                if( components.size() > 2 )
                {
                    language_name = std::move(components[1]);
                    SO::MakeTrim(*language_name);
                    SO::MakeUpper(*language_name);

                    message_for_processing_buffer.MakeModifiable().append(FormatText("(%s)", language_name->c_str()));
                }

                std::string question_text = std::move(components[language_name.has_value() ? 2 : 1]);
                SO::MakeTrim(question_text);


                // check that the name is a block or field
                CDEItemBase* item_base = nullptr;

                // if the name isn't valid, preface it with each dictionary name and check
                for( const std::string& dictionary_prefix : dictionary_prefixes )
                {
                    const std::string capi_field_name_for_testing = dictionary_prefix + capi_field_name;
                    const auto& lookup = all_blocks_fields.find(capi_field_name_for_testing);

                    if( lookup != all_blocks_fields.end() )
                    {
                        item_base = lookup->second;
                        break;
                    }
                }

                if( item_base == nullptr )
                    throw ProcessingStep::InvalidField;


                // if a language is specified, check that it is valid
                if( language_name.has_value() )
                {
                    const auto& lookup = std::find_if(m_pAplDoc->m_questionManager->GetLanguages().cbegin(),
                                                      m_pAplDoc->m_questionManager->GetLanguages().cend(),
                                                      [&](const Language& l) { return SO::EqualsNoCase(*language_name, l.GetName()); });

                    if( lookup == m_pAplDoc->m_questionManager->GetLanguages().cend() )
                        throw ProcessingStep::InvalidLanguage;
                }


                // add or modify the question text
                m_pAplDoc->SetCapiTextForAllConditions(item_base,
                                                       ConstructHtmlFromText(question_text),
                                                       language_name.has_value() ? *language_name : SO::Empty_string);

                processing_step = ProcessingStep::Success;
            }

            catch( const ProcessingStep error_processing_step )
            {
                processing_step = error_processing_step;
            }

            // add the message to the appropriate buffer
            ++lines_processed;
            processing_buffers[static_cast<size_t>(processing_step)].append("    ")
                                                                    .append(*message_for_processing_buffer)
                                                                    .append(SO::Newline_crlf_sv);
        });


    if( lines_processed == 0 )
    {
        AfxMessageBox(L"No suitable content found on the clipboard");
        return;
    }

    std::string heading = FormatText("%d line%s of text from the clipboard processed:",
                                        lines_processed, PluralizeWord(lines_processed));

    std::string clipboard_paste_report;

    for( int i = 0; i < _countof(processing_buffers); ++i )
    {
        if( !processing_buffers[i].empty() )
        {
            if( !clipboard_paste_report.empty() )
                clipboard_paste_report.append(SO::Newline_crlf_sv);

            clipboard_paste_report.append(( i == 0 ) ? "Lines that could not be processed:" :
                                          ( i == 1 ) ? "Invalid block or field names:" :
                                          ( i == 2 ) ? "Invalid language names:" :
                                                       "Blocks or fields whose question text was successfully added or modified:");

            clipboard_paste_report.append(SO::Newline_crlf_sv)
                                  .append(processing_buffers[i]);
        }
    }

    TextReportDlg text_report_dialog(std::move(heading), std::move(clipboard_paste_report));
    text_report_dialog.DoModal();
}
