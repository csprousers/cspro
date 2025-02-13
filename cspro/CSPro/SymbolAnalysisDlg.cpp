#include "StdAfx.h"
#include "SymbolAnalysisDlg.h"
#include <zUtilO/TextSource.h>


// this class will send messages to the dialog when the caret changes
class SymbolAnalysisLogicCtrl : public CLogicCtrl
{
public:
    SymbolAnalysisLogicCtrl(SymbolAnalysisDlg& symbol_Analysis_dlg)
        :   m_symbolAnalysisDlg(symbol_Analysis_dlg)
    {
    }

protected:
    LRESULT OnUpdateStatusPaneCaretPos(WPARAM /*wParam*/, LPARAM /*lParam*/) override
    {
        int line_number = LineFromPosition(GetCurrentPos());
        m_symbolAnalysisDlg.OnSymbolUsesLineChanged(line_number);

        return 0;
    }

private:
    SymbolAnalysisDlg& m_symbolAnalysisDlg;
};


BEGIN_MESSAGE_MAP(SymbolAnalysisDlg, CDialog)
    ON_NOTIFY(TVN_SELCHANGED, IDC_SYMBOL_ANALYSIS_TREE, OnSymbolsTreeSelectionChanged)
END_MESSAGE_MAP()


SymbolAnalysisDlg::SymbolAnalysisDlg(Application& application, const SymbolAnalysisCompiler& symbol_analysis_compiler, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_SYMBOL_ANALYSIS, pParent),
        m_application(application),
        m_symbolAnalysisCompiler(symbol_analysis_compiler),
        m_usesLogicCtrl(std::make_unique<SymbolAnalysisLogicCtrl>(*this))
{
}


void SymbolAnalysisDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_SYMBOL_ANALYSIS_SYMBOLS_LABEL, m_symbolsLabelCtrl);
    DDX_Control(pDX, IDC_SYMBOL_ANALYSIS_TREE, m_symbolsTreeCtrl);
    DDX_Control(pDX, IDC_SYMBOL_ANALYSIS_USES_LABEL, m_usesLabelCtrl);
    DDX_Control(pDX, IDC_SYMBOL_ANALYSIS_USES, *m_usesLogicCtrl);
    DDX_Control(pDX, IDC_SYMBOL_ANALYSIS_CONTEXT, m_contextLogicCtrl);
}


BOOL SymbolAnalysisDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_symbolsLabelCtrl.SetWindowText(FormatText(_T("Symbols (%d)"), (int)m_symbolAnalysisCompiler.GetSymbolUseMap().size()));

    m_usesLogicCtrl->ReplaceCEdit(this, false, false);
    m_contextLogicCtrl.ReplaceCEdit(this, false, true);    

    // build the tree and select the first symbol
    BuildTree();

    HTREEITEM hItem = m_symbolsTreeCtrl.GetChildItem(TVI_ROOT);
    ASSERT(hItem != nullptr);

    m_symbolsTreeCtrl.SelectItem(hItem);
    m_symbolsTreeCtrl.SetFocus();

    return FALSE;
}


void SymbolAnalysisDlg::BuildTree()
{
    // add each symbol to the tree in sorted order
    std::map<std::wstring, const Symbol*> symbol_name_map;

    for( const auto& [symbol, symbol_uses] : m_symbolAnalysisCompiler.GetSymbolUseMap() )
        symbol_name_map.try_emplace(SO::ToUpper(symbol->GetName()), symbol);

    TV_INSERTSTRUCT tvi { };
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;

    for( const auto& [symbol_name, symbol] : symbol_name_map )
    {
        tvi.item.pszText = const_cast<TCHAR*>(symbol_name.c_str());
        tvi.item.cchTextMax = symbol_name.length();
        tvi.item.lParam = (LPARAM)symbol;
        
        m_symbolsTreeCtrl.InsertItem(&tvi);
    }
}


void SymbolAnalysisDlg::OnSymbolsTreeSelectionChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
    auto pnmtv = (const NMTREEVIEWW*)pNMHDR;
    auto symbol = (const Symbol*)pnmtv->itemNew.lParam;

    if( symbol == nullptr )
        return;

    m_usesLabelCtrl.SetWindowText(FormatText(_T("Uses of %s"), symbol->GetName().c_str()));

    const auto& symbol_uses = m_symbolAnalysisCompiler.GetSymbolUseMap().at(symbol);

    // format the compilation unit names, proc names, and line numbers
    std::vector<std::wstring> use_locations;
    size_t max_use_location_length = 0;

    for( const auto& symbol_use : symbol_uses )
    {
        std::wstring& use_location = use_locations.emplace_back();

        if( !symbol_use.compilation_unit_name.empty() )
        {
            // only show the compilation unit name for external code (because reports are named)
            if( m_application.GetReportNamedTextSource(symbol_use.proc_name, true) == nullptr )
                use_location = PortableFunctions::PathGetFilenameWithoutExtension(symbol_use.compilation_unit_name);
        }

        if( !symbol_use.proc_name.empty() )
            SO::AppendWithSeparator(use_location, symbol_use.proc_name, '/');

        SO::AppendFormat(use_location, _T("(%d)"), symbol_use.adjusted_line_number);

        max_use_location_length = std::max(max_use_location_length, use_location.length());
    }

    // set the uses text
    std::wstring symbol_uses_text;

    for( size_t i = 0; i < symbol_uses.size(); ++i )
    {
        std::wstring tabs_to_spaces_line = symbol_uses[i].logic_line;
        SO::Replace(tabs_to_spaces_line, _T("\t"), _T("    "));

        SO::AppendWithSeparator(symbol_uses_text,
                                FormatText(_T("%s %-*s %s  %s"), m_application.GetLogicSettings().GetMultilineCommentStart().c_str(),
                                                                 (int)max_use_location_length, use_locations[i].c_str(),
                                                                 m_application.GetLogicSettings().GetMultilineCommentEnd().c_str(),
                                                                 tabs_to_spaces_line.c_str()),
                                _T("\r\n"));
    }

    m_usesLogicCtrl->SetReadOnly(FALSE);
    m_usesLogicCtrl->SetText(symbol_uses_text);
    m_usesLogicCtrl->SetReadOnly(TRUE);

    OnSymbolUsesLineChanged(0, symbol);
}


void SymbolAnalysisDlg::OnSymbolUsesLineChanged(int uses_line_number, const Symbol* symbol/* = nullptr*/)
{
    bool need_to_refresh_bookmarks = false;

    if( symbol != nullptr )
    {
        need_to_refresh_bookmarks = true;
        m_lastShownSymbol = symbol;
    }

    else if( !m_lastShownSymbol.has_value() )
        return;

    // quit if the uses line number is not valid
    const auto& symbol_uses = m_symbolAnalysisCompiler.GetSymbolUseMap().at(*m_lastShownSymbol);

    if( uses_line_number < 0 || uses_line_number >= (int)symbol_uses.size() )
        return;

    const auto& selected_symbol_use = symbol_uses[uses_line_number];

    if( m_lastShownCompilationUnit != selected_symbol_use.compilation_unit_name )
    {
        need_to_refresh_bookmarks = true;
        m_lastShownCompilationUnit = selected_symbol_use.compilation_unit_name;

        // update the logic
        const TextSource* text_source = GetTextSource(selected_symbol_use.compilation_unit_name);

        if( text_source != nullptr )
        {
            m_contextLogicCtrl.SetReadOnly(FALSE);
            m_contextLogicCtrl.SetText(text_source->GetText());
            m_contextLogicCtrl.SetReadOnly(TRUE);
        }
    }

    // show bookmarks of all uses in this compilation unit
    if( need_to_refresh_bookmarks )
    {
        m_contextLogicCtrl.ClearErrorAndWarningMarkers();

        for( const auto& symbol_use : symbol_uses )
        {
            // Scintilla uses zero-based line numbers
            if( symbol_use.compilation_unit_name == selected_symbol_use.compilation_unit_name )
                m_contextLogicCtrl.AddErrorOrWarningMarker(true, symbol_use.adjusted_line_number - 1);
        }
    }

    m_contextLogicCtrl.GotoLine(selected_symbol_use.adjusted_line_number - 1);
}


const TextSource* SymbolAnalysisDlg::GetTextSource(const std::wstring& compilation_unit_name)
{
    // search for the proper text source only once (per compilation unit)
    const auto& lookup = m_textSources.find(compilation_unit_name);

    if( lookup != m_textSources.cend() )
        return lookup->second;

    // look up for text source
    const TextSource* text_source = nullptr;

    for( const CodeFile& code_file : m_application.GetCodeFiles() )
    {
        bool matches;

        // if no compilation unit, use the main logic
        if( compilation_unit_name.empty() )
        {
            matches = code_file.IsLogicMain();
        }

        // otherwise check the external code filename
        else
        {
            matches = ( !code_file.IsLogicMain() &&
                        SO::EqualsNoCase(code_file.GetFilename(), compilation_unit_name) );
        }

        if( matches )
        {
            text_source = &code_file.GetTextSource();
            break;
        }
    }

    // if not found, check the reports
    if( text_source == nullptr )
    {
        const NamedTextSource* report_named_text_source = m_application.GetReportNamedTextSource(compilation_unit_name, false);

        if( report_named_text_source != nullptr )
            text_source = report_named_text_source->text_source.get();
    }

    ASSERT(text_source != nullptr);

    return m_textSources.try_emplace(compilation_unit_name, text_source).first->second;
}
