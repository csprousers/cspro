#pragma once

#include <zEngineO/Nodes/Set.h>
#include <zLogicO/FunctionTable.h>


namespace VerifyDictionaryFlag
{
    // base flags
    constexpr int External          = 0x01;
    constexpr int OneLevel          = 0x02;
    constexpr int Writeable         = 0x04;
    constexpr int NeedsIndex        = 0x08;
    constexpr int CannotHaveIndex   = 0x10;
    constexpr int NotWorkingStorage = 0x20;

    // common combinations
    constexpr int External_OneLevel            = External | OneLevel;
    constexpr int External_OneLevel_NeedsIndex = External | OneLevel | NeedsIndex;
}


enum class SelcaseMarkType  : int { None = 0, Multiple = 1, MultipleAutomark = 9 };
enum class SelcaseQueryType : int { All = -1, Unmarked = 0, Marked = 1 };


namespace Nodes
{
    struct CaseIO
    {
        FunctionCode function_code;
        int data_repository_dictionary_symbol_index;
        int case_dictionary_symbol_index;
        int key_arguments_list_node;
    };


    struct CaseSearch
    {
        static constexpr int SearchByUuidCode      = -1;
        static constexpr int SearchByKeyPrefixCode = -2;

        FunctionCode function_code;
        int dictionary_symbol_index;
        int operator_code;
        int key_expression;
    };


    struct CountCases
    {
        FunctionCode function_code;
        int data_repository_dictionary_symbol_index;
        int where_expression;
        int dictionary_access;
        int starts_with_expression;
        int case_dictionary_symbol_index;
    };


    struct ForDictionary
    {
        int st_code;
        int next_st;
        int data_repository_dictionary_symbol_index;
        int query_type_or_where_expression; // 0: unmarked, 1: marked, -1: all
        int block_expression;
        int dictionary_access;
        int starts_with_expression;
        int case_dictionary_symbol_index;
    };


    struct NMembers
    {
        FunctionCode function_code;
        int dictionary_symbol_index;
        SelcaseQueryType query_type;
    };


    struct Retrieve
    {
        FunctionCode function_code;
        int data_repository_dictionary_symbol_index;
        int case_dictionary_symbol_index;
    };


    struct SetAccessFirstLast
    {
        FunctionCode function_code;
        int next_st;
        SetAction set_action;
        int dictionary_symbol_index;
        int dictionary_access;
    };
}
