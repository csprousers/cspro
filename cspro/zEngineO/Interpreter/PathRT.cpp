#include "stdafx.h"
#include "IncludesRT.h"


const std::string& LogicInterpreter::GetCurrentApplicationFilePath()
{
    return ( m_engineData->pff != nullptr ) ? UTF8_TODO::Create_Reference(m_engineData->pff->GetAppFName()) :
                                              ReturnProgrammingError(SO::Empty_string);
}


const std::string& LogicInterpreter::GetCurrentWorkingDirectory()
{
    if( m_currentWorkingDirectory.empty() )
    {
        m_currentWorkingDirectory = ( m_engineData->pff != nullptr ) ? GetWorkingDirectory(GetCurrentApplicationFilePath()) :
                                                                       GetWorkingDirectory();
    }

    return m_currentWorkingDirectory;
}


std::string LogicInterpreter::GetAbsolutePath(std::string path)
{
    return MakeFullPath(GetCurrentWorkingDirectory(), std::move(path));
}


void LogicInterpreter::MakeAbsolutePath(std::string& path)
{
    path = GetAbsolutePath(std::move(path));
}


std::string LogicInterpreter::EvaluatePath(const int program_index)
{
    return GetAbsolutePath(EvaluateSharableString(program_index).GetString());
}


SharableString LogicInterpreter::EvaluatePathOrUrl(const int program_index)
{
    SharableString path_or_url = EvaluateSharableString(program_index);

    if( !Encoders::IsDataOrHttpUrl(*path_or_url) )
        MakeAbsolutePath(path_or_url.MakeModifiable());

    return path_or_url;
}


void LogicInterpreter::MakeAbsolutePath(ConnectionString& connection_string)
{
    connection_string.AdjustRelativePath(GetCurrentWorkingDirectory());
}


ConnectionString LogicInterpreter::EvaluateConnectionString(const int program_index)
{
    ConnectionString connection_string(EvaluateSharableString(program_index).GetString());
    MakeAbsolutePath(connection_string);
    return connection_string;
}
