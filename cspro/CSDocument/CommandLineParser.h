#pragma once


class CommandLineParser : public CCommandLineInfo
{
public:
    bool DoCommandLineBuild() const { return !m_inputFilePaths.empty(); }

    const std::vector<std::string>& GetInputFilePaths() const { return m_inputFilePaths; }
    const std::string& GetOutputPath() const                  { return m_outputPath; }
    const std::string& GetDocSetFilePath() const              { return m_docSetFilePath; }
    const std::string& GetBuildSettingsFilePath() const       { return m_buildSettingsFilePath; }
    const std::string& GetBuildNameOrType() const             { return m_buildNameOrType; }

    bool CreateNotepadPlusPlusColorizer() const { return m_createNotepadPlusPlusColorizer; }

    const std::vector<std::string>& GetFilePaths() const { return m_filePaths; }

protected:
    void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override;

private:
    static std::string EvaluatePath(std::string path);

private:
    std::vector<std::string> m_filePaths;

    std::variant<std::monostate, std::string*, std::vector<std::string>*> m_nextBuildValue;
    std::vector<std::string> m_inputFilePaths;
    std::string m_outputPath;
    std::string m_docSetFilePath;
    std::string m_buildSettingsFilePath;
    std::string m_buildNameOrType;

    bool m_createNotepadPlusPlusColorizer = false;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::string CommandLineParser::EvaluatePath(std::string path)
{
    return MakeFullPath(GetWorkingDirectory(), std::move(path));
}


inline void CommandLineParser::ParseParam(const TCHAR* const pszParam, const BOOL bFlag, const BOOL bLast)
{
    std::string param = TC::ToUtf8(pszParam);

    if( bFlag )
    {
        auto get_next_build_value_type = [&]()
        {
            if( std::holds_alternative<std::string*>(m_nextBuildValue) )
            {
                if( std::get<std::string*>(m_nextBuildValue) == &m_outputPath )
                    return "directory or filename";

                if( std::get<std::string*>(m_nextBuildValue) == &m_buildNameOrType )
                    return "build name or type";
            }

            return "filename";
        };

        if( std::holds_alternative<std::string*>(m_nextBuildValue) ||
            ( std::holds_alternative<std::vector<std::string>*>(m_nextBuildValue) && std::get<std::vector<std::string>*>(m_nextBuildValue)->empty() ) )
        {
            throw CSProException("A %s, not a flag, was expected: %s", get_next_build_value_type(), param.c_str());
        }

        else if( m_inputFilePaths.empty() && SO::EqualsNoCase("input", param) )
        {
            m_nextBuildValue = &m_inputFilePaths;
        }

        else if( m_outputPath.empty() && SO::EqualsNoCase("output", param) )
        {
            m_nextBuildValue = &m_outputPath;
        }

        else if( m_docSetFilePath.empty() && SO::EqualsNoCase("documentSet", param) )
        {
            m_nextBuildValue = &m_docSetFilePath;
        }

        else if( m_buildSettingsFilePath.empty() && SO::EqualsNoCase("buildSettings", param) )
        {
            m_nextBuildValue = &m_buildSettingsFilePath;
        }

        else if( m_buildNameOrType.empty() && SO::EqualsNoCase("build", param) )
        {
            m_nextBuildValue = &m_buildNameOrType;
        }

        else if( !m_createNotepadPlusPlusColorizer && SO::EqualsNoCase("Notepad++", param) )
        {
            m_createNotepadPlusPlusColorizer = true;
        }

        else
        {
            throw CSProException("Unknown or duplicate command line flag: %s", param.c_str());
        }

        if( !std::holds_alternative<std::monostate>(m_nextBuildValue) && bLast )
            throw CSProException("The flag must be followed by a %s: %s", get_next_build_value_type(), param.c_str());
    }

    else if( std::holds_alternative<std::string*>(m_nextBuildValue) )
    {
        std::string* const value = std::get<std::string*>(m_nextBuildValue);
        *value = ( value == &m_buildNameOrType ) ? std::move(param) :
                                                   EvaluatePath(std::move(param));
        m_nextBuildValue.emplace<std::monostate>();
    }

    else if( std::holds_alternative<std::vector<std::string>*>(m_nextBuildValue) )
    {
        std::get<std::vector<std::string>*>(m_nextBuildValue)->emplace_back(EvaluatePath(std::move(param)));
    }

    else
    {
        m_filePaths.emplace_back(EvaluatePath(std::move(param)));
    }
}
