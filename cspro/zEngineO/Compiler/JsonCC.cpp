#include "stdafx.h"
#include "IncludesCC.h"
#include <zJson/Json.h>


class CompilerJsonReaderInterface : public JsonReaderInterface
{
public:
    CompilerJsonReaderInterface(LogicCompiler& logic_compiler, const Application* application)
        :   m_logicCompiler(logic_compiler)
    {
        if( application != nullptr )
            m_directory = GetWorkingFolder(application->GetApplicationFilename());
    }

protected:
    void OnLogWarning(const std::wstring message) override
    {
        m_logicCompiler.IssueWarning(MGF::JSON_text_has_warnings_100431, message.c_str());
    }

private:
    LogicCompiler& m_logicCompiler;
};



int LogicCompiler::CompileJsonText(const std::function<void(const JsonNode<wchar_t>& json_node)>& json_node_callback/* = { }*/)
{
    // compiles a string and, if a string literal, checks that it is valid JSON
    return CompileStringExpressionWithStringLiteralCheck(
        [&](const std::wstring& text)
        {
            try
            {
                CompilerJsonReaderInterface compiler_json_reader_interface(*this, m_engineData->application);
                const JsonNode<wchar_t> json_node = Json::Parse(text, &compiler_json_reader_interface);

                if( json_node_callback )
                    json_node_callback(json_node);
            }

            catch( const JsonParseException& exception )
            {
                IssueError(MGF::JSON_invalid_text_100430, exception.GetErrorMessage().c_str());
            }
        });
}
