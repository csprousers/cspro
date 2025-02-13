#include "StdAfx.h"
#include "DocSetSpecFrame.h"
#include "DocSetTreeView.h"


IMPLEMENT_DYNCREATE(DocSetSpecFrame, DocSetBaseFrame)

BEGIN_MESSAGE_MAP(DocSetSpecFrame, DocSetBaseFrame)

    ON_WM_SIZE()
    ON_WM_DESTROY()

    ON_COMMAND(ID_COMPILE_DOCSET_SPEC_ONLY, OnCompileSpecOnly)
    ON_COMMAND(ID_COMPILE, OnCompileSpecAndComponents)

END_MESSAGE_MAP()


DocSetSpecFrame::DocSetSpecFrame()
    :   m_docSetTreeView(nullptr),
        m_textEditView(nullptr)
{
}


BOOL DocSetSpecFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    ASSERT(pContext->m_pNewViewClass == RUNTIME_CLASS(TextEditView));

    const GlobalSettings& global_settings = GetMainFrame().GetGlobalSettings();
    m_splitterWnd.SetWidthProportion(global_settings.doc_set_tree_window_proportion); 

    if( !m_splitterWnd.CreateStatic(this, 1, 2) ||
        !m_splitterWnd.CreateView(0, 0, RUNTIME_CLASS(DocSetTreeView), CSize(1, 1), pContext) ||
        !m_splitterWnd.CreateView(0, 1, RUNTIME_CLASS(TextEditView), CSize(1, 1), pContext) )
    {
        return FALSE;
    }

    m_docSetTreeView = assert_cast<DocSetTreeView*>(m_splitterWnd.GetPane(0, 0));
    m_textEditView = assert_cast<TextEditView*>(m_splitterWnd.GetPane(0, 1));

    return TRUE;        
}


void DocSetSpecFrame::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy);

    // when this is true, the splitter has been fully set up, so resize the two panes proportionally
    if( m_textEditView != nullptr && nType != SIZE_MINIMIZED )
        m_splitterWnd.SizeProportionally(cx);
}


void DocSetSpecFrame::OnDestroy()
{
    GlobalSettings& global_settings = GetMainFrame().GetGlobalSettings();
    global_settings.doc_set_tree_window_proportion = m_splitterWnd.GetWidthProportion();

    __super::OnDestroy();
}


void DocSetSpecFrame::OnCompileSpec(bool compile_components)
{
    std::wstring action = _T("CSPro Document Set compilation");

    if( !compile_components )
        action.append(_T(" (specification only)"));

    DocSetSpec& doc_set_spec = GetDocSetSpec();

    CompileWrapper(
        std::move(action),
        true,
        [&](DocSetCompiler& doc_set_compiler, std::variant<JsonNode<wchar_t>, std::wstring> input)
        {
            ASSERT(std::holds_alternative<JsonNode<wchar_t>>(input));

            doc_set_compiler.CompileSpec(doc_set_spec, std::get<JsonNode<wchar_t>>(input), compile_components ? DocSetCompiler::SpecCompilationType::SpecAndComponents :
                                                                                                                DocSetCompiler::SpecCompilationType::SpecOnly);
        });

    m_docSetTreeView->RebuildTreeIfNecessary();
}


void DocSetSpecFrame::WriteFormattedComponent(JsonWriter& json_writer, DocSetCompiler& doc_set_compiler, const JsonNode<wchar_t>& json_node, bool detailed_format)
{
    // compile the spec to update the values prior to the format
    DocSetSpec& doc_set_spec = GetDocSetSpec();
    doc_set_compiler.CompileSpec(doc_set_spec, json_node, DocSetCompiler::SpecCompilationType::SpecOnly);

    const GlobalSettings& global_settings = GetMainFrame().GetGlobalSettings();
    doc_set_spec.WriteJsonSpecOnly(json_writer, &json_node, &global_settings, detailed_format);
}
