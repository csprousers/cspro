#pragma once

#include <zAppO/Application.h>
#include <zEdit2O/LogicCtrl.h>
#include <Zsrcmgro/SymbolAnalysisCompiler.h>


class SymbolAnalysisDlg : public CDialog
{
public:
    SymbolAnalysisDlg(Application& application, const SymbolAnalysisCompiler& symbol_analysis_compiler, CWnd* pParent = nullptr);

    void OnSymbolUsesLineChanged(int uses_line_number, const Symbol* symbol = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSymbolsTreeSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);

private:
    void BuildTree();

    const TextSource* GetTextSource(const std::wstring& compilation_unit_name);

private:
    Application& m_application;
    const SymbolAnalysisCompiler& m_symbolAnalysisCompiler;

    CStatic m_symbolsLabelCtrl;
    CTreeCtrl m_symbolsTreeCtrl;    
    CStatic m_usesLabelCtrl;
    std::unique_ptr<CLogicCtrl> m_usesLogicCtrl;
    CLogicCtrl m_contextLogicCtrl;

    std::map<std::wstring, const TextSource*> m_textSources;

    std::optional<const Symbol*> m_lastShownSymbol;
    std::optional<std::wstring> m_lastShownCompilationUnit;
};
