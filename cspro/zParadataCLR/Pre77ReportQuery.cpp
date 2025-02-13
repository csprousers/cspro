#include "Stdafx.h"
#include "Pre77ReportQuery.h"
#include <zReportO/Pre77ReportException.h>
#include <zReportO/Pre77ReportManager.h>
#include <zReportO/Pre77ReportNodes.h>
#include <zToolsO/Utf8Convert.h>

namespace CSPro
{
    namespace ParadataViewer
    {
        namespace Metadata
        {
            #define MetadataPrefix "data-paradata-viewer-"
            const char* Prefix = MetadataPrefix;
            const char* Grouping = MetadataPrefix "grouping";
            const char* ReportTypes = MetadataPrefix "report-types";
            const char* ColumnLabelPrefix = MetadataPrefix "label-column-";
            const char* ColumnFormatPrefix = MetadataPrefix "format-column-";
            const char* ColumnTypePrefix = MetadataPrefix "type-column-";

            #define ReportTypeTable "table"
            #define ReportTypeSummaryTable "summary_table"
            #define ReportTypeChart "chart"

            #define ColumnTypeTimestamp "timestamp"

            #define UndefinedGrouping "<Undefined>"
        }


        System::String^ Utf8ToTrimmedString(const std::string& sText)
        {
            CString csText = UTF8Convert::UTF8ToWide<CString>(sText);
            auto text = gcnew System::String(csText);
            return text->Trim();
        }


        ReportQuery::ReportQuery(Pre77Report::ReportQueryNode* pReportQueryNode)
        {
            // set the default report type, which may be modified later
            DefaultReportType = ReportType::Table;
            m_iSupportedReportTypes = (int)DefaultReportType;
            IsTabularQuery = true;

            Columns = gcnew System::Collections::Generic::List<ReportQueryColumn^>();


            // process the main attributes
            Name = Utf8ToTrimmedString(pReportQueryNode->GetName());
            Description = Utf8ToTrimmedString(pReportQueryNode->GetDescription());
            SqlQuery = Utf8ToTrimmedString(pReportQueryNode->GetQuery());

            // if the description isn't defined, use the name
            if( System::String::IsNullOrWhiteSpace(Description) )
                Description = Name;

            // remove any tabs (that follow a newline) in the query and then replace the tabs with spaces
            while( true )
            {
                int iInitialLength = SqlQuery->Length;
                SqlQuery = SqlQuery->Replace("\n\t","\n");

                if( SqlQuery->Length == iInitialLength )
                    break;
            }

            SqlQuery = SqlQuery->Replace("\t"," ");

            // process the Paradata Viewer metadata
            std::vector<std::string> aAttributes;
            std::vector<std::string> aValues;
            pReportQueryNode->GetMetadata(Metadata::Prefix,aAttributes,aValues);

            for( size_t i = 0; i < aAttributes.size(); i++ )
            {
                const std::string& sAttribute = aAttributes[i];
                System::String^ value = Utf8ToTrimmedString(aValues[i]);

                if( sAttribute.compare(Metadata::Grouping) == 0 )
                    Grouping = value;

                else if( sAttribute.compare(Metadata::ReportTypes) == 0 )
                {
                    m_iSupportedReportTypes = 0;

                    auto tokens = value->Split((array<System::String^>^)nullptr,System::StringSplitOptions::RemoveEmptyEntries);

                    for( int j = 0; j < tokens->Length; j++ )
                    {
                        ReportType reportType;

                        if( tokens[j]->Equals(ReportTypeTable,System::StringComparison::InvariantCultureIgnoreCase) )
                            reportType = ReportType::Table;

                        else if( tokens[j]->Equals(ReportTypeSummaryTable,System::StringComparison::InvariantCultureIgnoreCase) )
                        {
                            reportType = ReportType::Table;
                            IsTabularQuery = false;
                        }

                        else if( tokens[j]->Equals(ReportTypeChart,System::StringComparison::InvariantCultureIgnoreCase) )
                        {
                            reportType = ReportType::Chart;
                            IsTabularQuery = false;
                        }

                        else
                        {
                            // quit out of the loop and throw the exception
                            m_iSupportedReportTypes = 0;
                            break;
                        }

                        if( j == 0 )
                            DefaultReportType = reportType;

                        m_iSupportedReportTypes |= (int)reportType;
                    }

                    if( m_iSupportedReportTypes == 0 )
                        throw gcnew System::Exception(System::String::Format("Invalid or unspecified report type detected: \"{0}\"",value));
                }

                else
                {
                    bool bLabel = ( sAttribute.find(Metadata::ColumnLabelPrefix) == 0 );
                    bool bFormat = !bLabel && ( sAttribute.find(Metadata::ColumnFormatPrefix) == 0 );

                    if( bLabel || bFormat || ( sAttribute.find(Metadata::ColumnTypePrefix) == 0 ) )
                    {
                        // calculate the column number
                        int iColumnNumberPosition = strlen(
                            bLabel ? Metadata::ColumnLabelPrefix :
                            bFormat ? Metadata::ColumnFormatPrefix :
                            Metadata::ColumnTypePrefix);
                        std::string sColumnNumber = sAttribute.substr(iColumnNumberPosition);

                        const int MaximumNumberColumns = 1024;
                        int iColumnNumber = atoi(sColumnNumber.c_str());

                        if( iColumnNumber < 1 || iColumnNumber > MaximumNumberColumns )
                            throw gcnew System::Exception(System::String::Format("Specified column numbers must be between 1 and {0} and cannot be {1}",MaximumNumberColumns,iColumnNumber));

                        // create the column (and any missing ones)
                        while( Columns->Count < iColumnNumber )
                            Columns->Add(gcnew ReportQueryColumn());

                        // the column number is one-based
                        auto reportQueryColumn = Columns[iColumnNumber - 1];

                        if( bLabel )
                            reportQueryColumn->Label = value;

                        else if( bFormat )
                            reportQueryColumn->Format = value;

                        else // type
                        {
                            if( value->Equals(ColumnTypeTimestamp,System::StringComparison::InvariantCultureIgnoreCase) )
                                reportQueryColumn->IsTimestamp = true;

                            else
                                throw gcnew System::Exception(System::String::Format("Invalid or unspecified column type detected: \"{0}\"",value));
                        }
                    }

                    else
                    {
                        throw gcnew System::Exception(System::String::Format(
                            "Unknown metadata detected: {0}=\"{1}\"",Utf8ToTrimmedString(sAttribute),value));
                    }
                }
            }

            // set the grouping if it wasn't defined
            if( Grouping == nullptr )
                Grouping = UndefinedGrouping;
        }

        bool ReportQuery::SupportsReportType(ReportType reportType)
        {
            return ( ( m_iSupportedReportTypes & (int)reportType ) != 0 );
        }

        bool ReportQuery::CanViewAsTable::get()
        {
            return SupportsReportType(ReportType::Table);
        }

        bool ReportQuery::CanViewAsChart::get()
        {
            return SupportsReportType(ReportType::Chart);
        }


        System::Collections::Generic::List<ReportQuery^>^ ReportManager::LoadParadataQueries(System::String^ workingDirectory)
        {
            try
            {
                auto reportQueries = gcnew System::Collections::Generic::List<ReportQuery^>();
                Pre77Report::ReportManager reportManager(nullptr);
                std::vector<Pre77Report::ReportQueryNode*> aQueries;

                reportManager.LoadQueries(CString(workingDirectory),aQueries);

                for( auto pReportQueryNode : aQueries )
                {
                    // filter for only paradata filters
                    CString csDataSourceName = UTF8Convert::UTF8ToWide<CString>(pReportQueryNode->GetDataSource());

                    if( csDataSourceName.CompareNoCase(_T("paradata")) != 0 )
                        continue;

                    reportQueries->Add(gcnew ReportQuery(pReportQueryNode));
                }

                return reportQueries;
            }

            catch( const Pre77Report::Exception& exception )
            {
                throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
            }
        }
    }
}
