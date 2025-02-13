#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/List.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Various.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Hash.h>
#include <zToolsO/Utf8Convert.h>
#include <zZipo/IZip.h>


double CIntDriver::excompress(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    std::wstring zip_filename = EvalFullPathFileName(va_node.arguments[0]);

    if( PortableFunctions::FileExists(zip_filename) )
    {
        if( !PortableFunctions::FileDelete(zip_filename) )
            return DEFAULT;
    }

    else
    {
        // ensure that the directory for the zip file exists
        PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(zip_filename));
    }

    // get a list of files to compress
    std::wstring root_directory_for_files;
    std::vector<std::wstring> filenames;

    if( va_node.arguments[1] >= 0 ) // a character string
    {
        std::wstring filename = EvalFullPathFileName(va_node.arguments[1]);

        root_directory_for_files = PortableFunctions::PathGetDirectory(filename);

        DirectoryLister().AddFilenamesWithPossibleWildcard(filenames, filename, true);
    }

    else // a list of files
    {
        const LogicList& logic_list = GetSymbolLogicList(-1 * va_node.arguments[1]);
        size_t list_count = logic_list.GetCount();

        for( size_t i = 1; i <= list_count; ++i ) // lists are one-based
        {
            std::wstring filename = logic_list.GetString(i);
            MakeFullPathFileName(filename);

            std::wstring directory = PortableFunctions::PathGetDirectory(filename);

            // set the root directory to the smallest directory
            if( root_directory_for_files.empty() )
            {
                root_directory_for_files = directory;
            }

            else
            {
                root_directory_for_files = PortableFunctions::PathGetCommonRoot(root_directory_for_files, directory);
            }

            filenames.emplace_back(std::move(filename));
        }
    }

    // make sure that the root directory ends with a slash
    root_directory_for_files = PortableFunctions::PathEnsureTrailingSlash(root_directory_for_files);

    // compress the files
    try
    {
        std::unique_ptr<IZip> pZip = std::unique_ptr<IZip>(IZip::Create());
        std::vector<std::wstring> filenames_in_zip;
        size_t files_compressed = 0;

        for( const std::wstring& filename : filenames )
        {
            std::wstring directory = PortableFunctions::PathGetDirectory(filename);

            ASSERT(filename.find(root_directory_for_files) == 0);

            std::wstring name_from_root_directory = filename.substr(root_directory_for_files.length());
            filenames_in_zip.emplace_back(name_from_root_directory);

            if( PortableFunctions::FileIsRegular(filename) )
            {
                // only count files, not directories
                ++files_compressed;
            }

            else if( !PortableFunctions::FileIsDirectory(filename) )
            {
                // abort if this is not a file or directory
                return DEFAULT;
            }
        }

        pZip->AddFiles(zip_filename, filenames, filenames_in_zip, true);

        return files_compressed;
    }

    catch( const CZipError& )
    {
        return DEFAULT;
    }
}


double CIntDriver::exdecompress(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring zip_filename = EvalFullPathFileName(fnn_node.fn_expr[0]);
    std::wstring output_directory;

    if( fnn_node.fn_nargs > 1 )
    {
        output_directory = EvalFullPathFileName(fnn_node.fn_expr[1]);
    }

    else
    {
        output_directory = PortableFunctions::PathGetDirectory(output_directory);
    }

    try
    {
        std::unique_ptr<IZip> pZip = std::unique_ptr<IZip>(IZip::Create());
        return pZip->ExtractAllFiles(zip_filename, output_directory);
    }

    catch( const CZipError& )
    {
        return DEFAULT;
    }
}


double CIntDriver::exhash(int iExpr)
{
    const auto& hash_node = GetNode<Nodes::Hash>(iExpr);

    const std::variant<double, std::wstring> value = EvaluateVariantExpression(hash_node.value_data_type, hash_node.value_expression);
    const size_t hash_length = std::min(Hash::MaxHashLength, EvaluateOptionalNumericExpression<size_t>(hash_node.length_expression, Hash::DefaultHashLength));
    const std::optional<std::wstring> salt = EvaluateOptionalStringExpression(hash_node.salt_expression);

    std::wstring hash;

    if( IsString(hash_node.value_data_type) )
    {
        hash = salt.has_value() ? Hash::Hash(std::get<std::wstring>(value), hash_length, *salt) :
                                  Hash::Hash(std::get<std::wstring>(value), hash_length);
    }

    else
    {
        ASSERT(IsNumeric(hash_node.value_data_type));

        hash = salt.has_value() ? Hash::Hash(reinterpret_cast<const std::byte*>(&std::get<double>(value)), sizeof(double), hash_length, *salt) :
                                  Hash::Hash(reinterpret_cast<const std::byte*>(&std::get<double>(value)), sizeof(double), hash_length);
    }

    if( m_usingLogicSettingsV0 )
        SO::MakeUpper(hash); // Encoders::HexChars changed to lowercase for 8.0, so make it uppercase to match the pre-8.0 results

    return AssignAlphaValue(std::move(hash));
}
