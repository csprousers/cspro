#include "stdafx.h"
#include "IncludesRT.h"


#ifdef EV_TODO

double CIntDriver::ex_open(const int program_index)
{
    const auto& fn8_node = GetNode<FN8_NODE>(program_index);
    Symbol& symbol = NPT_Ref(fn8_node.symbol_index);
    bool success = true;

    const bool create = ( fn8_node.extra_parameter == static_cast<int>(Nodes::SetFile::Mode::Create) );
    const bool append = ( fn8_node.extra_parameter == static_cast<int>(Nodes::SetFile::Mode::Append) );

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();

        const ConnectionString& connection_string = engine_data_repository.GetLastClosedConnectionString().IsDefined() ?
            engine_data_repository.GetLastClosedConnectionString() :
            engine_data_repository.GetDataRepository().GetConnectionString();

        success = ex_setfile_dictionary(engine_dictionary, connection_string, create, append);
    }

    else if( symbol.IsA(SymbolType::Pre80Dictionary) )
    {
        DICT& pDicT = assert_cast<DICT&>(symbol);
        DICX* const pDicX = pDicT.GetDicX();

        const ConnectionString& connection_string = pDicX->GetLastClosedConnectionString().IsDefined() ?
            pDicX->GetLastClosedConnectionString() :
            pDicX->GetDataRepository().GetConnectionString();

        success = ex_setfile_dictionary(&pDicT, connection_string, create, append);
    }

    else if( symbol.IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(symbol);

        if( !logic_file.Open(create, append, true) )
            success = false;
    }

    return success ? 1 : 0;
}


double CIntDriver::ex_setfile(const int program_index)
{
    const auto& setfile_node = GetNode<Nodes::SetFile>(program_index);
    Symbol& symbol = NPT_Ref(setfile_node.symbol_index);
    bool create = ( setfile_node.mode == Nodes::SetFile::Mode::Create );
    const bool append = ( setfile_node.mode == Nodes::SetFile::Mode::Append );

    if( symbol.IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(symbol);

        // close any existing file
        logic_file.Close();

        std::string file_path = EvaluatePath(setfile_node.filename_expression);

        // create the directory if necessary
        if( ( create || append ) && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(file_path)) )
            return 0;

        bool truncate = true;

        if( append )
        {
            create = true;
            truncate = false;
        }

        logic_file.SetFilePath(std::move(file_path));

        if( !logic_file.GetFilePath().empty() )
        {
            if( logic_file.Open(create, append, truncate) )
                return 1;
        }
    }

    else if( symbol.IsA(SymbolType::Dictionary) )
    {
        const ConnectionString connection_string = EvaluateConnectionString(setfile_node.filename_expression);
        return ex_setfile_dictionary(assert_cast<EngineDictionary&>(symbol), connection_string, create, append);
    }

    else if( symbol.IsA(SymbolType::Pre80Dictionary) )
    {
        const ConnectionString connection_string = EvaluateConnectionString(setfile_node.filename_expression);
        return ex_setfile_dictionary(assert_cast<DICT*>(&symbol), connection_string, create, append);
    }

    else
    {
        ASSERT(false);
    }

    return 0;
}


double CIntDriver::ex_close(const int program_index)
{
    const auto& fn8_node = GetNode<FN8_NODE>(program_index);
    Symbol& symbol = NPT_Ref(fn8_node.symbol_index);
    bool success = true;

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        EngineDataRepository& engine_data_repository = engine_dictionary.GetEngineDataRepository();

        // set to a null repository (OpenRepository will close the current repository)
        m_pEngineDriver->OpenRepository(engine_data_repository, ConnectionString::CreateNullRepositoryConnectionString(),
                                        DataRepositoryOpenFlag::CreateNew, true);

        if( Issamod == ModuleType::Entry && engine_dictionary.GetSubType() == SymbolSubType::Input )
            EntryInputRepositoryChangingActions();
    }

    else if( symbol.IsA(SymbolType::Pre80Dictionary) )
    {
        DICT& pDicT = assert_cast<DICT&>(symbol);
        DICX* pDicX = pDicT.GetDicX();

        // set to a null repository (OpenRepository will close the current repository)
        m_pEngineDriver->OpenRepository(pDicX, ConnectionString::CreateNullRepositoryConnectionString(),
                                        DataRepositoryOpenFlag::CreateNew, true);

        if( Issamod == ModuleType::Entry && pDicT.GetSubType() == SymbolSubType::Input )
            EntryInputRepositoryChangingActions();
    }

    else if( symbol.IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(symbol);

        if( !logic_file.Close() )
            success = false;
    }

    else
    {
        success = ReturnProgrammingError(false);
    }

    return success ? 1 : 0;
}


// Read a line until \n.
// 20140326 for variable length strings (used by fileread)
bool ReadLine(CFile& cFile, CString* pStr, Encoding encoding)
{
    bool    bRet=true;

    try {
    UINT    nBytes, nTotalBytes=0;
#define BUF256                  256
        char    lpBuffer[BUF256];
#ifdef WIN32
        TCHAR   wBuffer[BUF256];
#else
        std::wstring wBuffer;
#endif
        UINT    wBytes = 0;

        ULONGLONG    lCurrentPos=cFile.GetPosition();

        int             iLen=0;
        int             iPosNewLine=-1;

        while( iPosNewLine == -1 && (nBytes=cFile.Read( lpBuffer, BUF256 )) > 0 ) {

            if( encoding == Encoding::Utf8 )
            {
                if( ( BUF256 - nBytes ) < 4 ) // don't let the buffer end in the middle of a character sequence
                {
                    int goBackChars = 0;

                    while( lpBuffer[nBytes + goBackChars - 1] >> 6 == 2 ) // we're in the middle of a sequence
                        goBackChars--;

                    if( lpBuffer[nBytes + goBackChars - 1] & 0xC0 ) // the beginning of a sequence
                        goBackChars--;

                    if( goBackChars )
                    {
                        cFile.Seek(goBackChars,CFile::current);
                        nBytes += goBackChars;
                    }
                }
#ifdef WIN32
                wBytes = MultiByteToWideChar(CP_UTF8,0,lpBuffer,nBytes,wBuffer,BUF256);
#else
                wBuffer = TC::ToWide(lpBuffer, nBytes);
                wBytes = wBuffer.length();
#endif
            }

            else if( encoding == Encoding::Ansi )
            {
#ifdef WIN32
                wBytes = MultiByteToWideChar(CP_ACP,0,lpBuffer,nBytes,wBuffer,BUF256);
#else
                wBuffer = TextConverter::WindowsAnsiToWide(lpBuffer,nBytes);
                wBytes = wBuffer.length();
#endif
            }

            else
            {
                ASSERT(0); // no other encoding supported
            }

            nTotalBytes += nBytes;

            TCHAR * pBuff = pStr->GetBuffer(iLen + wBytes);

            // first fill pBuff
            for( UINT i = 0; i < wBytes && wBuffer[i] != _T('\n'); i++ )
            {
                if( wBuffer[i] != _T('\r') )
                    pBuff[iLen++] = wBuffer[i];
            }

            // now search for the endline in the ANSI/UTF8 string
            for( UINT i = 0; iPosNewLine == -1 && i < nBytes ; i++ )
            {
                if( lpBuffer[i] == '\n' )
                    iPosNewLine = nTotalBytes - nBytes + i;
            }

        }

        pStr->ReleaseBuffer(iLen);

        if( nTotalBytes == 0 )
            bRet = false;

        // Some newline was found
        if( iPosNewLine != -1 ) {
            cFile.Seek( lCurrentPos+iPosNewLine+1, CFile::begin );
        }
    }
    catch(...) {
        bRet = false;
    }

    return bRet;
}


// Open the file if it was closed. Only allows file handler as parameter
double CIntDriver::exfileread(int iExpr)
{
    const auto& file_node = GetNode<Nodes::File>(iExpr);
    const Nodes::List& elements_list = GetListNode(file_node.elements_list_node);

    LogicFile& logic_file = GetSymbolLogicFile(-1 * file_node.symbol_index_or_string_expression);

    // open the file if it is closed
    if( !logic_file.IsOpen() && !logic_file.Open(false, false, false) )
        return 0;

    CFile& cFile2 = logic_file.GetFile();
    CString line;
    bool line_read = false;

    auto read_line = [&]()
    {
        if( ReadLine(cFile2, &line, logic_file.GetEncoding()) )
        {
            line_read = true;
            return true;
        }

         return false;
    };

    LogicList* logic_list = nullptr;
    int destination_expression = 0;

    if( elements_list.elements[0] == -1 )
    {
        destination_expression = elements_list.elements[1];
    }

    else
    {
        ASSERT(elements_list.elements[0] == (int)SymbolType::List);
        logic_list = &GetSymbolLogicList(elements_list.elements[1]);
    }

    // read all lines into a string list...
    if( logic_list != nullptr )
    {
        if( logic_list->IsReadOnly() )
        {
            issaerror(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, logic_list->GetName().c_str());
            return DEFAULT;
        }

        logic_list->Reset();

        while( read_line() )
            logic_list->AddValue<SharableString>(UTF8_TODO::GetUtf8(line));
    }

    // ...or read a single line
    else if( read_line() )
    {
        AssignValueToSymbol(GetNode<Nodes::SymbolValue>(destination_expression), SharableString(UTF8_TODO::GetUtf8(line)));
    }

    return line_read ? 1 : 0;
}


// Open the file if it was closed. Only allows file handler as parameter
double CIntDriver::exfilewrite(int iExpr)
{
    const auto& file_node = GetNode<Nodes::File>(iExpr);
    const Nodes::List& elements_list = GetListNode(file_node.elements_list_node);
    ASSERT(elements_list.number_elements == 1);

    LogicFile& logic_file = GetSymbolLogicFile(-1 * file_node.symbol_index_or_string_expression);

    // open the file if it is closed
    if( !logic_file.IsOpen() && !logic_file.Open(true, false, true) )
        return 0;

    CFile& cFile2 = logic_file.GetFile();

    auto write_line = [&](wstring_view line_sv)
    {
        std::unique_ptr<std::wstring> escaped_line_for_v0;

        if( m_usingLogicSettingsV0 )
        {
            escaped_line_for_v0 = std::make_unique<std::wstring>(line_sv);

            SO::Replace(*escaped_line_for_v0, _T("\\\\"), _T("\a"));    // BMD 17 Jan 2006
            SO::Replace(*escaped_line_for_v0, _T("\\n"),  _T("\n"));
            SO::Replace(*escaped_line_for_v0, _T("\\f"),  _T("\f"));
            SO::Replace(*escaped_line_for_v0, _T("\a"),   _T("\\"));    // BMD 17 Jan 2006
            line_sv = *escaped_line_for_v0;
        }

        ASSERT(logic_file.GetEncoding() == Encoding::Utf8);

        const std::string utf_buffer = UTF8_TODO::GetUtf8(line_sv);
        cFile2.Write(utf_buffer.c_str(), utf_buffer.length());

        constexpr char NewlineChar = '\n';
        cFile2.Write(&NewlineChar, 1);
    };

    try
    {
        // write a string list...
        if( elements_list.elements[0] < 0 )
        {
            const LogicList& logic_list = GetSymbolLogicList(-1 * elements_list.elements[0]);
            const size_t line_count = logic_list.GetCount();

            for( size_t i = 1; i <= line_count; ++i )
                write_line(UTF8_TODO::GetWide(logic_list.GetValue<SharableString>(i).GetString()));
        }

        // ...or write formatted text
        else
        {
            write_line(UTF8_TODO::GetWide(*EvaluateUserMessage(elements_list.elements[0], FunctionCode::FNFILE_WRITE_CODE)));
        }
    }

    catch(...)
    {
        return 0;
    }

    return 1;
}

#endif
