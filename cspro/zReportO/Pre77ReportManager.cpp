#include "stdafx.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/Interapp.h>
#include <zJson/Json.h>
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteHelpers.h>
#include <zPlatformO/PlatformInterface.h>


namespace Pre77Report
{
    ReportManager::ReportManager(std::unique_ptr<IReportManagerAssistant> pReportManagerAssistant)
        :   m_pReportManagerAssistant(std::move(pReportManagerAssistant))
    {
    }


    ReportManager::~ReportManager()
    {
        // delete any temporary files created
        for( CString csFilename : m_aTemporaryFilenames )
            PortableFunctions::FileDelete(csFilename);

        // delete any source scripts loaded
        for( auto kp : m_mapSourceScriptReports )
        {
            for( auto pReport : kp.second )
                delete pReport;
        }
    }


    void ReportManager::ClearReportData()
    {
        m_mapData.clear();
    }


    void ReportManager::SetReportData(CString csAttribute,const std::string& sValue)
    {
        ASSERT(( sValue[0] == '{' ) && ( sValue[sValue.length() - 1] == '}' ));
        m_mapData[csAttribute] = sValue;
    }


    std::string ReportManager::GetAllReportData() const
    {
        std::string sAllValues;

        if( !m_mapData.empty() )
        {
            for( const auto& itr : m_mapData )
            {
                const auto& sValue = itr.second;
                int iStartingPos = 1; // skip past the {
                int iEndingPos = sValue.length() - 1; // don't include the }

                if( sValue[iEndingPos - 1] == '\n' ) // if the JSON is created using the PrettyWriter, it will have a newline
                    iEndingPos--;

                sAllValues = sAllValues + ( sAllValues.empty() ? "\n{" : ",\n" ) + sValue.substr(iStartingPos, iEndingPos - iStartingPos);
            }

            sAllValues = sAllValues + "\n}\n";
        }

        // if no report data has been set, define an object to allow the report to be created
        else
            sAllValues = "{}";

        return sAllValues;
    }


    void ReportManager::SetReportData(CString csAttribute, sqlite3* db, const std::string& sql_query)
    {
        sqlite3_stmt* stmt = nullptr;

        std::vector<std::string> aSqlStatements = SQLiteHelpers::SplitSqlStatement(sql_query);

        if( aSqlStatements.size() == 0 )
            throw Exception("Empty SQL statement");

        // execute any helper statements
        for( size_t i = 0; i < ( aSqlStatements.size() - 1 ); i++ )
        {
            if( sqlite3_exec(db,aSqlStatements[i].c_str(),nullptr,nullptr,nullptr) != SQLITE_OK )
                throw Exception(_T("SQL syntax: %s"), (LPCTSTR)FromUtf8(sqlite3_errmsg(db)));
        }

        if( sqlite3_prepare_v2(db,aSqlStatements[aSqlStatements.size() - 1].c_str(),-1,&stmt,nullptr) != SQLITE_OK )
            throw Exception(_T("SQL syntax: %s"), (LPCTSTR)FromUtf8(sqlite3_errmsg(db)));

        auto jsw = Json::CreateStringWriter<char>();

        jsw->BeginObject();

        jsw->Key(csAttribute);

        int iSqlResult = sqlite3_step(stmt);

        if( iSqlResult != SQLITE_ROW )
        {
            jsw->Write(0);
        }

        else
        {
            const int MaximumRowsToRead = 10000;
            int iNumberColumns = sqlite3_column_count(stmt);
            int iRowNumber = 0;

            std::vector<std::string> aColumnNames;

            for( int iColumn = 0; iColumn < iNumberColumns; iColumn++ )
                aColumnNames.push_back(sqlite3_column_name(stmt,iColumn));

            jsw->BeginArray();

            do
            {
                iRowNumber++;

                jsw->BeginObject();

                for( int iColumn = 0; iColumn < iNumberColumns; iColumn++ )
                {
                    jsw->Key(aColumnNames[iColumn]);

                    int iColumnType = sqlite3_column_type(stmt,iColumn);

                    if( iColumnType == SQLITE_NULL )
                        jsw->WriteNull();

                    else if( iColumnType == SQLITE_TEXT )
                        jsw->Write(sqlite3_column_text(stmt, iColumn));

                    else
                        jsw->Write(sqlite3_column_double(stmt, iColumn));
                }

                jsw->EndObject();

            } while( ( iRowNumber < MaximumRowsToRead ) && ( sqlite3_step(stmt) == SQLITE_ROW ) );

            jsw->EndArray();
        }

        safe_sqlite3_finalize(stmt);

        jsw->EndObject();

        SetReportData(csAttribute, jsw->GetString());
    }


    CString ReportManager::GetOutputDirectory() const
    {
        return m_csOutputDirectory;
    }


    void ReportManager::CreateReport(CString csTemplateFilename,CString* pcsOutputFilename)
    {
        // read in the template
        std::string sInputTemplate = ReadUtf8File(csTemplateFilename);

        // if an output filename is not supplied, create a temporary filename based off the template filename
        if( pcsOutputFilename->IsEmpty() )
        {
            CString csExtension = PortableFunctions::PathGetFileExtension<CString>(csTemplateFilename);

            for( int i = 1; i < 100000; i++ )
            {
                pcsOutputFilename->Format(_T("%s%05d%s%s"),
                    PortableFunctions::PathRemoveFileExtension(csTemplateFilename).c_str(),
                    i,
                    csExtension.IsEmpty() ? _T("") : _T("."),
                    (LPCTSTR)csExtension
                );

                if( !PortableFunctions::FileExists(*pcsOutputFilename) )
                    break;
            }

            m_aTemporaryFilenames.push_back(*pcsOutputFilename);
        }

        // create and save the report
        m_csOutputDirectory = PortableFunctions::PathGetDirectory<CString>(*pcsOutputFilename);

        std::string sOutputHtml = CreateReport(sInputTemplate);

        WriteUtf8File(*pcsOutputFilename,sOutputHtml);
    }


    std::string ReportManager::CreateReport(const std::string& sTemplate)
    {
        Report report(sTemplate);
        std::ostringstream os;

        report.CreateReport(this,os);

        return os.str();
    }


    const std::vector<Report*>& ReportManager::GetSourceScriptReports(SourceScriptLocation eSourceScriptLocation)
    {
        CString csDirectory;

        if( eSourceScriptLocation == SourceScriptLocation::OutputDirectory )
            csDirectory = m_csOutputDirectory;

        else // the reports distributed with CSPro
            csDirectory = PortableFunctions::PathAppendToPath(WS2CS(CSProExecutables::GetApplicationOrAssetsDirectory()), _T("Reports"));

        // read in the scripts in the directory if they haven't been read in already
        if( m_mapSourceScriptReports.find(csDirectory) == m_mapSourceScriptReports.end() )
        {
            m_mapSourceScriptReports[csDirectory] = std::vector<Report*>();
            auto& apReports = m_mapSourceScriptReports[csDirectory];

            for( const std::wstring& report_filename : DirectoryLister().SetNameFilter(FileExtensions::Wildcard::Pre77Report)
                                                                        .GetPaths(csDirectory) )
            {
                Report* pReport = new Report(ReadUtf8File(report_filename));
                pReport->SetReportIsSourceScript(this, WS2CS(report_filename));
                apReports.push_back(pReport);
            }
        }

        return m_mapSourceScriptReports[csDirectory];
    }


    const ReportTemplateEngineNode* ReportManager::FindReportTemplateEngineNode(const std::string& sTemplateEngineName)
    {
        for( int i = 0; i < 2; i++ )
        {
            auto apReports = GetSourceScriptReports(( i == 0 ) ? SourceScriptLocation::OutputDirectory : SourceScriptLocation::CSProReportsDirectory);

            for( auto pReport : apReports )
            {
                const ReportTemplateEngineNode* pReportTemplateEngineNode = pReport->GetReportTemplateEngineNode(sTemplateEngineName);

                if( pReportTemplateEngineNode != nullptr )
                    return pReportTemplateEngineNode;
            }
        }

        return nullptr;
    }


    const ReportQueryNode* ReportManager::FindReportQueryNode(const std::string& sQueryName)
    {
        for( int i = 0; i < 2; i++ )
        {
            auto apReports = GetSourceScriptReports(( i == 0 ) ? SourceScriptLocation::OutputDirectory : SourceScriptLocation::CSProReportsDirectory);

            for( auto pReport : apReports )
            {
                const ReportQueryNode* pReportQueryNode = pReport->GetReportQueryNode(sQueryName);

                if( pReportQueryNode != nullptr )
                    return pReportQueryNode;
            }
        }

        return nullptr;
    }


    void ReportManager::ExecuteQueryIfNecessary(const ReportQueryNode* pReportQueryNode)
    {
        CString csQueryName = UTF8Convert::UTF8ToWide<CString>(pReportQueryNode->GetName());

        if( m_mapData.find(csQueryName) != m_mapData.end() )
            return; // the query has already been executed

        CString csDataSourceName = UTF8Convert::UTF8ToWide<CString>(pReportQueryNode->GetDataSource());

        sqlite3* db = m_pReportManagerAssistant->GetSqlite(csDataSourceName);

        if( db == nullptr )
            throw Exception(_T("The data source %s could not be located"), (LPCTSTR)csDataSourceName);

        SetReportData(csQueryName, db, pReportQueryNode->GetQuery());
    }


    std::string ReportManager::ReadUtf8File(NullTerminatedString filename)
    {
        try
        {
            return FileIO::ReadText<std::string>(filename);
        }

        catch( const std::exception& )
        {
            throw Exception(_T("Could not read the file %s"), filename.c_str());
        }
    }


    void ReportManager::WriteUtf8File(NullTerminatedString filename, const std::string& sText)
    {
        CString csDirectory = PortableFunctions::PathGetDirectory<CString>(filename);

        if( PortableFunctions::PathMakeDirectories(csDirectory) )
        {
            bool bSuccess = false;
            FILE* pFile = PortableFunctions::FileOpen(filename, _T("wb"));

            if( pFile != nullptr )
            {
                if( ( fwrite(Utf8BOM_sv.data(), 1, Utf8BOM_sv.length(), pFile) == Utf8BOM_sv.length() ) &&
                    ( fwrite(sText.c_str(), 1, sText.length(), pFile) == sText.length() ) )
                {
                    bSuccess = true;
                }

                fclose(pFile);
            }

            if( bSuccess )
                return;
        }

        throw Exception(_T("Could not create or write to the file %s"), filename.c_str());
    }


    // used by the Paradata Viewer to get the queries
    void ReportManager::LoadQueries(CString csWorkingDirectory,std::vector<ReportQueryNode*>& aQueries)
    {
        m_csOutputDirectory = csWorkingDirectory;

        for( int i = 0; i < 2; i++ )
        {
            auto apReports = GetSourceScriptReports(( i == 0 ) ? SourceScriptLocation::OutputDirectory : SourceScriptLocation::CSProReportsDirectory);

            for( auto pReport : apReports )
                pReport->GetReportQueryNodes(aQueries);
        }
    }
}
