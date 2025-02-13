#include "StandardSystemIncludes.h"
#include "Engine.h"
#include "Preprocessor.h"
#include <zAppO/Application.h>
#include <zJson/JsonKeys.h>


EnginePreprocessor::EnginePreprocessor(Logic::BasicTokenCompiler& compiler, CEngineDriver* pEngineDriver)
    :   Logic::Preprocessor(compiler),
        m_pEngineDriver(pEngineDriver),
        m_symbolTable(m_pEngineDriver->getEngineAreaPtr()->GetSymbolTable()),
        m_initialSymbolTableSize(m_symbolTable.GetTableSize())
{            
}


const TCHAR* EnginePreprocessor::GetAppType()
{
    return ToString(m_pEngineDriver->m_pApplication->GetEngineAppType());
}


Symbol* EnginePreprocessor::FindSymbol(const std::wstring& name, const bool search_only_base_symbols)
{
    for( Symbol* symbol : m_symbolTable.FindSymbols(name) )
    {
        if( !search_only_base_symbols || symbol->GetSymbolIndex() < static_cast<int>(m_initialSymbolTableSize) )
            return symbol;
    }

    return nullptr;
}


template<>
std::optional<bool> EnginePreprocessor::ParseValue(const std::variant<double, std::wstring>& value)
{
    if( std::holds_alternative<double>(value) )
    {
        return ( std::get<double>(value) == 1 ) ? std::make_optional(true) :
               ( std::get<double>(value) == 0 ) ? std::make_optional(false) :
                                                  std::nullopt;
    }

    else
    {
        return ( std::get<std::wstring>(value) == _T("true") )  ? std::make_optional(true) :
               ( std::get<std::wstring>(value) == _T("false") ) ? std::make_optional(false) :
                                                                  std::nullopt;
    }
}


void EnginePreprocessor::SetProperty(Symbol* symbol, const std::wstring& attribute, const std::variant<double, std::wstring>& value)
{
    auto issue_value_error = [&]()
    {
        std::wstring error_message = FormatTextCS2WS(_T("the value '%s' is invalid for attribute '%s'"),
                                                     std::holds_alternative<double>(value) ? DoubleToString(std::get<double>(value)).c_str() : std::get<std::wstring>(value).c_str(),
                                                     attribute.c_str());

        if( symbol != nullptr )
            error_message.append(FormatTextCS2WS(_T(" for symbol type '%s'"), ToString(symbol->GetType())));

        IssueError(69, error_message.c_str());
    };

    if( symbol != nullptr && symbol->IsA(SymbolType::Pre80Dictionary) && attribute == JK::readOptimization ) // ENGINECR_TODO implement for non-DICT
    {
        const std::optional<bool> use_read_optimization = ParseValue<bool>(value);

        if( !use_read_optimization.has_value() )
            issue_value_error();

        if( !*use_read_optimization )
            assert_cast<DICT&>(*symbol).GetCaseAccess()->SetRequiresFullAccess();

        return;
    }

    std::wstring error_message = FormatTextCS2WS(_T("the attribute '%s' is invalid"), attribute.c_str());

    if( symbol != nullptr )
        error_message.append(FormatTextCS2WS(_T(" for symbol type '%s'"), ToString(symbol->GetType())));

    IssueError(69, error_message.c_str());
}

