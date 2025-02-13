#include "stdafx.h"
#include "IncludesCC.h"
#include "List.h"
#include "Nodes/Path.h"
#include <zUtilO/MediaStore.h>


int LogicCompiler::CompileDirectoryVariant(bool allow_string_expression, bool allow_path_type,
                                           bool allow_media_type, bool allow_symbol)
{
    auto create_node = [&](Nodes::DirectoryVariant::Type type, int code_or_expression)
    {
        auto& directory_variant_node = CreateNode<Nodes::DirectoryVariant>();

        directory_variant_node.type = type;
        directory_variant_node.code_or_expression = code_or_expression;

        return GetProgramIndex(directory_variant_node);
    };

    ASSERT(allow_string_expression || allow_path_type || allow_media_type || allow_symbol);

    // the directory can either be a path type...
    if( allow_path_type )
    {
        size_t path_type = NextKeyword({ _T("TEMP"),       _T("APPLICATION"),      _T("INPUTFILE"),
                                         _T("CSPRO"),      _T("DESKTOP"),          _T("WINDOWS"),
                                         _T("DOCUMENTS"),  _T("PROGRAMFILES32"),   _T("PROGRAMFILES64"),
                                         _T("CSENTRY"),    _T("CSENTRYEXTERNAL"),  _T("HTML"),
                                         _T("DOWNLOADS") });

        if( path_type != 0 )
        {
            NextToken();
            return create_node(Nodes::DirectoryVariant::Type::Path, static_cast<int>(path_type)); // path types are 1-based
        }
    };

    // ...or a media type
    if( allow_media_type && NextKeyword({ MediaStore::Text::Media }) == 1 )
    {
        NextToken();
        IssueErrorOnTokenMismatch(TOKPERIOD, MGF::dot_required_to_separate_options_7018);

        size_t media_type = NextKeywordOrError(MediaStore::GetMediaTypeStrings());
        NextToken();

        return create_node(Nodes::DirectoryVariant::Type::Media, static_cast<int>(media_type) - 1);  // media types are 0-based
    }

    // ...or a symbol
    if( allow_symbol )
    {
        // see if it's a dictionary or file
        MarkInputBufferToRestartLater();

        NextToken();

        if( Tkn == TOKDICT || Tkn == TOKDICT_PRE80 || Tkn == TOKFILE )
        {
            if( Tkn == TOKDICT )
                VerifyDictionaryObject();

            int symbol_index = Tokstindex;

            ClearMarkedInputBuffer();
            NextToken();

            return create_node(Nodes::DirectoryVariant::Type::Symbol, symbol_index);
        }

        RestartFromMarkedInputBuffer();
    }

    // ...or the fallback of a string expression
    if( allow_string_expression )
    {
        NextToken();
        return create_node(Nodes::DirectoryVariant::Type::String, CompileStringExpression());
    };

    // handle errors
    IssueError(MGF::Path_invalid_directory_100379);
}


int LogicCompiler::CompilePathFilter()
{
    // the filter is either a file type...
    if( NextKeyword({ Nodes::Path::Text::FileType }) == 1 )
    {
        NextToken();
        IssueErrorOnTokenMismatch(TOKPERIOD, 7018);

        size_t filter_type = NextKeywordOrError({ Nodes::Path::Text::Audio,
                                                  Nodes::Path::Text::Geometry,
                                                  Nodes::Path::Text::Image });
        NextToken();

        return static_cast<int>(( filter_type == 1 ) ? Nodes::Path::FilterType::Audio :
                                ( filter_type == 2 ) ? Nodes::Path::FilterType::Geometry :
                                                       Nodes::Path::FilterType::Image);
    }

    // ...or a string expression
    NextToken();
    return CompileStringExpression();
}


int LogicCompiler::CompilePathFunctions()
{
    // this function will compile the old-style functions (pathconcat and pathname),
    // dirlist, and the new path namespace functions
    FunctionCode function_code = CurrentToken.function_details->code;
    int program_index;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // compiling everything but path.selectFile and dirlist...
    if( function_code != FunctionCode::PATHFN_SELECTFILE_CODE && function_code != FunctionCode::FNDIRLIST_CODE )
    {
        bool concat_style_functions = ( function_code == FunctionCode::FNPATHCONCAT_CODE || function_code == FunctionCode::PATHFN_CONCAT_CODE );
        bool pathname_function = ( function_code == FunctionCode::FNPATHNAME_CODE );
        bool old_style_functions = ( pathname_function || function_code == FunctionCode::FNPATHCONCAT_CODE );

        std::vector<int> arguments;

        if( concat_style_functions || old_style_functions || function_code == FunctionCode::PATHFN_GETRELATIVEPATH_CODE )
            arguments.emplace_back(CompileDirectoryVariant(!pathname_function, true, false, old_style_functions));

        // read the path strings
        bool read_additional_arguments = ( arguments.empty() || Tkn == TOKCOMMA );

        while( read_additional_arguments )
        {
            NextToken();

            arguments.emplace_back(CompileStringExpression());

            if( Tkn == TOKRPAREN )
            {
                read_additional_arguments = false;
            }

            else
            {
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);
            }
        }

        size_t max_arguments_allowed = concat_style_functions ? SIZE_MAX : 1;

        if( function_code == FunctionCode::PATHFN_GETRELATIVEPATH_CODE )
        {
            if( arguments.size() < 2 )
               IssueError(MGF::function_call_missing_arguments_49);

            max_arguments_allowed = 2;
        }

        if( arguments.size() > max_arguments_allowed )
            IssueError(MGF::function_call_too_many_arguments_51);

        // the concat-style functions must include the number of arguments
        if( concat_style_functions )
            arguments.insert(arguments.begin(), arguments.size());

        program_index = CreateVariableArgumentsNode(function_code, arguments);
    }


    // compiling path.selectFile...
    else if( function_code == FunctionCode::PATHFN_SELECTFILE_CODE )
    {
        IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, MGF::deprecation_use_CS_Path_selectFile_95005);

        auto& path_select_file_node = CreateNode<Nodes::PathSelectFile>(function_code);
        static_assert(sizeof(path_select_file_node) == 24);
        InitializeNode(path_select_file_node, -1, 1);

        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

        optional_named_arguments_compiler.AddArgument(_T("title"), path_select_file_node.title_expression, DataType::String);
        optional_named_arguments_compiler.AddArgument(_T("showDirectories"), path_select_file_node.show_directories_expression, DataType::Numeric);
        optional_named_arguments_compiler.AddArgument(_T("filter"), path_select_file_node.filter_type_or_expression, [&]() { return CompilePathFilter(); });

        auto compile_directory = [&]() { return CompileDirectoryVariant(true, true, true, false); };
        optional_named_arguments_compiler.AddArgument(_T("startDirectory"), path_select_file_node.start_directory_variant_index, compile_directory);
        optional_named_arguments_compiler.AddArgument(_T("rootDirectory"), path_select_file_node.root_directory_variant_index, compile_directory);

        // if no optional arguments were encountered, but there is another argument, it should be the title
        if( optional_named_arguments_compiler.Compile(true) == 0  )
        {
            NextToken();

            if( Tkn != TOKRPAREN )
                path_select_file_node.title_expression = CompileStringExpression();
        }

        program_index = GetProgramIndex(path_select_file_node);
    }


    // compiling dirlist...
    else
    {
        ASSERT(function_code == FunctionCode::FNDIRLIST_CODE);

        auto& dirlist_node = CreateNode<Nodes::DirList>(function_code);

        dirlist_node.filter_type_or_expression = -1;
        dirlist_node.recursive_expression = -1;

        NextToken();
        IssueErrorOnTokenMismatch(TOKLIST, MGF::List_expected_960);

        dirlist_node.list_symbol_index = Tokstindex;
        const LogicList& logic_list = GetSymbolLogicList(dirlist_node.list_symbol_index);

        if( !logic_list.IsString() )
            IssueError(MGF::List_not_correct_data_type_961, ToString(DataType::String));

        if( logic_list.IsReadOnly() )
            IssueError(MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());

        NextToken();
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        dirlist_node.directory_variant_index = CompileDirectoryVariant(true, true, true, false);

        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

        optional_named_arguments_compiler.AddArgument(_T("filter"), dirlist_node.filter_type_or_expression, [&]() { return CompilePathFilter(); });
        optional_named_arguments_compiler.AddArgument(_T("recursive"), dirlist_node.recursive_expression, DataType::Numeric);

        // if no optional arguments were encountered but there are additional arguments, read them using the pre-7.7 style
        if( optional_named_arguments_compiler.Compile() == 0 && Tkn != TOKRPAREN )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            const std::vector<const TCHAR*> recursive_keyword = { _T("RECURSIVE") };

            if( NextKeyword(recursive_keyword) == 1 )
            {
                dirlist_node.recursive_expression = CreateNumericConstantNode(1);
                NextToken();
            }

            else
            {
                NextToken();
                dirlist_node.filter_type_or_expression = CompileStringExpression();
            }

            if( dirlist_node.filter_type_or_expression != -1 && Tkn != TOKRPAREN )
            {
                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                NextKeywordOrError(recursive_keyword);
                dirlist_node.recursive_expression = CreateNumericConstantNode(1);
                NextToken();
            }
        }

        program_index = GetProgramIndex(dirlist_node);
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return program_index;
}
