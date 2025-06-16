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


    void AddTabbedText(CString& reference_text, const NullTerminatedString text_to_add, int tabs, bool add_arrow)
    {
        int spaces = tabs * 4;

        if( add_arrow )
        {
            ASSERT(tabs > 0);
            spaces -= 2;
        }

        reference_text.AppendFormat(L"%s%s%s\n", UTF8_TODO::GetWide(SO::GetRepeatingCharacterString(' ', spaces)).c_str(), add_arrow ? L"` " : L"", text_to_add);
    }

    void AddCommaSeparatedList(CString& reference_text, const std::vector<CString>& list)
    {
        for( size_t i = 0; i < list.size(); ++i )
            reference_text.AppendFormat(L"%s%s", ( i == 0 ) ? L"" : L", ", list[i].GetString());

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

        constexpr const wchar_t* ActionInvokerTitle = L"Action Invoker Details";
        constexpr size_t ActionInvokerTitleLength = wstring_view(ActionInvokerTitle).length();
        reference_text.AppendFormat(L"\n%s\n%s\n", ActionInvokerTitle, UTF8_TODO::GetWide(SO::GetRepeatingCharacterString(L'‾', ActionInvokerTitleLength)).c_str());

        if( !function_definition->description.empty() )
            reference_text.AppendFormat(L"Description: %s\n", UTF8_TODO::GetWide(function_definition->description).c_str());

        if( !function_definition->parameters.empty() )
        {
            reference_text.Append(L"\nInput Parameters:\n");

            for( const GF::Parameter& parameter : function_definition->parameters )
            {
                reference_text.Append(L"\n");
                AddTabbedText(reference_text, FormatText(L"Name: %s", UTF8_TODO::GetWide(parameter.variable.name).c_str()), 1, true);

                if( !parameter.variable.description.empty() )
                    AddTabbedText(reference_text, FormatText(L"Description: %s", UTF8_TODO::GetWide(parameter.variable.description).c_str()), 1, false);

                AddTabbedText(reference_text, FormatText(L"Required: %s", parameter.required ? L"Yes" : L"No"), 1, false);
            }
        }

        if( !function_definition->returns.empty() )
        {
            reference_text.Append(L"\nReturns:");

            if( function_definition->returns.size() == 1 && function_definition->returns.front().name.empty() )
            {
                reference_text.AppendFormat(L" %s\n", UTF8_TODO::GetWide(function_definition->returns.front().description).c_str());
            }

            else
            {
                reference_text.AppendChar('\n');

                for( const GF::Variable& variable : function_definition->returns )
                {
                    ASSERT(!variable.name.empty() || !variable.description.empty());

                    reference_text.Append(L"\n");

                    if( !variable.name.empty() )
                        AddTabbedText(reference_text, FormatText(L"Name: %s", UTF8_TODO::GetWide(variable.name).c_str()), 1, true);

                    if( !variable.description.empty() )
                        AddTabbedText(reference_text, FormatText(L"Description: %s", UTF8_TODO::GetWide(variable.description).c_str()), 1, false);
                }
            }
        }
    }

    void AddEngineFunction(CString& reference_text, const Logic::FunctionDetails& function_details)
    {
        reference_text.AppendFormat(L"%s\n", UTF8_TODO::GetWide(function_details.tooltip).c_str());

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
                if( SO::EqualsNoCase(dict_level.GetName(), level_name) )
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

        const std::wstring formatter = FormatText(L"%%-%ds ｜ %%%ds%s%%%ds%s%%s\n",
                                                  max_label_length, dict_item.GetCompleteLen(),
                                                  has_to ? L" ｜ " : L"", has_to ? dict_item.GetCompleteLen() : 0,
                                                  has_special ? L" ｜ " : L"");

        for( const DictValue& dict_value : dict_value_set.GetValues() )
        {
            bool first_pair = true;

            for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
            {
                reference_text.AppendFormat(formatter.c_str(),
                                            first_pair ? dict_value.GetLabel().GetString() : L"",
                                            dict_value_pair.GetFrom().GetString(),
                                            dict_value_pair.GetTo().GetString(),
                                            dict_value.IsSpecial() ? UTF8_TODO::GetWide(SpecialValues::ValueToString(dict_value.GetSpecialValue(), false)).c_str() : L"");
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
            return UTF8_TODO::GetCString(dictionary.MakeQualifiedName(record->GetName()));
        }

        else
        {
            return UTF8_TODO::GetCString(record->GetName());
        }
    }

    void AddHierarchy(CString& reference_text, const std::vector<const DictBase*>& hierarchy, size_t levels_to_display = SIZE_MAX)
    {
        levels_to_display = std::min(hierarchy.size(), levels_to_display);

        if( levels_to_display >= 1 )
        {
            const CDataDict* dictionary = assert_cast<const CDataDict*>(hierarchy[0]);
            AddTabbedText(reference_text, UTF8_TODO::GetCString(dictionary->GetName()), 0, false);

            if( levels_to_display >= 2 )
            {
                const DictLevel& dict_level = assert_cast<const DictLevel&>(*hierarchy[1]);
                AddTabbedText(reference_text, UTF8_TODO::GetCString(dict_level.GetName()), 1, true);

                if( levels_to_display >= 3 )
                {
                    const CDictRecord* record = assert_cast<const CDictRecord*>(hierarchy[2]);
                    AddTabbedText(reference_text, GetRecordName(*dictionary, dict_level, record), 2, true);

                    if( levels_to_display >= 4 )
                    {
                        AddTabbedText(reference_text, UTF8_TODO::GetCString(assert_cast<const CDictItem*>(hierarchy[3])->GetName()), 3, true);

                        if( levels_to_display >= 5 )
                            AddTabbedText(reference_text, UTF8_TODO::GetCString(assert_cast<const DictValueSet&>(*hierarchy[4]).GetName()), 4, true);
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
                reference_text.Append(L"IDs: ");
            }

            else
            {
                reference_text.AppendFormat(L"Level %s's IDs: ", UTF8_TODO::GetWide(dict_level.GetName()).c_str());
            }

            const CDictRecord* record = dict_level.GetIdItemsRec();
            std::vector<CString> id_names;

            for( int i = 0; i < record->GetNumItems(); ++i )
                id_names.emplace_back(UTF8_TODO::GetCString(record->GetItem(i)->GetName()));

            AddCommaSeparatedList(reference_text, id_names);
        }

        reference_text.AppendChar('\n');

        if( display_dictionary_hierarchy )
            reference_text.Append(L"Dictionary Hierarchy:\n\n");

        // add the dictionary tree with the levels and records expanded
        AddTabbedText(reference_text, UTF8_TODO::GetCString(dictionary->GetName()), 0, false);

        for( const DictLevel& dict_level : dictionary->GetLevels() )
        {
            if( desired_level != nullptr && &dict_level != desired_level )
                continue;

            AddTabbedText(reference_text, UTF8_TODO::GetCString(dict_level.GetName()), 1, true);

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

            reference_text.Append(L"Variable Type: ");

            if( engine_dictionary->IsDictionaryObject() )
            {
                reference_text.AppendFormat(L"Dictionary (%s)\n", UTF8_TODO::GetWide(ToString(engine_dictionary->GetSubType())).c_str());
            }

            else if( engine_dictionary->IsCaseObject() )
            {
                reference_text.AppendFormat(L"Case (%s)\n", UTF8_TODO::GetWide(dictionary->GetName()).c_str());
            }

            else
            {
                reference_text.AppendFormat(L"DataSource (%s)\n", UTF8_TODO::GetWide(dictionary->GetName()).c_str());
            }
        }

        else
        {
            const DICT* pDicT = assert_cast<const DICT*>(symbol);
            dictionary = pDicT->GetDataDict();
            reference_text.Append(L"Variable Type: Dictionary\n");
        }

        reference_text.AppendFormat(L"Label: %s\n", dictionary->GetLabel().GetString());

        AddDictionaryLevelHierarchy(reference_text, dictionary, nullptr, false);
    }

    void AddLevel(CString& reference_text, const DictLevel& dict_level, const CDataDict* dictionary, bool display_dictionary_hierarchy)
    {
        reference_text.Append(L"Variable Type: Level\n");
        reference_text.AppendFormat(L"Label: %s\n", dict_level.GetLabel().GetString());
        AddDictionaryLevelHierarchy(reference_text, dictionary, &dict_level, display_dictionary_hierarchy);
    }

    void AddRecord(CString& reference_text, const CDictRecord& record)
    {
        reference_text.Append(L"Variable Type: Record\n");
        reference_text.AppendFormat(L"Label: %s\n", record.GetLabel().GetString());
        reference_text.AppendFormat(L"Occurrences: %d (%s)\n\n", record.GetMaxRecs(), record.GetRequired() ? L"Required" : L"Not Required");

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

                    AddTabbedText(reference_text, UTF8_TODO::GetCString(dict_item->GetName()), ( dict_item->GetItemType() == ItemType::Item ) ? 3 : 4, add_arrow);
                }
            };

        DictionaryLocator(record.GetDataDict(), &record, dictionary_locator_callback);
    }

    void AddItem(CString& reference_text, const VART* variable, const std::vector<std::shared_ptr<CDEFormFile>>* form_files, const Logic::SymbolTable& symbol_table)
    {
        const CDictItem& dict_item = *variable->GetDictItem();

        reference_text.Append(L"Variable Type: Item\n");
        reference_text.AppendFormat(L"Label: %s\n", dict_item.GetLabel().GetString());
        reference_text.AppendFormat(L"Data Type: %s\n", UTF8_TODO::GetWide(ToString(dict_item.GetContentType())).c_str());

        // length
        reference_text.AppendFormat(L"Length: %d", dict_item.GetLen());

        if( dict_item.GetContentType() == ContentType::Numeric )
        {
            reference_text.AppendFormat(L" (%s", CString('X', dict_item.GetIntegerLen()).GetString());

            if( dict_item.GetDecimal() > 0 )
                reference_text.AppendFormat(L".%s", CString('x', dict_item.GetDecimal()).GetString());

            reference_text.AppendChar(')');
        }

        reference_text.AppendChar('\n');

        // item type
        reference_text.AppendFormat(L"Item Type: %s\n", ( dict_item.GetItemType() == ItemType::Item ) ? L"Item" : L"Subitem");

        const CDictItem* parent_dict_item = nullptr;

        if( dict_item.GetItemType() == ItemType::Subitem )
        {
            parent_dict_item = dict_item.GetParentItem();
            reference_text.AppendFormat(L"Parent Item: %s\n", UTF8_TODO::GetWide(parent_dict_item->GetName()).c_str());
        }

        // check it there are any subitems
        else
        {
            std::vector<CString> subitem_names;

            for( const VART* next_variable = variable;
                ( next_variable = assert_nullable_cast<const VART*>(next_variable->next_symbol) ) != nullptr && next_variable->GetOwnerVarT() == variable; )
            {
                subitem_names.emplace_back(UTF8_TODO::GetCString(next_variable->GetName()));
            }

            if( !subitem_names.empty() )
            {
                reference_text.Append(L"Child Items: ");
                AddCommaSeparatedList(reference_text, subitem_names);
            }
        }

        // occurrences
        const CDictRecord* record = variable->GetSPT()->GetDictRecord();
        reference_text.AppendFormat(L"Record Occurrences: %d (%s)\n", record->GetMaxRecs(), record->GetRequired() ? L"Required" : L"Not Required");

        reference_text.AppendFormat(L"Item Occurrences: %d\n",
            (( parent_dict_item != nullptr ) ? parent_dict_item : &dict_item)->GetOccurs());

        if( parent_dict_item != nullptr )
            reference_text.AppendFormat(L"Subitem Occurrences: %d\n", dict_item.GetOccurs());

        // dictionary hierarchy
        reference_text.Append(L"\nDictionary Hierarchy:\n\n");

        std::function<void(const std::vector<const DictBase*>&)> dictionary_locator_callback =
            [&](const std::vector<const DictBase*>& hierarchy)
            {
                AddHierarchy(reference_text, hierarchy, hierarchy.size() - 1);

                int tabs = 3;

                if( parent_dict_item != nullptr )
                    AddTabbedText(reference_text, UTF8_TODO::GetCString(parent_dict_item->GetName()), tabs++, true);

                AddTabbedText(reference_text, UTF8_TODO::GetCString(dict_item.GetName()), tabs++, true);

                int v = 0;
                for( const DictValueSet& dict_value_set : dict_item.GetValueSets() )
                    AddTabbedText(reference_text, UTF8_TODO::GetCString(dict_value_set.GetName()), tabs, ( v++ == 0 ));
            };

        DictionaryLocator(variable->GetDataDict(), &dict_item, dictionary_locator_callback);

        if( form_files != nullptr )
        {
            if( variable->GetFormSymbol() == 0 )
            {
                reference_text.AppendFormat(L"\nNot Located on a Form\n");
            }

            else
            {
                // form hierarchy
                reference_text.Append(L"\nForm Hierarchy:\n\n");

                std::function<void(const std::vector<const CDEFormBase*>&)> form_file_locator_callback =
                    [&](const std::vector<const CDEFormBase*>& hierarchy)
                    {
                        AddFormFileHierarchy(reference_text, hierarchy);
                    };

                FormFileLocator(form_files, UTF8_TODO::GetCString(variable->GetName()), form_file_locator_callback);

                // field capture type (ideally we could call VART::GetEvaluatedCaptureInfo
                // but it's not accessible so the code is somewhat duplicated here
                CaptureInfo capture_info = variable->GetCaptureInfo().MakeValid(dict_item,
                    dict_item.GetFirstValueSetOrNull());

                reference_text.AppendFormat(L"\nField Capture Type: %s\n", UTF8_TODO::GetWide(capture_info.GetDescription()).c_str());
            }
        }

        // first value set
        if( dict_item.HasValueSets() )
        {
            reference_text.Append(L"\nFirst Value Set:\n\n");
            AddValueSetValues(reference_text, dict_item, dict_item.GetValueSet(0));
        }
    }

    void AddValueSet(CString& reference_text, const ValueSet& value_set)
    {
        reference_text.Append(L"Variable Type: ValueSet\n");
        reference_text.AppendFormat(L"Label: %s\n", value_set.GetLabel().GetString());
        reference_text.AppendFormat(L"Data Type: %s\n", value_set.IsNumeric() ? L"Numeric" : L"String");

        if( value_set.IsDynamic() )
            return;

        const DictValueSet& dict_value_set = value_set.GetDictValueSet();
        const VART* variable = value_set.GetVarT();
        const CDictItem& dict_item = *variable->GetDictItem();

        if( dict_item.GetNumValueSets() > 1 )
        {
            reference_text.AppendFormat(L"Other Value Sets of %s: ", UTF8_TODO::GetWide(dict_item.GetName()).c_str());

            bool add_comma = false;

            for( const DictValueSet& this_dict_value_set : dict_item.GetValueSets() )
            {
                if( &this_dict_value_set != &dict_value_set )
                {
                    reference_text.AppendFormat(L"%s%s", add_comma ? L", " : L"", UTF8_TODO::GetWide(this_dict_value_set.GetName()).c_str());
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
        reference_text.Append(L"Variable Type: Relation\n");
        reference_text.AppendFormat(L"Base Object: %s\n", UTF8_TODO::GetWide(symbol_table.GetAt(relation.GetBaseObjIndex()).GetName()).c_str());

        for( size_t i = 1; i < relation.m_aTarget.size(); ++i )
        {
            const int relation_type = relation.m_aTarget[i].iTargetRelationType;
            const wchar_t* const relation_type_string =
                ( relation_type == USE_INDEX_RELATION )        ? L"Parallel" :
                ( relation_type == USE_LINK_RELATION )         ? L"Linked" :
                ( relation_type == USE_WHERE_RELATION_SINGLE ) ? L"Where (single)" :
                ( relation_type == USE_WHERE_RELATION_SINGLE ) ? L"Where (multiple)" : L"";

            reference_text.AppendFormat(L"Linkage to %s: %s\n", UTF8_TODO::GetWide(symbol_table.GetAt(relation.m_aTarget[i].iTargetSymbolIndex).GetName()).c_str(), relation_type_string);
        }
    }

    void AddFlow(CString& reference_text, const FLOW& flow)
    {
        const CDEFormFile* form_file = flow.GetFormFile();

        reference_text.Append(L"Variable Type: Form File\n");
        reference_text.AppendFormat(L"Label: %s\n\n", form_file->GetLabel().GetString());

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

            if( LevelLocator(dictionaries, UTF8_TODO::GetCString(group.GetName()), &dictionary, &dict_level) )
                AddLevel(reference_text, *dict_level, dictionary, true);
        }

        else
        {
            reference_text.AppendFormat(L"Variable Type: %s\n",
                                        form_group->isA(CDEFormBase::eItemType::Roster) ? L"Roster" : L"Group");
            reference_text.AppendFormat(L"Label: %s\n", form_group->GetLabel().GetString());
            reference_text.AppendFormat(L"Occurrences: %d\n", group.GetMaxOccs());
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
            reference_text.Append(L"\nForm Hierarchy:\n\n");
            FormFileLocator(form_files, form_group->GetName(), form_file_locator_callback);
        }
    }

    void AddEngineBlock(CString& reference_text, const std::vector<std::shared_ptr<CDEFormFile>>* form_files, const EngineBlock& engine_block)
    {
        const CDEBlock& form_block = engine_block.GetFormBlock();

        reference_text.Append(L"Variable Type: Block\n");
        reference_text.AppendFormat(L"Label: %s\n", form_block.GetLabel().GetString());
        reference_text.AppendFormat(L"Display on Same Screen: %s\n\n", form_block.GetDisplayTogether() ? L"Yes" : L"No");

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
            FormFileLocator(form_files, UTF8_TODO::GetCString(engine_block.GetName()), form_file_locator_callback);
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
                reference_text.Append(L"Variable Type: Audio\n");
                break;
            }

            case SymbolType::Document:
            {
                reference_text.Append(L"Variable Type: Document\n");
                break;
            }

            case SymbolType::File:
            {
                reference_text.Append(L"Variable Type: File\n");
                break;
            }

            case SymbolType::Geometry:
            {
                reference_text.Append(L"Variable Type: Geometry\n");
                break;
            }

            case SymbolType::Image:
            {
                reference_text.Append(L"Variable Type: Image\n");
                break;
            }

            case SymbolType::List:
            {
                const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
                reference_text.AppendFormat(L"Variable Type: List\nData Type: %s\n", logic_list.IsNumeric() ? L"Numeric" : L"String");
                break;
            }

            case SymbolType::Map:
            {
                reference_text.Append(L"Variable Type: Map\n");
                break;
            }

            case SymbolType::Pff:
            {
                reference_text.Append(L"Variable Type: Pff\n");
                break;
            }

            case SymbolType::SystemApp:
            {
                reference_text.Append(L"Variable Type: SystemApp\n");
                break;
            }

            case SymbolType::WorkString:
            {
                if( symbol.GetSubType() == SymbolSubType::WorkAlpha )
                {
                    reference_text.AppendFormat(L"Variable Type: Alpha\nLength: %d\n", static_cast<int>(assert_cast<const WorkAlpha&>(symbol).GetWideLength()));
                }

                else
                {
                    reference_text.Append(L"Variable Type: String\nLength: Unlimited\n");
                }

                break;
            }

            case SymbolType::WorkVariable:
            {
                reference_text.Append(L"Variable Type: Numeric\n");
                break;
            }
        }
    }

    void AddArray(CString& reference_text, const LogicArray& logic_array, const Logic::SymbolTable& symbol_table)
    {
        reference_text.Append(L"Variable Type: Array\n");

        reference_text.AppendFormat(L"Data Type: %s", logic_array.IsNumeric() ? L"Numeric" : L"");

        if( logic_array.IsString() )
        {
            if( logic_array.GetPaddingStringLength() == 0 )
            {
                reference_text.Append(L"String\nCell Length: Unlimited");
            }

            else
            {
                reference_text.AppendFormat(L"Alpha\nCell Length: %d", logic_array.GetPaddingStringLength());
            }
        }

        reference_text.AppendChar('\n');

        reference_text.AppendFormat(L"Dimensions: %d\n", (int)logic_array.GetNumberDimensions());

        reference_text.Append(L"Dimension Sizes: ");

        for( size_t i = 0; i < logic_array.GetNumberDimensions(); ++i )
        {
            if( i > 0 )
                reference_text.Append(L", ");

            int deckarray_symbol = logic_array.GetDeckArraySymbols()[i];

            if( deckarray_symbol != 0 )
            {
                reference_text.Append(UTF8_TODO::GetCString(symbol_table.GetAt(abs(deckarray_symbol)).GetName()));

                if( deckarray_symbol < 0 )
                    reference_text.Append(L"(+)");

                reference_text.Append(L" [");
            }

            reference_text.AppendFormat(L"%d", (int)logic_array.GetDimension(i) - 1);

            if( deckarray_symbol != 0 )
                reference_text.Append(L"]");
        }

        reference_text.AppendChar('\n');

        reference_text.AppendFormat(L"Save Array: %s\n", logic_array.GetUsingSaveArray() ? L"Yes" : L"No");
    }

    void AddHashMap(CString& reference_text, const LogicHashMap& hashmap)
    {
        reference_text.Append(L"Variable Type: HashMap\n");

        reference_text.AppendFormat(L"Data Type: %s\n", UTF8_TODO::GetWide(ToString(hashmap.GetValueType())).c_str());

        if( hashmap.HasDefaultValue() )
        {
            reference_text.AppendFormat(L"Default Value: %s\n", hashmap.IsValueTypeNumeric() ?
                                        UTF8_TODO::GetWide(DoubleToString(std::get<double>(*hashmap.GetDefaultValue()))).c_str() :
                                        UTF8_TODO::GetWide(*std::get<SharableString>(*hashmap.GetDefaultValue())).c_str());
        }

        reference_text.AppendFormat(L"Dimensions: %d\n", (int)hashmap.GetNumberDimensions());

        reference_text.Append(L"Dimension Types: ");

        for( size_t i = 0; i < hashmap.GetNumberDimensions(); ++i )
        {
            if( i > 0 )
                reference_text.Append(L", ");

            if( hashmap.GetDimensionType(i).has_value() )
            {
                reference_text.Append(UTF8_TODO::GetCString(ToString(*hashmap.GetDimensionType(i))));
            }

            else
            {
                reference_text.Append(L"Numeric/String");
            }
        }

        reference_text.AppendChar('\n');
    }

    void AddNamedFrequency(CString& reference_text, const NamedFrequency& named_frequency, CEngineArea* m_pEngineArea)
    {
        auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

        reference_text.Append(L"Variable Type: Freq\n");

        if( named_frequency.IsFunctionParameter() )
        {
            reference_text.Append(L"Function Parameter\n");
            return;
        }

        // list each variable included in the frequency
        const Frequency& frequency = *m_pEngineArea->GetEngineData().frequencies[named_frequency.GetFrequencyIndex()];

        if( frequency.GetFrequencyEntries().empty() )
            return;

        reference_text.Append(L"\nVariables Tallied:\n\n");

        for( const FrequencyEntry& frequency_entry : frequency.GetFrequencyEntries() )
        {
            const Symbol* symbol = NPT(frequency_entry.symbol_index);

            auto output_variable = [&](const CString& record_details, const CString& item_subitem_details)
            {
                CString occurrences = record_details;

                if( !item_subitem_details.IsEmpty() )
                    occurrences.AppendFormat(occurrences.IsEmpty() ? L"%s" : L", %s", item_subitem_details.GetString());

                reference_text.AppendFormat(occurrences.IsEmpty() ? L"%s\n" : L"%s(%s)\n", UTF8_TODO::GetWide(symbol->GetName()).c_str(), occurrences.GetString());
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
                                output_variable(record_details, L"*");
                            }

                            else
                            {
                                output_variable(record_details, UTF8_TODO::GetCString(IntToString(occurrence_details.min_item_subitem_occurrence + 1)));
                            }
                        }
                    };

                    if( !record_repeats )
                    {
                        add_record_details(CString());
                    }

                    else if( occurrence_details.combine_record_occurrences )
                    {
                        add_record_details(L"*");
                    }

                    else
                    {
                        if( occurrence_details.disjoint_record_occurrences )
                            add_record_details(L"disjoint");

                        for( size_t occurrence : occurrence_details.record_occurrences_to_explicitly_display )
                            add_record_details(UTF8_TODO::GetCString(IntToString(occurrence + 1)));
                    }
                }
            }
        }
    }

    void AddReport(CString& reference_text, const Application& application, const Report& report)
    {
        reference_text.Append(L"Variable Type: Report\n");

        if( report.IsFunctionParameter() )
        {
            reference_text.Append(L"Function Parameter\n");
        }

        else
        {
            reference_text.AppendFormat(L"Path: %s\n", UTF8_TODO::GetWide(GetRelativePathForDisplay(application.GetApplicationFilePath(), report.GetFilePath())).c_str());
        }
    }

    void AddStringWriter(CString& reference_text, const StringWriter& string_writer, const Logic::SymbolTable& symbol_table)
    {
        reference_text.Append(L"Variable Type: StringWriter\n");

        if( std::holds_alternative<int>(string_writer.GetOutput()) )
        {
            const Symbol& wrapped_symbol = symbol_table.GetAt(std::get<int>(string_writer.GetOutput()));
            reference_text.AppendFormat(L"Wraps: %s\n", UTF8_TODO::GetWide(wrapped_symbol.GetName()).c_str());
        }
    }

    void AddUserFunction(CString& reference_text, const UserFunction& user_function, const Logic::SymbolTable& symbol_table)
    {
        reference_text.Append(L"Variable Type: Function\n");

        reference_text.Append(L"Return Type: ");

        if( user_function.GetReturnType() == SymbolType::WorkVariable )
        {
            reference_text.Append(L"Numeric");
        }

        else if( user_function.GetReturnPaddingStringLength() == 0 )
        {
            reference_text.Append(L"String");
        }

        else
        {
            reference_text.AppendFormat(L"Alpha (%d)", user_function.GetReturnPaddingStringLength());
        }

        reference_text.AppendChar('\n');

        if( user_function.IsSqlCallbackFunction() )
            reference_text.Append(L"SQL Callback Function: Yes\n");

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
                    ( parameter_symbol.IsA(SymbolType::WorkVariable) ) ? L"Numeric" :
                    ( parameter_symbol.IsA(SymbolType::WorkString) )   ? L"String" :
                    ( parameter_symbol.IsA(SymbolType::UserFunction) ) ? L"Function" :
                                                                         UTF8_TODO::GetCString(ToString(parameter_symbol.GetType()));

                if( parameter_symbol.GetSubType() == SymbolSubType::WorkAlpha )
                {
                    parameter_type_string.Format(L"Alpha (%d)", static_cast<int>(assert_cast<const WorkAlpha&>(parameter_symbol).GetWideLength()));
                }

                else if( parameter_symbol.IsA(SymbolType::List) )
                {
                    parameter_type_string.AppendFormat(L" (%s)", assert_cast<const LogicList&>(parameter_symbol).IsNumeric() ? L"numeric" : L"string");
                }

                else if( parameter_symbol.IsA(SymbolType::ValueSet) )
                {
                    parameter_type_string.AppendFormat(L" (%s)", assert_cast<const ValueSet&>(parameter_symbol).IsNumeric() ? L"numeric" : L"string");
                }

                parameter_information.emplace_back(i + 1, parameter_type_string, UTF8_TODO::GetCString(parameter_symbol.GetName()), parameter_is_optional);

                parameter_type_max_length = std::max(parameter_type_max_length, parameter_type_string.GetLength());
            }

            constexpr const wchar_t* OptionalText = L"(optional) ";

            CString formatter;
            formatter.Format(L"Parameter %%%dd %%%ds| %%-%ds | %%s\n",
                (int)log10(user_function.GetNumberParameters()) + 1,
                std::get<3>(parameter_information.back()) ? _tcslen(OptionalText) : 0,
                parameter_type_max_length);

            for( const auto& parameter : parameter_information )
            {
                reference_text.AppendFormat(formatter,
                    std::get<0>(parameter),
                    std::get<3>(parameter) ? OptionalText : L"",
                    std::get<1>(parameter).GetString(),
                    std::get<2>(parameter).GetString());
            }
        }
    }


    constexpr const char* declaration_start_words[] = { "do", "for" };
    constexpr const char* declaration_break_words[] = { "do", "while", "until", "in" };
    constexpr const char* scope_change_words[]      = { "proc", "function", "if", "elseif", "else", "do", "for" };


    bool FindSymbolInBuffer(const Symbol* const symbol, const Logic::SourceBuffer& source_buffer,
                            const Logic::BasicToken** name_token, const Logic::BasicToken** declaration_token)
    {
        const std::vector<Logic::BasicToken>& basic_tokens = source_buffer.GetTokens();
        std::vector<std::tuple<const Logic::BasicToken*, const Logic::BasicToken*>> declaration_candidates;

        // because functions can't be scoped, simply search for the first use of this name
        // that doesn't follow the "declare" token
        if( symbol->IsA(SymbolType::UserFunction) )
        {
            std::optional<std::tuple<const Logic::BasicToken*, size_t>> last_function_token_and_text_tokens_counter;
            size_t i = 0;

            for( const Logic::BasicToken& basic_token : basic_tokens )
            {
                if( basic_token.type == Logic::BasicToken::Type::Text )
                {
                    const std::string_view token_text_sv = basic_token.GetSV();

                    if( SO::EqualsNoCase(token_text_sv, "function") )
                    {
                        // only count function definitions, not declarations
                        if( i == 0 || !SO::EqualsNoCase(basic_tokens[i - 1].GetSV(), "declare") )
                            last_function_token_and_text_tokens_counter.emplace(&basic_token, 0);
                    }

                    else if( last_function_token_and_text_tokens_counter.has_value() )
                    {
                        if( SO::EqualsNoCase(token_text_sv, "end") )
                        {
                            last_function_token_and_text_tokens_counter.reset();
                        }

                        else if( SO::EqualsNoCase(token_text_sv, symbol->GetName()) )
                        {
                            // the function name and the "function" token should only be separated by a few text tokens to account for
                            // things like ("sql" or "string"), though this value 4 is imprecisely chosen as a good approximation;
                            // this can fail for declared functions that are immediately used, but that can be fixed at a later point, e.g.:
                            //     declare function AAA();
                            //     function BBB() AAA(); end;
                            //     function AAA() end;
                            // this algorithm will report BBB's body when searching for AAA
                            constexpr size_t FunctionTokenToNameMaxTokens = 4;

                            if( std::get<1>(*last_function_token_and_text_tokens_counter) <= FunctionTokenToNameMaxTokens  )
                            {
                                declaration_candidates.emplace_back(&basic_token, std::get<0>(*last_function_token_and_text_tokens_counter));
                                break;
                            }

                            last_function_token_and_text_tokens_counter.reset();
                        }

                        else
                        {
                            ++std::get<1>(*last_function_token_and_text_tokens_counter);
                        }
                    }
                }

                ++i;
            }
        }

        // otherwise search the buffer from the end to the beginning, looking for the symbol name
        else
        {
            std::vector<std::string> possible_declaration_text;

            for( const auto& [declaration_text, symbol_type] : Symbol::GetDeclarationTextMap() )
            {
                if( symbol->IsA(symbol_type) )
                    possible_declaration_text.emplace_back(declaration_text);
            }

            // function included because of implicit declaration of numeric parameters
            if( symbol->IsA(SymbolType::WorkVariable) )
                possible_declaration_text.emplace_back("function");

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
                        for( const std::string& declaration_text : possible_declaration_text )
                        {
                            if( SO::EqualsNoCase(token_itr->GetSV(), declaration_text) )
                            {
                                declaration_candidates.emplace_back(name_token_candidate, &(*token_itr));
                                break;
                            }
                        }

                        // check if this word ends any possible declaration
                        for( const char* const declaration_break_word : declaration_break_words )
                        {
                            if( SO::EqualsNoCase(token_itr->GetSV(), declaration_break_word) )
                            {
                                name_token_candidate = nullptr;
                                break;
                            }
                        }
                    }
                }

                if( token_itr->type == Logic::BasicToken::Type::Text )
                {
                    if( SO::EqualsNoCase(token_itr->GetSV(), symbol->GetName()) )
                    {
                        name_token_candidate = &(*token_itr);
                    }

                    // if there is already a possible declaration candidate, check if we are in a scope change
                    else if( !declaration_candidates.empty() )
                    {
                        bool break_processing = false;

                        for( const char* const scope_change_word : scope_change_words )
                        {
                            if( SO::EqualsNoCase(token_itr->GetSV(), scope_change_word) )
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

    std::optional<size_t> FindSymbolPositionInBuffer(const Symbol* const symbol, const Logic::SourceBuffer& source_buffer)
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
                    for( const char* const declaration_start_word : declaration_start_words )
                    {
                        if( SO::EqualsNoCase(previous_token->GetSV(), declaration_start_word) )
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
                        if( SO::EqualsNoCase(previous_token->GetSV(), "function") )
                        {
                            first_function_token = previous_token;
                        }

                        else if( SO::EqualsNoCase(previous_token->GetSV(), "end") )
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
                if( token_itr->type == Logic::BasicToken::Type::Text && SO::EqualsNoCase(token_itr->GetSV(), "end") )
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
                    for( const char* const declaration_break_word : declaration_break_words )
                    {
                        if( SO::EqualsNoCase(token_itr->GetSV(), declaration_break_word) )
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

        std::string GetSelectedNodeName() const
        {
            ASSERT(!IsTabulationApplication() && GetSelectedNode() == SelectedNode::Proc);

            const HTREEITEM hItem = m_treeControl->GetSelectedItem();
            const CDEFormBase* form_base = nullptr;

            if( IsEntryApplication() )
            {
                CFormID* const pFormID = reinterpret_cast<CFormID*>(m_treeControl->GetItemData(hItem));

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
                AppTreeNode* const app_tree_node = reinterpret_cast<AppTreeNode*>(m_treeControl->GetItemData(hItem));
                form_base = app_tree_node->GetFormBase();
            }

            return ( form_base != nullptr ) ? UTF8_TODO::GetUtf8(form_base->GetName()) :
                                              std::string();
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


        void PrepareCurrentBufferText(std::string& buffer_text)
        {
            // add a dummy PROC GLOBAL to external code
            if( GetSelectedNode() == LogicReferenceWorker::SelectedNode::ExternalCode )
                buffer_text.insert(0, "PROC GLOBAL\r\n");
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
                source_code->GetProc(proc_global_lines, L"GLOBAL");
                source_code->ArrayToString(&proc_global_lines, proc_global_buffer, true);

                source_buffer = std::make_unique<Logic::SourceBuffer>(UTF8_TODO::GetUtf8(proc_global_buffer));

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
                std::string buffer_text = m_logicControl->GetText();
                PrepareCurrentBufferText(buffer_text);

                source_buffer = std::make_unique<Logic::SourceBuffer>(std::move(buffer_text));
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


        void ShowReferenceWindow(const std::vector<std::string>& selected_words, CString& reference_text, CString& logic_text,
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
                CString title = UTF8_TODO::GetCString(SO::CreateSingleString(selected_words, "."));
                title.AppendFormat(L"\n%s\n", UTF8_TODO::GetWide(SO::GetRepeatingCharacterString(L'‾', title.GetLength())).c_str());

                // if declared in external code (not currently being edited), indicate where to find this declaration
                if( external_logic_text_source_or_compilation_location.has_value() &&
                    std::holds_alternative<const TextSource*>(*external_logic_text_source_or_compilation_location) )
                {
                    title.AppendFormat(L"External Logic: %s\n",
                                       UTF8_TODO::GetWide(PortableFunctions::PathGetFilename(std::get<const TextSource*>(*external_logic_text_source_or_compilation_location)->GetFilePath())).c_str());
                }

                reference_text.Insert(0, title);
            }

            // if the reference window appears for the first time without a valid word being displayed, show the default message
            else if( reference_control->GetText().empty() )
            {
                reference_text.Append(L"Click on a word while holding the\n"
                                      L"Control and Alt keys to obtain more information.\n");
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

                reference_control->SetReadOnlyText(UTF8_TODO::GetUtf8(reference_text));

                if( logic_text.IsEmpty() )
                {
                    reference_control->ToggleLexer(SCLEX_NULL);
                }

                else
                {
                    reference_control->ToggleLexer(Lexers::GetLexer_Logic(application));

                    int length_before_logic_text = reference_control->GetLength();

                    reference_control->AppendReadOnlyText(UTF8_TODO::GetUtf8(logic_text));

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



LRESULT CMainFrame::OnLogicReference(const WPARAM wParam, const LPARAM lParam)
{
    CLogicCtrl* const logic_control = reinterpret_cast<CLogicCtrl*>(lParam);
    const bool activated_by_f1 = ( wParam == ZEDIT2O_LOGIC_REFERENCE_HELP );
    const bool goto_word = ( wParam == ZEDIT2O_LOGIC_REFERENCE_GOTO );

    const std::vector<std::string> selected_words = logic_control->ReturnWordsAtCursorWithDotNotation();

    // for help requests, first check the function table, or bypass all checks if a name
    // wasn't selected, so that the help can be launched quickly
    const char* help_topic_filename = nullptr;
    const Logic::FunctionDetails* function_details = nullptr;

    if( selected_words.size() == 1 )
    {
        help_topic_filename = Logic::ContextSensitiveHelp::GetTopicFilename(selected_words.back(), &function_details);
    }

    else if( selected_words.size() >= 2 )
    {
        // pass all but the last word as the dot notation entries
        help_topic_filename = Logic::ContextSensitiveHelp::GetTopicFilename(cs::span<const std::string>(selected_words.data(), selected_words.data() + selected_words.size() - 1),
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
            [&](const size_t selected_words_index, Symbol* const parent_symbol) -> bool
            {
                const std::string& search_word = selected_words[selected_words_index];
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
                                reference_text.AppendFormat(L"There are multiple symbols with the name %s.\n"
                                                            L"Please provide additional qualifiers to avoid ambiguity.\n", UTF8_TODO::GetWide(search_word).c_str());
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
                        reference_text.AppendFormat(L"Base Name: %s\n", UTF8_TODO::GetWide(symbol->GetName()).c_str());

                    // show any aliases to the symbol
                    const std::vector<std::string> aliases = symbol_table.GetAliases(*symbol);

                    if( !aliases.empty() )
                        reference_text.AppendFormat(L"Aliases: %s\n", UTF8_TODO::GetWide(SO::CreateSingleString(aliases)).c_str());


                    // add the symbol
                    if( symbol->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
                    {
                        AddDictionary(reference_text, symbol);
                    }

                    else if( symbol->IsA(SymbolType::Record) )
                    {
                        AddRecord(reference_text, assert_cast<const EngineRecord&>(*symbol).GetDictRecord());
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

                    else if( symbol->IsA(SymbolType::StringWriter) )
                    {
                        AddStringWriter(reference_text, assert_cast<const StringWriter&>(*symbol), symbol_table);
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
                            std::string buffer_text = std::string(source_buffer->GetTokens().front().token_text, *position_in_buffer);
                            SO::MakeNewlineCRLF(buffer_text);
                            position_in_buffer = buffer_text.length();
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
                            source_buffer = std::make_unique<Logic::SourceBuffer>(logic_control->GetText());
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

                if( LevelLocator(dictionaries, UTF8_TODO::GetCString(selected_words.front()), &dictionary, &dict_level) )
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
                                              UTF8_TODO::GetWide(std::get<const TextSource*>(*external_logic_text_source_or_compilation_location)->GetFilePath()),
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
    std::vector<std::string> selected_words = logic_control->ReturnWordsAtCursorWithDotNotation();

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
    const std::string& word_to_complete = selected_words.back();

    std::string suggested_words;
    bool word_comes_from_fuzzy_matching;

    if( selected_words.size() == 1 )
    {
        std::tie(suggested_words, word_comes_from_fuzzy_matching) = LogicReferenceAutoCompleter.GetSuggestedWordString(word_to_complete);
    }

    else
    {
        // pass all but the last word as the dot notation entries
        suggested_words = LogicReferenceAutoCompleter.GetSuggestedWordString(cs::span<const std::string>(selected_words.data(), selected_words.data() + selected_words.size() - 1),
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


LRESULT CMainFrame::OnLogicInsertProcName(WPARAM /*wParam*/, const LPARAM lParam)
{
    CLogicCtrl* const logic_control = reinterpret_cast<CLogicCtrl*>(lParam);

    LogicReferenceWorker logic_reference_worker(this);

    // this will not work for tabulation applications
    if( !logic_reference_worker.IsValid() || logic_reference_worker.IsTabulationApplication() )
        return 0;


    std::string proc_or_function_name;

    // if on a PROC, insert the name of the currently selected node
    if( logic_reference_worker.GetSelectedNode() == LogicReferenceWorker::SelectedNode::Proc )
    {
        proc_or_function_name = logic_reference_worker.GetSelectedNodeName();
    }

    // if on a report, insert $ because best practice would be not to use report names in the report's logic
    else if( logic_reference_worker.GetSelectedNode() == LogicReferenceWorker::SelectedNode::Report )
    {
        proc_or_function_name = "$";
    }

    // otherwise (if on PROC GLOBAL or external code), get the logic buffer and search backwards for the last PROC or function name
    else
    {
        std::string buffer_text_up_to_cursor = logic_control->GetText().substr(0, logic_control->GetCurrentPos());
        logic_reference_worker.PrepareCurrentBufferText(buffer_text_up_to_cursor);

        auto source_buffer = std::make_shared<Logic::SourceBuffer>(std::move(buffer_text_up_to_cursor));

        auto background_compiler = logic_reference_worker.CreateCompiler();
        background_compiler->Compile(source_buffer);
        const std::vector<Logic::BasicToken>& basic_tokens = source_buffer->GetTokens();

        for( size_t i = basic_tokens.size() - 1; proc_or_function_name.empty() && i < basic_tokens.size(); --i )
        {
            if( basic_tokens[i].type == Logic::BasicToken::Type::Text )
            {
                // if in a function, return the last function added to the symbol table
                if( SO::EqualsNoCase(basic_tokens[i].GetSV(), "function") )
                {
                    const Logic::SymbolTable& symbol_table = background_compiler->GetCompiledSymbolTable();

                    for( size_t symbol_itr = symbol_table.GetTableSize() - 1; symbol_itr >= 1; --symbol_itr )
                    {
                        const Symbol& symbol = symbol_table.GetAt(symbol_itr);

                        if( symbol.IsA(SymbolType::UserFunction) )
                        {
                            proc_or_function_name = symbol.GetName();
                            break;
                        }
                    }
                }

                // if in a proc, return its name (with dot notation)
                else if( SO::EqualsNoCase(basic_tokens[i].GetSV(), "PROC") )
                {
                    bool last_token_was_dot = false;

                    for( size_t j = i + 1; j < basic_tokens.size(); ++j )
                    {
                        bool append_this_token = ( proc_or_function_name.empty() || last_token_was_dot );
                        last_token_was_dot = ( basic_tokens[j].type == Logic::BasicToken::Type::Operator && basic_tokens[j].token_code == TokenCode::TOKPERIOD );
                        append_this_token |= last_token_was_dot;

                        if( append_this_token )
                        {
                            proc_or_function_name.append(basic_tokens[j].GetSV());
                        }

                        else
                        {
                            break;
                        }
                    }

                    // don't ever display PROC GLOBAL
                    if( SO::EqualsNoCase(proc_or_function_name, "GLOBAL") )
                        proc_or_function_name.clear();
                }
            }
        }
    }


    if( !proc_or_function_name.empty() )
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
