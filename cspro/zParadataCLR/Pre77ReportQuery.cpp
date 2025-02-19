#include "Stdafx.h"
#include "Pre77ReportQuery.h"
#include <zReportO/Pre77ReportException.h>
#include <zReportO/Pre77ReportManager.h>
#include <zReportO/Pre77ReportNodes.h>


namespace CSPro::ParadataViewer::Metadata
{
    #define MetadataPrefix                             "data-paradata-viewer-"
    constexpr const char* Prefix                     = MetadataPrefix;
    constexpr const char* Grouping                   = MetadataPrefix "grouping";
    constexpr const char* ReportTypes                = MetadataPrefix "report-types";
    constexpr std::string_view ColumnLabelPrefix_sv  = MetadataPrefix "label-column-";
    constexpr std::string_view ColumnFormatPrefix_sv = MetadataPrefix "format-column-";
    constexpr std::string_view ColumnTypePrefix_sv   = MetadataPrefix "type-column-";

    constexpr const char* ReportTypeTable            = "table";
    constexpr const char* ReportTypeSummaryTable     = "summary_table";
    constexpr const char* ReportTypeChart            = "chart";

    constexpr const char* ColumnTypeTimestamp        = "timestamp";

    constexpr const char* UndefinedGrouping          = "<Undefined>";
}


CSPro::ParadataViewer::ReportQuery::ReportQuery(Pre77Report::ReportQueryNode* const report_query_node)
{
    // set the default report type, which may be modified later
    DefaultReportType = ReportType::Table;
    m_supportedReportTypes = static_cast<int>(DefaultReportType);
    IsTabularQuery = true;

    Columns = gcnew System::Collections::Generic::List<ReportQueryColumn^>();


    // process the main attributes
    Name = clr_helpers::to_SystemString(SO::Trim(report_query_node->GetName()));
    Description = clr_helpers::to_SystemString(SO::Trim(report_query_node->GetDescription()));
    SqlQuery = clr_helpers::to_SystemString(SO::Trim(report_query_node->GetQuery()));

    // if the description isn't defined, use the name
    if( System::String::IsNullOrWhiteSpace(Description) )
        Description = Name;

    // remove any tabs (that follow a newline) in the query and then replace the tabs with spaces
    while( true )
    {
        const int initial_length = SqlQuery->Length;
        SqlQuery = SqlQuery->Replace("\n\t", "\n");

        if( SqlQuery->Length == initial_length )
            break;
    }

    SqlQuery = SqlQuery->Replace("\t", " ");

    // process the Paradata Viewer metadata
    std::vector<std::string> attributes;
    std::vector<std::string> values;
    report_query_node->GetMetadata(Metadata::Prefix, attributes, values);

    for( size_t i = 0; i < attributes.size(); ++i )
    {
        const std::string& attribute = attributes[i];
        const std::string value(SO::Trim(values[i]));

        if( attribute == Metadata::Grouping )
        {
            Grouping = clr_helpers::to_SystemString(value);
        }

        else if( attribute == Metadata::ReportTypes )
        {
            m_supportedReportTypes = 0;

            const std::vector<std::string_view> token_svs = SO::SplitString<std::string_view>(value, SO::WhitespaceChars_sv, false, false);

            for( size_t j = 0; j < token_svs.size(); ++j )
            {
                const std::string_view token_sv = token_svs[j];
                ReportType report_type;

                if( SO::EqualsNoCase(token_sv, Metadata::ReportTypeTable) )
                {
                    report_type = ReportType::Table;
                }

                else if( SO::EqualsNoCase(token_sv, Metadata::ReportTypeSummaryTable) )
                {
                    report_type = ReportType::Table;
                    IsTabularQuery = false;
                }

                else if( SO::EqualsNoCase(token_sv, Metadata::ReportTypeChart) )
                {
                    report_type = ReportType::Chart;
                    IsTabularQuery = false;
                }

                else
                {
                    // quit out of the loop and throw the exception
                    m_supportedReportTypes = 0;
                    break;
                }

                if( j == 0 )
                    DefaultReportType = report_type;

                m_supportedReportTypes |= static_cast<int>(report_type);
            }

            if( m_supportedReportTypes == 0 )
                throw gcnew System::Exception(clr_helpers::to_FormattedSystemString("Invalid or unspecified report type detected: '%s'", value.c_str()));
        }

        else
        {
            const bool label = SO::StartsWith(attribute, Metadata::ColumnLabelPrefix_sv);
            const bool format = ( !label && SO::StartsWith(attribute, Metadata::ColumnFormatPrefix_sv) );

            if( label || format || SO::StartsWith(attribute, Metadata::ColumnTypePrefix_sv) )
            {
                // calculate the column number
                const size_t column_number_position = label  ? Metadata::ColumnLabelPrefix_sv.length() :
                                                      format ? Metadata::ColumnFormatPrefix_sv.length() :
                                                               Metadata::ColumnTypePrefix_sv.length();
                const int column_number = atoi(attribute.substr(column_number_position).c_str());

                constexpr int MaximumNumberColumns = 1024;

                if( column_number < 1 || column_number > MaximumNumberColumns )
                {
                    throw gcnew System::Exception(clr_helpers::to_FormattedSystemString("Specified column numbers must be between 1 and %d and cannot be %d",
                                                                                        MaximumNumberColumns, column_number));
                }

                // create the column (and any missing ones)
                while( Columns->Count < column_number )
                    Columns->Add(gcnew ReportQueryColumn());

                // the column number is one-based
                ReportQueryColumn^ report_query_column = Columns[column_number - 1];

                if( label )
                {
                    report_query_column->Label = clr_helpers::to_SystemString(value);
                }

                else if( format )
                {
                    report_query_column->Format = clr_helpers::to_SystemString(value);
                }

                else if( SO::EqualsNoCase(value, Metadata::ColumnTypeTimestamp) ) // type
                {
                    report_query_column->IsTimestamp = true;
                }

                else
                {
                    throw gcnew System::Exception(clr_helpers::to_FormattedSystemString("Invalid or unspecified column type detected: '%s'", value.c_str()));
                }
            }

            else
            {
                throw gcnew System::Exception(clr_helpers::to_FormattedSystemString("Unknown metadata detected: %s='%s'", std::string(SO::Trim(attribute)).c_str(),
                                                                                                                          value.c_str()));
            }
        }
    }

    // set the grouping if it wasn't defined
    if( Grouping == nullptr )
        Grouping = clr_helpers::to_SystemString(Metadata::UndefinedGrouping);
}


bool CSPro::ParadataViewer::ReportQuery::SupportsReportType(ReportType report_type)
{
    return ( ( m_supportedReportTypes & static_cast<int>(report_type) ) != 0 );
}


bool CSPro::ParadataViewer::ReportQuery::CanViewAsTable::get()
{
    return SupportsReportType(ReportType::Table);
}


bool CSPro::ParadataViewer::ReportQuery::CanViewAsChart::get()
{
    return SupportsReportType(ReportType::Chart);
}


System::Collections::Generic::List<CSPro::ParadataViewer::ReportQuery^>^ CSPro::ParadataViewer::ReportManager::LoadParadataQueries(System::String^ working_directory)
{
    try
    {
        auto report_queries = gcnew System::Collections::Generic::List<ReportQuery^>();

        Pre77Report::ReportManager report_manager(nullptr);
        const std::vector<Pre77Report::ReportQueryNode*> queries = report_manager.LoadQueries(clr_helpers::to_string(working_directory));

        for( Pre77Report::ReportQueryNode* const query_node : queries )
        {
            // filter for only paradata filters
            const std::string data_source_name = query_node->GetDataSource();

            if( !SO::EqualsNoCase(data_source_name, "paradata") )
                continue;

            report_queries->Add(gcnew ReportQuery(query_node));
        }

        return report_queries;
    }

    catch( const Pre77Report::Exception& exception )
    {
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }
}
