//----------------------------------------------------------------------
//  COMPISSA.CPP
//----------------------------------------------------------------------
#include "StandardSystemIncludes.h"

#ifdef GENCODE
#include "Exappl.h"
#else
#include "Tables.h"
#endif
#include "COMPILAD.H"
#include "Engine.h"
#include "Ctab.h"
#include "Preprocessor.h"
#include <zEngineO/JavaScriptProcessor.h>
#include <zEngineO/StringWriter.h>
#include <zToolsO/RaiiHelpers.h>
#include <zAppO/Application.h>
#include <zLogicO/LocalSymbolStack.h>
#include <zLogicO/TextTemplateTokenizer.h>
#include <zCapiO/CapiCondition.h>
#include <zCapiO/CapiLogicParameters.h>
#include <zDesignerF/UWM.h>


int CEngineCompFunc::rutasync(const int symbol_index, const std::function<void()>* const compilation_function/* = nullptr*/)
{
    clearSyntaxErrorStatus();

    m_allowMultVarWithoutIndex = false;

    const Symbol& compilation_symbol = NPT_Ref(symbol_index);
    SetCompilationSymbol(compilation_symbol);

    ObjInComp = compilation_symbol.GetType();
    InCompIdx = symbol_index;

    LvlInComp = SymbolCalculator::GetLevelNumber_base1(compilation_symbol);

    // preprocess the source buffer
    if( m_preprocessor == nullptr )
        m_preprocessor = std::make_unique<EnginePreprocessor>(*this, m_pEngineDriver);

    m_preprocessor->ProcessBuffer();

    // compile the source buffer
    if( !m_engineData->logic_byte_code.EnlargeBufferForOneProc() )
        ReportError(4);

    try
    {
        if( compilation_function != nullptr )
        {
            (*compilation_function)();
        }

        else if( ObjInComp == SymbolType::Application )
        {
            CompileApplication();
        }

        else
        {
            CompileSymbolProcs();
        }
    }

    catch( const Logic::ParserError& )
    {
        // the error should have already been reported
    }

    catch( const CSProException& exception )
    {
        ReportError(MGF::OpenMessage, exception.what());
    }

    catch(...)
    {
        ASSERT(false);
    }

    return GetSyntErr();
}


void CEngineCompFunc::CompileApplication()
{
    NextToken();

    if( Appl.ApplicationType == ModuleType::Entry || Appl.ApplicationType == ModuleType::Batch )
    {
        if( CompileDeclarations() )
        {
            if( Tkn != TOKEOP )
                SetSyntErr(1);
        }
    }
}


void CEngineCompFunc::CompileExternalCodeLogic(const CodeFile& code_file)
{
    ASSERT(code_file.GetCodeType() == CodeType::LogicExternal);

    try
    {
        SetSourceBuffer(code_file.GetTextSource());
    }

    catch( const CSProException& exception )
    {
        // report any errors reading the external code file
        ReportError(163, "Logic", exception.what());
        return;
    }

    try
    {
        if( rutasync(Appl.GetSymbolIndex()) )
            ReportError(GetSyntErr(), Path::GetFilename(code_file.GetFilePath()).c_str());
    }
    catch(...) { ASSERT(false); }
}


void CEngineCompFunc::CompileExternalCodeJavaScript(const CodeFile& code_file)
{
    ASSERT(code_file.IsJavaScript());

    // skip compiling files that haven't changed
    if( WindowsDesktopMessage::Send(UWM::Designer::CanCodeFileCompilationBeSkipped, &code_file) == 1 )
        return;

    // compile the JavaScript
    ClearSourceBuffer();
    SetCompilationUnitName(code_file.GetFilePath());

    auto report_error = [&](const cs::string_sz error_message, Logic::ParserMessage::ExtendedLocation extended_location)
    {
        ReportError(std::move(extended_location), MGF::OpenMessage, error_message.c_str());
    };

    try
    {
        EngineJavaScriptProcessor& javascript_processor = m_engineData->GetJavaScriptProcessor();
        javascript_processor.CompileCodeFile(code_file);

        // mark the JavaScript as successfully compiled
        WindowsDesktopMessage::Send(UWM::Designer::SetCodeFileSuccessfullyCompiled, &code_file);
    }

    catch( const JavaScript::Exception& exception )
    {
        // add the location details when the error occurs in this file
        if( exception.HasLocationDetails() && SO::EqualsNoCase(code_file.GetFilePath(), exception.GetFilePath()) )
        {
            report_error(exception.GetBaseMessage(), Logic::ParserMessage::LineNumberOverride { static_cast<size_t>(exception.GetLineNumber()) });
        }

        else
        {
            report_error(exception.what(), std::monostate());
        }
    }

    catch( const CSProException& exception )
    {
        report_error(exception.what(), std::monostate());
    }
}


void CEngineCompFunc::CompileSymbolProcs()
{
    Symbol& symbol = NPT_Ref(InCompIdx);

    ASSERT(symbol.IsOneOf(SymbolType::Group,
                          SymbolType::Block,
                          SymbolType::Variable,
                          SymbolType::Crosstab));

    // start the local symbol stack
    Logic::LocalSymbolStack local_symbol_stack = m_symbolTable.CreateLocalSymbolStack();

    std::set<ProcType> procs_defined;

    bool bCtab = ( ObjInComp == SymbolType::Crosstab );

    NextToken();

    while( Tkn != TOKEOP && Tkn != TOKERROR && GetSyntErr() == 0 )
    {
        ProcType proc_type = ProcType::None;

        if( Tkn == TOKPREPRO )
        {
            proc_type = ProcType::PreProc;
        }

        else if( Tkn == TOKONFOCUS )
        {
            proc_type = ProcType::OnFocus;
        }

        else if( Tkn == TOKONOCCCHANGE )
        {
            // make sure that this is a group
            if( m_pEngineArea->IsLevel(InCompIdx) || ObjInComp != SymbolType::Group )
                IssueError(60003);

            proc_type = ProcType::OnOccChange;
        }

        else if( Tkn == TOKKILLFOCUS )
        {
            proc_type = ProcType::KillFocus;
        }

        else if( Tkn == TOKTALLY )
        {
            if( !bCtab )
                IssueError(60004);

            proc_type = ProcType::Tally;
        }

        else if( Tkn == TOKPOSTCALC )
        {
            if( !bCtab && !m_pEngineArea->IsLevel(InCompIdx) )
                IssueError(60005);

            proc_type = ProcType::ExplicitCalc;
        }

        else if( Tkn == TOKPOSTPRO )
        {
            proc_type = ProcType::PostProc;
        }

        // default is postproc, but only allowed if they haven't specifed any procs
        else if( procs_defined.size() == 0 )
        {
            proc_type = ProcType::PostProc;
        }

        // a proc type has to be specified (once they've already specified at least one type)
        else
        {
            IssueError(60002);
        }

        // make sure that the they haven't already specified a proc of this type
        if( procs_defined.find(proc_type) != procs_defined.end() )
            IssueError(60000, GetProcTypeName(proc_type));

        // make sure that crosstabs don't have invalid proc types
        if( bCtab && proc_type != ProcType::Tally && proc_type != ProcType::ExplicitCalc )
            IssueError(60006);

        // make sure that the proc types come in proper order
        for( ProcType previous_proc_type : procs_defined )
        {
            if( !IsProcTypeOrderCorrect(previous_proc_type, proc_type) )
                IssueError(60001, GetProcTypeName(proc_type), GetProcTypeName(previous_proc_type));
        }

        procs_defined.insert(proc_type);

        SetCompilationProcType(proc_type);

        // allow a colon after the proc type
        if( Tkn == TOKPREPRO || Tkn == TOKONFOCUS || Tkn == TOKKILLFOCUS || Tkn == TOKPOSTPRO || Tkn == TOKONOCCCHANGE || Tkn == TOKTALLY || Tkn == TOKPOSTCALC )
        {
            NextToken();

            if( Tkn == TOKCOLON )
                NextToken();
        }

        int starting_proc_index = Prognext;
        int proc_index = -1;

        if( Tkn != TOKPREPRO && Tkn != TOKONFOCUS && Tkn != TOKKILLFOCUS && Tkn != TOKPOSTPRO && Tkn != TOKONOCCCHANGE  && Tkn != TOKTALLY && Tkn != TOKPOSTCALC )
            proc_index = instruc(false);

        if( GetSyntErr() != 0 )
        {
            // Should not be here in the near future
            // cause every error will be detected and handled by
            // exceptions
            return;
        }

        if( starting_proc_index != static_cast<int>(m_engineData->logic_byte_code.GetSize()) )
        {
            proc_index = WrapNodeAroundScopeChange(local_symbol_stack, proc_index);

            RunnableSymbol& runnable_symbol = dynamic_cast<RunnableSymbol&>(symbol);
            runnable_symbol.SetProcIndex(proc_type, proc_index);

            if( symbol.IsA(SymbolType::Variable) )
                assert_cast<VART&>(symbol).SetUsed(true);
        }
    }

    if( GetSyntErr() == 0 && Tkn != TOKEOP )
        SetSyntErr(1);
}


int CEngineCompFunc::CompileCapiLogic(const CapiLogicParameters& capi_logic_parameters)
{
    // lookup the symbol
    const Symbol* symbol = nullptr;

    if( std::holds_alternative<int>(capi_logic_parameters.symbol_index_or_name) )
    {
        if( std::get<int>(capi_logic_parameters.symbol_index_or_name) > 0 )
            symbol = NPT(std::get<int>(capi_logic_parameters.symbol_index_or_name));
    }

    else
    {
        try
        {
            symbol = &m_symbolTable.FindSymbolWithDotNotation(std::get<std::string>(capi_logic_parameters.symbol_index_or_name));
        }
        catch(...) { }
    }

    if( symbol == nullptr )
    {
        ASSERT(false);
        return -1;
    }

    ASSERT(symbol->IsOneOf(SymbolType::Block, SymbolType::Variable));

    const bool condition_type = std::holds_alternative<const CapiCondition*>(capi_logic_parameters.condition_or_text);
    int question_text_node_index = -1;

    const std::function<void()> compilation_function = [&]()
    {
        SetCompilationProcType(ProcType::OnFocus, ExtendedProcType::QuestionText);

        try
        {
            NextToken();

            // conditions are always numeric expressions,
            // whereas the question text is evaluated as a text template
            question_text_node_index = condition_type ? exprlog() :
                                                        instruc(false);

            if( Tkn != TOKEOP || GetSyntErr() != 0 )
            {
                ASSERT(condition_type);
                IssueError(48011);
            }
        }

        catch(...)
        {
            incrementErrors();
            throw;
        }
    };

    try
    {
        SetCompilationSymbol(*symbol);
        SetCapiLogicLocation(capi_logic_parameters.capi_logic_location);

        std::unique_ptr<Logic::SourceBuffer> source_buffer;
        std::optional<Logic::LocalSymbolStack> local_symbol_stack;

        if( condition_type )
        {
            const CapiCondition& condition = *std::get<const CapiCondition*>(capi_logic_parameters.condition_or_text);
            source_buffer = std::make_unique<Logic::SourceBuffer>(condition.GetLogic());
        }

        else
        {
            const CapiText& capi_text = *std::get<const CapiText*>(capi_logic_parameters.condition_or_text);

            ErrorReportingTextTemplateTokenizer text_template_tokenizer(*this, capi_text.FormatSupportsLogicEscapes());

            if( text_template_tokenizer.Tokenize(capi_text.GetText().GetString(), GetLogicSettings()) )
            {
                // if the question text does not use any fills or logic, there is no reason to
                // process it as a text template
                if( text_template_tokenizer.IsOnlyDirectTextUsed() )
                    return -1;

                source_buffer = ConvertTextTemplateToSourceBuffer(QuestionTextStringWriterName, text_template_tokenizer);
            }

            // ConvertTextTemplateToSourceBuffer reports errors but they are not thrown
            if( source_buffer == nullptr )
                throw std::exception();

            // add the question text StringWriter object for this compilation
            local_symbol_stack.emplace(m_symbolTable.CreateLocalSymbolStack());

            if( m_engineData->question_text_string_writer == nullptr )
                m_engineData->question_text_string_writer = std::make_unique<StringWriter>(QuestionTextStringWriterName, *m_engineData);

            m_engineData->question_text_string_writer->ResetForQuestionText(capi_text.GetEncodeType());

            m_symbolTable.AddReusableSymbol(m_engineData->question_text_string_writer);
        }

        ASSERT(source_buffer != nullptr);
        SetSourceBuffer(std::move(source_buffer));

        if( rutasync(symbol->GetSymbolIndex(), &compilation_function) )
            ReportError(GetSyntErr());
    }

    catch(...)
    {
        incrementErrors();
    }

    return question_text_node_index;
}
