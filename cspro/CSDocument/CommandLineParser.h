#pragma once


class CommandLineParser : public CCommandLineInfo
{
public:
    CommandLineParser()
        :   m_createNotepadPlusPlusColorizer(false)
    {
    }

    bool DoCommandLineBuild() const { return !m_inputFilenames.empty(); }

    const std::vector<std::wstring>& GetInputFilenames() const { return m_inputFilenames; }
    const std::wstring& GetOutputPath() const                  { return m_outputPath; }
    const std::wstring& GetDocSetFilename() const              { return m_docSetFilename; }
    const std::wstring& GetBuildSettingsFilename() const       { return m_buildSettingsFilename; }
    const std::wstring& GetBuildNameOrType() const             { return m_buildNameOrType; }

    bool CreateNotepadPlusPlusColorizer() const { return m_createNotepadPlusPlusColorizer; }

    const std::vector<std::wstring>& GetFilenames() const { return m_filenames; }

protected:
    void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override
    {
        if( bFlag )
        {
            auto get_next_build_value_type = [&]()
            {
                if( std::holds_alternative<std::wstring*>(m_nextBuildValue) )
                {
                    if( std::get<std::wstring*>(m_nextBuildValue) == &m_outputPath )
                        return _T("directory or filename");

                    if( std::get<std::wstring*>(m_nextBuildValue) == &m_buildNameOrType )
                        return _T("build name or type");
                }

                return _T("filename");
            };

            if( std::holds_alternative<std::wstring*>(m_nextBuildValue) ||
                ( std::holds_alternative<std::vector<std::wstring>*>(m_nextBuildValue) && std::get<std::vector<std::wstring>*>(m_nextBuildValue)->empty() ) )
            {
                throw CSProException(_T("A %s, not a flag, was expected: %s"), get_next_build_value_type(), pszParam);
            }

            else if( m_inputFilenames.empty() && SO::EqualsNoCase(_T("input"), pszParam) )
            {
                m_nextBuildValue = &m_inputFilenames;
            }

            else if( m_outputPath.empty() && SO::EqualsNoCase(_T("output"), pszParam) )
            {
                m_nextBuildValue = &m_outputPath;
            }

            else if( m_docSetFilename.empty() && SO::EqualsNoCase(_T("documentSet"), pszParam) )
            {
                m_nextBuildValue = &m_docSetFilename;
            }

            else if( m_buildSettingsFilename.empty() && SO::EqualsNoCase(_T("buildSettings"), pszParam) )
            {
                m_nextBuildValue = &m_buildSettingsFilename;
            }

            else if( m_buildNameOrType.empty() && SO::EqualsNoCase(_T("build"), pszParam) )
            {
                m_nextBuildValue = &m_buildNameOrType;
            }

            else if( !m_createNotepadPlusPlusColorizer && SO::EqualsNoCase(_T("Notepad++"), pszParam) )
            {
                m_createNotepadPlusPlusColorizer = true;
            }

            else
            {
                throw CSProException(_T("Unknown or duplicate command line flag: %s"), pszParam);
            }

            if( !std::holds_alternative<std::monostate>(m_nextBuildValue) && bLast )
                throw CSProException(_T("The flag must be followed by a %s: %s"), get_next_build_value_type(), pszParam);
        }

        else if( std::holds_alternative<std::wstring*>(m_nextBuildValue) )
        {
            std::wstring* value = std::get<std::wstring*>(m_nextBuildValue);
            *value = ( value == &m_buildNameOrType ) ? pszParam :
                                                       EvaluatePath(pszParam);
            m_nextBuildValue.emplace<std::monostate>();
        }

        else if( std::holds_alternative<std::vector<std::wstring>*>(m_nextBuildValue) )
        {
            std::get<std::vector<std::wstring>*>(m_nextBuildValue)->emplace_back(EvaluatePath(pszParam));
        }

        else
        {
            m_filenames.emplace_back(EvaluatePath(pszParam));
        }
    }

private:
    static std::wstring EvaluatePath(const TCHAR* pszParam)
    {
        return MakeFullPath(GetWorkingFolder(), pszParam);
    }

private:
    std::vector<std::wstring> m_filenames;

    std::variant<std::monostate, std::wstring*, std::vector<std::wstring>*> m_nextBuildValue;
    std::vector<std::wstring> m_inputFilenames;
    std::wstring m_outputPath;
    std::wstring m_docSetFilename;
    std::wstring m_buildSettingsFilename;
    std::wstring m_buildNameOrType;

    bool m_createNotepadPlusPlusColorizer;
};
