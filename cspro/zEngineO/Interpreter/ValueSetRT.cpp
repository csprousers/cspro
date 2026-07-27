#include "stdafx.h"
#include "IncludesRT.h"
#include "Array.h"
#include "SelectDlgHelper.h"
#include "ValueSet.h"
#include "Nodes/ValueSet.h"
#include <engine/Nodes.h>
#include <engine/VariableIterator.h>
#include <zDictO/Definitions.h>
#include <zDictO/NumericValueProcessor.h>


// --------------------------------------------------------------------------
// value set-related functions
// --------------------------------------------------------------------------

double LogicInterpreter::ex_minvalue_maxvalue(const int program_index)
{
    const auto& element_reference_single_node = GetNode<Nodes::ElementReferenceSingle>(program_index);
    const Symbol& symbol = NPT_Ref(element_reference_single_node.symbol_index);
    const ValueProcessor* value_processor;

    // variable
    if( symbol.IsA(SymbolType::Variable) )
    {
        const VART& vart = assert_cast<const VART&>(symbol);
        value_processor = &vart.GetCurrentValueProcessor();
    }

    // value set
    else
    {
        ASSERT(symbol.IsA(SymbolType::ValueSet));
        const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);
        value_processor = &value_set.GetValueProcessor();
    }

    const NumericValueProcessor& numeric_value_processor = assert_cast<const NumericValueProcessor&>(*value_processor);

    return ( element_reference_single_node.function_code == FunctionCode::FNMINVALUE_CODE )
        ? numeric_value_processor.GetMinValue()
        : numeric_value_processor.GetMaxValue();
}


double LogicInterpreter::ex_invalueset(const int program_index)
{
    const auto& invalueset_node = GetNode<Nodes::InValueSet>(program_index);
    const ValueProcessor* value_processor;
    bool numeric;

    // searching based on the item
    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_2_000_1)
        ? ( invalueset_node.value_set_symbol_index == -1 )
        : ( invalueset_node.value_set_symbol_index >= 0 && invalueset_node.value_set_symbol_index == 0 ) )
    {
        ASSERT(invalueset_node.item_symbol_index != -1);
        const VART& vart = *VPT(invalueset_node.item_symbol_index);
        value_processor = &vart.GetCurrentValueProcessor();
        numeric = vart.IsNumeric();
    }

    // searching based on the value set
    else
    {
        const ValueSet& value_set = GetSymbolValueSet(invalueset_node.value_set_symbol_index);
        value_processor = &value_set.GetValueProcessor();
        numeric = value_set.IsNumeric();
    }

    if( numeric )
    {
        const double value = Evaluate(invalueset_node.value_expression);
        return value_processor->IsValid(value);
    }

    else
    {
        const SharableString value = EvaluateSharableString(invalueset_node.value_expression);
        return value_processor->IsValid(*value);
    }
}


double LogicInterpreter::ex_getimage(const int program_index)
{
    const auto& function_node = GetNode<FNG_NODE>(program_index);
    const Symbol& symbol = NPT_Ref(function_node.symbol_index);
    const ValueProcessor* value_processor;

    // variable
    if( symbol.IsA(SymbolType::Variable) )
    {
        const VART& vart = assert_cast<const VART&>(symbol);
        value_processor = &vart.GetCurrentValueProcessor();
    }

    // value set
    else
    {
        ASSERT(symbol.IsA(SymbolType::ValueSet));
        const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);
        value_processor = &value_set.GetValueProcessor();
    }

    const DictValue* dict_value;

    if( IsNumeric(symbol) )
    {
        const double value = Evaluate(function_node.m_iExpr);
        dict_value = value_processor->GetDictValue(value);
    }

    else
    {
        ASSERT(IsString(symbol));
        const SharableString value = EvaluateSharableString(function_node.m_iExpr);
        dict_value = value_processor->GetDictValue(*value);
    }

    if( dict_value != nullptr )
        return AssignString(dict_value->GetImageFilePath());

    return AssignStringNull();
}


double LogicInterpreter::ex_setvalueset(const int program_index)
{
    if( m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        return ex_setvalueset_pre80(program_index);

    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    auto validate_symbol = [&](int symbol_index, const SymbolType symbol_type) -> Symbol*
    {
        // lookup the symbol by name
        if( symbol_index < 0 )
        {
            const SharableString symbol_name = EvaluateSharableString(-1 * symbol_index);
            symbol_index = SymbolTableSearchWithPreference_INTERPRETER_DLL_TODO(SO::Trim(*symbol_name), symbol_type);

            if( symbol_index <= 0 )
            {
                IssueMessage(MessageType::Error, MGF::ValueSet_symbol_does_not_exist_47165, symbol_name->c_str());
                return nullptr;
            }
        }

        Symbol& symbol = NPT_Ref(symbol_index);

        if( !symbol.IsA(symbol_type) )
        {
            IssueMessage(MessageType::Error, MGF::ValueSet_symbol_is_not_of_type_47164, symbol.GetName().c_str(), ToString(symbol_type));
            return nullptr;
        }

        return &symbol;
    };

    VART* const pVarT= assert_nullable_cast<VART*>(
        validate_symbol(va_node.arguments[0], SymbolType::Variable)
    );

    if( pVarT == nullptr )
        return 0;

    const ValueSet* const value_set = assert_nullable_cast<const ValueSet*>(
        validate_symbol(va_node.arguments[1], SymbolType::ValueSet)
    );

    if( value_set == nullptr )
        return 0;

    std::shared_ptr<const ValueSet> new_value_set;
    bool value_does_not_fit_in_value_set_warning = false;

    if( value_set->IsDynamic() )
    {
        new_value_set = assert_cast<const DynamicValueSet&>(*value_set).CreateValueSet(pVarT, value_does_not_fit_in_value_set_warning);
    }

    else
    {
        new_value_set = std::dynamic_pointer_cast<const ValueSet, const Symbol>(GetSharedSymbol(value_set->GetSymbolIndex()));
    }

    // check that the value set can apply to the variable
    if( pVarT->GetDataType() != new_value_set->GetDataType() )
    {
        IssueMessage(MessageType::Error, MGF::ValueSet_not_correct_data_type_941, ToString(pVarT->GetDataType()));
        return 0;
    }

    if( value_does_not_fit_in_value_set_warning )
        IssueMessage(MessageType::Warning, MGF::ValueSet_contains_values_not_valid_for_field_47161, pVarT->GetName().c_str());

    pVarT->SetCurrentValueSet(std::move(new_value_set));

    return 1;
}


double LogicInterpreter::ex_setvalueset_pre80(const int program_index)
{
    ASSERT(m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1));

    struct FNSETVALUESET_NODE
    {
        int fn_code;
        int m_iSymbol;          // Symbol
        int m_iIsAtAlpha;       // If m_iSymbol < 0 then use m_isAtAlpha. Default is 0.
        int m_iSymbolValues[2]; // array_num & array_alpha or value_set & not_used
        int _unused1;           // Only for Back Compatibility in Binaries. -MAXLONG
        int _unused2;
        int _unused3;
        int _unused4;
        int m_iImagesCtab;
    };

    const FNSETVALUESET_NODE* pFunc = &GetNode<FNSETVALUESET_NODE>(program_index);
    int iSymbol = pFunc->m_iSymbol;
    double dRet = 0;
    bool value_does_not_fit_in_value_set_warning = false;

    ASSERT(iSymbol != 0);

     // @ used
    if( iSymbol < 0 )
    {
        // the variable name is supplied as an alpha expression
        if( pFunc->m_iIsAtAlpha == 1 )
        {
            SharableString var_name = EvaluateSharableString(-iSymbol + 1);
            var_name.MakeTrimRight();

            if( !var_name->empty()  )
                iSymbol = SymbolTableSearch_INTERPRETER_DLL_TODO(*var_name, { SymbolType::Variable });
        }

        // the variable's symbol number is supplied
        else
        {
            iSymbol = Evaluate<int>(-iSymbol);
        }

        if( iSymbol <= 0 || iSymbol >= static_cast<int>(m_symbolTable.GetTableSize()) )
        {
            IssueMessage(MessageType::Error, 47110, iSymbol, static_cast<int>(m_symbolTable.GetTableSize()) - 1);
            return dRet;
        }
    }

    Symbol* pVariableSymbol = &NPT_Ref(iSymbol);

    if( !pVariableSymbol->IsA(SymbolType::Variable) )
    {
        IssueMessage(MessageType::Error, 47164, pVariableSymbol->GetName().c_str(), ToString(SymbolType::Variable));
        return dRet;
    }

    VART* pVarT = (VART*)pVariableSymbol;
    std::shared_ptr<const ValueSet> new_value_set;

    // value set is an alpha expression
    if( pFunc->m_iSymbolValues[1] == -1 )
    {
        SharableString value_set_name = EvaluateSharableString(pFunc->m_iSymbolValues[0]);
        value_set_name.MakeUpper();

        int value_set_symbol = SymbolTableSearch_INTERPRETER_DLL_TODO(*value_set_name, { SymbolType::ValueSet });

        if( value_set_symbol == 0 )
        {
            IssueMessage(MessageType::Error, 47164, value_set_name->c_str(), ToString(SymbolType::ValueSet));
            return dRet;
        }

        new_value_set = std::dynamic_pointer_cast<const ValueSet, const Symbol>(GetSharedSymbol(value_set_symbol));
    }

    else
    {
        Symbol* pSymbol = &NPT_Ref(pFunc->m_iSymbolValues[0]);

        // a value set name was specified
        if( pSymbol->IsA(SymbolType::ValueSet) )
        {
            const ValueSet& value_set = assert_cast<const ValueSet&>(*pSymbol);

            if( value_set.IsDynamic() )
            {
                new_value_set = assert_cast<const DynamicValueSet&>(value_set).CreateValueSet(pVarT, value_does_not_fit_in_value_set_warning);
            }

            else
            {
                new_value_set = std::dynamic_pointer_cast<const ValueSet, const Symbol>(GetSharedSymbol(value_set.GetSymbolIndex()));
            }
        }

        // an array was specified so we will create a dynamic value set
        else if( pSymbol->IsA(SymbolType::Array) )
        {
            const LogicArray& codes_array = assert_cast<const LogicArray&>(*pSymbol);
            const LogicArray& labels_array = GetSymbolLogicArray(pFunc->m_iSymbolValues[1]);
            const LogicArray* images_array = ( pFunc->m_iImagesCtab > 0 ) ? &GetSymbolLogicArray(pFunc->m_iImagesCtab) :
                                                                            nullptr;

            // check that the codes array matches the type of the variable
            if( pVarT->IsAlpha() && !codes_array.IsString() )
            {
                IssueMessage(MessageType::Error, 47156);
                return dRet;
            }

            if( pVarT->IsNumeric() && !codes_array.IsNumeric() )
            {
                IssueMessage(MessageType::Error, 47158);
                return dRet;
            }

            // create a dynamic value set
            std::vector<double> numeric_codes;
            std::vector<SharableString> string_codes;
            size_t number_codes = 0;
            size_t start_processing_row = LogicArray::CalculateProcessingStartingRow(std::vector<const LogicArray*> { &codes_array, &labels_array });

            // process the codes
            if( codes_array.IsNumeric() )
            {
                numeric_codes = codes_array.GetFilledCells<double>(start_processing_row);
                number_codes = numeric_codes.size();
            }

            else
            {
                string_codes = codes_array.GetFilledCells<SharableString>(start_processing_row);
                number_codes = string_codes.size();
            }

            size_t end_processing_row = start_processing_row + number_codes;

            // process the labels
            const std::vector<SharableString> labels = labels_array.GetFilledCells<SharableString>(start_processing_row, end_processing_row);

            // process the image file paths
            const std::vector<SharableString> image_file_paths =
                ( images_array != nullptr ) ? images_array->GetFilledCells<SharableString>(start_processing_row, end_processing_row) :
                                              std::vector<SharableString>();

            // create a temporary dynamic value set object and add all the values from the arrays
            DynamicValueSet dynamic_value_set(std::string(), *m_engineData);
            dynamic_value_set.SetNumeric(codes_array.IsNumeric());

            for( size_t i = 0; i < number_codes; ++i )
            {
                SharableString label = ( i < labels.size() ) ? labels[i] :
                                                               SharableString();
                std::string image_file_path;

                if( i < image_file_paths.size() )
                {
                    image_file_path = image_file_paths[i].GetString();

                    // convert the image paths to absolute paths
                    if( !image_file_path.empty() )
                        MakeAbsolutePath(image_file_path);
                }

                if( codes_array.IsNumeric() )
                {
                    dynamic_value_set.AddValue(std::move(label), std::move(image_file_path), DictionaryDefaults::ValueLabelTextColor, numeric_codes[i], std::nullopt);
                }

                else
                {
                    dynamic_value_set.AddValue(std::move(label), std::move(image_file_path), DictionaryDefaults::ValueLabelTextColor, std::move(string_codes[i]));
                }
            }

            new_value_set = dynamic_value_set.CreateValueSet(pVarT, value_does_not_fit_in_value_set_warning);

            // the return value for a dynamic value set is the number of values in the new value set
            dRet = static_cast<double>(new_value_set->GetDictValueSet().GetNumValues());
        }

        // the symbol wasn't a value set or an array
        else
        {
            ASSERT(false);
        }
    }

    ASSERT(new_value_set != nullptr);

    // check that the value set can apply to the variable
    if( !new_value_set->IsDynamic() )
    {
        if( pVarT->IsNumeric() != new_value_set->IsNumeric() )
        {
            IssueMessage(MessageType::Error, 941, ToString(pVarT->GetDataType()));
            return dRet;
        }

        dRet = 1;
    }

    if( value_does_not_fit_in_value_set_warning )
        IssueMessage(MessageType::Warning, 47161, pVarT->GetName().c_str());

    pVarT->SetCurrentValueSet(new_value_set);

    return dRet;
}


double LogicInterpreter::ex_setvaluesets(const int program_index)
{
    // for changing the value sets of all items to those matching the string passed
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString value_set_pattern = EvaluateSharableString(fnn_node.fn_expr[0]);
    size_t num_value_sets_changed = 0;

    // process each of the value sets
    for( const ValueSet* const value_set : m_engineData->value_sets_not_dynamic )
    {
        // change the value set only if the search pattern was found
        if( value_set->GetName().find(*value_set_pattern) == std::string::npos )
            continue;

        VART* const vart = value_set->GetVarT();
        ASSERT(vart != nullptr);

        vart->SetCurrentValueSet(std::dynamic_pointer_cast<const ValueSet, const Symbol>(GetSharedSymbol(value_set->GetSymbolIndex())));

        ++num_value_sets_changed;
    }

    return static_cast<double>(num_value_sets_changed);
}


double LogicInterpreter::ex_randomizevs(const int program_index)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);
    Symbol& symbol = NPT_Ref(va_with_size_node.arguments[0]);

    const bool numeric = symbol.IsOneOf(SymbolType::Variable, SymbolType::ValueSet)
        ? IsNumeric(symbol)
        : true;

    std::variant<std::vector<double>, std::vector<SharableString>> exclusions = numeric
        ? std::variant<std::vector<double>, std::vector<SharableString>>(std::vector<double>())
        : std::variant<std::vector<double>, std::vector<SharableString>>(std::vector<SharableString>());

    const int exclusion_end_index = m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1)
        ? va_with_size_node.number_arguments
        : ( va_with_size_node.number_arguments + 1 );

    for( int i = 1; i < exclusion_end_index; ++i )
    {
        if( numeric )
        {
            std::get<0>(exclusions).emplace_back(Evaluate(va_with_size_node.arguments[i]));
        }

        else
        {
            SharableString& exclusion = std::get<1>(exclusions).emplace_back(EvaluateSharableString(va_with_size_node.arguments[i]));
            exclusion.MakeTrimRight();
        }
    }

    std::vector<ValueSet*> valuesets_to_randomize;

    // randomize a value set
    if( symbol.IsA(SymbolType::ValueSet) )
    {
        valuesets_to_randomize.emplace_back(assert_cast<ValueSet*>(&symbol));
    }

    // randomize an encompassing symbol (like a dictionary)
    else
    {
        ForeachVariable(GetSymbolTable(), symbol,
            [&](VART& vart)
            {
                const ValueSet* const value_set = vart.GetCurrentValueSet();

                if( value_set != nullptr )
                {
                    valuesets_to_randomize.emplace_back(const_cast<ValueSet*>(value_set));
                    return 1;
                }

                return 0;
            });
    }

    // do the randomizations
    for( ValueSet* const value_set : valuesets_to_randomize )
        value_set->Randomize(exclusions);

    return static_cast<double>(valuesets_to_randomize.size());
}



// --------------------------------------------------------------------------
// ValueSet object functions
// --------------------------------------------------------------------------

double LogicInterpreter::ex_ValueSet_compute(const int program_index)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(program_index);
    ValueSet& lhs_value_set = GetSymbolValueSet(symbol_compute_node.lhs_symbol_index);
    const ValueSet& rhs_value_set = GetSymbolValueSet(symbol_compute_node.rhs_symbol_index);

    if( !lhs_value_set.IsDynamic() )
    {
        IssueMessage(MessageType::Error, MGF::ValueSet_invalid_operation_for_dict_value_set_47170,
                     "=", lhs_value_set.GetName().c_str());
    }

    // only do the assignment if they're not assigning a value set to itself
    else if( &lhs_value_set != &rhs_value_set )
    {
        DynamicValueSet& lhs_dynamic_value_set = assert_cast<DynamicValueSet&>(lhs_value_set);
        lhs_dynamic_value_set.Reset();
        lhs_dynamic_value_set.AddValues(rhs_value_set);
    }

    return 0;
}


double LogicInterpreter::ex_ValueSet_add(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);
    size_t number_values_added = 0;

    if( !value_set.IsDynamic() )
    {
        IssueMessage(MessageType::Error, MGF::ValueSet_invalid_operation_for_dict_value_set_47170,
                     "add", value_set.GetName().c_str());
        return DEFAULT;
    }

    DynamicValueSet& dynamic_value_set = assert_cast<DynamicValueSet&>(value_set);

    const int& valueset_symbol_index = symbol_va_node.arguments[0];
    const int& label_expression = symbol_va_node.arguments[1];
    const int& image_file_path_expression = symbol_va_node.arguments[2];
    const int& from_code_expression = symbol_va_node.arguments[3];
    const int& to_code_expression = symbol_va_node.arguments[4];
    const int& text_color_expression = symbol_va_node.arguments[5];

    if( valueset_symbol_index != -1 )
    {
        const ValueSet& rhs_value_set = GetSymbolValueSet(valueset_symbol_index);

        if( &rhs_value_set == &dynamic_value_set )
        {
            IssueMessage(MessageType::Error, MGF::ValueSet_add_cannot_add_self_47172, value_set.GetName().c_str());
            return 0;
        }

        // add the entire value set
        // (the second condition is here because the initial 7.3 compilation
        // had the default value of from_code_expression as 0, not -1)
        if( from_code_expression == -1 || from_code_expression == 0 )
        {
            number_values_added = dynamic_value_set.AddValues(rhs_value_set);
        }

        // or add a single or range of numeric values
        else if( dynamic_value_set.IsNumeric() )
        {
            const double from_value = Evaluate(from_code_expression);
            const double to_value = ( to_code_expression == -1 ) ? from_value : Evaluate(to_code_expression);

            rhs_value_set.ForeachValue(
                [&](const ValueSet::ForeachValueInfo& info, const double low_value, const std::optional<double>& high_value)
                {
                    if( to_value >= low_value )
                    {
                        bool add_value = false;
                        double low_value_to_add = low_value;
                        std::optional<double> high_value_to_add;

                        // potentially add a single value
                        if( !high_value.has_value() )
                        {
                            add_value = ( from_value <= low_value );
                        }

                        // or add a range
                        else if( from_value <= *high_value )
                        {
                            add_value = true;
                            low_value_to_add = std::max(from_value, low_value);
                            high_value_to_add = std::min(to_value, *high_value);

                            if( low_value_to_add == high_value_to_add )
                                high_value_to_add.reset();
                        }

                        if( add_value )
                        {
                            dynamic_value_set.AddValue(
                                info.label,
                                info.image_file_path,
                                info.text_color,
                                low_value_to_add,
                                std::move(high_value_to_add)
                            );

                            ++number_values_added;
                        }
                    }
                });
        }

        // or add a single string value
        else
        {
            const SharableString value = EvaluateSharableString(from_code_expression);
            const DictValue* const dict_value = rhs_value_set.GetValueProcessor().GetDictValue(*value);

            if( dict_value != nullptr )
            {
                dynamic_value_set.AddValue(
                    UTF8_TODO::GetUtf8(dict_value->GetLabel()),
                    dict_value->GetImageFilePath(),
                    dict_value->GetTextColor(),
                    UTF8_TODO::GetUtf8(dict_value->GetValuePair(0).GetFrom())
                );

                ++number_values_added;
            }
        }
    }

    else
    {
        const SharableString label = EvaluateSharableString(label_expression);

        std::string image_file_path = ( image_file_path_expression != -1 )
            ? EvaluatePath(image_file_path_expression)
            : std::string();

        std::optional<PortableColor> text_color;

        if( text_color_expression != -1 )
        {
            const SharableString text_color_text = EvaluateSharableString(text_color_expression);
            text_color = PortableColor::FromString(*text_color_text);

            if( !text_color.has_value() )
                IssueMessage(MessageType::Error, MGF::color_invalid_2036, text_color_text->c_str());
        }

        if( !text_color.has_value() )
            text_color = DictionaryDefaults::ValueLabelTextColor;

        if( dynamic_value_set.IsString() )
        {
            dynamic_value_set.AddValue(
                std::move(label),
                std::move(image_file_path),
                std::move(*text_color),
                EvaluateSharableString(from_code_expression) // value
            );
        }

        else
        {
            const double from_value = Evaluate(from_code_expression);
            std::optional<double> to_value = EvaluateOptional(to_code_expression);

            try
            {
                dynamic_value_set.ValidateNumericFromTo(from_value, to_value);
            }

            catch( const CSProException& exception )
            {
                IssueMessage(MessageType::Error, MGF::OpenMessage_32001, exception.what());
                return 0;
            }

            dynamic_value_set.AddValue(
                std::move(label),
                std::move(image_file_path),
                std::move(*text_color),
                from_value,
                std::move(to_value)
            );
        }

        number_values_added = 1;
    }

    return static_cast<double>(number_values_added);
}


double LogicInterpreter::ex_ValueSet_clear(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    if( !value_set.IsDynamic() )
    {
        IssueMessage(MessageType::Error, MGF::ValueSet_invalid_operation_for_dict_value_set_47170,
                     "clear", value_set.GetName().c_str());
        return DEFAULT;
    }

    value_set.Reset();

    return 1;
}


double LogicInterpreter::ex_ValueSet_length(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    return static_cast<double>(value_set.GetLength());
}


double LogicInterpreter::ex_ValueSet_remove(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    if( !value_set.IsDynamic() )
    {
        IssueMessage(MessageType::Error, MGF::ValueSet_invalid_operation_for_dict_value_set_47170,
                     "remove", value_set.GetName().c_str());
        return DEFAULT;
    }

    DynamicValueSet& dynamic_value_set = assert_cast<DynamicValueSet&>(value_set);

    return static_cast<double>(
        dynamic_value_set.IsNumeric()
        ? dynamic_value_set.RemoveValue(Evaluate<double>(symbol_va_node.arguments[0]))
        : dynamic_value_set.RemoveValue(Evaluate<SharableString>(symbol_va_node.arguments[0]).GetString())
    );
}


double LogicInterpreter::ex_ValueSet_removeDuplicates(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    if( !value_set.IsDynamic() )
    {
        IssueMessage(MessageType::Error, MGF::ValueSet_invalid_operation_for_dict_value_set_47170,
                     "removeDuplicates", value_set.GetName().c_str());
        return DEFAULT;
    }

    DynamicValueSet& dynamic_value_set = assert_cast<DynamicValueSet&>(value_set);

    return static_cast<double>(dynamic_value_set.RemoveDuplicates(
        static_cast<DynamicValueSet::RemoveDuplicatesType>(symbol_va_node.arguments[0])
    ));
}


double LogicInterpreter::ex_ValueSet_show(const int program_index)
{
    if( !UseHtmlDialogs() )
        return ex_ValueSet_show_pre77(program_index);

    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    SelectDlg select_dlg(true, 1);

    if( symbol_va_node.arguments[0] != -1 )
        select_dlg.SetTitle(EvaluateSharableString(symbol_va_node.arguments[0]));

    // numeric value sets will return the code;
    // string value sets will return the label index (not code)
    std::unique_ptr<std::vector<double>> codes;

    if( value_set.IsNumeric() )
    {
        codes = std::make_unique<std::vector<double>>();

        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, const double low_value, const std::optional<double>& high_value)
            {
                // use the from code, unless the to code is special, in which case that will be used
                codes->emplace_back(( high_value.has_value() && IsSpecial(*high_value) ) ? *high_value : low_value);
                select_dlg.AddRow(info.label, info.text_color);
            });
    }

    else
    {
        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, const SharableString& /*value*/)
            {
                select_dlg.AddRow(info.label, info.text_color);
            });
    }

    SelectDlgHelper select_dlg_helper(GetEngineParadataDriver_INTERPRETER_DLL_TODO(), select_dlg, Paradata::OperatorSelectionEvent::Source::ValueSetShow);
    const size_t selected_row_base_one = select_dlg_helper.GetSingleSelection();

    if( value_set.IsNumeric() && selected_row_base_one > 0 )
    {
        return codes->at(selected_row_base_one - 1);
    }

    else
    {
        return static_cast<double>(selected_row_base_one);
    }
}


double LogicInterpreter::ex_ValueSet_show_pre77(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    const SharableString heading = EvaluateNullableSharableString(symbol_va_node.arguments[0]);
    std::vector<double> codes;
    std::vector<std::vector<CString>*> labels;
    std::vector<PortableColor> row_text_colors;

    if( value_set.IsNumeric() )
    {
        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, const double low_value, const std::optional<double>& high_value)
            {
                // use the from code, unless the to code is special, in which case that will be used
                codes.emplace_back(( high_value.has_value() && IsSpecial(*high_value) ) ? *high_value : low_value);
                labels.emplace_back(new std::vector<CString> { UTF8_TODO::GetCString(info.label) });
                row_text_colors.emplace_back(info.text_color);
            });
    }

    else
    {
        // string value sets will return the label index (not code)
        size_t index = 0;

        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, const SharableString& /*value*/)
            {
                codes.emplace_back(static_cast<double>(++index));
                labels.emplace_back(new std::vector<CString> { UTF8_TODO::GetCString(info.label) });
                row_text_colors.emplace_back(info.text_color);
            });
    }

    const int selection = SelectDlgHelper_pre77(symbol_va_node.function_code, UTF8_TODO::GetCString(heading), &labels, nullptr, nullptr, &row_text_colors);

    for( const std::vector<CString>* const label : labels )
        delete label;

    return ( selection == 0 ) ? 0 : codes[selection - 1];
}


double LogicInterpreter::ex_ValueSet_sort(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    value_set.Sort(
        ( symbol_va_node.arguments[0] == 0 ), // ascending
        ( symbol_va_node.arguments[1] == 0 )  // sort_by_label
    );

    return 1;
}
