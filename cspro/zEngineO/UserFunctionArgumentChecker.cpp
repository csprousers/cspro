#include "stdafx.h"
#include "UserFunctionArgumentChecker.h"
#include "AllSymbols.h"
#include <engine/Ctab.h>


// --------------------------------------------------------------------------
// UserFunctionArgumentChecker
// --------------------------------------------------------------------------

UserFunctionArgumentChecker::UserFunctionArgumentChecker(const UserFunction& user_function)
    :   m_userFunction(user_function),
        m_parameterNumber(SIZE_MAX),
        m_parameterSymbol(nullptr)
{
}


void UserFunctionArgumentChecker::CheckNumberArguments(const size_t number_arguments) const
{
    if( number_arguments >= m_userFunction.GetNumberRequiredParameters() &&
        number_arguments <= m_userFunction.GetNumberParameters() )
    {
        return;
    }

    if( m_userFunction.GetNumberParameters() == m_userFunction.GetNumberRequiredParameters() )
    {
        throw CheckError("%d argument%s",
                         static_cast<int>(m_userFunction.GetNumberParameters()),
                         PluralizeWord(m_userFunction.GetNumberParameters()));
    }

    else
    {
        throw CheckError("at least %d (and up to %d) arguments",
                         static_cast<int>(m_userFunction.GetNumberRequiredParameters()),
                         static_cast<int>(m_userFunction.GetNumberParameters()));
    }
}


void UserFunctionArgumentChecker::IssueArgumentError(const cs::string_sz extra_error_text/* = ""*/) const
{
    ASSERT(m_parameterNumber != SIZE_MAX && m_parameterSymbol != nullptr);

    std::string error_message = GetExpectedArgumentText();

    if( !extra_error_text.empty() )
    {
        error_message.append(" ")
                     .append(extra_error_text.c_str());
    }

    error_message.append(" as argument #")
                 .append(IntToString(m_parameterNumber + 1));

    // add the symbol name if it is not a function pointer's parameter (which will start with an underscore)
    if( m_parameterSymbol->GetName().front() != '_' )
    {
        error_message.append(" ('")
                     .append(m_parameterSymbol->GetName())
                     .append("')");
    }

    throw CheckError(error_message);
}


const char* UserFunctionArgumentChecker::GetExpectedArgumentText(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Array:          return "an Array";
        case SymbolType::Audio:          return "an Audio object";
        case SymbolType::Dictionary:     return assert_cast<const EngineDictionary&>(symbol).IsCaseObject() ?
                                                "a Case" :
                                                "a DataSource";
        case SymbolType::Document:       return "a Document object";
        case SymbolType::File:           return "a File handler";
        case SymbolType::Geometry:       return "a Geometry object";
        case SymbolType::HashMap:        return "a HashMap";
        case SymbolType::Image:          return "an Image object";
        case SymbolType::List:           return "a List";
        case SymbolType::Map:            return "a Map";
        case SymbolType::NamedFrequency: return "a named frequency";
        case SymbolType::Pff:            return "a Pff object";
        case SymbolType::Report:         return "a Report";
        case SymbolType::StringWriter:   return "a StringWriter";
        case SymbolType::SystemApp:      return "a SystemApp";
        case SymbolType::UserFunction:   return "a function pointer";
        case SymbolType::ValueSet:       return "a value set";
        case SymbolType::WorkString:     return "a string expression";
        case SymbolType::WorkVariable:   return "a numeric expression";
        default:                         return ReturnProgrammingError(ToString(symbol.GetType()));
    }
}


const char* UserFunctionArgumentChecker::GetExpectedArgumentText() const
{
    ASSERT(m_parameterSymbol != nullptr);

    return GetExpectedArgumentText(*m_parameterSymbol);
}


bool UserFunctionArgumentChecker::SymbolTypeIsAnExpression(const SymbolType symbol_type)
{
    return ( symbol_type == SymbolType::WorkVariable ||
             symbol_type == SymbolType::WorkString );
}


bool UserFunctionArgumentChecker::ArgumentShouldBeExpression(const size_t parameter_number) const
{
    return SymbolTypeIsAnExpression(m_userFunction.GetParameterSymbol(parameter_number).GetType());
}


void UserFunctionArgumentChecker::CheckExpressionArgument(const size_t parameter_number, const SymbolType argument_type)
{
    ASSERT(ArgumentShouldBeExpression(parameter_number));

    m_parameterNumber = parameter_number;
    m_parameterSymbol = &m_userFunction.GetParameterSymbol(parameter_number);

    if( !m_parameterSymbol->IsA(argument_type) )
        IssueArgumentError();
}


void UserFunctionArgumentChecker::CheckExpressionArgument(const size_t parameter_number, const bool argument_is_numeric_expression)
{
    CheckExpressionArgument(parameter_number, argument_is_numeric_expression ? SymbolType::WorkVariable :
                                                                               SymbolType::WorkString);
}


void UserFunctionArgumentChecker::CheckSymbolArgument(const size_t parameter_number, Symbol* const argument_symbol)
{
    ASSERT(!ArgumentShouldBeExpression(parameter_number));

    m_parameterNumber = parameter_number;
    m_parameterSymbol = &m_userFunction.GetParameterSymbol(parameter_number);

    if( argument_symbol == nullptr )
        IssueArgumentError();

    SymbolType argument_symbol_type = argument_symbol->GetType();

    // items are not allowed as parameters (as of 8.0), so use the wrapped type
    ASSERT(!m_parameterSymbol->IsA(SymbolType::Item));
    ASSERT(argument_symbol->IsA(SymbolType::Item) || argument_symbol->GetWrappedType() == SymbolType::None);

    if( argument_symbol_type == SymbolType::Item )
        argument_symbol_type = argument_symbol->GetWrappedType();

    if( SymbolTypeIsAnExpression(argument_symbol_type) )
        IssueArgumentError();

    auto argument_can_be_implicity_converted_to_parameter = [&]()
    {
        if( m_parameterSymbol->IsA(SymbolType::Array) )
        {
            return ( argument_symbol_type == SymbolType::Crosstab );
        }

        else
        {
            return false;
        }
    };

    // make sure the symbol type matches
    if( m_parameterSymbol->GetType() != argument_symbol_type &&
        !argument_can_be_implicity_converted_to_parameter() )
    {
        IssueArgumentError();
    }

    // make sure the symbol's data type matches (when applicable)
    const std::optional<DataType> parameter_data_type = SymbolCalculator::GetOptionalDataType(*m_parameterSymbol);

    if( parameter_data_type.has_value() && parameter_data_type != SymbolCalculator::GetDataType(*argument_symbol) )
    {
        const std::string data_type_text = SO::ToLower(ToString(*parameter_data_type));

        if( m_parameterSymbol->IsA(SymbolType::UserFunction) )
        {
            IssueArgumentError(FormatText("that returns '%s'", data_type_text.c_str()));
        }

        else
        {
            IssueArgumentError(FormatText("of type '%s'", data_type_text.c_str()));
        }
    }

    // additional checks/operations for symbols
    try
    {
        if( m_parameterSymbol->IsA(SymbolType::Array) )
        {
            CheckLogicArrayArgument(*argument_symbol);
        }

        else if( m_parameterSymbol->IsA(SymbolType::Dictionary) )
        {
            CheckEngineDictionaryArgument(assert_cast<EngineDictionary&>(*argument_symbol));
        }

        else if( m_parameterSymbol->IsA(SymbolType::File) )
        {
            CheckLogicFileArgument(assert_cast<LogicFile&>(*argument_symbol));
        }

        else if( m_parameterSymbol->IsA(SymbolType::HashMap) )
        {
            assert_cast<const LogicHashMap&>(*argument_symbol).CompareDeclarationAttributes(*m_parameterSymbol);
        }

        else if( m_parameterSymbol->IsA(SymbolType::UserFunction) )
        {
            CheckUserFunctionArgument(assert_cast<UserFunction&>(*argument_symbol));
        }
    }

    catch( const Symbol::CompareDeclarationAttributesException& exception )
    {
        IssueArgumentError(SO::Concatenate("with ", exception.what()));
    }
}


std::optional<size_t> UserFunctionArgumentChecker::FindFirstInvalidParameter(const cs::span<const SymbolType> valid_parameter_symbol_types,
                                                                             const bool include_numeric_and_string_parameters) const noexcept
{
    for( size_t i = 0; i < m_userFunction.GetNumberParameters(); ++i )
    {
        const SymbolType symbol_type = m_userFunction.GetParameterSymbol(i).GetType();

        if( !include_numeric_and_string_parameters && ( symbol_type == SymbolType::WorkString ||
                                                        symbol_type == SymbolType::WorkVariable ) )
        {
            continue;
        }

        if( std::find(std::cbegin(valid_parameter_symbol_types), std::cend(valid_parameter_symbol_types),
                      m_userFunction.GetParameterSymbol(i).GetType()) == std::cend(valid_parameter_symbol_types) )
        {
            return i;
        }
    }

    return std::nullopt;
}



// --------------------------------------------------------------------------
// Array (additional checks...the data type has already been checked)
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::CheckLogicArrayArgument(const Symbol& argument_symbol) const
{
    const LogicArray& parameter_array = assert_cast<const LogicArray&>(*m_parameterSymbol);

    if( argument_symbol.IsA(SymbolType::Array) )
    {
        parameter_array.CompareDeclarationAttributes(argument_symbol);
    }

    else
    {
#ifdef WIN_DESKTOP
        // because arrays were previously crosstabs, there is some code that manipulates tables by
        // passing a crosstab to a function; we will allow this for numeric crosstabs
        ASSERT(argument_symbol.IsA(SymbolType::Crosstab));
        const CTAB& argument_crosstab = assert_cast<const CTAB&>(argument_symbol);

        // make sure the number of dimensions is the same
        if( parameter_array.GetNumberDimensions() != static_cast<size_t>(argument_crosstab.GetNumDim()) )
        {
            IssueArgumentError(FormatText("with %d dimensions",
                                          static_cast<int>(parameter_array.GetNumberDimensions())));
        }
#endif
    }
}



// --------------------------------------------------------------------------
// dictionary (Case/DataSource additional checks/operations)
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::CheckEngineDictionaryArgument(EngineDictionary& argument_engine_dictionary) const
{
    const EngineDictionary& parameter_engine_dictionary = assert_cast<const EngineDictionary&>(*m_parameterSymbol);

    // make sure that a Case matches a Case/dictionary and a DataSource matches a DataSource/dictionary
    if( ( parameter_engine_dictionary.IsCaseObject() && !argument_engine_dictionary.HasEngineCase() ) ||
        ( parameter_engine_dictionary.IsDataRepositoryObject() && !argument_engine_dictionary.HasEngineDataRepository() ) )
    {
        IssueArgumentError();
    }

    // make sure the dictionary matches
    if( !parameter_engine_dictionary.DictionaryMatches(argument_engine_dictionary) )
    {
        IssueArgumentError(FormatText("based on the dictionary '%s'", parameter_engine_dictionary.GetDictionary().GetName().c_str()));
    }

    // Case checks
    if( parameter_engine_dictionary.IsCaseObject() )
    {
        // the case must be an external dictionary
        // ENGINECR_TODO should also make sure dictionaries for external forms can't be used
        if( argument_engine_dictionary.GetSubType() != SymbolSubType::External )
            IssueArgumentError("from an external dictionary");
    }

    // DataSource checks
    else
    {
        ASSERT(parameter_engine_dictionary.IsDataRepositoryObject());
        const EngineDataRepository& parameter_engine_data_repository = parameter_engine_dictionary.GetEngineDataRepository();

        // the DataSource argument must assume any permissions that occur to the data repository in the function
        // ENGINECR_TODO ... need to think through how to do this...maybe some runtime checks need to be
        // added because ApplyPermissions can lead to conflicting flags (like m_needsIndex and m_cannotHaveIndex)
        argument_engine_dictionary.GetEngineDataRepository().ApplyPermissions(parameter_engine_data_repository);

        // if the data repository is written to, make sure that this is an external dictionary
        // ENGINECR_TODO is this necessary?
        if( parameter_engine_data_repository.GetIsWriteable() &&
            argument_engine_dictionary.GetSubType() != SymbolSubType::External )
        {
            IssueArgumentError("from an external dictionary");
        }
    }
}



// --------------------------------------------------------------------------
// File (additional operations)
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::CheckLogicFileArgument(LogicFile& argument_file) const
{
    const LogicFile& parameter_file = assert_cast<const LogicFile&>(*m_parameterSymbol);

    // mark the file as used...
    argument_file.SetUsed();

    argument_file.CopyCompileTimeAttributes(parameter_file);
}



// --------------------------------------------------------------------------
// user-defined functions (additional checks...the return value has already
//                         been checked)
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::CheckUserFunctionArgument(UserFunction& argument_user_function) const
{
    const UserFunction& parameter_user_function = assert_cast<const UserFunction&>(*m_parameterSymbol);

    // check that the parameters match...
    if( parameter_user_function.GetNumberParameters() != argument_user_function.GetNumberParameters() )
    {
        IssueArgumentError(FormatText("with %d parameters",
                                      static_cast<int>(parameter_user_function.GetNumberParameters())));
    }

    // ...and are compatible
    UserFunctionArgumentChecker parameter_user_function_argument_checker(parameter_user_function);

    for( size_t i = 0; i < parameter_user_function.GetNumberParameters(); ++i )
    {
        const Symbol& parameter_parameter_symbol = parameter_user_function.GetParameterSymbol(i);
        Symbol& argument_parameter_symbol = argument_user_function.GetParameterSymbol(i);
        bool symbols_are_compatible = ( parameter_parameter_symbol.GetType() == argument_parameter_symbol.GetType() );

        if( symbols_are_compatible )
        {
            // expressions do not need to be checked further
            if( SymbolTypeIsAnExpression(parameter_parameter_symbol.GetType()) )
                continue;

            try
            {
                parameter_user_function_argument_checker.CheckSymbolArgument(i, &argument_parameter_symbol);
            }

            catch( const CheckError& )
            {
                symbols_are_compatible = false;
            }
        }

        if( !symbols_are_compatible )
        {
            IssueArgumentError(FormatText("with a matching parameter #%d (%s)",
                                          static_cast<int>(i) + 1, GetExpectedArgumentText(parameter_parameter_symbol)));
        }
    }
}



// --------------------------------------------------------------------------
// these functions perform any operations done in the above methods on a
// symbol so that the symbol encompasses all possible uses
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::MarkSymbolAsDynamicallyBoundToFunctionParameter(Symbol& symbol)
{
    // dictionary
    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);

        if( engine_dictionary.IsDataRepositoryObject() )
        {
            // ENGINECR_TODO ... look at the above notes (also marked with ENGINECR_TODO)
            // about issues with ApplyPermissions
        }
    }

    // file
    else if( symbol.IsA(SymbolType::File) )
    {
        LogicFile& logic_file = assert_cast<LogicFile&>(symbol);

        // because we do not know exactly how the file will be used, mark it as written to cover all bases
        logic_file.SetIsWrittenTo();
    }
}


void UserFunctionArgumentChecker::MarkParametersAsUsedInFunctionPointerUse(UserFunction& user_function)
{
    for( size_t i = 0; i < user_function.GetNumberParameters(); ++i )
        MarkSymbolAsDynamicallyBoundToFunctionParameter(user_function.GetParameterSymbol(i));
}
