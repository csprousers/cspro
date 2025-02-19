#include "stdafx.h"
#include "IncludesRT.h"
#include "List.h"
#include "Nodes/Various.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Hash.h>
#include <zZipo/ZipFile.h>


double LogicInterpreter::ex_compress(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    std::string zip_file_path = EvaluatePath(va_node.arguments[0]);

    // get a list of files to compress
    std::vector<std::string> file_paths;

    // a string
    if( va_node.arguments[1] >= 0 )
    {
        const std::string file_path = EvaluatePath(va_node.arguments[1]);
        DirectoryLister().AddFilePathsWithPossibleWildcard(file_paths, file_path, true);
    }

    // a list of files
    else
    {
        const LogicList& logic_list = GetSymbolLogicList(-1 * va_node.arguments[1]);
        const size_t list_count = logic_list.GetCount();

        // lists are one-based
        for( size_t i = 1; i <= list_count; ++i )
            file_paths.emplace_back(GetAbsolutePath(logic_list.GetValue<SharableString>(i).GetString()));
    }

    // compress the files
    try
    {
        ZipCreator zip_creator(std::move(zip_file_path));
        return zip_creator.AddFiles(file_paths);
    }

    catch( const CSProException& )
    {
        return DEFAULT;
    }
}


double LogicInterpreter::ex_decompress(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    const std::string zip_file_path = EvaluatePath(fnn_node.fn_expr[0]);

    const std::string output_directory = ( fnn_node.fn_nargs > 1 )                ? EvaluatePath(fnn_node.fn_expr[1]) :
                                         ( m_engineData->application != nullptr ) ? GetWorkingDirectory(m_engineData->application->GetApplicationFilePath()) :
                                                                                    ReturnProgrammingError(PortableFunctions::PathGetDirectory(zip_file_path));
    try
    {
        ZipReader zip_reader(zip_file_path);
        return zip_reader.ExtractAll(output_directory);
    }

    catch( const CSProException& )
    {
        return DEFAULT;
    }
}


double LogicInterpreter::ex_hash(const int program_index)
{
    const auto& hash_node = GetNode<Nodes::Hash>(program_index);

    const std::variant<double, SharableString> value = EvaluateVariant<SharableString>(hash_node.value_data_type, hash_node.value_expression);
    const size_t hash_length = std::min(Hash::MaxHashLength, EvaluateOptional<size_t>(hash_node.length_expression, Hash::DefaultHashLength));
    const std::optional<SharableString> salt = EvaluateOptional<SharableString>(hash_node.salt_expression);

    std::string hash;

    if( IsString(hash_node.value_data_type) )
    {
        hash = salt.has_value() ? Hash::Hash(*std::get<SharableString>(value), hash_length, salt->GetString()) :
                                  Hash::Hash(*std::get<SharableString>(value), hash_length);
    }

    else
    {
        ASSERT(IsNumeric(hash_node.value_data_type));

        hash = salt.has_value() ? Hash::Hash(reinterpret_cast<const std::byte*>(&std::get<double>(value)), sizeof(double), hash_length, salt->GetString()) :
                                  Hash::Hash(reinterpret_cast<const std::byte*>(&std::get<double>(value)), sizeof(double), hash_length);
    }

    if( m_usingLogicSettingsV0 )
        SO::MakeUpper(hash); // Encoders::HexChars changed to lowercase for 8.0, so make it uppercase to match the pre-8.0 results

    return AssignString(std::move(hash));
}
