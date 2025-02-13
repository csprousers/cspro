#include "StdAfx.h"
#include "HtmlDialogTemplatesDlg.h"
#include <zToolsO/FileIO.h>
#include <zUtilF/DynamicLayoutControlResizer.h>
#include <zUtilF/HtmlDialogFunctionRunner.h>


BEGIN_MESSAGE_MAP(HtmlDialogTemplatesDlg, CDialog)
    ON_WM_SIZE()
    ON_NOTIFY(TVN_SELCHANGED, IDC_SAMPLES_TREE, OnSampleSelectionChanged)
    ON_COMMAND(IDC_SHOW_DIALOG, OnShowDialog)
    ON_COMMAND(IDC_REPLACE_HTML, OnReplaceHtml)
    ON_COMMAND(IDC_REPLACE_INPUT, OnReplaceInput)
    ON_MESSAGE(UWM::CSCode::ActiveDocChanged, OnActiveDocChanged)
END_MESSAGE_MAP()


HtmlDialogTemplatesDlg::HtmlDialogTemplatesDlg(const CDocument* active_doc, CWnd* pParent)
    :   CDialog(IDD_HTML_DIALOG_TEMPLATES, pParent),
        m_initialSamplePairToSelect(nullptr),
        m_selectedSamplePair(nullptr),
        m_activeDocIsHtmlDialog(true)
{
    // get the usable samples
    for( const HtmlDialogTemplate& html_dialog_template : m_htmlDialogTemplateFile.GetTemplates() )
    {
        if( !html_dialog_template.description.empty() && !html_dialog_template.subdescription.empty() )
        {
            for( const HtmlDialogTemplate::Sample& sample : html_dialog_template.samples )
            {
                if( !sample.description.empty() )
                    m_usableSamples.emplace_back(SamplePair { html_dialog_template, sample });
            }
        }
    }

    if( m_usableSamples.empty() )
        throw CSProException("No usable HTML dialog templates are present.");


    // set the initial selection based on the current document
    if( active_doc != nullptr )
    {
        std::wstring doc_filename = PortableFunctions::PathGetFilename(active_doc->GetPathName());

        const auto& lookup = std::find_if(m_usableSamples.cbegin(), m_usableSamples.cend(),
            [&](const SamplePair& sample_pair)
            {
                return SO::EqualsNoCase(sample_pair.dialog_template.filename, doc_filename);
            });

        if( lookup != m_usableSamples.cend() )
            m_initialSamplePairToSelect = &(*lookup);
    }


    // create the dialog
    if( !Create(IDD_HTML_DIALOG_TEMPLATES) )
        throw ProgrammingErrorException();
}


void HtmlDialogTemplatesDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_SAMPLES_TREE, m_samplesTreeCtrl);
    DDX_Control(pDX, IDC_FILENAME, m_filenameCaption);
    DDX_Control(pDX, IDC_DESCRIPTION, m_descriptionCaption);
    DDX_Control(pDX, IDC_JSON_INPUT, m_inputLogicCtrl);
    DDX_Control(pDX, IDC_SHOW_DIALOG, m_showDialogButton);
    DDX_Control(pDX, IDC_REPLACE_HTML, m_replaceHtmlButton);
    DDX_Control(pDX, IDC_REPLACE_INPUT, m_replaceInputButton);
}


BOOL HtmlDialogTemplatesDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    // populate the samples tree
    HTREEITEM initial_node_to_select = BuildSamplesTree();

    // set up the read-only Scintilla control to show the JSON input text
    m_inputLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_JSON);

    // select the initial node  
    m_samplesTreeCtrl.SelectItem(initial_node_to_select);

    return TRUE;
}


HTREEITEM HtmlDialogTemplatesDlg::BuildSamplesTree()
{
    std::optional<HTREEITEM> initial_node_to_select;
    const HtmlDialogTemplate* previous_dialog_template = nullptr;
    HTREEITEM dialog_template_node = TVI_ROOT;

    TVINSERTSTRUCT tvi;
    tvi.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvi.hInsertAfter = nullptr;

    for( const SamplePair& sample_pair : m_usableSamples )
    {
        auto add_tree_node = [&](HTREEITEM parent_node, const std::wstring& text)
        {
            tvi.hParent = parent_node;
            tvi.item.pszText = const_cast<TCHAR*>(text.c_str());
            tvi.item.cchTextMax = text.length();
            tvi.item.lParam = reinterpret_cast<LPARAM>(&sample_pair);

            return m_samplesTreeCtrl.InsertItem(&tvi);
        };

        if( &sample_pair.dialog_template != previous_dialog_template )
        {
            dialog_template_node = add_tree_node(TVI_ROOT, sample_pair.dialog_template.description);

            if( !initial_node_to_select.has_value() )
                initial_node_to_select = dialog_template_node;

            previous_dialog_template = &sample_pair.dialog_template;
        }

        HTREEITEM sample_node = add_tree_node(dialog_template_node, sample_pair.sample.description);

        if( m_initialSamplePairToSelect == &sample_pair )
            initial_node_to_select = sample_node;
    }        

    ASSERT(initial_node_to_select.has_value());

    return *initial_node_to_select;
}


void HtmlDialogTemplatesDlg::PostNcDestroy()
{
    assert_cast<CMainFrame*>(AfxGetMainWnd())->PostMessage(UWM::CSCode::ModelessDialogDestroyed, reinterpret_cast<WPARAM>(m_hWnd));

    __super::PostNcDestroy();
}


void HtmlDialogTemplatesDlg::OnSize(UINT nType, int cx, int cy)
{
    // the Scintilla control doesn't seem to respond to Dynamic Layout settings
    if( m_dynamicLayoutControlResizer == nullptr )
        m_dynamicLayoutControlResizer = std::make_unique<DynamicLayoutControlResizer>(*this, std::initializer_list<CWnd*>{ &m_inputLogicCtrl });

    __super::OnSize(nType, cx, cy);

    m_dynamicLayoutControlResizer->OnSize(cx, cy);
}


void HtmlDialogTemplatesDlg::OnCancel()
{   
    DestroyWindow();
}


void HtmlDialogTemplatesDlg::OnSampleSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);

    m_selectedSamplePair = reinterpret_cast<const SamplePair*>(pNMTreeView->itemNew.lParam);
    ASSERT(m_selectedSamplePair != nullptr);

    // show the full filename in the dialogs folder (if it exists)
    m_selectedDialogTemplatePath = PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Dialogs),
                                                                       m_selectedSamplePair->dialog_template.filename);

    if( !PortableFunctions::FileIsRegular(*m_selectedDialogTemplatePath) )
        m_selectedDialogTemplatePath.reset();

    m_filenameCaption.SetWindowText(( m_selectedDialogTemplatePath.has_value() ? *m_selectedDialogTemplatePath:
                                                                                 m_selectedSamplePair->dialog_template.filename ).c_str());
    m_descriptionCaption.SetWindowText(m_selectedSamplePair->dialog_template.subdescription.c_str());
    m_inputLogicCtrl.SetReadOnlyText(m_selectedSamplePair->sample.input);

    m_showDialogButton.EnableWindow(m_selectedDialogTemplatePath.has_value());
    m_replaceHtmlButton.EnableWindow(m_activeDocIsHtmlDialog && m_selectedDialogTemplatePath.has_value());

    // expand the current node (in case they selected a dialog template and not the sample)
    m_samplesTreeCtrl.Expand(pNMTreeView->itemNew.hItem, TVE_EXPAND);

    *pResult = 0;
}


void HtmlDialogTemplatesDlg::OnShowDialog()
{
    ASSERT(m_selectedSamplePair != nullptr && m_selectedDialogTemplatePath.has_value());

    std::optional<std::wstring> input_data;
    std::optional<std::wstring> display_options_json;

    HtmlDialogFunctionRunner::ParseSingleInputText(m_selectedSamplePair->sample.input, input_data, display_options_json);
    ASSERT(input_data.has_value());

    HtmlDialogFunctionRunner html_dialog_function_runner(NavigationAddress::CreateHtmlFilenameReference(*m_selectedDialogTemplatePath),
                                                         std::move(*input_data),
                                                         std::move(display_options_json));

    html_dialog_function_runner.DoModal();
}


void HtmlDialogTemplatesDlg::OnReplaceHtml()
{
    CodeDoc* active_code_doc = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveCodeDoc();
    ASSERT(active_code_doc != nullptr && m_selectedDialogTemplatePath.has_value());

    try
    {
        std::string html = FileIO::ReadText<std::string>(*m_selectedDialogTemplatePath);
        active_code_doc->GetPrimaryCodeView().GetLogicCtrl()->SetText(html);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void HtmlDialogTemplatesDlg::OnReplaceInput()
{
    CodeDoc* active_code_doc = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveCodeDoc();
    ASSERT(active_code_doc != nullptr &&
           active_code_doc->GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog &&
           active_code_doc->GetSecondaryCodeView() != nullptr);

    active_code_doc->GetSecondaryCodeView()->GetLogicCtrl()->SetText(m_selectedSamplePair->sample.input);
}


LRESULT HtmlDialogTemplatesDlg::OnActiveDocChanged(WPARAM wParam, LPARAM /*lParam*/)
{
    const CodeDoc* code_doc = reinterpret_cast<const CodeDoc*>(wParam);

    m_activeDocIsHtmlDialog = ( code_doc != nullptr && code_doc->GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog );

    m_replaceHtmlButton.EnableWindow(m_activeDocIsHtmlDialog && m_selectedDialogTemplatePath.has_value());
    m_replaceInputButton.EnableWindow(m_activeDocIsHtmlDialog);

    return 1;
}
