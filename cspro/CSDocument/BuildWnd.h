#pragma once

#include <zDocument/DocumentBuildMessageTarget.h>
#include <zDesignerF/BuildWnd.h>

class TextEditDoc;
class TextEditView;


class CSDocumentBuildWnd : public BuildWnd, public DocumentBuildMessageTarget
{
protected:
    // BuildWnd override
    CLogicCtrl* ActivateDocumentAndGetLogicCtrl(std::variant<const CLogicCtrl*, const std::string*> source_logic_ctrl_or_file_path) override final;

    // DocumentBuildMessageTarget overrides
    void AddDocumentError(const std::string& file_path_with_error, const std::string& error_text) override final;
    void AddDocumentWarning(const std::string& file_path_with_error, const std::string& error_text) override final;

private:
    TextEditView* FindTextEditView(const std::variant<const CLogicCtrl*, const std::string*>& source_logic_ctrl_or_file_path);

    TextEditDoc* OpenDocumentOnMessageClick(const std::string& file_path);
};
