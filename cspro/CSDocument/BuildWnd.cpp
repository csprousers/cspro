#include "StdAfx.h"
#include "BuildWnd.h"
#include "CSDocument.h"


TextEditView* CSDocumentBuildWnd::FindTextEditView(const std::variant<const CLogicCtrl*, const std::string*>& source_logic_ctrl_or_file_path)
{
    TextEditView* matched_text_edit_view = nullptr;

    ForeachViewOfType<TextEditView>(
        [&](TextEditView& text_edit_view)
        {
            bool matches;

            if( std::holds_alternative<const CLogicCtrl*>(source_logic_ctrl_or_file_path) )
            {
                matches = ( text_edit_view.GetLogicCtrl() == std::get<const CLogicCtrl*>(source_logic_ctrl_or_file_path) );
            }

            else
            {
                matches = SO::EqualsNoCase(text_edit_view.GetDocument()->GetPathName(), *std::get<const std::string*>(source_logic_ctrl_or_file_path));
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


CLogicCtrl* CSDocumentBuildWnd::ActivateDocumentAndGetLogicCtrl(const std::variant<const CLogicCtrl*, const std::string*> source_logic_ctrl_or_file_path)
{
    TextEditView* matched_text_edit_view = FindTextEditView(source_logic_ctrl_or_file_path);

    if( matched_text_edit_view != nullptr )
    {
        matched_text_edit_view->GetParentFrame()->ActivateFrame();
    }

    // if not matched, try to open the file
    else if( std::holds_alternative<const std::string*>(source_logic_ctrl_or_file_path) )
    {
        TextEditDoc* const text_edit_doc = OpenDocumentOnMessageClick(*std::get<const std::string*>(source_logic_ctrl_or_file_path));

        if( text_edit_doc != nullptr )
            matched_text_edit_view = text_edit_doc->GetTextEditView();
    }

    return ( matched_text_edit_view != nullptr ) ? matched_text_edit_view->GetLogicCtrl() :
                                                   nullptr;
}


TextEditDoc* CSDocumentBuildWnd::OpenDocumentOnMessageClick(const std::string& file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        return nullptr;

    CSDocumentApp& csdoc_app = *assert_cast<CSDocumentApp*>(AfxGetApp());

    if( CSDocumentApp::DocumentCanBeOpenedDirectly(file_path) )
        return assert_nullable_cast<TextEditDoc*>(csdoc_app.OpenDocumentFile(TC::ToWide(file_path).c_str()));

    // if the document cannot be opened directly, find the Document Set associated with the source of these errors
    if( GetSourceLogicSource() != nullptr )
    {
        TextEditView* const text_edit_view = FindTextEditView(GetSourceLogicSource());

        if( text_edit_view != nullptr )
        {
            std::shared_ptr<DocSetSpec> doc_set_spec = text_edit_view->GetTextEditDoc().GetSharedAssociatedDocSetSpec();

            if( doc_set_spec == nullptr )
                return ReturnProgrammingError(nullptr);

            const std::shared_ptr<DocSetComponent> doc_set_component = doc_set_spec->FindComponent(file_path, true);

            if( doc_set_component != nullptr )
            {
                csdoc_app.SetDocSetParametersForNextOpen(*doc_set_component, std::move(doc_set_spec));
                return assert_nullable_cast<TextEditDoc*>(csdoc_app.OpenDocumentFile(TC::ToWide(file_path).c_str(), FALSE));
            }
        }
    }

    return nullptr;
}
