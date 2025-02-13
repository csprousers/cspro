#pragma once


namespace MGF
{
    constexpr int statement_invalid_1                                   =      1;
    constexpr int equals_expected_in_assignment_5                       =      5;
    constexpr int expecting_then_keyword_6                              =      6;
    constexpr int expecting_else_elseif_endif_keyword_7                 =      7;
    constexpr int expecting_endif_keyword_8                             =      8;
    constexpr int expecting_do_keyword_9                                =      9;
    constexpr int expecting_enddo_keyword_10                            =     10;
    constexpr int expecting_variable_11                                 =     11;
    constexpr int left_parenthesis_expected_in_function_call_14         =     14;
    constexpr int right_parenthesis_expected_in_function_call_17        =     17;
    constexpr int right_parenthesis_expected_19                         =     19;
    constexpr int two_consecutive_operators_20                          =     20;
    constexpr int arithmetic_expression_invalid_21                      =     21;
    constexpr int left_parenthesis_expected_in_element_reference_22     =     22;
    constexpr int right_parenthesis_expected_in_element_reference_24    =     24;
    constexpr int single_variable_cannot_have_subscript_25              =     25;
    constexpr int expecting_semicolon_30                                =     30;

    constexpr int function_call_missing_arguments_49                    =     49;
    constexpr int function_call_arguments_count_mismatch_50             =     50;
    constexpr int function_call_too_many_arguments_51                   =     51;

    constexpr int switch_expecting_array_57                             =     57;
    constexpr int switch_expecting_double_colon_58                      =     58;

    constexpr int integer_constant_expected_82                          =     82;
    constexpr int declaration_invalid_92                                =     92;
    constexpr int variable_belongs_to_lower_level_93                    =     93;

    constexpr int string_substring_invalid_95                           =     95;
    constexpr int string_expression_invalid_96                          =     96;

    constexpr int symbol_name_in_use_102                                =    102;

    constexpr int FileIO_error_163                                      =    163;

    constexpr int message_language_syntax_invalid_170                   =    170;
    constexpr int message_language_name_invalid_171                     =    171;
    constexpr int message_syntax_invalid_172                            =    172;
    constexpr int message_translation_syntax_invalid_173                =    173;
    constexpr int message_number_invalid_174                            =    174;
    constexpr int message_verbatim_string_literal_not_allowed_175       =    175;
    constexpr int message_text_not_specified_176                        =    176;
    constexpr int message_contents_text_defined_with_different_text_177 =    177;
    constexpr int message_invalid_text_after_processed_line_178         =    178;

    constexpr int dictionary_external_expected_525                      =    525;

    constexpr int function_call_comma_expected_528                      =    528;
    constexpr int function_call_comma_expected_detailed_529             =    529;
    constexpr int function_call_arguments_count_mismatch_detailed_530   =    530;
    constexpr int function_call_too_few_arguments_detailed_531          =    531;
    constexpr int function_call_too_many_arguments_detailed_532         =    532;

    constexpr int secondary_indices_no_longer_supported_543             =    543;
    constexpr int dictionary_expected_544                               =    544;
    constexpr int dictionary_one_level_expected_545                     =    545;
    constexpr int dictionary_specified_key_length_mismatch_547          =    547;
    constexpr int dictionary_specified_key_zero_fill_mismatch_548       =    548;
    constexpr int dictionary_use_not_possible_with_NoIndex_549          =    549;
    constexpr int dictionary_use_not_possible_because_of_NoIndex_550    =    550;
    constexpr int dictionary_use_not_valid_for_working_storage_551      =    551;

    constexpr int argument_invalid_560                                  =    560;

    constexpr int Array_invalid_subscript_608                           =    608;
    constexpr int Array_subscript_count_mismatch_609                    =    609;

    constexpr int symbol_assignment_not_allowed_in_proc_global_687      =    687;

    constexpr int LoopStack_nested_for_loop_751                         =    751;
    constexpr int LoopStack_nested_dictionary_loop_753                  =    753;

    constexpr int dictionary_or_file_expected_930                       =    930;
    constexpr int setfile_invalid_use_of_create_in_non_entry_931        =    931;

    constexpr int ValueSet_not_correct_data_type_941                    =    941;

    constexpr int Array_not_correct_data_type_955                       =    955;
    constexpr int Array_not_correct_dimensions_956                      =    956;

    constexpr int List_expected_960                                     =    960;
    constexpr int List_not_correct_data_type_961                        =    961;
    constexpr int List_mismatched_types_962                             =    962;
    constexpr int List_cannot_assign_to_itself_963                      =    963;
    constexpr int List_read_only_cannot_be_modified_965                 =    965;

    constexpr int dictionary_form_symbol_expected_1108                  =   1108;

    constexpr int getos_invalid_argument_1201                           =   1201;

    constexpr int width_height_both_must_be_specified_2033              =   2033;
    constexpr int color_invalid_2036                                    =   2036;

    constexpr int case_search_invalid_relational_operator_4014          =   4014;
    constexpr int dictionary_set_access_use_invalid_in_entry_4015       =   4015;
    constexpr int locate_invalid_on_input_dict_in_entry_4016            =   4016;
    constexpr int dictionary_special_output_not_allowed_4018            =   4018;

    constexpr int option_defined_more_than_once_7017                    =   7017;
    constexpr int dot_required_to_separate_options_7018                 =   7018;

    constexpr int trace_filename_argument_error_7103                    =   7103;

    constexpr int Do_invalid_initial_expression_8001                    =   8001;
    constexpr int Do_expecting_while_until_keyword_8002                 =   8002;
    constexpr int Do_by_must_have_a_varying_variable_8003               =   8003;
    constexpr int NextBreak_must_be_used_in_loop_8004                   =   8004;

    constexpr int savepartial_invalid_in_level0_8050                    =   8050;
    constexpr int savepartial_invalid_in_proc_8054                      =   8054;

    constexpr int dictionary_item_expected_8100                         =   8100;

    constexpr int Impute_repeated_clause_8101                           =   8101;
    constexpr int Impute_dict_item_has_no_value_sets_8102               =   8102;
    constexpr int Impute_invalid_value_set_index_8103                   =   8103;
    constexpr int Impute_value_set_does_not_belong_to_item_8104         =   8104;

    constexpr int alpha_specificy_length_8203                           =   8203;
    constexpr int alpha_length_too_long_8204                            =   8204;
    constexpr int alias_invalid_8206                                    =   8206;
    constexpr int ensure_type_error_8207                                =   8207;
    constexpr int ensure_assignment_invalid_8208                        =   8208;
    constexpr int Array_too_few_initialization_values_8213              =   8213;
    constexpr int Array_too_many_initialization_values_8214             =   8214;
    constexpr int string_literal_will_be_truncated_8216                 =   8216;
    constexpr int numeric_constant_expected_8217                        =   8217;
    constexpr int string_literal_expected_8218                          =   8218;

    constexpr int Array_repeating_cells_error_8222                      =   8222;
    constexpr int Array_repeating_cells_not_evently_repeated_8224       =   8224;
    constexpr int Array_too_many_cells_8225                             =   8225;
    constexpr int Array_specify_dimension_8226                          =   8226;

    constexpr int GPS_invalid_command_8251                              =   8251;
    constexpr int GPS_readDuration_argument_invalid_8252                =   8252;

    constexpr int Query_invalid_array_8272                              =   8272;
    constexpr int Query_record_must_be_working_storage_8273             =   8273;
    constexpr int Query_paradata_concat_invalid_arguments_8274          =   8274;
    constexpr int Query_paradata_concat_invalid_output_argument_8275    =   8275;

    constexpr int SpecialFunction_invalid_case_9112                     =   9112;

    constexpr int CS_action_missing_9201                                =   9201;
    constexpr int CS_action_invalid_9202                                =   9202;
    constexpr int CS_action_invalid_case_9203                           =   9203;
    constexpr int CS_missing_required_arguments_9204                    =   9204;
    constexpr int CS_json_argument_error_9205                           =   9205;
    constexpr int CS_access_without_token_prompt_9208                   =   9208;

    constexpr int SaveArray_only_allowed_in_proc_global_19004           =  19004;

    constexpr int setoutput_valid_only_for_batch_29006                  =  29006;

    constexpr int item_expected_33109                                   =  33109;
    constexpr int object_of_type_expected_33116                         =  33116;
    constexpr int object_of_type_expected_detailed_33117                =  33117;

    constexpr int string_expression_expected_45006                      =  45006;

    constexpr int ValueSet_invalid_operation_for_dict_value_set_47170   =  47170;
    constexpr int ValueSet_add_not_correct_data_type_47171              =  47171;
    constexpr int ValueSet_add_cannot_add_self_47172                    =  47172;

    constexpr int Pff_load_argument_invalid_47190                       =  47190;
    constexpr int Pff_invalid_assignment_47193                          =  47193;
    constexpr int Pff_property_invalid_with_dictionary_47194            =  47194;

    constexpr int Audio_invalid_assignment_47201                        =  47201;

    constexpr int Case_dictionary_placement_47251                       =  47251;
    constexpr int Case_assignment_invalid_47252                         =  47252;
    constexpr int Case_assignment_dictionary_mismatch_47253             =  47253;
    constexpr int Case_assignment_must_be_from_external_case_47254      =  47254;
    constexpr int DataSource_dictionary_placement_47261                 =  47261;

    constexpr int Document_invalid_assignment_47221                     =  47221;

    constexpr int HashMap_invalid_assignment_47231                      =  47231;
    constexpr int HashMap_invalid_dimension_47232                       =  47232;
    constexpr int HashMap_invalid_assignment_HashMap_value_47233        =  47233;
    constexpr int HashMap_invalid_assignment_HashMap_dimensions_47234   =  47234;
    constexpr int HashMap_invalid_value_for_dimension_47235             =  47235;
    constexpr int HashMap_invalid_default_value_47236                   =  47236;
    constexpr int HashMap_getKeys_requires_List_47237                   =  47237;

    constexpr int Image_invalid_assignment_47241                        =  47241;
    constexpr int Image_load_invalid_argument_47242                     =  47242;
    constexpr int Image_resample_invalid_arguments_47243                =  47243;

    constexpr int dictionary_expected_47301                             =  47301;
    constexpr int dictionary_expected_not_Case_or_DataSource_name_47302 =  47302;
    constexpr int dictionary_or_Case_expected_47303                     =  47303;
    constexpr int dictionary_or_Case_not_DataSource_expected_47304      =  47304;
    constexpr int dictionary_or_DataSource_expected_47305               =  47305;
    constexpr int dictionary_or_DataSource_not_Case_expected_47306      =  47306;
    constexpr int dictionary_Case_dictionary_does_not_match_47307       =  47307;
    constexpr int dictionary_Case_required_to_receive_data_47308        =  47308;

    constexpr int countcases_with_where_requires_case_47311             =  47311;

    constexpr int Geometry_invalid_assignment_47321                     =  47321;
    constexpr int Geometry_Map_argument_expected_47322                  =  47322;

    constexpr int DeckArray_only_numeric_arrays_47501                   =  47501;
    constexpr int DeckArray_value_set_size_invalid_47502                =  47502;
    constexpr int DeckArray_leftover_cell_error_47503                   =  47503;

    constexpr int Report_unbalanced_escapes_48101                       =  48101;
    constexpr int Report_end_reached_while_in_logic_or_fill_48102       =  48102;
    constexpr int Report_unsupported_functionality_48103                =  48103;
    constexpr int Report_write_in_invalid_locatation_48104              =  48104;

    constexpr int UserFunction_expects_argument_50000                   =  50000;
    constexpr int UserFunction_function_pointer_invalid_50001           =  50001;
    constexpr int UserFunction_sql_callback_invalid_50002               =  50002;
    constexpr int UserFunction_parameter_must_be_optional_50003         =  50003;
    constexpr int UserFunction_type_invalid_as_optional_50004           =  50004;
    constexpr int UserFunction_must_terminate_with_end_50005            =  50005;

    constexpr int prompt_invalid_combination_51102                      =  51102;
    constexpr int timestamp_argument_error_51111                        =  51111;

    constexpr int randomizevs_invalid_element_91710                     =  91710;
    constexpr int randomizevs_exclude_expected_91711                    =  91711;

    constexpr int unbalanced_multiline_comment_92180                    =  92180;

    constexpr int symbol_invalid_type_93008                             =  93008;

    constexpr int Variable_destination_invalid_93011                    =  93011;
    constexpr int Variable_destination_not_correct_data_type_93012      =  93012;
    constexpr int Variable_destination_function_error_93013             =  93013;

    constexpr int variable_modifier_duplicated_94100                    =  94100;
    constexpr int variable_modifier_invalid_94101                       =  94101;
    constexpr int CommonStore_cannot_be_opened_94102                    =  94102;
    constexpr int CommonStore_setting_not_found_94103                   =  94103;
    constexpr int CommonStore_setting_not_numeric_94104                 =  94104;
    constexpr int variable_modifier_persistent_duplicate_names_94105    =  94105;

    constexpr int Map_argument_must_be_function_94200                   =  94200;
    constexpr int Map_zoomTo_invalid_arguments_94201                    =  94201;
    constexpr int Map_argument_must_be_Geometry_94202                   =  94202;

    constexpr int Freq_unnamed_freqs_not_allowed_in_proc_global_94500   =  94500;
    constexpr int Freq_invalid_command_94501                            =  94501;
    constexpr int Freq_duplicate_command_94502                          =  94502;
    constexpr int Freq_command_cannot_be_used_with_named_freqs_94503    =  94503;
    constexpr int Freq_invalid_breakdown_94504                          =  94504;
    constexpr int Freq_headings_must_be_string_literals_94505           =  94505;
    constexpr int Freq_invalid_percentiles_94506                        =  94506;
    constexpr int Freq_invalid_decimals_94507                           =  94507;
    constexpr int Freq_invalid_page_length_94508                        =  94508;
    constexpr int Freq_invalid_sort_94509                               =  94509;
    constexpr int Freq_improper_use_of_formatting_commands_94510        =  94510;
    constexpr int Freq_command_requires_variable_94511                  =  94511;
    constexpr int Freq_occurrence_number_invalid_94512                  =  94512;
    constexpr int Freq_exclude_cannot_be_used_without_include_94513     =  94513;
    constexpr int Freq_access_from_invalid_level_94514                  =  94514;
    constexpr int Freq_no_valid_variables_to_include_94515              =  94515;
    constexpr int Freq_invalid_occurrence_record_is_not_repeating_94516 =  94516;
    constexpr int Freq_invalid_record_occurrence_94517                  =  94517;
    constexpr int Freq_invalid_occurrence_item_is_not_repeating_94518   =  94518;
    constexpr int Freq_invalid_number_occurrences_94519                 =  94519;
    constexpr int Freq_invalid_item_occurrence_94520                    =  94520;
    constexpr int Freq_no_variables_after_exclusions_94521              =  94521;
    constexpr int Freq_individual_tally_access_not_allowed_94522        =  94522;
    constexpr int Freq_value_sets_must_be_from_dictionary_94523         =  94523;
    constexpr int Freq_value_set_not_associated_with_freq_94524         =  94524;
    constexpr int Freq_cannot_be_saved_to_non_HTML_report_94533         =  94533;

    constexpr int DataAccess_data_not_available_until_lower_level_94601 =  94601;

    constexpr int argument_invalid_case_94700                           =  94700;
    constexpr int argument_duplicate_named_argument_94701               =  94701;
    constexpr int argument_invalid_type_94702                           =  94702;
    constexpr int argument_argument_must_be_of_types_94703              =  94703;
    constexpr int argument_integer_out_of_range_94704                   =  94704;
    constexpr int argument_integer_too_low_94705                        =  94705;
    constexpr int argument_integer_too_high_94706                       =  94706;
    constexpr int argument_type_automatic_casting_not_supported_94707   =  94707;

    constexpr int deprecation_use_string_not_alpha_95004                =  95004;
    constexpr int deprecation_use_CS_Path_selectFile_95005              =  95005;
    constexpr int deprecation_use_CS_UI_showDialog_95006                =  95006;
    constexpr int deprecation_writecase_with_ids_95011                  =  95011;
    constexpr int deprecation_use_setProperty_95015                     =  95015;
    constexpr int deprecation_removed_Array_with_old_type_95019         =  95019;
    constexpr int deprecation_setvalueset_with_Array_95020              =  95020;
    constexpr int deprecation_removed_old_recode_95022                  =  95022;
    constexpr int deprecation_impute_with_vset_95024                    =  95024;
    constexpr int deprecation_freq_without_parentheses_95025            =  95025;
    constexpr int deprecation_freq_weight_command_95026                 =  95026;
    constexpr int deprecation_ValueSet_image_filename_95028             =  95028;
    constexpr int deprecation_removed_old_reporting_system_95029        =  95029;
    constexpr int deprecation_config_without_type_95030                 =  95030;
    constexpr int deprecation_setvalueset_with_at_string_literal_95032  =  95032;

    constexpr int regex_invalid_100260                                  = 100260;

    constexpr int Path_invalid_directory_100379                         = 100379;

    constexpr int Item_subscript_cannot_be_empty_100400                 = 100400;
    constexpr int Item_subscript_too_many_subcripts_100401              = 100401;
    constexpr int Item_subscript_type_invalid_100402                    = 100402;
    constexpr int Item_subscript_separator_invalid_100403               = 100403;
    constexpr int Item_subscript_must_be_one_100404                     = 100404;
    constexpr int Item_subscript_out_of_range_100405                    = 100405;
    constexpr int Item_subscript_implicit_cannot_be_calculated_100406   = 100406;

    constexpr int JSON_invalid_text_100430                              = 100430;
    constexpr int JSON_text_has_warnings_100431                         = 100431;
}


namespace MGF_TODO
{
    constexpr int m_770     =    770;
    constexpr int m_771     =    771;
    constexpr int m_772     =    772;
    constexpr int m_773     =    773;
    constexpr int m_775     =    775;
    constexpr int m_776     =    776;
    constexpr int m_777     =    777;
    constexpr int m_779     =    779;
    constexpr int m_780     =    780;
    constexpr int m_781     =    781;
    constexpr int m_782     =    782;
    constexpr int m_783     =    783;
    constexpr int m_784     =    784;
    constexpr int m_785     =    785;
    constexpr int m_786     =    786;
    constexpr int m_787     =    787;
    constexpr int m_788     =    788;
    constexpr int m_789     =    789;
    constexpr int m_790     =    790;
    constexpr int m_33054   =  33054;
    constexpr int m_33055   =  33055;
    constexpr int m_33056   =  33056;
    constexpr int m_33059   =  33059;
    constexpr int m_90003   =  90003;
    constexpr int m_90004   =  90004;
    constexpr int m_91703   =  91703;
    constexpr int m_91704   =  91704;
    constexpr int m_95001   =  95001;
}
