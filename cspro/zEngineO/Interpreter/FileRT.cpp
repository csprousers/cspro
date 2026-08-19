#include "stdafx.h"
#include "IncludesRT.h"
#include "File.h"
#include "List.h"
#include "Nodes/File.h"


Engine::Value LogicInterpreter::ex_File_open(LogicFile& logic_file, const bool create, const bool append, const int file_path_expression)
{
    // the behavior from open, as opposed to setfile or File.open, is slightly different
    const bool called_from_open = ( file_path_expression == -1 );
    const bool create_if_not_exist = ( append && !called_from_open );

    try
    {
        // when called via setfile or File.open, a file path indicates the new file to open
        if( !called_from_open )
        {
            // close any existing file
            logic_file.Close();

            logic_file.SetFilePath(EvaluatePath(file_path_expression));
        }

        logic_file.Open(create, append, create_if_not_exist);

        return Engine::Value::Bool(true);
    }

    catch(...)
    {
        return Engine::Value::Bool(false);
    }
}


Engine::Value LogicInterpreter::ex_File_close(LogicFile& logic_file)
{
    try
    {
        logic_file.Close();

        return Engine::Value::Bool(true);
    }

    catch(...)
    {
        return Engine::Value::Bool(false);
    }
}


Engine::Value LogicInterpreter::ex_File_read(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const Nodes::List& elements_list = GetListNode(file_node.elements_list_node);
    LogicFile& logic_file = GetSymbolLogicFile(-1 * file_node.symbol_index_or_string_expression);

    try
    {
        // open the file if it is closed
        if( !logic_file.IsOpen() )
        {
            constexpr bool create_if_not_exist = false;
            logic_file.Open(false, false, create_if_not_exist);
        }

        FileIO::TextFile& text_file = logic_file.GetTextFile();
        logic_file.StartOperation(false); // reading

        std::string line;

        // read a single line...
        if( elements_list.elements[0] == -1 )
        {
            if( !text_file.ReadLine(line) )
                return Engine::Value::Bool(false);

            AssignValueToSymbol<SharableString>(
                GetNode<Nodes::SymbolValue>(elements_list.elements[1]),
                std::move(line)
            );

            return Engine::Value::Bool(true);
        }

        // ...or read all lines into a string list
        else
        {
            ASSERT(elements_list.elements[0] == static_cast<int>(SymbolType::List));
            LogicList& logic_list = GetSymbolLogicList(elements_list.elements[1]);

            if( logic_list.IsReadOnly() )
            {
                IssueMessage(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());
                return Engine::Value::Invalid<double>();
            }

            logic_list.Reset();

            while( text_file.ReadLine(line) )
                logic_list.AddValue<SharableString>(std::move(line));

            return Engine::Value::Bool(
                ( logic_list.GetCount() > 0 )
            );
        }
    }

    catch(...)
    {
        return Engine::Value::Bool(false);
    }
}


Engine::Value LogicInterpreter::ex_File_write(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const Nodes::List& elements_list = GetListNode(file_node.elements_list_node);
    LogicFile& logic_file = GetSymbolLogicFile(-1 * file_node.symbol_index_or_string_expression);

    try
    {
        // open the file if it is closed, creating it if necessary
        if( !logic_file.IsOpen() )
        {
            constexpr bool create_if_not_exist = true;
            logic_file.Open(false, false, create_if_not_exist);
        }

        FileIO::TextFile& text_file = logic_file.GetTextFile();
        logic_file.StartOperation(true); // writing

        auto write_line = [&](SharableString line)
        {
            if( m_usingLogicSettingsV0 )
            {
                std::string& modifiable_line = line.MakeModifiable();
                SO::Replace(modifiable_line, "\\\\", "\a");    // BMD 17 Jan 2006
                SO::Replace(modifiable_line, "\\n",  "\n");
                SO::Replace(modifiable_line, "\\f",  "\f");
                SO::Replace(modifiable_line, "\a",   "\\");    // BMD 17 Jan 2006
            }

            text_file.WriteLine(line.Release());
        };

        // write a string list...
        if( elements_list.elements[0] < 0 )
        {
            const LogicList& logic_list = GetSymbolLogicList(-1 * elements_list.elements[0]);
            const size_t line_count = logic_list.GetCount();

            for( size_t i = 1; i <= line_count; ++i )
                write_line(logic_list.GetValue<SharableString>(i));
        }

        // ...or write formatted text
        else
        {
            write_line(EvaluateUserMessage(elements_list.elements[0], FunctionCode::FILEFN_WRITE_CODE));
        }

        return Engine::Value::Bool(true);
    }

    catch(...)
    {
        return Engine::Value::Bool(false);
    }
}
