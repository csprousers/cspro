#include "stdafx.h"
#include "KeywordTable.h"
#include <zToolsO/Special.h>

using namespace Logic;


namespace
{
    const KeywordDetails Keywords[] =
    {
        { "and",                "operators.html",                       TokenCode::TOKANDOP },
        { "or",                 "operators.html",                       TokenCode::TOKOROP },
        { "not",                "operators.html",                       TokenCode::TOKNOTOP },
        { "if",                 "if_statement.html",                    TokenCode::TOKIF },
        { "then",               "if_statement.html",                    TokenCode::TOKTHEN },
        { "else",               "if_statement.html",                    TokenCode::TOKELSE },
        { "elseif",             "if_statement.html",                    TokenCode::TOKELSEIF },
        { "endif",              "if_statement.html",                    TokenCode::TOKENDIF },
        { "do",                 "do_statement.html",                    TokenCode::TOKDO },
        { "enddo",              "do_statement.html",                    TokenCode::TOKENDDO },
        { "while",              "while_statement.html",                 TokenCode::TOKWHILE },
        { "box",                "recode_statement.html",                TokenCode::TOKRECODE },
        { "endbox",             "recode_statement.html",                TokenCode::TOKENDRECODE },
        { "recode",             "recode_statement.html",                TokenCode::TOKRECODE },
        { "endrecode",          "recode_statement.html",                TokenCode::TOKENDRECODE },
        { "exit",               "exit_statement.html",                  TokenCode::TOKEXIT },
        { "end",                "function_statement.html",              TokenCode::TOKEND },
        { "where",              nullptr,                                TokenCode::TOKWHERE },
        { "stop",               "stop_function.html",                   TokenCode::TOKSTOP },
        { "endcase",            "endcase_statement.html",               TokenCode::TOKENDCASE }, // GHM 20100310
        { "universe",           "universe_statement.html",              TokenCode::TOKUNIVERSE }, // GHM 20100310
        { "skip",               "skip_statement.html",                  TokenCode::TOKSKIP },
        { "move",               "move_statement.html",                  TokenCode::TOKMOVE }, // RHF Dec 09, 2003
        { "Case",               "Case.html",                            TokenCode::TOKKWCASE },
        { "to",                 nullptr,                                TokenCode::TOKTO },
        { "next",               "next_statement.html",                  TokenCode::TOKNEXT },
        { "page",               nullptr,                                TokenCode::TOKPAGE },
        { "endsect",            "deprecated_features.html",             TokenCode::TOKENDSECT },
        { "endgroup",           "endgroup_statement.html",              TokenCode::TOKENDSECT }, // RHF Mar 19, 2001
        { "reenter",            "reenter_statement.html",               TokenCode::TOKREENTER },
        { "noinput",            "noinput_statement.html",               TokenCode::TOKNOINPUT },
        { "advance",            "advance_statement.html",               TokenCode::TOKADVANCE },
        { "enter",              "enter_statement.html",                 TokenCode::TOKENTER },
        { "level",              nullptr,                                TokenCode::TOKLEVEL },
        { "endlevel",           "endlevel_statement.html",              TokenCode::TOKENDLEVL },
        { "table",              nullptr,                                TokenCode::TOKTABLE },
        { "stable",             nullptr,                                TokenCode::TOKSTABLE },
        { "hotdeck",            nullptr,                                TokenCode::TOKHOTDECK },
        { "mean",               nullptr,                                TokenCode::TOKMEAN },
        { "smean",              nullptr,                                TokenCode::TOKSMEAN },
        { "exclude",            nullptr,                                TokenCode::TOKEXCLUDE },
        { "include",            nullptr,                                TokenCode::TOKINCLUDE },
        { "break",              "break_statement.html",                 TokenCode::TOKBREAK },
        { "Freq",               "Freq_statement_unnamed.html",          TokenCode::TOKKWFREQ },
        { "Frequency",          "Freq_statement_unnamed.html",          TokenCode::TOKKWFREQ },
        { "title",              nullptr,                                TokenCode::TOKTITLE },
        { "stub",               nullptr,                                TokenCode::TOKSTUB },
        { "intervals",          nullptr,                                TokenCode::TOKINTERVAL },
        { "highest",            nullptr,                                TokenCode::TOKHIGHEST },
        { "lowers",             nullptr,                                TokenCode::TOKLOWER },
        { "by",                 nullptr,                                TokenCode::TOKBY },
        { "all",                nullptr,                                TokenCode::TOKALL },
        { "weighted",           nullptr,                                TokenCode::TOKWEIGHT },
        { "function",           "function_statement.html",              TokenCode::TOKKWFUNCTION },
        { "add",                nullptr,                                TokenCode::TOKADD },
        { "modify",             nullptr,                                TokenCode::TOKMODIFY }, // RHF Nov 13, 2001
        { "verify",             nullptr,                                TokenCode::TOKVERIFY },
        { "crosstab",           nullptr,                                TokenCode::TOKKWCTAB },
        { "select",             "errmsg_function.html",                 TokenCode::TOKSELECT },
        { "for",                "for_statement.html",                   TokenCode::TOKFOR },
        { "noprint",            nullptr,                                TokenCode::TOKNOPRINT },
        { "noauto",             nullptr,                                TokenCode::TOKNOAUTO },
        { "nobreak",            nullptr,                                TokenCode::TOKNOBREAK },
        { "nofreq",             nullptr,                                TokenCode::TOKNOFREQ },
        { "cell",               nullptr,                                TokenCode::TOKCELLTYPE },
        { "linked",             nullptr,                                TokenCode::TOKLINKED },
        { "Array",              "Array_statement.html",                 TokenCode::TOKKWARRAY },
        { "save",               nullptr,                                TokenCode::TOKSAVE }, // JH 3/13/06
        { "export",             "export_statement.html",                TokenCode::TOKEXPORT },
        { "case_id",            nullptr,                                TokenCode::TOKCASEID },
        { "rec_name",           nullptr,                                TokenCode::TOKRECNAME }, // For export command
        { "rec_type",           nullptr,                                TokenCode::TOKRECTYPE }, // For export command
        { "group",              nullptr,                                TokenCode::TOKKWGROUP },
        { "missing",            "special_values.html",                  TokenCode::TOKMISSING },
        { "default",            "special_values.html",                  TokenCode::TOKDEFAULT },
        { "notappl",            "special_values.html",                  TokenCode::TOKNOTAPPL },
        { "preproc",            "preproc_statement.html",               TokenCode::TOKPREPRO },
        { "postproc",           "postproc_statement.html",              TokenCode::TOKPOSTPRO },
        { "tally",              nullptr,                                TokenCode::TOKTALLY },
        { "postcalc",           nullptr,                                TokenCode::TOKPOSTCALC },

        { "onfocus",            "onfocus_statement.html",               TokenCode::TOKONFOCUS },
        { "killfocus",          "killfocus_statement.html",             TokenCode::TOKKILLFOCUS },
        { "onoccchange",        "onoccchange_statement.html",           TokenCode::TOKONOCCCHANGE },

        { "set",                "set_attributes_statement.html",        TokenCode::TOKSET },
        { "multiple",           nullptr,                                TokenCode::TOKMULTIPLE },
        { "endfor",             "for_statement.html",                   TokenCode::TOKENDFOR },
        { "in",                 "in_operator.html",                     TokenCode::TOKIN },
        { "has",                "has_operator.html",                    TokenCode::TOKHAS }, // GHM 20120429
        { "until",              "do_statement.html",                    TokenCode::TOKUNTIL },
        { "varying",            "do_statement.html",                    TokenCode::TOKVARYING },
        { "numeric",            "numeric_statement.html",               TokenCode::TOKNUMERIC },
        { "alpha",              "alpha_statement.html",                 TokenCode::TOKALPHA },
        { "Relation",           "relation_statement.html",              TokenCode::TOKKWRELATION }, // RHF Sep 19, 2001
        { "Item",               nullptr,                                TokenCode::TOKKWITEM },  // RHF Jun 14, 2002
        { "unit",               nullptr,                                TokenCode::TOKUNIT },  // RHF Jun 14, 2002
        { "endunit",            nullptr,                                TokenCode::TOKENDUNIT },  // RHF Jul 14, 2005
        { "stat",               nullptr,                                TokenCode::TOKSTAT },  // RHF Jun 14, 2002
        { "statistics",         nullptr,                                TokenCode::TOKSTAT },  // RHF Jun 14, 2002
        { "row",                nullptr,                                TokenCode::TOKROW },
        { "column",             nullptr,                                TokenCode::TOKCOLUMN },
        { "layer",              nullptr,                                TokenCode::TOKLAYER },
        { "tablogic",           nullptr,                                TokenCode::TOKTABLOGIC },  // RHF Jul 02, 2002
        { "endlogic",           nullptr,                                TokenCode::TOKENDLOGIC }, // RHF Jul 02, 2002
        { "vset",               nullptr,                                TokenCode::TOKKWVSET }, // RHF Jul 03, 2002

        { "using",              "sort_function.html",                   TokenCode::TOKUSING },      // Chirag, Sep 11, 2002
        { "ascending",          "sort_function.html",                   TokenCode::TOKASCENDING },  // Chirag, Sep 11, 2002
        { "descending",         "sort_function.html",                   TokenCode::TOKDESCENDING }, // Chirag, Sep 11, 2002

        { "subtable",           nullptr,                                TokenCode::TOKSUBTABLE }, // RHF Jul 31, 2002
        { "File",               "File_statement.html",                  TokenCode::TOKKWFILE },

        { "alias",              "alias_statement.html",                 TokenCode::TOKALIAS },
        { "string",             "string_statement.html",                TokenCode::TOKSTRING },
        { "List",               "List_statement.html",                  TokenCode::TOKKWLIST },
        { "config",             "config_modifier.html",                 TokenCode::TOKCONFIG },
        { "forcase",            "forcase_statement.html",               TokenCode::TOKFORCASE },
        { "ask",                "ask_statement.html",                   TokenCode::TOKASK },
        { "sql",                "sqlite_callback_functions.html",       TokenCode::TOKSQL },
        { "ensure",             "ensure_modifier.html",                 TokenCode::TOKENSURE },
        { "continue",           "errmsg_function.html",                 TokenCode::TOKCONTINUE },
        { "Map",                "Map_statement.html",                   TokenCode::TOKKWMAP },
        { "ValueSet",           "ValueSet_statement.html",              TokenCode::TOKKWVALUESET },
        { "false",              "boolean_values.html",                  TokenCode::TOKFALSE },
        { "true",               "boolean_values.html",                  TokenCode::TOKTRUE },
        { "Pff",                "Pff_statement.html",                   TokenCode::TOKKWPFF },
        { "when",               "when_statement.html",                  TokenCode::TOKWHEN },
        { "endwhen",            "when_statement.html",                  TokenCode::TOKENDWHEN },
        { "SystemApp",          "SystemApp_statement.html",             TokenCode::TOKKWSYSTEMAPP },
        { "refused",            "refused_value.html",                   TokenCode::TOKREFUSED },
        { "Audio",              "Audio_statement.html",                 TokenCode::TOKKWAUDIO },
        { "ref",                "function_arguments_ref.html",          TokenCode::TOKREF },
        { "optional",           "function_parameters_optional.html",    TokenCode::TOKOPTIONAL },
        { "HashMap",            "HashMap_statement.html",               TokenCode::TOKKWHASHMAP },
        // TODO_DISABLED_FOR_CSPRO77 { "DataSource",         "ENGINECR_TODO.html",                   TokenCode::TOKKWDATASOURCE },
        { "Image",              "Image_statement.html",                 TokenCode::TOKKWIMAGE },
        { "Document",           "Document_statement.html",              TokenCode::TOKKWDOCUMENT },
        { "Geometry",           "Geometry_statement.html",              TokenCode::TOKKWGEOMETRY },
        { "persistent",         "persistent_modifier.html",             TokenCode::TOKPERSISTENT },
        { "declare",            "declare_modifier.html",                TokenCode::TOKDECLARE },
        { "StringWriter",       "StringWriter_statement.html",          TokenCode::TOKKWSTRINGWRITER },
        // VIDEO_TODO_RESTORE_FOR_CSPRO8X { "Video",              "Video_statement.html",                 TokenCode::TOKKWVIDEO },
    };
}


const ReservedWordsTable<KeywordDetails>& KeywordTable::GetKeywords()
{
    static const ReservedWordsTable<KeywordDetails> keyword_table(cs::span<const KeywordDetails>(Keywords, Keywords + _countof(Keywords)));
    return keyword_table;
}


bool KeywordTable::IsKeyword(const std::string_view text_sv, const KeywordDetails** keyword_details/* = nullptr*/)
{
    return GetKeywords().IsEntry(text_sv, keyword_details);
}


std::optional<double> KeywordTable::GetKeywordConstant(const TokenCode token_code)
{
    switch( token_code )
    {
        case TokenCode::TOKFALSE:       return 0;
        case TokenCode::TOKTRUE:        return 1;
        case TokenCode::TOKMISSING:     return MISSING;
        case TokenCode::TOKDEFAULT:     return DEFAULT;
        case TokenCode::TOKNOTAPPL:     return NOTAPPL;
        case TokenCode::TOKREFUSED:     return REFUSED;
        case TokenCode::TOKADD:         return 1;
        case TokenCode::TOKMODIFY:      return 2;
        case TokenCode::TOKVERIFY:      return 3;
        default:                        return std::nullopt;
    }
}


const char* KeywordTable::GetKeywordName(const TokenCode token_code)
{
    return GetKeywords().GetName(
        [token_code](const KeywordDetails& keyword_details)
        {
            return ( keyword_details.token_code == token_code );
        });
}
