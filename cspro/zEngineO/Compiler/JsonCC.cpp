#include "stdafx.h"
#include "IncludesCC.h"
#include <zJson/Json.h>


class CompilerJsonReaderInterface : public JsonReaderInterface
{
public:
    CompilerJsonReaderInterface(LogicCompiler& logic_compiler, const Application* const application)
        :   m_logicCompiler(logic_compiler)
    {
        if( application != nullptr )
            m_directory = GetWorkingDirectory(application->GetApplicationFilePath());
    }

protected:
    void OnLogWarning(const std::string message) override
    {
        m_logicCompiler.IssueWarning(MGF::JSON_text_has_warnings_100431, message.c_str());
    }

private:
    LogicCompiler& m_logicCompiler;
};



int LogicCompiler::CompileJsonText(const std::function<void(const JsonNode& json_node)>& json_node_callback/* = { }*/)
{
    // compiles a string and, if a string literal, checks that it is valid JSON
    return CompileStringExpressionWithStringLiteralCheck(
        [&](const std::string text)
        {
            try
            {
                CompilerJsonReaderInterface compiler_json_reader_interface(*this, m_engineData->application);
                const JsonNode json_node = Json::Parse(text, &compiler_json_reader_interface);

                if( json_node_callback )
                    json_node_callback(json_node);
            }

            catch( const JsonParseException& exception )
            {
                IssueError(MGF::JSON_invalid_text_100430, exception.what());
            }
        });
}
