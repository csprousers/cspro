#pragma once

#include <zHtml/DocumentVirtualFileMappingHandler.h>
#include <zUtilO/MimeType.h>


class CodeDocVirtualFileMappingHandler : public DocumentVirtualFileMappingHandler
{
public:
    CodeDocVirtualFileMappingHandler(CMainFrame& main_frame, const CDocument& document)
        :   DocumentVirtualFileMappingHandler(document),
            m_mainFrame(main_frame)
    {
    }

protected:
    bool ServeDocumentContent(VirtualFileMappingResponse& response) override;

    std::optional<std::string> GetDocumentDefaultMimeType() const override
    {
        return MimeType::Type::Text;
    }

private:
    CMainFrame& m_mainFrame;
};



inline bool CodeDocVirtualFileMappingHandler::ServeDocumentContent(VirtualFileMappingResponse& response)
{
    // find the document, first searching by the filename (in case a document is closed and the new one happens to have the exact same pointer)
    CodeDoc* code_doc = !m_filePath.empty() ? m_mainFrame.FindDocument(m_filePath) :
                                              nullptr;

    // if not found, search by pointer
    if( code_doc == nullptr )
        code_doc = m_mainFrame.FindDocument(&m_document);

    // if the document is still open, get the current text
    if( code_doc != nullptr )
    {
        std::string text;

        // the text has to be retrieved from the UI thread
        if( m_mainFrame.SendMessage(UWM::CSCode::GetCodeTextForDoc, reinterpret_cast<WPARAM>(code_doc), reinterpret_cast<LPARAM>(&text)) == 1 )
        {
            // determine a MIME type for this content based on either the filename or the lexer type
            std::optional<std::string> mime_type;

            if( !m_filePath.empty() )
                mime_type = MimeType::GetServerTypeFromFileExtension(PortableFunctions::PathGetFileExtension(m_filePath));

            if( !mime_type.has_value() )
                mime_type = Lexers::GetLexerDefaultServerMimeType(code_doc->GetLanguageSettings().GetLexerLanguage());

            response.SetContent(text, *mime_type);

            return true;
        }
    }

    return false;
}
