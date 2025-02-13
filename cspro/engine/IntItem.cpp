#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "BinaryStorageFor80.h"
#include <zEngineO/Audio.h>
#include <zEngineO/Block.h>
#include <zEngineO/Document.h>
#include <zEngineO/EngineItem.h>
#include <zEngineO/Geometry.h>
#include <zEngineO/Image.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Messages/EngineMessages.h>


// --------------------------------------------------------------------------
// Item node handling and occurrence calculators
// --------------------------------------------------------------------------

const Nodes::SymbolVariableArgumentsWithSubscript& CIntDriver::GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(int program_index)
{
    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        return GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);

    // check the cache of already converted nodes
    const auto& lookup = m_convertedPre80Nodes.find(program_index);

    if( lookup != m_convertedPre80Nodes.cend() )
        return *reinterpret_cast<const Nodes::SymbolVariableArgumentsWithSubscript*>(lookup->second.get());

    // convert a SymbolVariableArguments node to a SymbolVariableArgumentsWithSubscript node
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);

    // we don't know how many arguments were specified, so assume up to 1000
    const size_t arguments = std::min<size_t>(1000, m_engineData->logic_byte_code.GetSize() - program_index);

    auto byte_code = std::make_unique_for_overwrite<int[]>(( sizeof(Nodes::SymbolVariableArgumentsWithSubscript) / sizeof(int) ) + arguments);
    auto& symbol_va_with_subscript_node = *reinterpret_cast<Nodes::SymbolVariableArgumentsWithSubscript*>(byte_code.get());

    symbol_va_with_subscript_node.function_code = symbol_va_node.function_code;

    symbol_va_with_subscript_node.symbol_index = symbol_va_node.symbol_index;
    ASSERT(NPT_Ref(symbol_va_with_subscript_node.symbol_index).IsOneOf(SymbolType::Audio, SymbolType::Document, SymbolType::Image));

    symbol_va_with_subscript_node.subscript_compilation = -1;

    memcpy(symbol_va_with_subscript_node.arguments, symbol_va_node.arguments, sizeof(int) * arguments);

    m_convertedPre80Nodes.try_emplace(program_index, std::move(byte_code));

    return symbol_va_with_subscript_node;
}


const Nodes::SymbolComputeWithSubscript& CIntDriver::GetOrConvertPre80SymbolComputeWithSubscriptNode(int program_index)
{
    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        return GetNode<Nodes::SymbolComputeWithSubscript>(program_index);

    // check the cache of already converted nodes
    const auto& lookup = m_convertedPre80Nodes.find(program_index);

    if( lookup != m_convertedPre80Nodes.cend() )
        return *reinterpret_cast<const Nodes::SymbolComputeWithSubscript*>(lookup->second.get());

    // convert a SymbolCompute node to a SymbolComputeWithSubscript node
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(program_index);

    auto byte_code = std::make_unique_for_overwrite<int[]>(sizeof(Nodes::SymbolComputeWithSubscript) / sizeof(int));
    auto& symbol_compute_with_subscript_node = *reinterpret_cast<Nodes::SymbolComputeWithSubscript*>(byte_code.get());

    symbol_compute_with_subscript_node.function_code = symbol_compute_node.function_code;
    symbol_compute_with_subscript_node.next_st = symbol_compute_node.next_st;
    symbol_compute_with_subscript_node.lhs_symbol_index = symbol_compute_node.lhs_symbol_index;
    symbol_compute_with_subscript_node.lhs_subscript_compilation = -1;
    symbol_compute_with_subscript_node.rhs_symbol_index = symbol_compute_node.rhs_symbol_index;
    symbol_compute_with_subscript_node.rhs_subscript_compilation = -1;

    m_convertedPre80Nodes.try_emplace(program_index, std::move(byte_code));

    return symbol_compute_with_subscript_node;
}


const Symbol* CIntDriver::GetCurrentProcSymbol() const
{
    return ( m_FieldSymbol != 0 ) ? NPT(m_FieldSymbol) :
           ( m_iExSymbol != 0 )   ? NPT(m_iExSymbol) :
                                    nullptr;
}


int CIntDriver::CalculateEngineItemImplicitOccurrence(const EngineItem& engine_item, bool get_record_occurrence)
{
    // returns a zero-based occurrence (or -1 if an implicit occurrence cannot be calculated)
    constexpr int InvalidOccurrence = -1;

    const Symbol* current_proc_symbol = GetCurrentProcSymbol();

    if( current_proc_symbol == nullptr )
        return ReturnProgrammingError(InvalidOccurrence);

    // BINARY_TYPES_TO_ENGINE_TODO this code is not perfect because it does not take into account circumstances like for loops

    // get the dictionary item for the group or field
    const CDictItem* dict_item =
        ( current_proc_symbol->IsA(SymbolType::Group) )    ? assert_cast<const GROUPT*>(current_proc_symbol)->GetFirstDictItem() :
        ( current_proc_symbol->IsA(SymbolType::Block) )    ? assert_cast<const EngineBlock*>(current_proc_symbol)->GetGroupT()->GetFirstDictItem() :
        ( current_proc_symbol->IsA(SymbolType::Variable) ) ? assert_cast<const VART*>(current_proc_symbol)->GetDictItem() :
                                                             nullptr;

    // the dictionary item can only be used if it shares the same record
    if( dict_item == nullptr || dict_item->GetRecord() != engine_item.GetDictItem().GetRecord() )
        return InvalidOccurrence;

    ASSERT(!IsBinary(*dict_item));

    if( !get_record_occurrence )
    {
        // a repeating binary item will not have another item at the same repeating level
        if( engine_item.GetDictItem().IsSubitem() )
            return InvalidOccurrence;

        ASSERT(engine_item.GetDictItem().GetParentItem() != nullptr && engine_item.GetDictItem().GetParentItem()->GetOccurs() > 1);

        // a repeating binary subitem can only be used if it shares the same non-null parent item
        if( engine_item.GetDictItem().GetParentItem() == nullptr || engine_item.GetDictItem().GetParentItem() != dict_item->GetParentItem() )
            return InvalidOccurrence;
    }

    // the dictionary item is comparable at this point
    const VART* pVarT = VPT(dict_item->GetSymbol());
    const GROUPT* pGroupT = pVarT->GetParentGPT();
    ASSERT(pGroupT != nullptr);

    // code based on CIntDriver::AssignParser
    const bool bUseBatchLogic = ( Issamod != ModuleType::Entry );

    if( pGroupT->GetDimType() == CDimension::Record )
    {
        if( get_record_occurrence )
            return GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1;
    }

    else
    {
        if( pGroupT->GetDimType() == CDimension::Item ||
            pGroupT->GetDimType() == CDimension::SubItem )
        {
            if( !get_record_occurrence )
                return GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1;
        }

        if( get_record_occurrence )
        {
            pGroupT = pGroupT->GetParentGPT();

            if( pGroupT != nullptr && pGroupT->GetDimType() == CDimension::Record )
                return GetCurOccFromGroup(pGroupT, bUseBatchLogic) - 1;
        }
    }

    return InvalidOccurrence;
}


EvaluatedEngineItemSubscript CIntDriver::EvaluateEngineItemSubscript(const EngineItem& engine_item, const Nodes::ItemSubscript& item_subscript_node)
{
    // if singly-occurring, no need to evaluate the subscripts
    if( item_subscript_node.subscript_type == Nodes::ItemSubscript::SubscriptType::ImplicitSinglyOccurring )
        return EvaluatedEngineItemSubscript();

    // otherwise evaluate the subscripts, subtracting 1 to make them zero-based
    const bool subscript_is_fully_implicit = ( item_subscript_node.subscript_type == Nodes::ItemSubscript::SubscriptType::ImplicitMustEvaluate );
    ASSERT(subscript_is_fully_implicit || item_subscript_node.subscript_type == Nodes::ItemSubscript::SubscriptType::Specified);

    EvaluatedEngineItemSubscript evaluated_engine_item_subscript;

    auto evaluate_specified_occurrence = [&](size_t arguments_index)
    {
        if( static_cast<SubscriptValueType>(item_subscript_node.subscript_arguments[arguments_index]) == SubscriptValueType::ConstantInteger )
        {
            return item_subscript_node.subscript_arguments[arguments_index + 1] - 1;
        }

        else
        {
            ASSERT(static_cast<SubscriptValueType>(item_subscript_node.subscript_arguments[arguments_index]) == SubscriptValueType::Expression);
            return evalexpr<int>(item_subscript_node.subscript_arguments[arguments_index + 1]) - 1;
        }
    };

    // record occurrence
    if( subscript_is_fully_implicit || static_cast<SubscriptValueType>(item_subscript_node.subscript_arguments[0]) == SubscriptValueType::Implicit )
    {
        // because binary dictionary items are not allowed on forms, we must use the section to
        // get more details about the current/total occurrences
        const SECT* pSecT = SPT(engine_item.GetDictItem().GetRecord()->GetSymbol());

        if( pSecT->GetMaxOccs() > 1 )
            evaluated_engine_item_subscript.record_occurrence = CalculateEngineItemImplicitOccurrence(engine_item, true);
    }

    else
    {
        evaluated_engine_item_subscript.record_occurrence = evaluate_specified_occurrence(0);
    }

    // item/subitem occurrence
    if( subscript_is_fully_implicit || static_cast<SubscriptValueType>(item_subscript_node.subscript_arguments[2]) == SubscriptValueType::Implicit )
    {
        if( engine_item.GetDictItem().GetItemSubitemOccurs() > 1 )
            evaluated_engine_item_subscript.item_subitem_occurrence = CalculateEngineItemImplicitOccurrence(engine_item, false);
    }

    else
    {
        evaluated_engine_item_subscript.item_subitem_occurrence = evaluate_specified_occurrence(2);
    }

    return evaluated_engine_item_subscript;
}


namespace
{
    template<typename T>
    inline T EvaluateSymbolReference_GetSymbol(const Logic::SymbolTable& symbol_table, int symbol_index)
    {
        if constexpr(std::is_same_v<T, Symbol*>)
        {
            return &symbol_table.GetAt(symbol_index);
        }

        else
        {
            return symbol_table.GetSharedAt(symbol_index);
        }
    }
}


template<typename T/* = Symbol* */>
SymbolReference<T> CIntDriver::EvaluateSymbolReference(int symbol_index, int subscript_compilation)
{
    ASSERT(symbol_index != -1);

    SymbolReference<T> symbol_reference
    {
        EvaluateSymbolReference_GetSymbol<T>(GetSymbolTable(), symbol_index),
        subscript_compilation,
        std::monostate()
    };

    // if an item, evaluate the subscript
    if( symbol_reference.symbol->IsA(SymbolType::Item) )
    {
        ASSERT(subscript_compilation != -1);
        const auto& item_subscript_node = GetNode<Nodes::ItemSubscript>(subscript_compilation);

        symbol_reference.evaluated_subscript = EvaluateEngineItemSubscript(assert_cast<const EngineItem&>(*symbol_reference.symbol), item_subscript_node);
    }

    // if not an item, there is nothing to evaluate
    else
    {
        ASSERT(subscript_compilation == -1);
    }

    return symbol_reference;
}

template SymbolReference<Symbol*> CIntDriver::EvaluateSymbolReference(int symbol_index, int subscript_compilation);
template SymbolReference<std::shared_ptr<Symbol>> CIntDriver::EvaluateSymbolReference(int symbol_index, int subscript_compilation);


template<typename SymbolT>
SymbolT CIntDriver::GetFromSymbolOrEngineItemWorker(const SymbolReference<SymbolT>& symbol_reference, bool use_exceptions)
{
    // if an invalid reference, or not an item, return the symbol directly
    if( symbol_reference.symbol == nullptr || std::holds_alternative<std::monostate>(symbol_reference.evaluated_subscript) )
        return symbol_reference.symbol;

    // evaluate the item occurrence
    ASSERT(std::holds_alternative<EvaluatedEngineItemSubscript>(symbol_reference.evaluated_subscript));
    const EvaluatedEngineItemSubscript& evaluated_engine_item_subscript = std::get<EvaluatedEngineItemSubscript>(symbol_reference.evaluated_subscript);
    const EngineItem& engine_item = assert_cast<const EngineItem&>(*symbol_reference.symbol);

    const Symbol* current_proc_symbol = GetCurrentProcSymbol();

    if( current_proc_symbol != nullptr && SymbolCalculator::GetLevelNumber_base1(*current_proc_symbol) < SymbolCalculator::GetLevelNumber_base1(engine_item) )
    {
        const DICT* dict = SymbolCalculator(GetSymbolTable()).GetDicT(engine_item);
        ASSERT(dict != nullptr);

        // data in external dictionaries and working storage dictionaries can be accessed at any point [ENGINECR_TODO is this true of external forms?]
        if( dict->GetSubType() != SymbolSubType::External && dict->GetSubType() != SymbolSubType::Work )
        {
            issaerror_or_throw(use_exceptions, MessageType::Warning, MGF::DataAccess_data_not_available_until_lower_level_94601, engine_item.GetName().c_str());
            return nullptr;
        }
    }

    VART* pVarT = const_cast<VART*>(&engine_item.GetVarT());
    const GROUPT* pGroupT = pVarT->GetParentGPT();
    ASSERT(pGroupT != nullptr);

    // because binary dictionary items are not allowed on forms (for now), validate the record
    // occurrences by using the section (exsoccurs returns a one-based occurrence)
    const CDictItem& dict_item = engine_item.GetDictItem();
    const SECT* pSecT = SPT(dict_item.GetRecord()->GetSymbol());
    const int record_occurrences = exsoccurs(pSecT, true);

    if( evaluated_engine_item_subscript.record_occurrence < 0 || evaluated_engine_item_subscript.record_occurrence >= record_occurrences )
    {
        issaerror_or_throw(use_exceptions, MessageType::Warning, 34092, pVarT->GetName().c_str(), evaluated_engine_item_subscript.record_occurrence + 1);
        return nullptr;
    }

    // validate the item/subitem occurrence
    if( evaluated_engine_item_subscript.item_subitem_occurrence < 0 || evaluated_engine_item_subscript.item_subitem_occurrence >= static_cast<int>(dict_item.GetItemSubitemOccurs()) )
    {
        issaerror_or_throw(use_exceptions, MessageType::Warning, 34093, pVarT->GetName().c_str(), evaluated_engine_item_subscript.record_occurrence + 1, evaluated_engine_item_subscript.item_subitem_occurrence + 1);
        return nullptr;
    }

    int index[] = { evaluated_engine_item_subscript.record_occurrence, 0, 0 };
    index[( pGroupT->GetDimType() == CDimension::SubItem ) ? 2 : 1] = evaluated_engine_item_subscript.item_subitem_occurrence;
    static_assert(_countof(index) == DIM_MAXDIM);

    CNDIndexes theIndex(ZERO_BASED, index);
    ASSERT(CheckIndexArray(pVarT, theIndex));

    ASSERT(pVarT->IsNumeric());
    double* value_address = GetVarFloatAddr(pVarT, theIndex);
    ASSERT(value_address != nullptr);

    DICT* pDicT = pVarT->GetDPT();
    BinaryStorageFor80* binary_storage;

    // an entry may already exist for this item
    if( *value_address != NOTAPPL && *value_address < pDicT->m_binaryStorageFor80.size() )
    {
        binary_storage = pDicT->m_binaryStorageFor80[static_cast<size_t>(*value_address)].get();
    }

    // or one must be created
    else
    {
        *value_address = pDicT->m_binaryStorageFor80.size();

        binary_storage = pDicT->m_binaryStorageFor80.emplace_back(std::make_shared<BinaryStorageFor80>(BinaryStorageFor80 { BinaryDataAccessor(), nullptr })).get();
    }

    // if a symbol has not been created for this item yet, create one
    if( binary_storage->wrapped_symbol == nullptr )
    {
        switch( engine_item.GetWrappedType() )
        {
            case SymbolType::Audio:
                binary_storage->wrapped_symbol = std::make_shared<LogicAudio>(engine_item, index, &binary_storage->binary_data_accessor);
                break;

            case SymbolType::Document:
                binary_storage->wrapped_symbol = std::make_shared<LogicDocument>(engine_item, index, &binary_storage->binary_data_accessor);
                break;

            case SymbolType::Geometry:
                binary_storage->wrapped_symbol = std::make_shared<LogicGeometry>(engine_item, index, &binary_storage->binary_data_accessor);
                break;

            case SymbolType::Image:
                binary_storage->wrapped_symbol = std::make_shared<LogicImage>(engine_item, index, &binary_storage->binary_data_accessor);
                break;

            default:
                return ReturnProgrammingError(nullptr);
        }
    }

    if constexpr(std::is_same_v<SymbolT, Symbol*>)
    {
        return binary_storage->wrapped_symbol.get();
    }

    else
    {
        return binary_storage->wrapped_symbol;
    }
}

template Symbol* CIntDriver::GetFromSymbolOrEngineItemWorker(const SymbolReference<Symbol*>& symbol_reference, bool use_exceptions);
template std::shared_ptr<Symbol> CIntDriver::GetFromSymbolOrEngineItemWorker(const SymbolReference<std::shared_ptr<Symbol>>& symbol_reference, bool use_exceptions);


Symbol& CIntDriver::GetWrappedEngineItemSymbol(EngineItem& engine_item, const TCHAR* subscript_text)
{
    auto create_symbol_reference_and_get_symbol = [&](const Nodes::ItemSubscript& item_subscript_node) -> Symbol&
    {
        const SymbolReference<Symbol*> symbol_reference
        {
            &engine_item,
            -1,
            EvaluateEngineItemSubscript(engine_item, item_subscript_node)
        };

        return *GetFromSymbolOrEngineItemWorker<Symbol*>(symbol_reference, true);
    };

    // create item subscript nodes based on an implicit subscript
    if( subscript_text == nullptr || SO::IsWhitespace(subscript_text) )
    {
        Nodes::ItemSubscript item_subscript_node;
        item_subscript_node.subscript_type = Nodes::ItemSubscript::SubscriptType::ImplicitMustEvaluate;
        return create_symbol_reference_and_get_symbol(item_subscript_node);
    }

    // or an explicit one
    else
    {
        const ItemIndexHelper& item_index_helper = engine_item.GetItemIndexHelper();
        size_t occurrences[ItemIndex::NumberDimensions];
        const unsigned occurrences_defined = item_index_helper.ParseOccurrencesFromText(subscript_text, occurrences);

        if( occurrences_defined == 0 )
        {
            throw CSProException(_T("The subscript provided for the symbol '%s' is not valid: %s"),
                                 engine_item.GetName().c_str(), subscript_text);
        }

        ASSERT(occurrences_defined == 1 || occurrences_defined == 2);

        // create a buffer that allows for four arguments
        auto item_subscript_node_buffer = std::make_unique_for_overwrite<int[]>(sizeof(Nodes::ItemSubscript) / sizeof(int) + 3);
        Nodes::ItemSubscript& item_subscript_node = *reinterpret_cast<Nodes::ItemSubscript*>(item_subscript_node_buffer.get());

        // the values in the occurrences array are 0-based, so we add 1 to fit the expectation of SubscriptValueType::ConstantInteger
        item_subscript_node.subscript_type = Nodes::ItemSubscript::SubscriptType::Specified;

        // record + item/subitem occurrence
        if( occurrences_defined == 2 )
        {
            item_subscript_node.subscript_arguments[0] = static_cast<int>(SubscriptValueType::ConstantInteger);
            item_subscript_node.subscript_arguments[1] = occurrences[0] + 1;
            item_subscript_node.subscript_arguments[2] = static_cast<int>(SubscriptValueType::ConstantInteger);
            item_subscript_node.subscript_arguments[3] = occurrences[1] + 1;
        }

        // item/subitem occurrence only
        else if( item_index_helper.HasItemSubitemOccurrences() )
        {
            item_subscript_node.subscript_arguments[0] = static_cast<int>(SubscriptValueType::Implicit);
            item_subscript_node.subscript_arguments[2] = static_cast<int>(SubscriptValueType::ConstantInteger);
            item_subscript_node.subscript_arguments[3] = occurrences[0] + 1;
        }

        // record occurrence only
        else
        {
            item_subscript_node.subscript_arguments[0] = static_cast<int>(SubscriptValueType::ConstantInteger);
            item_subscript_node.subscript_arguments[1] = occurrences[0] + 1;
            item_subscript_node.subscript_arguments[2] = static_cast<int>(SubscriptValueType::Implicit);
        }

        return create_symbol_reference_and_get_symbol(item_subscript_node);
    }
}



// --------------------------------------------------------------------------
// Item logic functions
// --------------------------------------------------------------------------

std::tuple<EngineItemAccessor*, bool> CIntDriver::GetEngineItemAccessorAndVisualValueFlag(const Nodes::SymbolVariableArgumentsWithSubscript& symbol_va_with_subscript_node, int visual_value_argument_index)
{
    std::optional<bool> visual_value = ( symbol_va_with_subscript_node.arguments[visual_value_argument_index] != -1 ) ? std::make_optional(ConditionalValueIsTrue(symbol_va_with_subscript_node.arguments[visual_value_argument_index])) :
                                                                                                                        std::nullopt;

    const Symbol* symbol = GetFromSymbolOrEngineItem(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( symbol == nullptr )
        return std::tuple<EngineItemAccessor*, bool>(nullptr, false);

    if( !visual_value.has_value() ) // BINARY_TYPES_TO_ENGINE_TODO + ENGINECR_TODO look at VART::IsAlwaysVisualValue property for default handling
    {
        // visual_value = VART::IsAlwaysVisualValue();
    }

    return std::make_tuple(symbol->GetEngineItemAccessor(), visual_value.value_or(false));
}


double CIntDriver::exItem_getValueLabel(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const std::optional<std::wstring> language = EvaluateOptionalStringExpression(symbol_va_with_subscript_node.arguments[1]);
    EngineItemAccessor* engine_item_accessor;
    bool visual_value;
    std::tie(engine_item_accessor, visual_value) = GetEngineItemAccessorAndVisualValueFlag(symbol_va_with_subscript_node, 0);

    if( engine_item_accessor == nullptr )
        return AssignBlankAlphaValue();

    if( visual_value ) // BINARY_TYPES_TO_ENGINE_TODO + ENGINECR_TODO if on a form and skipped, return AssignBlankAlphaValue
    {
    }

    return AssignAlphaValue(engine_item_accessor->GetValueLabel(language));
}


double CIntDriver::exItem_hasValue_isValid(int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    EngineItemAccessor* engine_item_accessor;
    bool visual_value;
    std::tie(engine_item_accessor, visual_value) = GetEngineItemAccessorAndVisualValueFlag(symbol_va_with_subscript_node, 0);

    if( engine_item_accessor == nullptr )
        return DEFAULT;

    if( visual_value ) // BINARY_TYPES_TO_ENGINE_TODO + ENGINECR_TODO if on a form and skipped, return 0
    {
    }

    if( symbol_va_with_subscript_node.function_code == FunctionCode::ITEMFN_HASVALUE_CODE )
    {
        return engine_item_accessor->HasValue();
    }

    else
    {
        ASSERT(symbol_va_with_subscript_node.function_code == FunctionCode::ITEMFN_ISVALID_CODE);
        return engine_item_accessor->IsValid();
    }
}
