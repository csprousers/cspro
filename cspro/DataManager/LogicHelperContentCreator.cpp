#include "StdAfx.h"
#include "LogicHelperContentCreator.h"
#include <zEdit2O/ScintillaColorizer.h>
#include <zDataO/CaseIteratorSettings.h>
#include <sstream>


LogicHelperContentCreator::LogicHelperContentCreator(DataSourceDoc& data_source_doc)
    :   m_dataSourceDoc(data_source_doc)
{
}


const wchar_t* LogicHelperContentCreator::GetSaveTitle() const
{
    return L"Save Logic";
}


std::vector<const char*> LogicHelperContentCreator::GetSaveFormats() const
{
    return { FileExtensions::Logic,
             FileExtensions::HTML };
}


std::string LogicHelperContentCreator::GetSaveSuggestedFilename() const
{
    std::string filename = SO::CreateParentheticalExpression("Logic", m_dataSourceDoc.GetDictionary().GetName());

    if( m_currentCase != nullptr )
    {
        filename.append(" - Case ")
                .append(m_currentCase->GetSingleLineKey());
    }

    return filename;
}


SharableString LogicHelperContentCreator::GetTextContent()
{
    DataRepository& data_repository = m_dataSourceDoc.GetDataRepository();
    m_currentCase = m_dataSourceDoc.GetSharedCurrentCase();

    const std::shared_ptr<const ViewableCaseIteratorSettings> settings = m_dataSourceDoc.GetSettings<ViewableCaseIteratorSettings>();

    std::stringstream logic;

    const std::string& dictionary_name = data_repository.GetCaseAccess().GetDataDict().GetName();

    const std::string connection_string_text = PortableFunctions::PathToForwardSlash(data_repository.GetConnectionString().ToString());

    std::string case_status;

    if( settings->GetStatus() != CaseIterationCaseStatus::NotDeletedOnly )
    {
        case_status = "CaseStatus." + ( settings->GetStatus() == CaseIterationCaseStatus::All )          ? "All" :
                                      ( settings->GetStatus() == CaseIterationCaseStatus::PartialsOnly ) ? "Partial" :
                                                                                                           "Duplicate";
    }

    std::string dictionary_access_parameters = ( settings->GetMethod() == CaseIterationMethod::KeyOrder ) ? std::string() :
                                                                                                            "OrderType.Sequential";

    if( settings->GetOrder() != CaseIterationOrder::Ascending )
        SO::AppendWithSeparator(dictionary_access_parameters, "Order.Descending", ", ");

    if( !case_status.empty() )
        SO::AppendWithSeparator(dictionary_access_parameters, case_status, ", ");

    if( !dictionary_access_parameters.empty() )
        dictionary_access_parameters = "(" + dictionary_access_parameters + ")";


    if( m_currentCase != nullptr )
    {
        const std::string escaped_key_string = Encoders::ToLogicString(m_currentCase->GetKey());
        ASSERT(dictionary_name == Encoders::ToEscapedString(dictionary_name));

        logic << "// ------- Case Operations -------";

        logic << "\n\n// to load this case using the key:\n";
        logic << "loadcase(" << dictionary_name << ", " << escaped_key_string << ");";


        if( !m_currentCase->GetUuid().empty() )
        {
            ASSERT(m_currentCase->GetUuid() == Encoders::ToEscapedString(m_currentCase->GetUuid()));

            logic << "\n\n// to load this case using the UUID (necessary for loading duplicates or deleted cases):\n";
            logic << "locate(" << dictionary_name << ", uuid, \"" << m_currentCase->GetUuid() << "\");\n";
            logic << "retrieve(" << dictionary_name << ");";
        }


        logic << "\n\n// to delete this case:\n";
        logic << "delcase(" << dictionary_name << ", " << escaped_key_string << ");";


        logic << "\n\n\n\n";
    }


    logic << "// ------- Data Source Operations -------";


    logic << "\n\n// to open this data source as an external dictionary:\n";
    logic << "setfile(" << dictionary_name << ", \"" << connection_string_text << "\");";


    logic << "\n\n// to count the number of cases:\n";
    logic << "numeric number_cases = countcases(" << dictionary_name;

    if( !case_status.empty() )
        logic << "(" << case_status << ")";

    logic << ");";


    logic << "\n\n// to get a list of all of the case keys:\n";
    logic << "string list case_keys;\n";
    logic << "keylist(" << dictionary_name << dictionary_access_parameters << ", case_keys);";


    logic << "\n\n// to iterate through all of the cases:\n";
    logic << "forcase " << dictionary_name << dictionary_access_parameters << " do\n"
          << "\t// ...\n"
             "endfor;";

    logic << "\n";

    return logic.str();
}


SharableString LogicHelperContentCreator::GetHtmlContent(bool /*embed_resources = false*/)
{
    // colorize the logic
    constexpr int lexer_language = Lexers::GetLexer_LogicV8_0();
    ScintillaColorizer colorizer(lexer_language, GetTextContent().GetString());
    return colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::FullHtml);
}
