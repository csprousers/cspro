#include "StdAfx.h"
#include <zUtilO/ArrUtil.h>
#include <zEdit2O/LogicView.h>
#include <zLogicO/ActionInvoker.h>
#include <zLogicO/AutoComplete.h>
#include <zLogicO/ContextSensitiveHelp.h>
#include <zLogicO/FunctionTable.h>
#include <zLogicO/GeneralizedFunction.h>
#include <zLogicO/SourceBuffer.h>
#include <zFreqO/Frequency.h>
#include <Zsrcmgro/BackgroundCompiler.h>
#include <Zsrcmgro/ProcGlobalConditionalCompiler.h>
#include <zEngineO/AllSymbols.h>
#include <engine/Engarea.h>


namespace
{
    Logic::AutoComplete LogicReferenceAutoCompleter;


    void AddTabbedText(CString& reference_text, const TCHAR* text_to_add, int tabs, bool add_arrow)
    {
        int spaces = tabs * 4;

        if( add_arrow )
        {
            ASSERT(tabs > 0);
            spaces -= 2;
        }

        reference_text.AppendFormat(_T("%s%s%s\n"), SO::GetRepeatingCharacterString(' ', spaces), add_arrow ? _T("` ") : _T(""), text_to_add);
    }

    void AddCommaSeparatedList(CString& reference_text, const std::vector<CString>& list)
    {
        for( size_t i = 0; i < list.size(); ++i )
            reference_text.AppendFormat(_T("%s%s"), ( i == 0 ) ? _T("") : _T(", "), list[i].GetString());

        reference_text.AppendChar('\n');
    }


    // --------------------------------------------------------------------------
    // ENGINE FUNCTIONS
    // --------------------------------------------------------------------------

    void AddActionInvokerFunction(CString& reference_text, ActionInvoker::Action action)
    {
        const GF::Function* function_definition = GetFunctionDefinition(action);

        if( function_definition == nullptr )
            return;

        constexpr const TCHAR* ActionInvokerTitle = _T("Action Invoker Details");
        constexpr size_t ActionInvokerTitleLength = wstring_view(ActionInvokerTitle).length();
        reference_text.AppendFormat(_T("\n%s\n%s\n"), ActionInvokerTitle, SO::GetRepeatingCharacterString(_T('‾'), ActionInvokerTitleLength));

        if( !function_definition->description.empty() )
            reference_text.AppendFormat(_T("Description: %s\n"), function_definition->description.c_str());

        if( !function_definition->parameters.empty() )
        {
            reference_text.Append(_T("\nInput Parameters:\n"));

            for( const GF::Parameter& parameter : function_definition->parameters )
            {
                reference_text.Append(_T("\n"));
                AddTabbedText(reference_text, FormatText(_T("Name: %s"), parameter.variable.name.c_str()), 1, true);

                if( !parameter.variable.description.empty() )
                    AddTabbedText(reference_text, FormatText(_T("Description: %s"), parameter.variable.description.c_str()), 1, false);

                AddTabbedText(reference_text, FormatText(_T("Required: %s"), parameter.required ? _T("Yes") : _T("No")), 1, false);
            }
        }

        if( !function_definition->returns.empty() )
        {
            reference_text.Append(_T("\nReturns:"));

            if( function_definition->returns.size() == 1 && function_definition->returns.front().name.empty() )
            {
                reference_text.AppendFormat(_T(" %s\n"), function_definition->returns.front().description.c_str());
            }

            else
            {
                reference_text.AppendChar('\n');

                for( const GF::Variable& variable : function_definition->returns )
                {
                    ASSERT(!variable.name.empty() || !variable.description.empty());

                    reference_text.Append(_T("\n"));

                    if( !variable.name.empty() )
                        AddTabbedText(reference_text, FormatText(_T("Name: %s"), variable.name.c_str()), 1, true);

                    if( !variable.description.empty() )
                        AddTabbedText(reference_text, FormatText(_T("Description: %s"), variable.description.c_str()), 1, false);
                }
            }
        }
    }

    void AddEngineFunction(CString& reference_text, const Logic::FunctionDetails& function_details)
    {
        reference_text.AppendFormat(_T("%s\n"), function_details.tooltip);

        if( function_details.compilation_type == Logic::FunctionCompilationType::CS )
            AddActionInvokerFunction(reference_text, static_cast<ActionInvoker::Action>(function_details.number_arguments));
    }


    // --------------------------------------------------------------------------
    // DICTIONARY + FORM ELEMENTS
    // --------------------------------------------------------------------------

    void DictionaryLocator(const CDataDict* dictionary, const void* desired_object,
                           const std::function<void(const std::vector<const DictBase*>&)>& dictionary_locator_callback)
    {
        std::vector<const DictBase*> hierarchy;
        bool keep_processing = true;

        auto add_to_hierarchy = [&](const DictBase& dict_base)
        {
            hierarchy.emplace_back(&dict_base);

            if( &dict_base == desired_object )
            {
                dictionary_locator_callback(hierarchy);
                keep_processing = false;
            }
        };

        add_to_hierarchy(*dictionary);

        for( const DictLevel& dict_level : dictionary->GetLevels() )
        {
            if( !keep_processing )
                break;

            add_to_hierarchy(dict_level);

            for( int r = -1; keep_processing && r < dict_level.GetNumRecords(); ++r )
            {
                const CDictRecord* dict_record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);
                add_to_hierarchy(*dict_record);

                for( int i = 0; keep_processing && i < dict_record->GetNumItems(); ++i )
                {
                    const CDictItem* dict_item = dict_record->GetItem(i);
                    add_to_hierarchy(*dict_item);

                    for( size_t v = 0; keep_processing && v < dict_item->GetNumValueSets(); ++v )
                    {
                        add_to_hierarchy(dict_item->GetValueSet(v));
                        hierarchy.pop_back();
                    }

                    hierarchy.pop_back();
                }

                hierarchy.pop_back();
            }

            hierarchy.pop_back();
        }
    }

    bool LevelLocator(const std::vector<const CDataDict*>& dictionaries,
                      const CString& level_name, const CDataDict** out_dictionary, const DictLevel** out_dict_level)
    {
        for( const CDataDict* dictionary : dictionaries )
        {
            for( const DictLevel& dict_level : dictionary->GetLevels() )
            {
                if( dict_level.GetName().CompareNoCase(level_name) == 0 )
                {
                    *out_dictionary = dictionary;
                    *out_dict_level = &dict_level;
                    return true;
                }
            }
        }

        return false;
    }

    void FormFileLocator(const std::vector<std::shared_ptr<CDEFormFile>>* form_files, const CString& desired_symbol_name,
                         const std::function<void(const std::vector<const CDEFormBase*>&)>& form_file_locator_callback)
    {
        std::vector<const CDEFormBase*> hierarchy;
        bool keep_processing = true;

        auto add_to_hierarchy = [&](const CDEFormBase* form_base)
        {
            hierarchy.emplace_back(form_base);

            if( desired_symbol_name.CompareNoCase(form_base->GetName()) == 0 )
            {
                form_file_locator_callback(hierarchy);
                keep_processing = false;
            }
        };

        std::function<void(const CDEGroup* group)> process_group =
            [&](const CDEGroup* group)
            {
                add_to_hierarchy(group);

                int close_block_index = -1;

                for( int i = 0; keep_processing && i < group->GetNumItems(); ++i )
                {
                    const CDEItemBase* item_base = group->GetItem(i);

                    if( item_base->isA(CDEFormBase::eItemType::Group) || item_base->isA(CDEFormBase::eItemType::Roster) )
                    {
                        process_group(assert_cast<const CDEGroup*>(item_base));
                    }

                    else if( item_base->isA(CDEFormBase::eItemType::Block) )
                    {
                        const CDEBlock* block = assert_cast<const CDEBlock*>(item_base);
                        add_to_hierarchy(block);
                        close_block_index = i + block->GetNumFields();
                    }

                    else if( item_base->isA(CDEFormBase::eItemType::Field) )
                    {
                        add_to_hierarchy(item_base);
                        hierarchy.pop_back();
                    }

                    if( i == close_block_index )
                        hierarchy.pop_back();
                }

                hierarchy.pop_back();
            };

        for( size_t ff = 0; keep_processing && ff < form_files->size(); ++ff )
        {
            const auto& form_file = (*form_files)[ff];
            add_to_hierarchy(form_file.get());

            for( int l = 0; keep_processing && l < form_file->GetNumLevels(); ++l )
            {
                const CDELevel* level = form_file->GetLevel(l);
                add_to_hierarchy(level);

                for( int g = 0; keep_processing && g < level->GetNumGroups(); ++g )
                    process_group(level->GetGroup(g));

                hierarchy.pop_back();
            }

            hierarchy.pop_back();
        }
    }

    void AddFormFileHierarchy(CString& reference_text, const std::vector<const CDEFormBase*>& hierarchy)
    {
        for( size_t i = 0; i < hierarchy.size(); ++i )
            AddTabbedText(reference_text, hierarchy[i]->GetName(), i, ( i > 0 ));
    }

    void AddValueSetValues(CString& reference_text, const CDictItem& dict_item, const DictValueSet& dict_value_set)
    {
        int max_label_length = 1;
        bool has_to = false;
        bool has_special = false;

        // first see how long the longest label is so that we can format things properly
        for( const DictValue& dict_value : dict_value_set.GetValues() )
        {
            max_label_length = std::max(max_label_length, dict_value.GetLabel().GetLength());

            has_special = ( has_special || dict_value.IsSpecial() );
            has_to = ( has_to || ( dict_value.GetNumToValues() > 0 ) );
        }

        CString formatter;
        formatter.Format(_T("%%-%ds ｜ %%%ds%s%%%ds%s%%s\n"),
            max_label_length, dict_item.GetCompleteLen(),
            has_to ? _T(" ｜ ") : _T(""), has_to ? dict_item.GetCompleteLen() : 0,
            has_special ? _T(" ｜ ") : _T(""));

        for( const DictValue& dict_value : dict_value_set.GetValues() )
        {
            bool first_pair = true;

            for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
            {
                reference_text.AppendFormat(formatter,
                                            first_pair ? dict_value.GetLabel().GetString() : _T(""),
                                            dict_value_pair.GetFrom().GetString(),
                                            dict_value_pair.GetTo().GetString(),
                                            dict_value.IsSpecial() ? SpecialValues::ValueToString(dict_value.GetSpecialValue(), false) : _T(""));
                first_pair = false;
            }
        }
    }

    CString GetRecordName(const CDataDict& dictionary, const DictLevel& dict_level, const CDictRecord* record)
    {
        // delimit the ID record because with multiple dictionaries there will be
        // more than one record with names like _IDS0
        if( dict_level.GetIdItemsRec() == record )
        {
            return FormatText(_T("%s.%s"), dictionary.GetName().GetString(), record->GetName().GetString());
        }

        else
        {
            return record->GetName();
        }
    }

    void AddHierarchy(CString& reference_text, const std::vector<const DictBase*>& hierarchy, size_t levels_to_display = SIZE_MAX)
    {
        levels_to_display = std::min(hierarchy.size(), levels_to_display);

        if( levels_to_display >= 1 )
        {
            const CDataDict* dictionary = assert_cast<const CDataDict*>(hierarchy[0]);
            AddTabbedText(reference_text, dictionary->GetName(), 0, false);

            if( levels_to_display >= 2 )
            {
                const DictLevel& dict_level = assert_cast<const DictLevel&>(*hierarchy[1]);
                AddTabbedText(reference_text, dict_level.GetName(), 1, true);

                if( levels_to_display >= 3 )
                {
                    const CDictRecord* record = assert_cast<const CDictRecord*>(hierarchy[2]);
                    AddTabbedText(reference_text, GetRecordName(*dictionary, dict_level, record), 2, true);

                    if( levels_to_display >= 4 )
                    {
                        AddTabbedText(reference_text, assert_cast<const CDictItem*>(hierarchy[3])->GetName(), 3, true);

                        if( levels_to_display >= 5 )
                            AddTabbedText(reference_text, assert_cast<const DictValueSet&>(*hierarchy[4]).GetName(), 4, true);
                    }
                }
            }
        }
    }

    void AddDictionaryLevelHierarchy(CString& reference_text, const CDataDict* dictionary,
                                     const DictLevel* desired_level, bool display_dictionary_hierarchy)
    {
        // add each level's IDs
        for( const DictLevel& dict_level : dictionary->GetLevels() )
        {
            if( desired_level != nullptr && &dict_level != desired_level )
                continue;

            if( desired_level != nullptr || dictionary->GetNumLevels() == 1 )
            {
                reference_text.Append(_T("IDs: "));
            }

            else
            {
                reference_text.AppendFormat(_T("Level %s's IDs: "), dict_level.GetName().GetString());
            }

            const CDictRecord* record = dict_level.GetIdItemsRec();
            std::vector<CString> id_names;

            for( int i = 0; i < record->GetNumItems(); ++i )
                id_names.emplace_back(record->GetItem(i)->GetName());

            AddCommaSeparatedList(reference_text, id_names);
        }

        reference_text.AppendChar('\n');

        if( display_dictionary_hierarchy )
            reference_text.Append(_T("Dictionary Hierarchy:\n\n"));

        // add the dictionary tree with the levels and records expanded
        AddTabbedText(reference_text, dictionary->GetName(), 0, false);

        for( const DictLevel& dict_level : dictionary->GetLevels() )
        {
            if( desired_level != nullptr && &dict_level != desired_level )
                continue;

            AddTabbedText(reference_text, dict_level.GetName(), 1, true);

            for( int r = -1; r < dict_level.GetNumRecords(); ++r )
            {
                const CDictRecord* record = dict_level.GetRecord(( r == -1 ) ? COMMON : r);
                AddTabbedText(reference_text, GetRecordName(*dictionary, dict_level, record), 2, ( r == -1 ));
            }
        };
    }

    void AddDictionary(CString& reference_text, const Symbol* symbol)
    {
        const CDataDict* dictionary;

        if( symbol->IsA(SymbolType::Dictionary) )
        {
            const EngineDictionary* engine_dictionary = assert_cast<const EngineDictionary*>(symbol);
            dictionary = &engine_dictionary->GetDictionary();

            reference_text.Append(_T("Variable Type: "));

            if( engine_dictionary->IsDictionaryObject() )
            {
                reference_text.AppendFormat(_T("Dictionary (%s)\n"), ToString(engine_dictionary->GetSubType()));
            }

            else if( engine_dictionary->IsCaseObject() )
            {
                reference_text.AppendFormat(_T("Case (%s)\n"), dictionary->GetName().GetString());
            }

            else
            {
                reference_text.AppendFormat(_T("DataSource (%s)\n"), dictionary->GetName().GetString());
            }
        }

        else
        {
            const DICT* pDicT = assert_cast<const DICT*>(symbol);
            dictionary = pDicT->GetDataDict();
            reference_text.Append(_T("Variable Type: Dictionary\n"));
        }

        reference_text.AppendFormat(_T("Label: %s\n"), dictionary->GetLabel().GetString());

        AddDictionaryLevelHierarchy(reference_text, dictionary, nullptr, false);
    }

    void AddLevel(CString& reference_text, const DictLevel& dict_level, const CDataDict* dictionary, bool display_dictionary_hierarchy)
    {
        reference_text.Append(_T("Variable Type: Level\n"));
        reference_text.AppendFormat(_T("Label: %s\n"), dict_level.GetLabel().GetString());
        AddDictionaryLevelHierarchy(reference_text, dictionary, &dict_level, display_dictionary_hierarchy);
    }

    void AddRecord(CString& reference_text, const CDictRecord& record)
    {
        reference_text.Append(_T("Variable Type: Record\n"));
        reference_text.AppendFormat(_T("Label: %s\n"), record.GetLabel().GetString());
        reference_text.AppendFormat(_T("Occurrences: %d (%s)\n\n"), record.GetMaxRecs(), record.GetRequired() ? _T("Required") : _T("Not Required"));

        std::function<void(const std::vector<const DictBase*>&)> dictionary_locator_callback =
            [&](const std::vector<const DictBase*>& hierarchy)
            {
                AddHierarchy(reference_text, hierarchy);

                for( int i = 0; i < record.GetNumItems(); ++i )
                {
                    const CDictItem* dict_item = record.GetItem(i);
                    bool add_arrow = false;

                    if( dict_item->GetItemType() == ItemType::Item )
                    {
                        add_arrow = ( i == 0 );
                    }

                    else
                    {
                        add_arrow = ( dict_item->GetParentItem() == record.GetItem(i - 1) );
                    }

                    AddTabbedText(reference_text, dict_item->GetName(), ( dict_item->GetItemType() == ItemType::Item ) ? 3 : 4, add_arrow);
                }
            };

        DictionaryLocator(record.GetDataDict(), &record, dictionary_locator_callback);
    }

    void AddItem(CString& reference_text, const VART* variable, const std::vector<std::shared_ptr<CDEFormFile>>* form_files, const Logic::SymbolTable& symbol_table)
    {
        const CDictItem& dict_item = *variable->GetDictItem();

        reference_text.Append(_T("Variable Type: Item\n"));
        reference_text.AppendFormat(_T("Label: %s\n"), dict_item.GetLabel().GetString());
        reference_text.AppendFormat(_T("Data Type: %s\n"), ToString(dict_item.GetContentType()));

        // length
        reference_text.AppendFormat(_T("Length: %d"), dict_item.GetLen());

        if( dict_item.GetContentType() == ContentType::Numeric )
        {
            reference_text.AppendFormat(_T(" (%s"), CString('X', dict_item.GetIntegerLen()).GetString());

            if( dict_item.GetDecimal() > 0 )
                reference_text.AppendFormat(_T(".%s"), CString('x', dict_item.GetDecimal()).GetString());

            reference_text.AppendChar(')');
        }

        reference_text.AppendChar('\n');

        // item type
        reference_text.AppendFormat(_T("Item Type: %s\n"), ( dict_item.GetItemType() == ItemType::Item ) ? _T("Item") : _T("Subitem"));

        const CDictItem* parent_dict_item = nullptr;

        if( dict_item.GetItemType() == ItemType::Subitem )
        {
            parent_dict_item = dict_item.GetParentItem();
            reference_text.AppendFormat(_T("Parent Item: %s\n"), parent_dict_item->GetName().GetString());
        }

        // check it there are any subitems
        else
        {
            std::vector<CString> subitem_names;

            for( const VART* next_variable = variable;
                ( next_variable = assert_nullable_cast<const VART*>(next_variable->next_symbol) ) != nullptr && next_variable->GetOwnerVarT() == variable; )
            {
                subitem_names.emplace_back(WS2CS(next_variable->GetName()));
            }

            if( !subitem_names.empty() )
            {
                reference_text.Append(_T("Child Items: "));
                AddCommaSeparatedList(reference_text, subitem_names);
            }
        }

        // occurrences
        const CDictRecord* record = variable->GetSPT()->GetDictRecord();
        reference_text.AppendFormat(_T("Record Occurrences: %d (%s)\n"), record->GetMaxRecs(), record->GetRequired() ? _T("Required") : _T("Not Required"));

        reference_text.AppendFormat(_T("Item Occurrences: %d\n"),
            (( parent_dict_item != nullptr ) ? parent_dict_item : &dict_item)->GetOccurs());

        if( parent_dict_item != nullptr )
            reference_text.AppendFormat(_T("Subitem Occurrences: %d\n"), dict_item.GetOccurs());

        // dictionary hierarchy
        reference_text.Append(_T("\nDictionary Hierarchy:\n\n"));

        std::function<void(const std::vector<const DictBase*>&)> dictionary_locator_callback =
            [&](const std::vector<const DictBase*>& hierarchy)
            {
                AddHierarchy(reference_text, hierarchy, hierarchy.size() - 1);

                int tabs = 3;

                if( parent_dict_item != nullptr )
                    AddTabbedText(reference_text, parent_dict_item->GetName(), tabs++, true);

                AddTabbedText(reference_text, dict_item.GetName(), tabs++, true);

                int v = 0;
                for( const DictValueSet& dict_value_set : dict_item.GetValueSets() )
                    AddTabbedText(reference_text, dict_value_set.GetName(), tabs, ( v++ == 0 ));
            };

        DictionaryLocator(variable->GetDataDict(), &dict_item, dictionary_locator_callback);

        if( form_files != nullptr )
        {
            if( variable->GetFormSymbol() == 0 )
            {
                reference_text.AppendFormat(_T("\nNot Located on a Form\n"));
            }

            else
            {
                // form hierarchy
                reference_text.Append(_T("\nForm Hierarchy:\n\n"));

                std::function<void(const std::vector<const CDEFormBase*>&)> form_file_locator_callback =
                    [&](const std::vector<const CDEFormBase*>& hierarchy)
                    {
                        AddFormFileHierarchy(reference_text, hierarchy);
                    };

                FormFileLocator(form_files, WS2CS(variable->GetName()), form_file_locator_callback);

                // field capture type (ideally we could call VART::GetEvaluatedCaptureInfo
                // but it's not accessible so the code is somewhat duplicated here
                CaptureInfo capture_info = variable->GetCaptureInfo().MakeValid(dict_item,
                    dict_item.GetFirstValueSetOrNull());

                reference_text.AppendFormat(_T("\nField Capture Type: %s\n"), capture_info.GetDescription().GetString());
            }
        }

        // first value set
        if( dict_item.HasValueSets() )
        {
            reference_text.Append(_T("\nFirst Value Set:\n\n"));
            AddValueSetValues(reference_text, dict_item, dict_item.GetValueSet(0));
        }
    }

    void AddValueSet(CString& reference_text, const ValueSet& value_set)
    {
        reference_text.Append(_T("Variable Type: ValueSet\n"));
        reference_text.AppendFormat(_T("Label: %s\n"), value_set.GetLabel().GetString());
        reference_text.AppendFormat(_T("Data Type: %s\n"), value_set.IsNumeric() ? _T("Numeric") : _T("String"));

        if( value_set.IsDynamic() )
            return;

        const DictValueSet& dict_value_set = value_set.GetDictValueSet();
        const VART* variable = value_set.GetVarT();
        const CDictItem& dict_item = *variable->GetDictItem();

        if( dict_item.GetNumValueSets() > 1 )
        {
            reference_text.AppendFormat(_T("Other Value Sets of %s: "), dict_item.GetName().GetString());

            bool add_comma = false;

            for( const DictValueSet& this_dict_value_set : dict_item.GetValueSets() )
            {
                if( &this_dict_value_set != &dict_value_set )
                {
                    reference_text.AppendFormat(_T("%s%s"), add_comma ? _T(", ") : _T(""), this_dict_value_set.GetName().GetString());
                    add_comma = true;
                }
            }

            reference_text.AppendChar('\n');
        }

        // display the hierarchy
        reference_text.AppendChar('\n');

        std::function<void(const std::vector<const DictBase*>&)> dictionary_locator_callback =
            [&](const std::vector<const DictBase*>& hierarchy)
            {
                AddHierarchy(reference_text, hierarchy);
            };

        DictionaryLocator(variable->GetDataDict(), &dict_value_set, dictionary_locator_callback);

        // display the value set values
        reference_text.AppendChar('\n');
        AddValueSetValues(reference_text, dict_item, dict_value_set);
    }

    void AddRelation(CString& reference_text, const RELT& relation, const Logic::SymbolTable& symbol_table)
    {
        reference_text.Append(_T("Variable Type: Relation\n"));
        reference_text.AppendFormat(_T("Base Object: %s\n"), symbol_table.GetAt(relation.GetBaseObjIndex()).GetName().c_str());

        for( size_t i = 1; i < relation.m_aTarget.size(); ++i )
        {
            int relation_type = relation.m_aTarget[i].iTargetRelationType;
            const TCHAR* relation_type_string =
                ( relation_type == USE_INDEX_RELATION )        ? _T("Parallel") :
                ( relation_type == USE_LINK_RELATION )         ? _T("Linked") :
                ( relation_type == USE_WHERE_RELATION_SINGLE ) ? _T("Where (single)") :
                ( relation_type == USE_WHERE_RELATION_SINGLE ) ? _T("Where (multiple)") : _T("");

            reference_text.AppendFormat(_T("Linkage to %s: %s\n"), symbol_table.GetAt(relation.m_aTarget[i].iTargetSymbolIndex).GetName().c_str(), relation_type_string);
        }
    }

    void AddFlow(CString& reference_text, const FLOW& flow)
    {
        const CDEFormFile* form_file = flow.GetFormFile();

        reference_text.Append(_T("Variable Type: Form File\n"));
        reference_text.AppendFormat(_T("Label: %s\n\n"), form_file->GetLabel().GetString());

        AddTabbedText(reference_text, form_file->GetName(), 0, false);

        for( int i = 0; i < form_file->GetNumLevels(); ++i )
            AddTabbedText(reference_text, form_file->GetLevel(i)->GetName(), 1, ( i == 0 ));
    }

    void AddGroup(CString& reference_text, const std::vector<const CDataDict*>& dictionaries,
                  const std::vector<std::shared_ptr<CDEFormFile>>* form_files, const GROUPT& group)
    {
        const CDEGroup* form_group = group.GetCDEGroup();

        if( group.GetGroupType() == GROUPT::eGroupType::Level )
        {
            // find the dictionary level
            const CDataDict* dictionary = nullptr;
            const DictLevel* dict_level = nullptr;

            if( LevelLocator(dictionaries, WS2CS(group.GetName()), &dictionary, &dict_level) )
                AddLevel(reference_text, *dict_level, dictionary, true);
        }

        else
        {
            reference_text.AppendFormat(_T("Variable Type: %s\n"),
                form_group->isA(CDEFormBase::eItemType::Roster) ? _T("Roster") : _T("Group"));
            reference_text.AppendFormat(_T("Label: %s\n"), form_group->GetLabel().GetString());
            reference_text.AppendFormat(_T("Occurrences: %d\n"), group.GetMaxOccs());
        }

        std::function<void(const std::vector<const CDEFormBase*>&)> form_file_locator_callback =
            [&](const std::vector<const CDEFormBase*>& hierarchy)
            {
                AddFormFileHierarchy(reference_text, hierarchy);

                bool first_item = true;

                for( int i = 0; i < form_group->GetNumItems(); ++i )
                {
                    const CDEItemBase* item_base = form_group->GetItem(i);

                    if( !item_base->isA(CDEFormBase::eItemType::Text) && !item_base->isA(CDEFormBase::eItemType::UnknownItem) )
                    {
                        AddTabbedText(reference_text, item_base->GetName(), hierarchy.size(), first_item);
                        first_item = false;

                        if( item_base->isA(CDEFormBase::eItemType::Block) )
                            i += assert_cast<const CDEBlock*>(item_base)->GetNumFields();
                    }
                }
            };

        if( form_files != nullptr )
        {
            reference_text.Append(_T("\nForm Hierarchy:\n\n"));
            FormFileLocator(form_files, form_group->GetName(), form_file_locator_callback);
        }
    }

    void AddEngineBlock(CString& reference_text, const std::vector<std::shared_ptr<CDEFormFile>>* form_files, const EngineBlock& engine_block)
    {
        const CDEBlock& form_block = engine_block.GetFormBlock();

        reference_text.Append(_T("Variable Type: Block\n"));
        reference_text.AppendFormat(_T("Label: %s\n"), form_block.GetLabel().GetString());
        reference_text.AppendFormat(_T("Display on Same Screen: %s\n\n"), form_block.GetDisplayTogether() ? _T("Yes") : _T("No"));

        std::function<void(const std::vector<const CDEFormBase*>&)> form_file_locator_callback =
            [&](const std::vector<const CDEFormBase*>& hierarchy)
            {
                AddFormFileHierarchy(reference_text, hierarchy);

                const CDEGroup* group = assert_cast<const CDEGroup*>(hierarchy[hierarchy.size() - 2]);
                int first_field_index = group->FindItem(form_block.GetName()) + 1;

                for( int i = 0; i < form_block.GetNumFields(); ++i )
                    AddTabbedText(reference_text, group->GetItem(first_field_index + i)->GetName(), hierarchy.size(), ( i == 0 ));
            };

        if( form_files != nullptr )
            FormFileLocator(form_files, WS2CS(engine_block.GetName()), form_file_locator_callback);
    }


    // --------------------------------------------------------------------------
    // LOGIC SYMBOLS
    // --------------------------------------------------------------------------

    void AddBasicSymbol(CString& reference_text, const Symbol& symbol)
    {
        switch( symbol.GetType() )
        {
            case SymbolType::Audio:
            {
                reference_text.Append(_T("Variable Type: Audio\n"));
                break;
            }

            case SymbolType::Document:
            {
                reference_text.Append(_T("Variable Type: Document\n"));
                break;
            }

            case SymbolType::File:
            {
                reference_text.Append(_T("Variable Type: File\n"));
                break;
            }

            case SymbolType::Geometry:
            {
                reference_text.Append(_T("Variable Type: Geometry\n"));
                break;
            }

            case SymbolType::Image:
            {
                reference_text.Append(_T("Variable Type: Image\n"));
                break;
            }

            case SymbolType::List:
            {
                const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
                reference_text.AppendFormat(_T("Variable Type: List\nData Type: %s\n"), logic_list.IsNumeric() ? _T("Numeric") : _T("String"));
                break;
            }

            case SymbolType::Map:
            {
                reference_text.Append(_T("Variable Type: Map\n"));
                break;
            }

            case SymbolType::Pff:
            {
                reference_text.Append(_T("Variable Type: Pff\n"));
                break;
            }

            case SymbolType::SystemApp:
            {
                reference_text.Append(_T("Variable Type: SystemApp\n"));
                break;
            }

            case SymbolType::WorkString:
            {
                if( symbol.GetSubType() == SymbolSubType::WorkAlpha )
                {
                    reference_text.AppendFormat(_T("Variable Type: Alpha\nLength: %d\n"), assert_cast<const WorkAlpha&>(symbol).GetLength());
                }

                else
                {
                    reference_text.Append(_T("Variable Type: String\nLength: Unlimited\n"));
                }

                break;
            }

            case SymbolType::WorkVariable:
            {
                reference_text.Append(_T("Variable Type: Numeric\n"));
                break;
            }
        }
    }

    void AddArray(CString& reference_text, const LogicArray& logic_array, const Logic::SymbolTable& symbol_table)
    {
        reference_text.Append(_T("Variable Type: Array\n"));

        reference_text.AppendFormat(_T("Data Type: %s"), logic_array.IsNumeric() ? _T("Numeric") : _T(""));

        if( logic_array.IsString() )
        {
            if( logic_array.GetPaddingStringLength() == 0 )
            {
                reference_text.Append(_T("String\nCell Length: Unlimited"));
            }

            else
            {
                reference_text.AppendFormat(_T("Alpha\nCell Length: %d"), logic_array.GetPaddingStringLength());
            }
        }

        reference_text.AppendChar('\n');

        reference_text.AppendFormat(_T("Dimensions: %d\n"), (int)logic_array.GetNumberDimensions());

        reference_text.Append(_T("Dimension Sizes: "));

        for( size_t i = 0; i < logic_array.GetNumberDimensions(); ++i )
        {
            if( i > 0 )
                reference_text.Append(_T(", "));

            int deckarray_symbol = logic_array.GetDeckArraySymbols()[i];

            if( deckarray_symbol != 0 )
            {
                reference_text.AppendFormat(_T("%s"), symbol_table.GetAt(abs(deckarray_symbol)).GetName().c_str());

                if( deckarray_symbol < 0 )
                    reference_text.Append(_T("(+)"));

                reference_text.Append(_T(" ["));
            }

            reference_text.AppendFormat(_T("%d"), (int)logic_array.GetDimension(i) - 1);

            if( deckarray_symbol != 0 )
                reference_text.Append(_T("]"));
        }

        reference_text.AppendChar('\n');

        reference_text.AppendFormat(_T("Save Array: %s\n"), logic_array.GetUsingSaveArray() ? _T("Yes") : _T("No"));
    }

    void AddHashMap(CString& reference_text, const LogicHashMap& hashmap)
    {
        reference_text.Append(_T("Variable Type: HashMap\n"));

        reference_text.AppendFormat(_T("Data Type: %s\n"), ToString(hashmap.GetValueType()));

        if( hashmap.HasDefaultValue() )
        {
            reference_text.AppendFormat(_T("Default Value: %s\n"), hashmap.IsValueTypeNumeric() ?
                                        DoubleToString(std::get<double>(*hashmap.GetDefaultValue())).c_str() :
                                        std::get<std::wstring>(*hashmap.GetDefaultValue()).c_str());
        }

        reference_text.AppendFormat(_T("Dimensions: %d\n"), (int)hashmap.GetNumberDimensions());

        reference_text.Append(_T("Dimension Types: "));

        for( size_t i = 0; i < hashmap.GetNumberDimensions(); ++i )
        {
            if( i > 0 )
                reference_text.Append(_T(", "));

            if( hashmap.GetDimensionType(i).has_value() )
            {
                reference_text.Append(ToString(*hashmap.GetDimensionType(i)));
            }

            else
            {
                reference_text.Append(_T("Numeric/String"));
            }
        }

        reference_text.AppendChar('\n');
    }

    void AddNamedFrequency(CString& reference_text, const NamedFrequency& named_frequency, CEngineArea* m_pEngineArea)
    {
        auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

        reference_text.Append(_T("Variable Type: Freq\n"));

        if( named_frequency.IsFunctionParameter() )
        {
            reference_text.Append(_T("Function Parameter\n"));
            return;
        }

        // list each variable included in the frequency
        const Frequency& frequency = *m_pEngineArea->GetEngineData().frequencies[named_frequency.GetFrequencyIndex()];

        if( frequency.GetFrequencyEntries().empty() )
            return;

        reference_text.Append(_T("\nVariables Tallied:\n\n"));

        for( const FrequencyEntry& frequency_entry : frequency.GetFrequencyEntries() )
        {
            const Symbol* symbol = NPT(frequency_entry.symbol_index);

            auto output_variable = [&](const CString& record_details, const CString& item_subitem_details)
            {
                CString occurrences = record_details;

                if( !item_subitem_details.IsEmpty() )
                    occurrences.AppendFormat(occurrences.IsEmpty() ? _T("%s"): _T(", %s"), item_subitem_details.GetString());

                reference_text.AppendFormat(occurrences.IsEmpty() ? _T("%s\n") : _T("%s(%s)\n"), symbol->GetName().c_str(), occurrences.GetString());
            };

            if( frequency_entry.occurrence_details.empty() )
            {
                output_variable(CString(), CString());
            }

            else
            {
                ASSERT(symbol->IsA(SymbolType::Variable));
                const VART* pVarT = assert_cast<const VART*>(symbol);
                const CDictItem* item = pVarT->GetDictItem();
                bool record_repeats = ( item != nullptr && item->GetRecord()->GetMaxRecs() > 1 );
                bool item_subitem_repeats = ( item != nullptr && item->GetItemSubitemOccurs() > 1 );

                for( const auto& occurrence_details : frequency_entry.occurrence_details )
                {
                    auto add_record_details = [&](const CString& record_details)
                    {
                        if( !item_subitem_repeats )
                        {
                            output_variable(record_details, CString());
                        }

                        else
                        {
                            if( occurrence_details.min_item_subitem_occurrence != occurrence_details.max_item_subitem_occurrence )
                            {
                                output_variable(record_details, _T("*"));
                            }

                            else
                            {
                                output_variable(record_details, IntToString(occurrence_details.min_item_subitem_occurrence + 1));
                            }
                        }
                    };

                    if( !record_repeats )
                    {
                        add_record_details(CString());
                    }

                    else if( occurrence_details.combine_record_occurrences )
                    {
                        add_record_details(_T("*"));
                    }

                    else
                    {
                        if( occurrence_details.disjoint_record_occurrences )
                            add_record_details(_T("disjoint"));

                        for( size_t occurrence : occurrence_details.record_occurrences_to_explicitly_display )
                            add_record_details(IntToString(occurrence + 1));
                    }
                }
            }
        }
    }

    void AddReport(CString& reference_text, const Application& application, const Report& report)
    {
        reference_text.Append(_T("Variable Type: Report\n"));

        if( report.IsFunctionParameter() )
        {
            reference_text.Append(_T("Function Parameter\n"));
        }

        else
        {
            reference_text.AppendFormat(_T("Filename: %s\n"), GetRelativeFNameForDisplay(application.GetApplicationFilename(), report.GetFilename()).c_str());
        }
    }

    void AddUserFunction(CString& reference_text, const UserFunction& user_function, const Logic::SymbolTable& symbol_table)
    {
        reference_text.Append(_T("Variable Type: Function\n"));

        reference_text.Append(_T("Return Type: "));

        if( user_function.GetReturnType() == SymbolType::WorkVariable )
        {
            reference_text.Append(_T("Numeric"));
        }

        else if( user_function.GetReturnPaddingStringLength() == 0 )
        {
            reference_text.Append(_T("String"));
        }

        else
        {
            reference_text.AppendFormat(_T("Alpha (%d)"), user_function.GetReturnPaddingStringLength());
        }

        reference_text.AppendChar('\n');

        if( user_function.IsSqlCallbackFunction() )
            reference_text.Append(_T("SQL Callback Function: Yes\n"));

        if( user_function.GetNumberParameters() > 0 )
        {
            reference_text.AppendChar('\n');

            std::vector<std::tuple<int, CString, CString, bool>> parameter_information;
            int parameter_type_max_length = 0;

            for( size_t i = 0; i < user_function.GetNumberParameters(); ++i )
            {
                const Symbol& parameter_symbol = user_function.GetParameterSymbol(i);
                bool parameter_is_optional = ( i >= user_function.GetNumberRequiredParameters() );

                CString parameter_type_string =
                    ( parameter_symbol.IsA(SymbolType::WorkVariable) ) ? _T("Numeric") :
                    ( parameter_symbol.IsA(SymbolType::WorkString) )   ? _T("String") :
                    ( parameter_symbol.IsA(SymbolType::UserFunction) ) ? _T("Function") :
                                                                         ToString(parameter_symbol.GetType());

                if( parameter_symbol.GetSubType() == SymbolSubType::WorkAlpha )
                {
                    parameter_type_string.Format(_T("Alpha (%d)"), assert_cast<const WorkAlpha&>(parameter_symbol).GetLength());
                }

                else if( parameter_symbol.IsA(SymbolType::List) )
                {
                    parameter_type_string.AppendFormat(_T(" (%s)"), assert_cast<const LogicList&>(parameter_symbol).IsNumeric() ? _T("numeric") : _T("string"));
                }

                else if( parameter_symbol.IsA(SymbolType::ValueSet) )
                {
                    parameter_type_string.AppendFormat(_T(" (%s)"), assert_cast<const ValueSet&>(parameter_symbol).IsNumeric() ? _T("numeric") : _T("string"));
                }

                parameter_information.emplace_back(i + 1, parameter_type_string, WS2CS(parameter_symbol.GetName()), parameter_is_optional);

                parameter_type_max_length = std::max(parameter_type_max_length, parameter_type_string.GetLength());
            }

            const TCHAR* OptionalText = _T("(optional) ");

            CString formatter;
            formatter.Format(_T("Parameter %%%dd %%%ds| %%-%ds | %%s\n"),
                (int)log10(user_function.GetNumberParameters()) + 1,
                std::get<3>(parameter_information.back()) ? _tcslen(OptionalText) : 0,
                parameter_type_max_length);

            for( const auto& parameter : parameter_information )
            {
                reference_text.AppendFormat(formatter,
                    std::get<0>(parameter),
                    std::get<3>(parameter) ? OptionalText : _T(""),
                    std::get<1>(parameter).GetString(),
                    std::get<2>(parameter).GetString());
            }
        }
    }


    const std::vector<const TCHAR*> declaration_start_words = { _T("do"), _T("for") };

    const std::vector<const TCHAR*> declaration_break_words = { _T("do"), _T("while"), _T("until"), _T("in") };

    const std::vector<const TCHAR*> scope_change_words      = { _T("proc"), _T("function"), _T("if"), _T("elseif"),
                                                                _T("else"), _T("do"),       _T("for") };


    bool FindSymbolInBuffer(const Symbol* symbol, const Logic::SourceBuffer& source_buffer,
                            const Logic::BasicToken** name_token, const Logic::BasicToken** declaration_token)
    {
        const std::vector<Logic::BasicToken>& basic_tokens = source_buffer.GetTokens();
        std::vector<std::tuple<const Logic::BasicToken*, const Logic::BasicToken*>> declaration_candidates;

        // because functions can't be scoped, simply search for the first use of this name
        if( symbol->IsA(SymbolType::UserFunction) )
        {
            const Logic::BasicToken* last_function_token = nullptr;

            for( const Logic::BasicToken& basic_token : basic_tokens )
            {
                if( basic_token.type == Logic::BasicToken::Type::Text )
                {
                    if( SO::EqualsNoCase(basic_token.GetTextSV(), _T("function")) )
                    {
                        last_function_token = &basic_token;
                    }

                    else if( last_function_token != nullptr && SO::EqualsNoCase(basic_token.GetTextSV(), symbol->GetName()) )
                    {
                        declaration_candidates.emplace_back(&basic_token, last_function_token);
                        break;
                    }
                }
            }
        }

        // otherwise search the buffer from the end to the beginning, looking for the symbol name
        else
        {
            std::vector<std::wstring> possible_declaration_text;

            for( const auto& [declaration_text, symbol_type] : Symbol::GetDeclarationTextMap() )
            {
                if( symbol->IsA(symbol_type) )
                    possible_declaration_text.emplace_back(declaration_text);
            }

            // function included because of implicit declaration of numeric parameters
            if( symbol->IsA(SymbolType::WorkVariable) )
                possible_declaration_text.emplace_back(_T("function"));

            const Logic::BasicToken* name_token_candidate = nullptr;

            for( auto token_itr = basic_tokens.crbegin(); token_itr != basic_tokens.crend(); ++token_itr )
            {
                if( name_token_candidate != nullptr )
                {
                    // a semicolon breaks any current declaration
                    if( token_itr->token_code == TokenCode::TOKSEMICOLON )
                    {
                        name_token_candidate = nullptr;
                    }

                    else if( token_itr->type == Logic::BasicToken::Type::Text )
                    {
                        // check if this word could begin the declaration
                        for( const std::wstring& declaration_text : possible_declaration_text )
                        {
                            if( SO::EqualsNoCase(token_itr->GetTextSV(), declaration_text) )
                            {
                                declaration_candidates.emplace_back(name_token_candidate, &(*token_itr));
                                break;
                            }
                        }

                        // check if this word ends any possible declaration
                        for( const TCHAR* declaration_break_word : declaration_break_words )
                        {
                            if( SO::EqualsNoCase(token_itr->GetTextSV(), declaration_break_word) )
                            {
                                name_token_candidate = nullptr;
                                break;
                            }
                        }
                    }
                }

                if( token_itr->type == Logic::BasicToken::Type::Text )
                {
                    if( SO::EqualsNoCase(token_itr->GetTextSV(), symbol->GetName()) )
                    {
                        name_token_candidate = &(*token_itr);
                    }

                    // if there is already a possible declaration candidate, check if we are in a scope change
                    else if( !declaration_candidates.empty() )
                    {
                        bool break_processing = false;

                        for( const TCHAR* scope_change_word : scope_change_words )
                        {
                            if( SO::EqualsNoCase(token_itr->GetTextSV(), scope_change_word) )
                            {
                                break_processing = true;
                                break;
                            }
                        }

                        if( break_processing )
                            break;
                    }
                }
            }
        }

        if( !declaration_candidates.empty() )
        {
            *name_token = std::get<0>(declaration_candidates.back());
            *declaration_token = std::get<1>(declaration_candidates.back());
            return true;
        }

        return false;
    }

    std::optional<size_t> FindSymbolPositionInBuffer(const Symbol* symbol, const Logic::SourceBuffer& source_buffer)
    {
        const Logic::BasicToken* name_token = nullptr;
        const Logic::BasicToken* declaration_token = nullptr;

        if( FindSymbolInBuffer(symbol, source_buffer, &name_token, &declaration_token) )
            return source_buffer.GetPositionInBuffer(*name_token);

        return std::nullopt;
    }

    void GetSymbolDeclarationLogic(CString& logic_text, const Symbol* symbol, const Logic::SourceBuffer& source_buffer)
    {
        const Logic::BasicToken* name_token = nullptr;
        const Logic::BasicToken* declaration_start_token = nullptr;

        if( !FindSymbolInBuffer(symbol, source_buffer, &name_token, &declaration_start_token) )
            return;

        const std::vector<Logic::BasicToken>& basic_tokens =  source_buffer.GetTokens();
        const Logic::BasicToken* first_token = &(basic_tokens.front());
        bool symbol_is_function_parameter = false;

        if( declaration_start_token != first_token )
        {
            const Logic::BasicToken* previous_token = declaration_start_token - 1;
            bool symbol_is_declared_in_loop = false;

            // numeric variables can be declared as part of a loop so include the beginning of the loop declaration
            if( symbol->IsA(SymbolType::WorkVariable) )
            {
                if( previous_token->type == Logic::BasicToken::Type::Text )
                {
                    for( const TCHAR* declaration_start_word : declaration_start_words )
                    {
                        if( SO::EqualsNoCase(previous_token->GetTextSV(), declaration_start_word) )
                        {
                            symbol_is_declared_in_loop = true;
                            declaration_start_token = previous_token;
                            break;
                        }
                    }
                }
            }

            // variables may be parameters of a function, in which case we will show the full function declaration
            if( !symbol_is_declared_in_loop )
            {
                // search for the first 'function' from after the previous semicolon or 'end' (which will accomodate
                // this code processing function parameters)
                const Logic::BasicToken* first_function_token = nullptr;

                // because numeric function parameters variables can be created implicitly, don't start
                // processing at previous_token because it could currently be pointing to 'function'
                if( symbol->IsA(SymbolType::WorkVariable) )
                    previous_token++;

                for( ; previous_token >= first_token && previous_token->token_code != TokenCode::TOKSEMICOLON; previous_token-- )
                {
                    if( previous_token->type == Logic::BasicToken::Type::Text )
                    {
                        if( SO::EqualsNoCase(previous_token->GetTextSV(), _T("function")) )
                        {
                            first_function_token = previous_token;
                        }

                        else if( SO::EqualsNoCase(previous_token->GetTextSV(), _T("end")) )
                        {
                            break;
                        }
                    }
                }

                if( first_function_token != nullptr )
                {
                    symbol_is_function_parameter = true;
                    declaration_start_token = first_function_token;
                }
            }
        }


        // find the end of the declaration, which will be a semicolon for anything other than a function;
        // functions must end with 'end' (and then an optional semicolon);
        // function parameters have to be handled by processing parentheses
        const Logic::BasicToken* declaration_end_token = nullptr;

        if( symbol_is_function_parameter )
        {
            size_t number_parentheses = SIZE_MAX;

            for( auto token_itr = basic_tokens.cbegin() + ( declaration_start_token - first_token + 1 );
                 declaration_end_token == nullptr && token_itr != basic_tokens.cend();
                 ++token_itr )
            {
                if( token_itr->token_code == TokenCode::TOKLPAREN )
                {
                    if( number_parentheses == SIZE_MAX )
                    {
                        number_parentheses = 1;
                    }

                    else
                    {
                        number_parentheses++;
                    }
                }

                if( token_itr->token_code == TokenCode::TOKRPAREN )
                {
                    if( --number_parentheses == 0 )
                        declaration_end_token = &(*token_itr);
                }
            }
        }

        for( auto token_itr = basic_tokens.cbegin() + ( name_token - first_token + 1 );
             declaration_end_token == nullptr && token_itr != basic_tokens.cend();
             ++token_itr )
        {
            if( symbol->IsA(SymbolType::UserFunction) )
            {
                if( token_itr->type == Logic::BasicToken::Type::Text && SO::EqualsNoCase(token_itr->GetTextSV(), _T("end")) )
                {
                    declaration_end_token = &(*token_itr);

                    if( ++token_itr != basic_tokens.cend() && token_itr->token_code == TokenCode::TOKSEMICOLON )
                        declaration_end_token++;
                }
            }

            else
            {
                if( token_itr->token_code == TokenCode::TOKSEMICOLON )
                {
                    declaration_end_token = &(*token_itr);
                }

                // end numeric variables declared as part of a loop
                else if( symbol->IsA(SymbolType::WorkVariable) && token_itr->type == Logic::BasicToken::Type::Text )
                {
                    for( const TCHAR* declaration_break_word : declaration_break_words )
                    {
                        if( SO::EqualsNoCase(token_itr->GetTextSV(), declaration_break_word) )
                        {
                            declaration_end_token = &(*token_itr) - 1;
                            break;
                        }
                    }
                }
            }
        }

        if( declaration_end_token != nullptr )
        {
            size_t declaration_length = ( declaration_end_token->token_text + declaration_end_token->token_length ) - declaration_start_token->token_text;
            logic_text = CString(declaration_start_token->token_text, (int)declaration_length);
        }
    }



    class LogicReferenceWorker
    {
    public:
        LogicReferenceWorker(CMainFrame* main_frame)
        {
            m_currentMdiWindow = main_frame->MDIGetActive();

            if( m_currentMdiWindow == nullptr )
                return;

            m_activeDocument = m_currentMdiWindow->GetActiveDocument();
            m_applicationDocument = main_frame->ProcessFOForSrcCode(*m_activeDocument);

            if( IsEntryApplication() )
            {
                CFormChildWnd* form_window = assert_cast<CFormChildWnd*>(m_currentMdiWindow);
                m_applicationWindow = form_window;
                m_logicControl = form_window->GetSourceView()->GetLogicCtrl();
                m_logicReferenceWindow = &form_window->GetLogicReferenceWnd();
                m_treeControl = assert_cast<CFormDoc*>(m_activeDocument)->GetFormTreeCtrl();
            }

            else if( IsBatchApplication() )
            {
                COrderChildWnd* order_window = assert_cast<COrderChildWnd*>(m_currentMdiWindow);
                m_applicationWindow = order_window;
                m_logicControl = order_window->GetOSourceView()->GetLogicCtrl();
                m_logicReferenceWindow = &order_window->GetLogicReferenceWnd();
                m_treeControl = assert_cast<COrderDoc*>(m_activeDocument)->GetOrderTreeCtrl();
            }

            else if( IsTabulationApplication() )
            {
                CTableChildWnd* table_window = assert_cast<CTableChildWnd*>(m_currentMdiWindow);
                m_applicationWindow = table_window;
                m_logicControl = table_window->GetSourceView()->GetLogicCtrl();
                m_logicReferenceWindow = &table_window->GetLogicReferenceWnd();
                m_treeControl = assert_cast<CTabulateDoc*>(m_activeDocument)->GetTabTreeCtrl();
            }
        }


        bool IsValid() const                 { return ( m_applicationWindow != nullptr ); }

        bool IsEntryApplication() const      { return ( m_currentMdiWindow->IsKindOf(RUNTIME_CLASS(CFormChildWnd)) ); }
        bool IsBatchApplication() const      { return ( m_currentMdiWindow->IsKindOf(RUNTIME_CLASS(COrderChildWnd)) ); }
        bool IsTabulationApplication() const { return ( m_currentMdiWindow->IsKindOf(RUNTIME_CLASS(CTableChildWnd)) ); }

        CDocument* GetActiveDocument()       { return m_activeDocument; }
        CAplDoc* GetApplicationDocument()    { return m_applicationDocument; }
        Application& GetApplication()        { return m_applicationDocument->GetAppObject(); }


        enum class SelectedNode { Global, ExternalCode, Report, Proc };

        std::optional<std::tuple<const TextSource*, SelectedNode>> GetExternalTextSourceCurrentlyEditingDetails() const
        {
            std::optional<std::tuple<const TextSource*, SelectedNode>> external_text_source_details;

            auto process_item_data = [&](auto item_data, auto item_type, SelectedNode selected_node)
            {
                if( item_data != nullptr && item_data->GetItemType() == item_type )
                    external_text_source_details.emplace(item_data->GetTextSource(), selected_node);

                return external_text_source_details.has_value();
            };

            HTREEITEM hItem = m_treeControl->GetSelectedItem();

            if( IsEntryApplication() )
            {
                process_item_data(reinterpret_cast<const CFormID*>(m_treeControl->GetItemData(hItem)), eFFT_EXTERNALCODE, SelectedNode::ExternalCode)
                || process_item_data(reinterpret_cast<const CFormID*>(m_treeControl->GetItemData(hItem)), eFFT_REPORT, SelectedNode::Report);
            }

            else if( IsBatchApplication() )
            {
                const AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(m_treeControl->GetItemData(hItem));

                if( app_tree_node != nullptr )
                {
                    if( app_tree_node->GetAppFileType() == AppFileType::Code )
                    {
                        external_text_source_details.emplace(app_tree_node->GetTextSource(), SelectedNode::ExternalCode);
                    }

                    else if( app_tree_node->GetAppFileType() == AppFileType::Report )
                    {
                        external_text_source_details.emplace(app_tree_node->GetTextSource(), SelectedNode::Report);
                    }
                }
            }

            return external_text_source_details;
        }

        const TextSource* GetExternalLogicTextSourceCurrentlyEditing() const
        {
            auto external_text_source_details = GetExternalTextSourceCurrentlyEditingDetails();

            if( external_text_source_details.has_value() && std::get<1>(*external_text_source_details) == SelectedNode::ExternalCode )
                return std::get<0>(*external_text_source_details);

            return nullptr;
        }

        SelectedNode GetSelectedNode() const
        {
            HTREEITEM hItem = m_treeControl->GetSelectedItem();

            if( hItem == m_treeControl->GetRootItem() )
                return SelectedNode::Global;

            auto external_text_source_details = GetExternalTextSourceCurrentlyEditingDetails();

            if( external_text_source_details.has_value() )
            {
                return std::get<1>(*external_text_source_details);
            }

            else
            {
                return SelectedNode::Proc;
            }
        }

        CString GetSelectedNodeName() const
        {
            ASSERT(!IsTabulationApplication() && GetSelectedNode() == SelectedNode::Proc);

            HTREEITEM hItem = m_treeControl->GetSelectedItem();
            const CDEFormBase* form_base = nullptr;

            if( IsEntryApplication() )
            {
                CFormID* pFormID = reinterpret_cast<CFormID*>(m_treeControl->GetItemData(hItem));

                if( pFormID->GetItemType() == eFTT_GRIDFIELD )
                {
                    CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster, pFormID->GetItemPtr());
                    form_base = pRoster->GetCol(pFormID->GetColumnIndex())->GetField(pFormID->GetRosterField());
                }

                else
                {
                    form_base = pFormID->GetItemPtr();
                }
            }

            else if( IsBatchApplication() )
            {
                AppTreeNode* app_tree_node = reinterpret_cast<AppTreeNode*>(m_treeControl->GetItemData(hItem));
                form_base = app_tree_node->GetFormBase();
            }

            return ( form_base != nullptr ) ? form_base->GetName() : CString();
        }


        std::unique_ptr<BackgroundCompiler> CreateCompiler()
        {
            std::unique_ptr<ProcGlobalConditionalCompilerCreator> compiler_creator;

            m_externalLogicCompileNotificationCallback = std::make_shared<std::function<void(const TextSource&, std::shared_ptr<const Logic::SourceBuffer>)>>();

            const TextSource* external_logic_text_source = GetExternalLogicTextSourceCurrentlyEditing();

            // if on external code, the compiler should stop including external code before getting to this file
            if( external_logic_text_source != nullptr )
            {
                compiler_creator = ProcGlobalConditionalCompilerCreator::CompileSomeExternalCode(*external_logic_text_source, false);
            }

            // otherwise all external code should be compiled
            else
            {
                compiler_creator = ProcGlobalConditionalCompilerCreator::CompileAllExternalCode();
            }

            compiler_creator->SetCompileNotificationCallback(m_externalLogicCompileNotificationCallback);

            return std::make_unique<BackgroundCompiler>(GetApplication(), nullptr, compiler_creator.get());
        }


        void PrepareCurrentBufferText(CString& buffer_text)
        {
            // add a dummy PROC GLOBAL to external code
            if( GetSelectedNode() == LogicReferenceWorker::SelectedNode::ExternalCode )
                buffer_text.Insert(0, _T("PROC GLOBAL\r\n"));
        }


        std::shared_ptr<Logic::SourceBuffer> CompileProcGlobalIfNotEditing(BackgroundCompiler& background_compiler)
        {
            // if currently on a logic control, check if this is PROC GLOBAL (or external code), in which case
            // the full PROC GLOBAL shouldn't be compiled because it will be compiled as part of the current buffer
            std::shared_ptr<Logic::SourceBuffer> source_buffer;

            SelectedNode selected_node = GetSelectedNode();

            if( selected_node != SelectedNode::Global && selected_node != SelectedNode::ExternalCode )
            {
                CSourceCode* source_code = GetApplication().GetAppSrcCode();

                CStringArray proc_global_lines;
                CString proc_global_buffer;
                source_code->GetProc(proc_global_lines, _T("GLOBAL"));
                source_code->ArrayToString(&proc_global_lines, proc_global_buffer, true);

                source_buffer = std::make_shared<Logic::SourceBuffer>(CS2WS(proc_global_buffer));

                background_compiler.Compile(source_buffer);
            }

            return source_buffer;
        }

        std::shared_ptr<Logic::SourceBuffer> CompileCurrentBuffer(BackgroundCompiler& background_compiler)
        {
            std::shared_ptr<Logic::SourceBuffer> source_buffer;

            if( m_logicControl->GetParent()->IsKindOf(RUNTIME_CLASS(CLogicView)) )
            {
                // instead of compiling the whole buffer, compile up to the semicolon following the selected word
                CString buffer_text = WS2CS(m_logicControl->GetText());
                PrepareCurrentBufferText(buffer_text);

                source_buffer = std::make_shared<Logic::SourceBuffer>(CS2WS(buffer_text));
                source_buffer->Tokenize(GetApplication().GetLogicSettings());
                source_buffer->RemoveTokensAfterText(m_logicControl->GetCurrentPos(), TokenCode::TOKSEMICOLON);

                background_compiler.Compile(source_buffer);
            }

            return source_buffer;
        }

        void CompileToCurrentLocation()
        {
            std::unique_ptr<BackgroundCompiler> background_compiler = CreateCompiler();

            CompileProcGlobalIfNotEditing(*background_compiler);
            CompileCurrentBuffer(*background_compiler);
        }


        enum class CompilationLocation { ProcGlobalIfNotEditing, CurrentBuffer };

        template<typename PCCUC>
        void CompileToCurrentLocation(BackgroundCompiler& background_compiler, PCCUC post_compile_compilation_unit_callback)
        {
            ASSERT(m_externalLogicCompileNotificationCallback != nullptr);
            bool keep_processing = true;

            *m_externalLogicCompileNotificationCallback = [&](const TextSource& external_logic_text_source,
                std::shared_ptr<const Logic::SourceBuffer> post_compile_source_buffer)
            {
                ASSERT(GetExternalLogicTextSourceCurrentlyEditing() != &external_logic_text_source);

                // the callback is called before and after the external code is compiled;
                // only pass this to the callback function post-compile
                if( post_compile_source_buffer != nullptr )
                    keep_processing = post_compile_compilation_unit_callback(&external_logic_text_source, post_compile_source_buffer);
            };

            std::shared_ptr<Logic::SourceBuffer> source_buffer = CompileProcGlobalIfNotEditing(background_compiler);

            if( keep_processing && source_buffer != nullptr )
                keep_processing = post_compile_compilation_unit_callback(CompilationLocation::ProcGlobalIfNotEditing, source_buffer);

            if( keep_processing )
            {
                source_buffer = CompileCurrentBuffer(background_compiler);
                post_compile_compilation_unit_callback(CompilationLocation::CurrentBuffer, source_buffer);
            }
        }


        void ShowReferenceWindow(const std::vector<std::wstring>& selected_words, CString& reference_text, CString& logic_text,
                                 const std::optional<std::variant<const TextSource*, CompilationLocation>>& external_logic_text_source_or_compilation_location,
                                 const Application& application)
        {
            if( m_logicReferenceWindow == nullptr )
                return;

            m_applicationWindow->ShowControlBar(m_logicReferenceWindow, TRUE, FALSE);

            ReadOnlyEditCtrl* reference_control = m_logicReferenceWindow->GetEditCtrl();

            if( !reference_text.IsEmpty() )
            {
                // add the selected words as a title
                CString title = WS2CS(SO::CreateSingleString(selected_words, _T(".")));
                title.AppendFormat(_T("\n%s\n"), SO::GetRepeatingCharacterString(_T('‾'), title.GetLength()));

                // if declared in external code (not currently being edited), indicate where to find this declaration
                if( external_logic_text_source_or_compilation_location.has_value() &&
                    std::holds_alternative<const TextSource*>(*external_logic_text_source_or_compilation_location) )
                {
                    title.AppendFormat(_T("External Logic: %s\n"), PortableFunctions::PathGetFilename(
                        std::get<const TextSource*>(*external_logic_text_source_or_compilation_location)->GetFilename()));
                }

                reference_text.Insert(0, title);
            }

            // if the reference window appears for the first time without a valid word being displayed, show the default message
            else if( reference_control->GetText().empty() )
            {
                reference_text.Append(_T("Click on a word while holding the\n")
                                      _T("Control and Alt keys to obtain more information.\n"));
            }

            if( !reference_text.IsEmpty() )
            {
                ASSERT(reference_text[reference_text.GetLength() - 1] == '\n');

                if( !logic_text.IsEmpty() )
                {
                    reference_text.AppendChar('\n');

                    if( logic_text[logic_text.GetLength() - 1] != '\n' )
                        logic_text.AppendChar('\n');
                }

                reference_control->SetReadOnlyText(reference_text);

                if( logic_text.IsEmpty() )
                {
                    reference_control->ToggleLexer(SCLEX_NULL);
                }

                else
                {
                    reference_control->ToggleLexer(Lexers::GetLexer_Logic(application));

                    int length_before_logic_text = reference_control->GetLength();

                    reference_control->AppendReadOnlyText(CS2WS(logic_text));

                    reference_control->InitLogicControl(false, false);

                    reference_control->StartStyling(0, 0);
                    reference_control->SetStyling(length_before_logic_text, SCE_CSPRO_DEFAULT);
                }
            }
        }


    private:
        CMDIChildWnd* m_currentMdiWindow = nullptr;
        CDocument* m_activeDocument = nullptr;
        CAplDoc* m_applicationDocument = nullptr;
        COXMDIChildWndSizeDock* m_applicationWindow = nullptr;
        CLogicCtrl* m_logicControl = nullptr;
        LogicReferenceWnd* m_logicReferenceWindow = nullptr;
        CTreeCtrl* m_treeControl = nullptr;

        std::shared_ptr<std::function<void(const TextSource&, std::shared_ptr<const Logic::SourceBuffer>)>> m_externalLogicCompileNotificationCallback;
    };
}



LRESULT CMainFrame::OnLogicReference(WPARAM wParam, LPARAM lParam)
{
    CLogicCtrl* logic_control = reinterpret_cast<CLogicCtrl*>(lParam);
    bool activated_by_f1 = ( wParam == ZEDIT2O_LOGIC_REFERENCE_HELP );
    bool goto_word = ( wParam == ZEDIT2O_LOGIC_REFERENCE_GOTO );

    std::vector<std::wstring> selected_words = logic_control->ReturnWordsAtCursorWithDotNotation();

    // for help requests, first check the function table, or bypass all checks if a name
    // wasn't selected, so that the help can be launched quickly
    const TCHAR* help_topic_filename = nullptr;
    const Logic::FunctionDetails* function_details = nullptr;

    if( selected_words.size() == 1 )
    {
        help_topic_filename = Logic::ContextSensitiveHelp::GetTopicFilename(selected_words.back(), &function_details);
    }

    else if( selected_words.size() >= 2 )
    {
        // pass all but the last word as the dot notation entries
        help_topic_filename = Logic::ContextSensitiveHelp::GetTopicFilename(cs::span<const std::wstring>(selected_words.data(), selected_words.data() + selected_words.size() - 1),
                                                                            selected_words.back(), &function_details);
    }

    else if( activated_by_f1 && selected_words.empty() )
    {
        help_topic_filename = Logic::ContextSensitiveHelp::GetIntroductionTopicFilename();
    }

    if( help_topic_filename != nullptr && ( activated_by_f1 || goto_word ) )
    {
        HtmlHelp((DWORD_PTR)help_topic_filename, HH_DISPLAY_TOPIC);
        return 0;
    }


    LogicReferenceWorker logic_reference_worker(this);

    if( !logic_reference_worker.IsValid() )
        return 0;

    // generate the text for the reference window
    CString reference_text;
    CString logic_text;
    std::optional<std::variant<const TextSource*, LogicReferenceWorker::CompilationLocation>> external_logic_text_source_or_compilation_location;
    bool f1_help_processing_finished = false;
    std::optional<size_t> position_in_buffer;

    if( !selected_words.empty() )
    {
        auto background_compiler = logic_reference_worker.CreateCompiler();
        const Logic::SymbolTable& symbol_table = background_compiler->GetCompiledSymbolTable();
        std::shared_ptr<Logic::SourceBuffer> source_buffer;

        const std::vector<std::shared_ptr<CDEFormFile>>* form_files = logic_reference_worker.IsEntryApplication() ?
            &logic_reference_worker.GetApplication().GetRuntimeFormFiles() : nullptr;

        // a function to determine what the word is
        std::function<bool(size_t, Symbol*)> process_selected_words =
            [&](size_t selected_words_index, Symbol* parent_symbol) -> bool
            {
                const std::wstring& search_word = selected_words[selected_words_index];
                Symbol* symbol = nullptr;

                if( selected_words_index == 0 )
                {
                    std::vector<Symbol*> symbols = symbol_table.FindSymbols(search_word);

                    if( symbols.empty() )
                    {
                        // if the symbol was added in some scope, its name will no longer be accessible, so manually search for the name
                        if( external_logic_text_source_or_compilation_location.has_value() &&
                            std::holds_alternative<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) &&
                            std::get<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) == LogicReferenceWorker::CompilationLocation::CurrentBuffer )
                        {
                            for( size_t i = symbol_table.GetTableSize() - 1; i >= 1; i-- )
                            {
                                Symbol& possible_symbol = symbol_table.GetAt(i);

                                if( SO::EqualsNoCase(possible_symbol.GetName(), search_word) )
                                {
                                    symbols.emplace_back(&possible_symbol);
                                    break;
                                }
                            }
                        }

                        if( symbols.empty() )
                            return false;
                    }

                    if( selected_words.size() == 1 )
                    {
                        if( symbols.size() > 1 )
                        {
                            if( symbols.size() == 2 )
                            {
                                // the _FF has a flow and a group, so if there is a flow symbol, use it; additionally,
                                // external dictionary records have both a record and group, so use the record
                                auto symbol_search = std::find_if(symbols.begin(), symbols.end(),
                                    [](Symbol* searched_symbol)
                                    { return searched_symbol->IsOneOf(SymbolType::Pre80Flow, SymbolType::Section); });

                                // FLOW_TODO ... support the new flow

                                if( symbol_search != symbols.end() )
                                    symbol = *symbol_search;
                            }

                            if( symbol == nullptr )
                            {
                                reference_text.AppendFormat(_T("There are multiple symbols with the name %s.\n")
                                                            _T("Please provide additional qualifiers to avoid ambiguity.\n"), search_word.c_str());
                                return true;
                            }
                        }

                        symbol = symbols.front();
                    }

                    // if using dot notation, search under each possible symbol
                    else
                    {
                        for( Symbol* possible_symbol : symbols )
                        {
                            if( process_selected_words(selected_words_index + 1, possible_symbol) )
                                return true;
                        }

                        return false;
                    }
                }

                else
                {
                    ASSERT(parent_symbol != nullptr);
                    bool on_last_word = ( ( selected_words_index + 1 ) == selected_words.size() );

                    try
                    {
                        symbol = &symbol_table.FindSymbol(search_word, parent_symbol);

                        // if not on the last word, get the child symbol to this symbol
                        if( !on_last_word )
                            return process_selected_words(selected_words_index + 1, symbol);
                    }

                    catch( ... )
                    {
                        if( !on_last_word )
                            return false;
                    }

                    // if on the last word and this is not a symbol, check if this is a function
                    if( symbol == nullptr )
                    {
                        const Logic::FunctionDetails* function_details = nullptr;

                        if( Logic::FunctionTable::IsFunction(search_word, *parent_symbol, &function_details) )
                        {
                            if( activated_by_f1 || goto_word )
                            {
                                f1_help_processing_finished = true;
                                HtmlHelp((DWORD_PTR)function_details->help_filename, HH_DISPLAY_TOPIC);
                            }

                            else
                            {
                                AddEngineFunction(reference_text, *function_details);
                            }

                            return true;
                        }

                        else
                        {
                            return false;
                        }
                    }
                }

                ASSERT(symbol != nullptr);

                if( !goto_word )
                {
                    // if using an alias, show the base name
                    if( symbol->GetName() != selected_words.back() )
                        reference_text.AppendFormat(_T("Base Name: %s\n"), symbol->GetName().c_str());

                    // show any aliases to the symbol
                    std::vector<std::wstring> aliases = symbol_table.GetAliases(*symbol);

                    if( !aliases.empty() )
                        reference_text.AppendFormat(_T("Aliases: %s\n"), SO::CreateSingleString(aliases).c_str());


                    // add the symbol
                    if( symbol->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
                    {
                        AddDictionary(reference_text, symbol);
                    }

                    else if( symbol->IsA(SymbolType::Record) )
                    {
                        AddRecord(reference_text, assert_cast<const EngineRecord&>(*symbol).GetDictionaryRecord());
                    }

                    else if( symbol->IsA(SymbolType::Section) )
                    {
                        AddRecord(reference_text, *assert_cast<const SECT&>(*symbol).GetDictRecord());
                    }

                    else if( symbol->IsA(SymbolType::Variable) )
                    {
                        AddItem(reference_text, assert_cast<const VART*>(symbol), form_files, symbol_table);
                    }

                    else if( symbol->IsA(SymbolType::Item) )
                    {
                        AddItem(reference_text, &assert_cast<const EngineItem&>(*symbol).GetVarT(), form_files, symbol_table);
                    }

                    else if( symbol->IsA(SymbolType::ValueSet) )
                    {
                        AddValueSet(reference_text, assert_cast<const ValueSet&>(*symbol));
                    }

                    else if( symbol->IsA(SymbolType::Relation) )
                    {
                        AddRelation(reference_text, assert_cast<const RELT&>(*symbol), symbol_table);
                    }

                    else if( symbol->IsA(SymbolType::Pre80Flow) ) // FLOW_TODO ... support the new flow
                    {
                        AddFlow(reference_text, assert_cast<const FLOW&>(*symbol));
                    }

                    else if( symbol->IsA(SymbolType::Group) )
                    {
                        AddGroup(reference_text, logic_reference_worker.GetApplicationDocument()->GetAllDictsInApp(), form_files, assert_cast<const GROUPT&>(*symbol));
                    }

                    else if( symbol->IsA(SymbolType::Block) )
                    {
                        AddEngineBlock(reference_text, form_files, assert_cast<const EngineBlock&>(*symbol));
                    }

                    else if( symbol->IsA(SymbolType::Array) )
                    {
                        AddArray(reference_text, assert_cast<const LogicArray&>(*symbol), symbol_table);
                    }

                    else if( symbol->IsA(SymbolType::HashMap) )
                    {
                        AddHashMap(reference_text, assert_cast<const LogicHashMap&>(*symbol));
                    }

                    else if( symbol->IsA(SymbolType::NamedFrequency) )
                    {
                        AddNamedFrequency(reference_text, assert_cast<const NamedFrequency&>(*symbol), background_compiler->GetEngineArea());
                    }

                    else if( symbol->IsA(SymbolType::Report) )
                    {
                        AddReport(reference_text, logic_reference_worker.GetApplication(), assert_cast<const Report&>(*symbol));
                    }

                    else if( symbol->IsA(SymbolType::UserFunction) )
                    {
                        AddUserFunction(reference_text, assert_cast<const UserFunction&>(*symbol), symbol_table);
                    }

                    else if( symbol->IsOneOf(SymbolType::Audio, SymbolType::Document, SymbolType::File,
                                             SymbolType::Geometry, SymbolType::Image, SymbolType::List,
                                             SymbolType::Map, SymbolType::Pff, SymbolType::SystemApp,
                                             SymbolType::WorkString, SymbolType::WorkVariable) )
                    {
                        AddBasicSymbol(reference_text, *symbol);
                    }
                }

                bool search_for_symbol_declaration =
                    symbol->IsOneOf(SymbolType::Array, SymbolType::Audio, SymbolType::Document, SymbolType::File,
                                    SymbolType::Geometry, SymbolType::HashMap, SymbolType::Image,
                                    SymbolType::List, SymbolType::Map, SymbolType::NamedFrequency,
                                    SymbolType::Pff, SymbolType::Relation, SymbolType::SystemApp,
                                    SymbolType::UserFunction, SymbolType::WorkString, SymbolType::WorkVariable) ||
                    ( symbol->IsA(SymbolType::Dictionary) && !assert_cast<const EngineDictionary*>(symbol)->IsDictionaryObject() ) ||
                    ( symbol->IsA(SymbolType::ValueSet) && assert_cast<const ValueSet*>(symbol)->IsDynamic() );

                if( search_for_symbol_declaration && source_buffer != nullptr )
                {
                    if( goto_word )
                    {
                        position_in_buffer = FindSymbolPositionInBuffer(symbol, *source_buffer);

                        if( position_in_buffer.has_value() &&
                            external_logic_text_source_or_compilation_location.has_value() &&
                            std::holds_alternative<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) &&
                            std::get<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) == LogicReferenceWorker::CompilationLocation::ProcGlobalIfNotEditing )
                        {
                            // when we go to the position in the PROC GLOBAL buffer, we have to account for the fact that
                            // \n characters will be translated into \r\n characters
                            CString buffer_text = CString(source_buffer->GetTokens().front().token_text, *position_in_buffer);
                            buffer_text.Replace(_T("\n"), _T("\r\n"));
                            position_in_buffer = buffer_text.GetLength();
                        }

                        return true;
                    }

                    else
                    {
                        // because the buffer is only processed up to the semicolon, the user function may not be fully
                        // in the buffer, so get the whole buffer
                        if( symbol->IsA(SymbolType::UserFunction) &&
                            external_logic_text_source_or_compilation_location.has_value() &&
                            std::holds_alternative<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) &&
                            std::get<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) == LogicReferenceWorker::CompilationLocation::CurrentBuffer )
                        {
                            source_buffer = std::make_shared<Logic::SourceBuffer>(logic_control->GetText());
                            source_buffer->Tokenize(logic_reference_worker.GetApplication().GetLogicSettings());
                        }

                        GetSymbolDeclarationLogic(logic_text, symbol, *source_buffer);
                    }
                }

                return !reference_text.IsEmpty();
            };


        // prior to compiling any logic, check if the word is a function
        if( function_details != nullptr )
        {
            AddEngineFunction(reference_text, *function_details);
        }

        else if( logic_reference_worker.IsTabulationApplication() )
        {
            // TODO ... the background compiler doesn't work properly with tables because
            // when tables are actually compiled, a batch application is emulated;
            // simply doing SetEngineAppType(EngineAppType::Batch) doesn't work because a
            // flow doesn't get properly setup, so for now this is disabled
        }

        // or if the word is already in the symbol table
        else if( !process_selected_words(0, nullptr) )
        {
            // if not, compile the code to fully build the symbol table
            bool keep_processing = true;

            logic_reference_worker.CompileToCurrentLocation(*background_compiler,
                [&](std::variant<const TextSource*, LogicReferenceWorker::CompilationLocation> this_external_logic_text_source_or_compilation_location,
                    std::shared_ptr<const Logic::SourceBuffer> post_compile_source_buffer)
                {
                    if( keep_processing )
                    {
                        external_logic_text_source_or_compilation_location = this_external_logic_text_source_or_compilation_location;
                        source_buffer = std::const_pointer_cast<Logic::SourceBuffer>(post_compile_source_buffer);
                        keep_processing = !process_selected_words(0, nullptr);
                    }

                    return keep_processing;
                });

            // if still not found, check for external dictionary levels, which are not added to the
            // symbol table unless they are on an external form
            if( keep_processing && !goto_word && selected_words.size() == 1 )
            {
                const std::vector<const CDataDict*>& dictionaries = logic_reference_worker.GetApplicationDocument()->GetAllDictsInApp();
                const CDataDict* dictionary = nullptr;
                const DictLevel* dict_level = nullptr;

                if( LevelLocator(dictionaries, WS2CS(selected_words.front()), &dictionary, &dict_level) )
                    AddLevel(reference_text, *dict_level, dictionary, false);
            }
        }
    }

    if( f1_help_processing_finished )
        return 0;

    if( goto_word )
    {
        // go to the position in..
        if( position_in_buffer.has_value() )
        {
            ASSERT(external_logic_text_source_or_compilation_location.has_value());

            // ...the current buffer
            if( std::holds_alternative<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) &&
                std::get<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) == LogicReferenceWorker::CompilationLocation::CurrentBuffer )
            {
                logic_control->GotoPos((int)*position_in_buffer);
            }

            // ...PROC GLOBAL
            else if( std::holds_alternative<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) )
            {
                ASSERT(std::get<LogicReferenceWorker::CompilationLocation>(*external_logic_text_source_or_compilation_location) == LogicReferenceWorker::CompilationLocation::ProcGlobalIfNotEditing);
                OnViewLogic((int)*position_in_buffer);
            }

            // ...external code
            else
            {
                ASSERT(std::holds_alternative<const TextSource*>(*external_logic_text_source_or_compilation_location));
                GotoExternalLogicOrReportNode(logic_reference_worker.GetActiveDocument(), logic_control,
                    std::get<const TextSource*>(*external_logic_text_source_or_compilation_location)->GetFilename(),
                    false, (int)*position_in_buffer);
            }
        }

        return 0;
    }

    // if there is no reference text for a help request, launch the introductory logic topic (or ignore the goto)
    if( reference_text.IsEmpty() )
    {
        if( activated_by_f1 )
            HtmlHelp((DWORD_PTR)Logic::ContextSensitiveHelp::GetIntroductionTopicFilename(), HH_DISPLAY_TOPIC);
    }

    // otherwise set up the reference window
    else
    {
        logic_reference_worker.ShowReferenceWindow(selected_words, reference_text, logic_text,
            external_logic_text_source_or_compilation_location, logic_reference_worker.GetApplication());
    }

    return 0;
}


LRESULT CMainFrame::OnSymbolsAdded(WPARAM wParam, LPARAM lParam)
{
    bool reset_symbols = ( wParam == 1 );
    const Logic::SymbolTable& symbol_table = *reinterpret_cast<const Logic::SymbolTable*>(lParam);

    LogicReferenceAutoCompleter.UpdateWithCompiledSymbols(symbol_table, reset_symbols);

    return 0;
}


LRESULT CMainFrame::OnLogicAutoComplete(WPARAM /*wParam*/, LPARAM lParam)
{
    CLogicCtrl* logic_control = reinterpret_cast<CLogicCtrl*>(lParam);

    // compile up to the current buffer to look for any new symbols
    LogicReferenceWorker logic_reference_worker(this);

    if( !logic_reference_worker.IsValid() )
        return 0;

    logic_reference_worker.CompileToCurrentLocation();

    // determine what words to work with
    std::vector<std::wstring> selected_words = logic_control->ReturnWordsAtCursorWithDotNotation();

    // allow auto complete to work when a symbol_name/namespace. is entered
    if( selected_words.empty() )
    {
        int buffer_position_of_previous_word = logic_control->GetCurrentPos() - 2;

        if( buffer_position_of_previous_word >= 0 && logic_control->GetCharAt(buffer_position_of_previous_word + 1) == '.' )
            selected_words = logic_control->ReturnWordsAtCursorWithDotNotation(buffer_position_of_previous_word);

        if( selected_words.empty() )
            return 0;

        // add a blank entry to signify that nothing has been added after symbol_name/namespace.
        selected_words.emplace_back();
    }

    ASSERT(!selected_words.empty());
    const std::wstring& word_to_complete = selected_words.back();

    std::wstring suggested_words;
    bool word_comes_from_fuzzy_matching;

    if( selected_words.size() == 1 )
    {
        std::tie(suggested_words, word_comes_from_fuzzy_matching) = LogicReferenceAutoCompleter.GetSuggestedWordString(word_to_complete);
    }

    else
    {
        // pass all but the last word as the dot notation entries
        suggested_words = LogicReferenceAutoCompleter.GetSuggestedWordString(cs::span<const std::wstring>(selected_words.data(), selected_words.data() + selected_words.size() - 1),
                                                                             word_to_complete);
        word_comes_from_fuzzy_matching = false;
    }

    // on a fuzzy match, replace the entire word (because the beginning of the word may not match the beginning of the suggested word)
    if( word_comes_from_fuzzy_matching )
    {
        Sci_Position current_pos = logic_control->GetCurrentPos();
        Sci_Position start_pos = logic_control->WordStartPosition(current_pos, TRUE);

        logic_control->SetTargetRange(start_pos, logic_control->WordEndPosition(current_pos, TRUE));
        logic_control->ReplaceTarget(suggested_words.length(), suggested_words.c_str());

        logic_control->GotoPos(start_pos + suggested_words.length());
    }

    // otherwise show the Scintilla autocomplete; the second condition prevents suggestions for when the word is complete
    // (which led to either a Scintilla crash or weird insertion behavior)
    else if( !suggested_words.empty() && !SO::EqualsNoCase(suggested_words, word_to_complete) )
    {
        logic_control->AutoCSetChooseSingle(TRUE);
        logic_control->AutoCSetIgnoreCase(TRUE);
        logic_control->AutoCShow(word_to_complete.length(), suggested_words.c_str());
    }

    return 0;
}


LRESULT CMainFrame::OnLogicInsertProcName(WPARAM /*wParam*/, LPARAM lParam)
{
    CLogicCtrl* logic_control = reinterpret_cast<CLogicCtrl*>(lParam);

    LogicReferenceWorker logic_reference_worker(this);

    // this will not work for tabulation applications
    if( !logic_reference_worker.IsValid() || logic_reference_worker.IsTabulationApplication() )
        return 0;


    CString proc_or_function_name;

    // if on a PROC, insert the name of the currently selected node
    if( logic_reference_worker.GetSelectedNode() == LogicReferenceWorker::SelectedNode::Proc )
    {
        proc_or_function_name = logic_reference_worker.GetSelectedNodeName();
    }

    // if on a report, insert $ because best practice would be not to use report names in the report's logic
    else if( logic_reference_worker.GetSelectedNode() == LogicReferenceWorker::SelectedNode::Report )
    {
        proc_or_function_name = _T("$");
    }

    // otherwise (if on PROC GLOBAL or external code), get the logic buffer and search backwards for the last PROC or function name
    else
    {
        CString buffer_text_up_to_cursor = WS2CS(logic_control->GetText()).Left(logic_control->GetCurrentPos());
        logic_reference_worker.PrepareCurrentBufferText(buffer_text_up_to_cursor);

        std::shared_ptr<Logic::SourceBuffer> source_buffer = std::make_shared<Logic::SourceBuffer>(CS2WS(buffer_text_up_to_cursor));

        auto background_compiler = logic_reference_worker.CreateCompiler();
        background_compiler->Compile(source_buffer);
        const std::vector<Logic::BasicToken>& basic_tokens = source_buffer->GetTokens();

        for( size_t i = basic_tokens.size() - 1; proc_or_function_name.IsEmpty() && i < basic_tokens.size(); --i )
        {
            if( basic_tokens[i].type == Logic::BasicToken::Type::Text )
            {
                // if in a function, return the last function added to the symbol table
                if( SO::EqualsNoCase(basic_tokens[i].GetTextSV(), _T("function")) )
                {
                    const Logic::SymbolTable& symbol_table = background_compiler->GetCompiledSymbolTable();

                    for( size_t symbol_itr = symbol_table.GetTableSize() - 1; symbol_itr >= 1; --symbol_itr )
                    {
                        const Symbol& symbol = symbol_table.GetAt(symbol_itr);

                        if( symbol.IsA(SymbolType::UserFunction) )
                        {
                            proc_or_function_name = WS2CS(symbol.GetName());
                            break;
                        }
                    }
                }

                // if in a proc, return its name (with dot notation)
                else if( SO::EqualsNoCase(basic_tokens[i].GetTextSV(), _T("PROC")) )
                {
                    bool last_token_was_dot = false;

                    for( size_t j = i + 1; j < basic_tokens.size(); ++j )
                    {
                        bool append_this_token = ( proc_or_function_name.IsEmpty() || last_token_was_dot );
                        last_token_was_dot = ( basic_tokens[j].type == Logic::BasicToken::Type::Operator && basic_tokens[j].token_code == TokenCode::TOKPERIOD );
                        append_this_token |= last_token_was_dot;

                        if( append_this_token )
                        {
                            proc_or_function_name.Append(WS2CS(basic_tokens[j].GetText()));
                        }

                        else
                        {
                            break;
                        }
                    }

                    // don't ever display PROC GLOBAL
                    if( proc_or_function_name.CompareNoCase(_T("GLOBAL")) == 0 )
                        proc_or_function_name.Empty();
                }
            }
        }
    }


    if( !proc_or_function_name.IsEmpty() )
        logic_control->AddText(proc_or_function_name);

    return 0;
}


void CMainFrame::OnViewLogic(int position_in_buffer)
{
    CMDIChildWnd* pWnd = MDIGetActive();
    CDocument* pDoc = pWnd->GetActiveDocument();

    CTreeCtrl* pTreeCtrl = nullptr;
    CLogicView* pLogicView = nullptr;

    // TODO ... process numbers?

    if( pDoc->IsKindOf(RUNTIME_CLASS(FormFileBasedDoc)) )
    {
        pTreeCtrl = assert_cast<FormFileBasedDoc*>(pDoc)->GetTreeCtrl();
        pLogicView = assert_cast<ApplicationChildWnd*>(pWnd)->GetSourceLogicView();
    }

    // select the root node
    if( pTreeCtrl != nullptr )
    {
        if( pTreeCtrl->GetSelectedItem() != pTreeCtrl->GetRootItem() )
            pTreeCtrl->SelectItem(pTreeCtrl->GetRootItem());
    }

    // view logic and...
    pWnd->SendMessage(UWM::Designer::SwitchView, static_cast<WPARAM>(ViewType::Logic));

    // ...go to a certain position of the buffer
    if( pLogicView != nullptr )
    {
        CLogicCtrl* pLogicCtrl = pLogicView->GetLogicCtrl();
        pLogicCtrl->GotoPos(position_in_buffer);
        pLogicCtrl->SetFocus();
    }
}


void CMainFrame::OnViewTopLogic()
{
    OnViewLogic(0);
}
