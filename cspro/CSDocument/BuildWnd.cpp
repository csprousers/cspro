#include "StdAfx.h"
#include "BuildWnd.h"
#include "CSDocument.h"


TextEditView* CSDocumentBuildWnd::FindTextEditView(const std::variant<const CLogicCtrl*, const std::wstring*>& source_logic_ctrl_or_filename)
{
    TextEditView* matched_text_edit_view = nullptr;

    ForeachViewOfType<TextEditView>(
        [&](TextEditView& text_edit_view)
        {
            bool matches;

            if( std::holds_alternative<const CLogicCtrl*>(source_logic_ctrl_or_filename) )
            {
                matches = ( text_edit_view.GetLogicCtrl() == std::get<const CLogicCtrl*>(source_logic_ctrl_or_filename) );
            }

            else
            {
                matches = SO::EqualsNoCase(text_edit_view.GetDocument()->GetPathName(), *std::get<const std::wstring*>(source_logic_ctrl_or_filename));
            }

            if( matches )
            {
                matched_text_edit_view = &text_edit_view;
                return false;
            }

            return true;
        });

    return matched_text_edit_view;
}


CLogicCtrl* CSDocumentBuildWnd::ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::wstring*> source_logic_ctrl_or_filename)
{
    TextEditView* matched_text_edit_view = FindTextEditView(source_logic_ctrl_or_filename);

    if( matched_text_edit_view != nullptr )
    {
        matched_text_edit_view->GetParentFrame()->ActivateFrame();
    }

    // if not matched, try to open the file
    else if( std::holds_alternative<const std::wstring*>(source_logic_ctrl_or_filename) )
    {
        TextEditDoc* text_edit_doc = OpenDocumentOnMessageClick(*std::get<const std::wstring*>(source_logic_ctrl_or_filename));

        if( text_edit_doc != nullptr )
            matched_text_edit_view = text_edit_doc->GetTextEditView();
    }

    return ( matched_text_edit_view != nullptr ) ? matched_text_edit_view->GetLogicCtrl() :
                                                   nullptr;
}


TextEditDoc* CSDocumentBuildWnd::OpenDocumentOnMessageClick(const std::wstring& filename)
{
    if( !PortableFunctions::FileIsRegular(filename) )
        return nullptr;

    CSDocumentApp& csdoc_app = *assert_cast<CSDocumentApp*>(AfxGetApp());
    
    if( CSDocumentApp::DocumentCanBeOpenedDirectly(filename) )
        return assert_nullable_cast<TextEditDoc*>(csdoc_app.OpenDocumentFile(filename.c_str()));

    // if the document cannot be opened directly, find the Document Set associated with the source of these errors
    if( GetSourceLogicSource() != nullptr )
    {
        TextEditView* text_edit_view = FindTextEditView(GetSourceLogicSource());

        if( text_edit_view != nullptr )
        {
            std::shared_ptr<DocSetSpec> doc_set_spec = text_edit_view->GetTextEditDoc().GetSharedAssociatedDocSetSpec();

            if( doc_set_spec == nullptr )
                return ReturnProgrammingError(nullptr);

            std::shared_ptr<DocSetComponent> doc_set_component = doc_set_spec->FindComponent(filename, true);

            if( doc_set_component != nullptr )
            {
                csdoc_app.SetDocSetParametersForNextOpen(*doc_set_component, doc_set_spec);
                return assert_nullable_cast<TextEditDoc*>(csdoc_app.OpenDocumentFile(filename.c_str(), FALSE));
            }
        }
    }

    return nullptr;
}
