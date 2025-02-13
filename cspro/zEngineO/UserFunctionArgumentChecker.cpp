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


void UserFunctionArgumentChecker::CheckNumberArguments(size_t number_arguments) const
{
    if( number_arguments >= m_userFunction.GetNumberRequiredParameters() &&
        number_arguments <= m_userFunction.GetNumberParameters() )
    {
        return;
    }

    if( m_userFunction.GetNumberParameters() == m_userFunction.GetNumberRequiredParameters() )
    {
        throw UserFunctionArgumentChecker::CheckError(_T("%d argument%s"),
                                                      static_cast<int>(m_userFunction.GetNumberParameters()),
                                                      PluralizeWord(m_userFunction.GetNumberParameters()));
    }

    else
    {
        throw UserFunctionArgumentChecker::CheckError(_T("at least %d (and up to %d) arguments"),
                                                      static_cast<int>(m_userFunction.GetNumberRequiredParameters()),
                                                      static_cast<int>(m_userFunction.GetNumberParameters()));
    }
}


void UserFunctionArgumentChecker::IssueArgumentError(const TCHAR* extra_error_text/* = nullptr*/) const
{
    ASSERT(m_parameterNumber != SIZE_MAX && m_parameterSymbol != nullptr);

    std::wstring error_message = GetExpectedArgumentText();

    if( extra_error_text != nullptr )
        SO::Append(error_message, _T(" "), extra_error_text);

    SO::Append(error_message, _T(" as argument #"), IntToString(m_parameterNumber + 1));

    // add the symbol name if it is not a function pointer's parameter (which will have .p in the name)
    if( m_parameterSymbol->GetName().find(_T(".p")) == std::wstring::npos )
        SO::Append(error_message, _T(" ('"), m_parameterSymbol->GetName(), _T("')"));;

    throw UserFunctionArgumentChecker::CheckError(error_message);
}


const TCHAR* UserFunctionArgumentChecker::GetExpectedArgumentText(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Array:          return _T("an Array");
        case SymbolType::Audio:          return _T("an Audio object");
        case SymbolType::Dictionary:     return assert_cast<const EngineDictionary&>(symbol).IsCaseObject() ?
                                                         _T("a Case") :
                                                         _T("a DataSource");
        case SymbolType::Document:       return _T("a Document object");
        case SymbolType::File:           return _T("a File handler");
        case SymbolType::Geometry:       return _T("a Geometry object");
        case SymbolType::HashMap:        return _T("a HashMap");
        case SymbolType::Image:          return _T("an Image object");
        case SymbolType::List:           return _T("a List");
        case SymbolType::Map:            return _T("a Map");
        case SymbolType::NamedFrequency: return _T("a named frequency");
        case SymbolType::Pff:            return _T("a Pff object");
        case SymbolType::Report:         return _T("a Report");
        case SymbolType::SystemApp:      return _T("a SystemApp");
        case SymbolType::UserFunction:   return _T("a function pointer");
        case SymbolType::ValueSet:       return _T("a value set");
        case SymbolType::WorkString:     return _T("a string expression");
        case SymbolType::WorkVariable:   return _T("a numeric expression");
        default:                                  return ReturnProgrammingError(ToString(symbol.GetType()));
    }
}


const TCHAR* UserFunctionArgumentChecker::GetExpectedArgumentText() const
{
    ASSERT(m_parameterSymbol != nullptr);

    return GetExpectedArgumentText(*m_parameterSymbol);
}


bool UserFunctionArgumentChecker::SymbolTypeIsAnExpression(SymbolType symbol_type)
{
    return ( symbol_type == SymbolType::WorkVariable ||
             symbol_type == SymbolType::WorkString );
}


bool UserFunctionArgumentChecker::ArgumentShouldBeExpression(size_t parameter_number) const
{
    return SymbolTypeIsAnExpression(m_userFunction.GetParameterSymbol(parameter_number).GetType());
}


void UserFunctionArgumentChecker::CheckExpressionArgument(size_t parameter_number, SymbolType argument_type)
{
    ASSERT(ArgumentShouldBeExpression(parameter_number));

    m_parameterNumber = parameter_number;
    m_parameterSymbol = &m_userFunction.GetParameterSymbol(parameter_number);

    if( !m_parameterSymbol->IsA(argument_type) )
        IssueArgumentError();
}


void UserFunctionArgumentChecker::CheckExpressionArgument(size_t parameter_number, bool argument_is_numeric_expression)
{
    CheckExpressionArgument(parameter_number, argument_is_numeric_expression ? SymbolType::WorkVariable :
                                                                               SymbolType::WorkString);
}


void UserFunctionArgumentChecker::CheckSymbolArgument(size_t parameter_number, Symbol* argument_symbol)
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
    if( m_parameterSymbol->GetType() != argument_symbol_type && !argument_can_be_implicity_converted_to_parameter() )
    {
        IssueArgumentError();
    }

    // make sure the symbol's data type matches (when applicable)
    const std::optional<DataType> parameter_data_type = SymbolCalculator::GetOptionalDataType(*m_parameterSymbol);

    if( parameter_data_type.has_value() && parameter_data_type != SymbolCalculator::GetDataType(*argument_symbol) )
    {
        const std::wstring data_type_text = SO::ToLower(ToString(*parameter_data_type));

        if( m_parameterSymbol->IsA(SymbolType::UserFunction) )
        {
            IssueArgumentError(FormatText(_T("that returns '%s'"), data_type_text.c_str()));
        }

        else
        {
            IssueArgumentError(FormatText(_T("of type '%s'"), data_type_text.c_str()));
        }
    }

    // additional checks/operations for symbols
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
        CheckLogicHashMapArgument(assert_cast<const LogicHashMap&>(*argument_symbol));
    }

    else if( m_parameterSymbol->IsA(SymbolType::UserFunction) )
    {
        CheckUserFunctionArgument(assert_cast<UserFunction&>(*argument_symbol));
    }
}



// --------------------------------------------------------------------------
// Array (additional checks...the data type has already been checked)
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::CheckLogicArrayArgument(const Symbol& argument_symbol) const
{
    const LogicArray& parameter_array = assert_cast<const LogicArray&>(*m_parameterSymbol);
    size_t argument_dimensions;

    if( argument_symbol.IsA(SymbolType::Array) )
    {
        const LogicArray& argument_array = assert_cast<const LogicArray&>(argument_symbol);
        argument_dimensions = argument_array.GetNumberDimensions();

        // make sure the string/alpha setting is consistent
        if( parameter_array.GetPaddingStringLength() != argument_array.GetPaddingStringLength() )
        {
            if( parameter_array.GetPaddingStringLength() == 0 )
            {
                IssueArgumentError(_T("of type 'string'"));
            }

            else
            {
                IssueArgumentError(FormatText(_T("of type 'alpha (%d)'"), parameter_array.GetPaddingStringLength()));
            }
        }
    }

    else
    {
        // because arrays were previously crosstabs, there is some code that manipulates tables by
        // passing a crosstab to a function; we will allow this for numeric crosstabs
        ASSERT(argument_symbol.IsA(SymbolType::Crosstab));

#ifdef WIN_DESKTOP
        const CTAB& argument_crosstab = assert_cast<const CTAB&>(argument_symbol);
        argument_dimensions = argument_crosstab.GetNumDim();
#else
        argument_dimensions = SIZE_MAX;
#endif
    }

    // make sure the number of dimensions is the same
    if( parameter_array.GetNumberDimensions() != argument_dimensions )
    {
        IssueArgumentError(FormatText(_T("with %d dimensions"),
                                      static_cast<int>(parameter_array.GetNumberDimensions())));
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
        IssueArgumentError(FormatText(_T("based on the dictionary '%s'"), parameter_engine_dictionary.GetDictionary().GetName().GetString()));
    }

    // Case checks
    if( parameter_engine_dictionary.IsCaseObject() )
    {
        // the case must be an external dictionary
        // ENGINECR_TODO should also make sure dictionaries for external forms can't be used
        if( argument_engine_dictionary.GetSubType() != SymbolSubType::External )
            IssueArgumentError(_T("from an external dictionary"));
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
            IssueArgumentError(_T("from an external dictionary"));
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

    // ...and potentially written to
    if( parameter_file.IsWrittenTo() )
        argument_file.SetIsWrittenTo();
}



// --------------------------------------------------------------------------
// HashMap (additional checks...the data type has already been checked)
// --------------------------------------------------------------------------

void UserFunctionArgumentChecker::CheckLogicHashMapArgument(const LogicHashMap& argument_hashmap) const
{
    const LogicHashMap& parameter_hashmap = assert_cast<const LogicHashMap&>(*m_parameterSymbol);        

    // check the dimensions
    if( !parameter_hashmap.IsHashMapAssignable(argument_hashmap, false) )
        IssueArgumentError(_T("with different dimension types"));
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
        IssueArgumentError(FormatText(_T("with %d parameters"),
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

            catch( const UserFunctionArgumentChecker::CheckError& )
            {
                symbols_are_compatible = false;
            }
        }

        if( !symbols_are_compatible )
        {
            IssueArgumentError(FormatText(_T("with a matching parameter #%d (%s)"),
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
