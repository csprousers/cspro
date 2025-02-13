#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "SelectDlgHelper.h"
#include "VariableWorker.h"
#include <zEngineO/Array.h>
#include <zEngineO/ValueSet.h>
#include <zEngineO/Versioning.h>
#include <zUtilO/MemoryHelpers.h>
#include <zDictO/DDClass.h>
#include <zDictO/Definitions.h>
#include <zDictO/ValueProcessor.h>


double CIntDriver::exvaluelimit(int iExpr) // minvalue and maxvalue
{
    const auto& function_node = GetNode<FNC_NODE>(iExpr);
    const Symbol* symbol = NPT(function_node.isymb);
    const ValueProcessor* value_processor;

    if( symbol->IsA(SymbolType::Variable) )
    {
        const VART* pVarT = assert_cast<const VART*>(symbol);
        value_processor = &pVarT->GetCurrentValueProcessor();
    }

    else // value set
    {
        const ValueSet* value_set = assert_cast<const ValueSet*>(symbol);
        value_processor = &value_set->GetValueProcessor();
    }

    const NumericValueProcessor* numeric_value_processor = assert_cast<const NumericValueProcessor*>(value_processor);

    return ( function_node.fn_code == FNMINVALUE_CODE ) ? numeric_value_processor->GetMinValue() :
                                                          numeric_value_processor->GetMaxValue();
}


double CIntDriver::exinvalueset(int iExpr)
{
    const auto& function_node = GetNode<FNINVALUSET_NODE>(iExpr);
    const ValueProcessor* value_processor;
    bool numeric;
    bool in_value_set;

    // searching based on the item
    if( function_node.m_iSymVar >= 0 && function_node.m_iSymVSet == 0 )
    {
        const VART* pVarT = VPT(function_node.m_iSymVar);
        value_processor = &pVarT->GetCurrentValueProcessor();
        numeric = pVarT->IsNumeric();
    }

    // searching based on the value set
    else
    {
        const ValueSet& value_set = GetSymbolValueSet(function_node.m_iSymVSet);
        value_processor = &value_set.GetValueProcessor();
        numeric = value_set.IsNumeric();
    }

    if( numeric )
    {
        double value = evalexpr(function_node.m_iExpr);
        in_value_set = value_processor->IsValid(value);
    }

    else
    {
        CString value = EvalAlphaExpr<CString>(function_node.m_iExpr);
        in_value_set = value_processor->IsValid(value);
    }

    return in_value_set ? 1 : 0;
}


double CIntDriver::exgetimage(int iExpr)
{
    const auto& function_node = GetNode<FNG_NODE>(iExpr);
    const Symbol* symbol = NPT(function_node.symbol_index);
    const ValueProcessor* value_processor;

    if( symbol->IsA(SymbolType::Variable) )
    {
        const VART* pVarT = assert_cast<const VART*>(symbol);
        value_processor = &pVarT->GetCurrentValueProcessor();
    }

    else
    {
        const ValueSet* value_set = assert_cast<const ValueSet*>(symbol);
        value_processor = &value_set->GetValueProcessor();
    }

    const DictValue* dict_value;

    if( IsNumeric(*symbol) )
    {
        double value = evalexpr(function_node.m_iExpr);
        dict_value = value_processor->GetDictValue(value);
    }

    else
    {
        CString value = EvalAlphaExpr<CString>(function_node.m_iExpr);
        dict_value = value_processor->GetDictValue(value);
    }

    return ( dict_value != nullptr ) ? AssignAlphaValue(dict_value->GetImageFilename()) :
                                       AssignBlankAlphaValue();
}


double CIntDriver::exsetvalueset(int iExpr)
{
    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        return exsetvalueset_pre80(iExpr);

    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    auto validate_symbol = [&](int symbol_index, SymbolType symbol_type) -> Symbol*
    {
        // lookup the symbol by name
        if( symbol_index < 0 )
        {
            std::wstring symbol_name = EvalAlphaExpr(-1 * symbol_index);
            symbol_index = m_pEngineArea->SymbolTableSearchWithPreference(SO::Trim(symbol_name), symbol_type);

            if( symbol_index <= 0 )
            {
                issaerror(MessageType::Error, 47165, symbol_name.c_str());
                return nullptr;
            }
        }

        Symbol& symbol = NPT_Ref(symbol_index);

        if( !symbol.IsA(symbol_type) )
        {
            issaerror(MessageType::Error, 47164, symbol.GetName().c_str(), ToString(symbol_type));
            return nullptr;
        }

        return &symbol;
    };

    VART* pVarT = assert_nullable_cast<VART*>(validate_symbol(va_node.arguments[0], SymbolType::Variable));

    if( pVarT == nullptr )
        return 0;

    const ValueSet* value_set = assert_nullable_cast<const ValueSet*>(validate_symbol(va_node.arguments[1], SymbolType::ValueSet));

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
        issaerror(MessageType::Error, 941, ToString(pVarT->GetDataType()));
        return 0;
    }

    if( value_does_not_fit_in_value_set_warning )
        issaerror(MessageType::Warning, 47161, pVarT->GetName().c_str());

    pVarT->SetCurrentValueSet(std::move(new_value_set));

    return 1;
}


double CIntDriver::exsetvalueset_pre80(int iExpr)
{
    ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1));

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

    const FNSETVALUESET_NODE* pFunc = &GetNode<FNSETVALUESET_NODE>(iExpr);
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
            CString csVarName = EvalAlphaExpr<CString>(-iSymbol + 1);
            csVarName.TrimRight();

            if( csVarName.GetLength() > 0  )
                iSymbol = m_pEngineArea->SymbolTableSearch(csVarName, { SymbolType::Variable });
        }

        // the variable's symbol number is supplied
        else
        {
            iSymbol = evalexpr<int>(-iSymbol);
        }

        if( iSymbol <= 0 || iSymbol >= static_cast<int>(m_symbolTable.GetTableSize()) )
        {
            issaerror(MessageType::Error, 47110, iSymbol, static_cast<int>(m_symbolTable.GetTableSize()) - 1);
            return dRet;
        }
    }

    Symbol* pVariableSymbol = NPT(iSymbol);

    if( !pVariableSymbol->IsA(SymbolType::Variable) )
    {
        issaerror(MessageType::Error, 47164, pVariableSymbol->GetName().c_str(), ToString(SymbolType::Variable));
        return dRet;
    }

    VART* pVarT = (VART*)pVariableSymbol;
    std::shared_ptr<const ValueSet> new_value_set;

    // value set is an alpha expression
    if( pFunc->m_iSymbolValues[1] == -1 )
    {
        CString csValueSetName = EvalAlphaExpr<CString>(pFunc->m_iSymbolValues[0]);
        csValueSetName.MakeUpper();

        int value_set_symbol = m_pEngineArea->SymbolTableSearch(csValueSetName, { SymbolType::ValueSet });

        if( value_set_symbol == 0 )
        {
            issaerror(MessageType::Error, 47164, csValueSetName.GetString(), ToString(SymbolType::ValueSet));
            return dRet;
        }

        new_value_set = std::dynamic_pointer_cast<const ValueSet, const Symbol>(GetSharedSymbol(value_set_symbol));
    }

    else
    {
        Symbol* pSymbol = NPT(pFunc->m_iSymbolValues[0]);

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
        else if( NPT(pFunc->m_iSymbolValues[0])->GetType() == SymbolType::Array )
        {
            const LogicArray& codes_array = assert_cast<const LogicArray&>(*pSymbol);
            const LogicArray& labels_array = GetSymbolLogicArray(pFunc->m_iSymbolValues[1]);
            const LogicArray* images_array = ( pFunc->m_iImagesCtab > 0 ) ? &GetSymbolLogicArray(pFunc->m_iImagesCtab) :
                                                                            nullptr;

            // check that the codes array matches the type of the variable
            if( pVarT->IsAlpha() && !codes_array.IsString() )
            {
                issaerror(MessageType::Error, 47156);
                return dRet;
            }

            if( pVarT->IsNumeric() && !codes_array.IsNumeric() )
            {
                issaerror(MessageType::Error, 47158);
                return dRet;
            }

            // create a dynamic value set
            std::vector<double> numeric_codes;
            std::vector<std::wstring> string_codes;
            size_t number_codes = 0;
            size_t start_processing_row = LogicArray::CalculateProcessingStartingRow(std::vector<const LogicArray*> { &codes_array, &labels_array });

            // process the codes
            if( codes_array.IsNumeric() )
            {
                numeric_codes = codes_array.GetNumericFilledCells(start_processing_row);
                number_codes = numeric_codes.size();
            }

            else
            {
                string_codes = codes_array.GetStringFilledCells(start_processing_row);
                number_codes = string_codes.size();
            }

            size_t end_processing_row = start_processing_row + number_codes;

            // process the labels
            std::vector<std::wstring> labels = labels_array.GetStringFilledCells(start_processing_row, end_processing_row);

            // process the image filenames
            std::vector<std::wstring> image_filenames = ( images_array != nullptr ) ? images_array->GetStringFilledCells(start_processing_row, end_processing_row) :
                                                                                      std::vector<std::wstring>();

            // create a temporary dynamic value set object and add all the values from the arrays
            DynamicValueSet dynamic_value_set(std::wstring(), *m_engineData);
            dynamic_value_set.SetNumeric(codes_array.IsNumeric());

            for( size_t i = 0; i < number_codes; ++i )
            {
                std::wstring label = ( i < labels.size() ) ? labels[i] :
                                                             std::wstring();
                std::wstring image_filename;

                if( i < image_filenames.size() )
                {
                    image_filename = image_filenames[i];

                    // convert the image paths to absolute paths
                    if( !image_filename.empty() )
                        MakeFullPathFileName(image_filename);
                }

                if( codes_array.IsNumeric() )
                {
                    dynamic_value_set.AddValue(std::move(label), std::move(image_filename), DictionaryDefaults::ValueLabelTextColor, numeric_codes[i], std::nullopt);
                }

                else
                {
                    dynamic_value_set.AddValue(std::move(label), std::move(image_filename), DictionaryDefaults::ValueLabelTextColor, std::move(string_codes[i]));
                }
            }

            new_value_set = dynamic_value_set.CreateValueSet(pVarT, value_does_not_fit_in_value_set_warning);

            // the return value for a dynamic value set is the number of values in the new value set
            dRet = new_value_set->GetDictValueSet().GetNumValues();
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
            issaerror(MessageType::Error, 941, ToString(pVarT->GetDataType()));
            return dRet;
        }

        dRet = 1;
    }

    if( value_does_not_fit_in_value_set_warning )
        issaerror(MessageType::Warning, 47161, pVarT->GetName().c_str());

    pVarT->SetCurrentValueSet(new_value_set);

    return dRet;
}


double CIntDriver::exsetvaluesets(int iExpr)
{
    // for changing the value sets of all items to those matching the string passed
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring value_set_pattern = EvalAlphaExpr(fnn_node.fn_expr[0]);
    size_t num_value_sets_changed = 0;

    // process each of the value sets
    for( const ValueSet* value_set : m_engineData->value_sets_not_dynamic )
    {
        // change the value set if the search pattern was found
        if( value_set->GetName().find(value_set_pattern) != std::wstring::npos )
        {
            VART* pVarT = value_set->GetVarT();
            pVarT->SetCurrentValueSet(std::dynamic_pointer_cast<const ValueSet, const Symbol>(GetSharedSymbol(value_set->GetSymbolIndex())));
            ++num_value_sets_changed;
        }
    }

    return num_value_sets_changed;
}


double CIntDriver::exrandomizevs(int iExpr)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(iExpr);
    Symbol& symbol = NPT_Ref(va_with_size_node.arguments[0]);
    bool numeric = true;

    if( symbol.IsOneOf(SymbolType::Variable, SymbolType::ValueSet) )
        numeric = IsNumeric(symbol);

    std::vector<double> numeric_exclusions;
    std::vector<CString> string_exclusions;

    int exclusion_end_index = Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ?
        va_with_size_node.number_arguments : ( va_with_size_node.number_arguments + 1 );

    for( int i = 1; i < exclusion_end_index; ++i )
    {
        if( numeric )
        {
            double exclusion_value = evalexpr(va_with_size_node.arguments[i]);
            numeric_exclusions.emplace_back(exclusion_value);
        }

        else
        {
            CString exclusion_value = EvalAlphaExpr<CString>(va_with_size_node.arguments[i]);
            exclusion_value.TrimRight();
            string_exclusions.emplace_back(exclusion_value);
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
        VariableWorker(GetSymbolTable(), &symbol,
            [&](VART* pVarT) -> bool
            {
                const ValueSet* value_set = pVarT->GetCurrentValueSet();

                if( value_set != nullptr )
                    valuesets_to_randomize.emplace_back(const_cast<ValueSet*>(value_set));

                return 1;
            });
    }

    // do the randomizations
    for( ValueSet* value_set : valuesets_to_randomize )
        value_set->Randomize(numeric_exclusions, string_exclusions);

    return valuesets_to_randomize.size();
}


double CIntDriver::exvaluesetcompute(int iExpr)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(iExpr);
    ValueSet& lhs_value_set = GetSymbolValueSet(symbol_compute_node.lhs_symbol_index);
    const ValueSet& rhs_value_set = GetSymbolValueSet(symbol_compute_node.rhs_symbol_index);

    if( !lhs_value_set.IsDynamic() )
    {
        issaerror(MessageType::Error, 47170, _T("="), lhs_value_set.GetName().c_str());
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


double CIntDriver::exvaluesetadd(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);
    size_t number_values_added = 0;

    if( !value_set.IsDynamic() )
    {
        issaerror(MessageType::Error, 47170, _T("add"), value_set.GetName().c_str());
        return DEFAULT;
    }

    DynamicValueSet& dynamic_value_set = assert_cast<DynamicValueSet&>(value_set);

    const int& valueset_symbol_index = symbol_va_node.arguments[0];
    const int& label_expression = symbol_va_node.arguments[1];
    const int& image_filename_expression = symbol_va_node.arguments[2];
    const int& from_code_expression = symbol_va_node.arguments[3];
    const int& to_code_expression = symbol_va_node.arguments[4];
    int text_color_expression = Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_7_7_000_1) ? symbol_va_node.arguments[5] : -1;

    if( valueset_symbol_index != -1 )
    {
        const ValueSet& rhs_value_set = GetSymbolValueSet(valueset_symbol_index);

        if( &rhs_value_set == &dynamic_value_set )
        {
            issaerror(MessageType::Error, 47172, value_set.GetName().c_str());
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
            double from_value = evalexpr(from_code_expression);
            double to_value = ( to_code_expression == -1 ) ? from_value : evalexpr(to_code_expression);

            rhs_value_set.ForeachValue(
                [&](const ValueSet::ForeachValueInfo& info, double low_value, const std::optional<double>& high_value)
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
                            dynamic_value_set.AddValue(CS2WS(info.label), CS2WS(info.image_filename), info.text_color, low_value_to_add, std::move(high_value_to_add));
                            ++number_values_added;
                        }
                    }
                });
        }

        // or add a single string value
        else
        {
            CString value = EvalAlphaExpr<CString>(from_code_expression);
            const DictValue* dict_value = rhs_value_set.GetValueProcessor().GetDictValue(value);

            if( dict_value != nullptr )
            {
                dynamic_value_set.AddValue(CS2WS(dict_value->GetLabel()), CS2WS(dict_value->GetImageFilename()), dict_value->GetTextColor(),
                                           CS2WS(dict_value->GetValuePair(0).GetFrom()));
                ++number_values_added;
            }
        }
    }

    else
    {
        std::wstring label = EvalAlphaExpr(label_expression);
        std::wstring image_filename = ( image_filename_expression != -1 ) ? EvalFullPathFileName(image_filename_expression) :
                                                                            std::wstring();
        std::optional<PortableColor> text_color;

        if( text_color_expression != -1 )
        {
            std::wstring text_color_text = EvalAlphaExpr(text_color_expression);
            text_color = PortableColor::FromString(text_color_text);

            if( !text_color.has_value() )
                issaerror(MessageType::Error, 2036, text_color_text.c_str());
        }

        if( !text_color.has_value() )
            text_color = DictionaryDefaults::ValueLabelTextColor;

        if( dynamic_value_set.IsString() )
        {
            std::wstring value = EvalAlphaExpr(from_code_expression);
            dynamic_value_set.AddValue(std::move(label), std::move(image_filename), std::move(*text_color), std::move(value));
        }

        else
        {
            double from_value = evalexpr(from_code_expression);
            std::optional<double> to_value = EvaluateOptionalNumericExpression(to_code_expression);

            try
            {
                dynamic_value_set.ValidateNumericFromTo(from_value, to_value);
            }

            catch( const CSProException& exception )
            {
                issaerror(MessageType::Error, exception);
                return 0;
            }

            dynamic_value_set.AddValue(std::move(label), std::move(image_filename), std::move(*text_color), from_value, std::move(to_value));
        }

        number_values_added = 1;
    }

    return number_values_added;
}


double CIntDriver::exvaluesetclear(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    if( !value_set.IsDynamic() )
    {
        issaerror(MessageType::Error, 47170, _T("clear"), value_set.GetName().c_str());
        return DEFAULT;
    }

    value_set.Reset();

    return 1;
}


double CIntDriver::exvaluesetlength(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    return value_set.GetLength();
}


double CIntDriver::exvaluesetremove(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    if( !value_set.IsDynamic() )
    {
        issaerror(MessageType::Error, 47170, _T("remove"), value_set.GetName().c_str());
        return DEFAULT;
    }

    DynamicValueSet& dynamic_value_set = assert_cast<DynamicValueSet&>(value_set);

    return dynamic_value_set.IsNumeric() ? dynamic_value_set.RemoveValue(evalexpr(symbol_va_node.arguments[0])) :
                                           dynamic_value_set.RemoveValue(EvalAlphaExpr<CString>(symbol_va_node.arguments[0]));
}


double CIntDriver::exvaluesetshow(int iExpr)
{
    if( !UseHtmlDialogs() )
        return exvaluesetshow_pre77(iExpr);

    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    SelectDlg select_dlg(true, 1);

    if( symbol_va_node.arguments[0] != -1 )
        select_dlg.SetTitle(EvalAlphaExpr(symbol_va_node.arguments[0]));

    // numeric value sets will return the code;
    // string value sets will return the label index (not code)
    std::unique_ptr<std::vector<double>> codes;

    if( value_set.IsNumeric() )
    {
        codes = std::make_unique<std::vector<double>>();

        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, double low_value, const std::optional<double>& high_value)
            {
                // use the from code, unless the to code is special, in which case that will be used
                codes->emplace_back(( high_value.has_value() && IsSpecial(*high_value) ) ? *high_value : low_value);
                select_dlg.AddRow(CS2WS(info.label), info.text_color);
            });
    }

    else
    {
        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, const CString& /*value*/)
            {
                select_dlg.AddRow(CS2WS(info.label), info.text_color);
            });
    }

    int selected_row_base_one = SelectDlgHelper(*this, select_dlg, Paradata::OperatorSelectionEvent::Source::ValueSetShow).GetSingleSelection();

    if( value_set.IsNumeric() && selected_row_base_one > 0 )
    {
        return codes->at(selected_row_base_one - 1);
    }

    else
    {
        return selected_row_base_one;
    }
}


double CIntDriver::exvaluesetshow_pre77(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);

    CString heading = ( symbol_va_node.arguments[0] != -1 ) ? EvalAlphaExpr<CString>(symbol_va_node.arguments[0]) :
                                                              CString();

    std::vector<double> codes;
    std::vector<std::vector<CString>*> labels;
    std::vector<PortableColor> row_text_colors;

    if( value_set.IsNumeric() )
    {
        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, double low_value, const std::optional<double>& high_value)
            {
                // use the from code, unless the to code is special, in which case that will be used
                codes.emplace_back(( high_value.has_value() && IsSpecial(*high_value) ) ? *high_value : low_value);
                labels.emplace_back(new std::vector<CString> { info.label });
                row_text_colors.emplace_back(info.text_color);
            });
    }

    else
    {
        // string value sets will return the label index (not code)
        size_t index = 0;

        value_set.ForeachValue(
            [&](const ValueSet::ForeachValueInfo& info, const CString& /*value*/)
            {
                codes.emplace_back(++index);
                labels.emplace_back(new std::vector<CString> { info.label });
                row_text_colors.emplace_back(info.text_color);
            });
    }

    int selection = SelectDlgHelper_pre77(symbol_va_node.function_code, &heading, &labels, nullptr, nullptr, &row_text_colors);

    safe_delete_vector_contents(labels);

    return ( selection == 0 ) ? 0 : codes[selection - 1];
}


double CIntDriver::exvaluesetsort(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    ValueSet& value_set = GetSymbolValueSet(symbol_va_node.symbol_index);
    bool ascending = ( symbol_va_node.arguments[0] == 0 );
    bool sort_by_label = ( symbol_va_node.arguments[1] == 0 );

    value_set.Sort(ascending, sort_by_label);

    return 1;
}
