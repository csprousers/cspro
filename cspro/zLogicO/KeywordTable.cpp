#include "stdafx.h"
#include "KeywordTable.h"
#include <zToolsO/Special.h>

using namespace Logic;


namespace
{
    const KeywordDetails Keywords[] =
    {
        { _T("and"),               _T("operators.html"),                    TokenCode::TOKANDOP },
        { _T("or"),                _T("operators.html"),                    TokenCode::TOKOROP },
        { _T("not"),               _T("operators.html"),                    TokenCode::TOKNOTOP },
        { _T("if"),                _T("if_statement.html"),                 TokenCode::TOKIF },
        { _T("then"),              _T("if_statement.html"),                 TokenCode::TOKTHEN },
        { _T("else"),              _T("if_statement.html"),                 TokenCode::TOKELSE },
        { _T("elseif"),            _T("if_statement.html"),                 TokenCode::TOKELSEIF },
        { _T("endif"),             _T("if_statement.html"),                 TokenCode::TOKENDIF },
        { _T("do"),                _T("do_statement.html"),                 TokenCode::TOKDO },
        { _T("enddo"),             _T("do_statement.html"),                 TokenCode::TOKENDDO },
        { _T("while"),             _T("while_statement.html"),              TokenCode::TOKWHILE },
        { _T("box"),               _T("recode_statement.html"),             TokenCode::TOKRECODE },
        { _T("endbox"),            _T("recode_statement.html"),             TokenCode::TOKENDRECODE },
        { _T("recode"),            _T("recode_statement.html"),             TokenCode::TOKRECODE },
        { _T("endrecode"),         _T("recode_statement.html"),             TokenCode::TOKENDRECODE },
        { _T("exit"),              _T("exit_statement.html"),               TokenCode::TOKEXIT },
        { _T("end"),               _T("function_statement.html"),           TokenCode::TOKEND },
        { _T("where"),             nullptr,                                 TokenCode::TOKWHERE },
        { _T("stop"),              _T("stop_function.html"),                TokenCode::TOKSTOP },
        { _T("endcase"),           _T("endcase_statement.html"),            TokenCode::TOKENDCASE }, // GHM 20100310
        { _T("universe"),          _T("universe_statement.html"),           TokenCode::TOKUNIVERSE }, // GHM 20100310
        { _T("skip"),              _T("skip_statement.html"),               TokenCode::TOKSKIP },
        { _T("move"),              _T("move_statement.html"),               TokenCode::TOKMOVE }, // RHF Dec 09, 2003
        { _T("Case"),              _T("Case.html"),                         TokenCode::TOKKWCASE },
        { _T("to"),                nullptr,                                 TokenCode::TOKTO },
        { _T("next"),              _T("next_statement.html"),               TokenCode::TOKNEXT },
        { _T("page"),              nullptr,                                 TokenCode::TOKPAGE },
        { _T("endsect"),           _T("deprecated_features.html"),          TokenCode::TOKENDSECT },
        { _T("endgroup"),          _T("endgroup_statement.html"),           TokenCode::TOKENDSECT }, // RHF Mar 19, 2001
        { _T("reenter"),           _T("reenter_statement.html"),            TokenCode::TOKREENTER },
        { _T("noinput"),           _T("noinput_statement.html"),            TokenCode::TOKNOINPUT },
        { _T("advance"),           _T("advance_statement.html"),            TokenCode::TOKADVANCE },
        { _T("enter"),             _T("enter_statement.html"),              TokenCode::TOKENTER },
        { _T("level"),             nullptr,                                 TokenCode::TOKLEVEL },
        { _T("endlevel"),          _T("endlevel_statement.html"),           TokenCode::TOKENDLEVL },
        { _T("table"),             nullptr,                                 TokenCode::TOKTABLE },
        { _T("stable"),            nullptr,                                 TokenCode::TOKSTABLE },
        { _T("hotdeck"),           nullptr,                                 TokenCode::TOKHOTDECK },
        { _T("mean"),              nullptr,                                 TokenCode::TOKMEAN },
        { _T("smean"),             nullptr,                                 TokenCode::TOKSMEAN },
        { _T("exclude"),           nullptr,                                 TokenCode::TOKEXCLUDE },
        { _T("include"),           nullptr,                                 TokenCode::TOKINCLUDE },
        { _T("break"),             _T("break_statement.html"),              TokenCode::TOKBREAK },
        { _T("Freq"),              _T("Freq_statement_unnamed.html"),       TokenCode::TOKKWFREQ },
        { _T("Frequency"),         _T("Freq_statement_unnamed.html"),       TokenCode::TOKKWFREQ },
        { _T("title"),             nullptr,                                 TokenCode::TOKTITLE },
        { _T("stub"),              nullptr,                                 TokenCode::TOKSTUB },
        { _T("intervals"),         nullptr,                                 TokenCode::TOKINTERVAL },
        { _T("highest"),           nullptr,                                 TokenCode::TOKHIGHEST },
        { _T("lowers"),            nullptr,                                 TokenCode::TOKLOWER },
        { _T("by"),                nullptr,                                 TokenCode::TOKBY },
        { _T("all"),               nullptr,                                 TokenCode::TOKALL },
        { _T("weighted"),          nullptr,                                 TokenCode::TOKWEIGHT },
        { _T("function"),          _T("function_statement.html"),           TokenCode::TOKKWFUNCTION },
        { _T("add"),               nullptr,                                 TokenCode::TOKADD },
        { _T("modify"),            nullptr,                                 TokenCode::TOKMODIFY }, // RHF Nov 13, 2001
        { _T("verify"),            nullptr,                                 TokenCode::TOKVERIFY },
        { _T("crosstab"),          nullptr,                                 TokenCode::TOKKWCTAB },
        { _T("select"),            _T("errmsg_function.html"),              TokenCode::TOKSELECT },
        { _T("for"),               _T("for_statement.html"),                TokenCode::TOKFOR },
        { _T("noprint"),           nullptr,                                 TokenCode::TOKNOPRINT },
        { _T("noauto"),            nullptr,                                 TokenCode::TOKNOAUTO },
        { _T("nobreak"),           nullptr,                                 TokenCode::TOKNOBREAK },
        { _T("nofreq"),            nullptr,                                 TokenCode::TOKNOFREQ },
        { _T("cell"),              nullptr,                                 TokenCode::TOKCELLTYPE },
        { _T("linked"),            nullptr,                                 TokenCode::TOKLINKED },
        { _T("Array"),             _T("Array_statement.html"),              TokenCode::TOKKWARRAY },
        { _T("save"),              nullptr,                                 TokenCode::TOKSAVE }, // JH 3/13/06
        { _T("export"),            _T("export_statement.html"),             TokenCode::TOKEXPORT },
        { _T("case_id"),           nullptr,                                 TokenCode::TOKCASEID },
        { _T("rec_name"),          nullptr,                                 TokenCode::TOKRECNAME }, // For export command
        { _T("rec_type"),          nullptr,                                 TokenCode::TOKRECTYPE }, // For export command
        { _T("group"),             nullptr,                                 TokenCode::TOKKWGROUP },
        { _T("missing"),           _T("special_values.html"),               TokenCode::TOKMISSING },
        { _T("default"),           _T("special_values.html"),               TokenCode::TOKDEFAULT },
        { _T("notappl"),           _T("special_values.html"),               TokenCode::TOKNOTAPPL },
        { _T("preproc"),           _T("preproc_statement.html"),            TokenCode::TOKPREPRO },
        { _T("postproc"),          _T("postproc_statement.html"),           TokenCode::TOKPOSTPRO },
        { _T("tally"),             nullptr,                                 TokenCode::TOKTALLY },
        { _T("postcalc"),          nullptr,                                 TokenCode::TOKPOSTCALC },
                                                                                
        { _T("onfocus"),           _T("onfocus_statement.html"),            TokenCode::TOKONFOCUS },
        { _T("killfocus"),         _T("killfocus_statement.html"),          TokenCode::TOKKILLFOCUS },
        { _T("onoccchange"),       _T("onoccchange_statement.html"),        TokenCode::TOKONOCCCHANGE },
                                                                                
        { _T("set"),               _T("set_attributes_statement.html"),     TokenCode::TOKSET },
        { _T("multiple"),          nullptr,                                 TokenCode::TOKMULTIPLE },
        { _T("endfor"),            _T("for_statement.html"),                TokenCode::TOKENDFOR },
        { _T("in"),                _T("in_operator.html"),                  TokenCode::TOKIN },
        { _T("has"),               _T("has_operator.html"),                 TokenCode::TOKHAS }, // GHM 20120429
        { _T("until"),             _T("do_statement.html"),                 TokenCode::TOKUNTIL },
        { _T("varying"),           _T("do_statement.html"),                 TokenCode::TOKVARYING },
        { _T("numeric"),           _T("numeric_statement.html"),            TokenCode::TOKNUMERIC },
        { _T("alpha"),             _T("alpha_statement.html"),              TokenCode::TOKALPHA },
        { _T("Relation"),          _T("relation_statement.html"),           TokenCode::TOKKWRELATION }, // RHF Sep 19, 2001
        { _T("Item"),              nullptr,                                 TokenCode::TOKKWITEM },  // RHF Jun 14, 2002
        { _T("unit"),              nullptr,                                 TokenCode::TOKUNIT },  // RHF Jun 14, 2002
        { _T("endunit"),           nullptr,                                 TokenCode::TOKENDUNIT },  // RHF Jul 14, 2005
        { _T("stat"),              nullptr,                                 TokenCode::TOKSTAT },  // RHF Jun 14, 2002
        { _T("statistics"),        nullptr,                                 TokenCode::TOKSTAT },  // RHF Jun 14, 2002
        { _T("row"),               nullptr,                                 TokenCode::TOKROW },
        { _T("column"),            nullptr,                                 TokenCode::TOKCOLUMN },
        { _T("layer"),             nullptr,                                 TokenCode::TOKLAYER },
        { _T("tablogic"),          nullptr,                                 TokenCode::TOKTABLOGIC },  // RHF Jul 02, 2002
        { _T("endlogic"),          nullptr,                                 TokenCode::TOKENDLOGIC }, // RHF Jul 02, 2002
        { _T("vset"),              nullptr,                                 TokenCode::TOKKWVSET }, // RHF Jul 03, 2002
                                                                            
        { _T("using"),             _T("sort_function.html"),                TokenCode::TOKUSING },      // Chirag, Sep 11, 2002
        { _T("ascending"),         _T("sort_function.html"),                TokenCode::TOKASCENDING },  // Chirag, Sep 11, 2002
        { _T("descending"),        _T("sort_function.html"),                TokenCode::TOKDESCENDING }, // Chirag, Sep 11, 2002
                                                                                
        { _T("subtable"),          nullptr,                                 TokenCode::TOKSUBTABLE }, // RHF Jul 31, 2002
        { _T("File"),              _T("File_statement.html"),               TokenCode::TOKKWFILE },
                                                                                
        { _T("alias"),             _T("alias_statement.html"),              TokenCode::TOKALIAS },
        { _T("string"),            _T("string_statement.html"),             TokenCode::TOKSTRING },
        { _T("List"),              _T("List_statement.html"),               TokenCode::TOKKWLIST },
        { _T("config"),            _T("config_modifier.html"),              TokenCode::TOKCONFIG },
        { _T("forcase"),           _T("forcase_statement.html"),            TokenCode::TOKFORCASE },
        { _T("ask"),               _T("ask_statement.html"),                TokenCode::TOKASK },
        { _T("sql"),               _T("sqlite_callback_functions.html"),    TokenCode::TOKSQL },
        { _T("ensure"),            _T("ensure_modifier.html"),              TokenCode::TOKENSURE },
        { _T("continue"),          _T("errmsg_function.html"),              TokenCode::TOKCONTINUE },
        { _T("Map"),               _T("Map_statement.html"),                TokenCode::TOKKWMAP },
        { _T("ValueSet"),          _T("ValueSet_statement.html"),           TokenCode::TOKKWVALUESET },
        { _T("false"),             _T("boolean_values.html"),               TokenCode::TOKFALSE },
        { _T("true"),              _T("boolean_values.html"),               TokenCode::TOKTRUE },
        { _T("Pff"),               _T("Pff_statement.html"),                TokenCode::TOKKWPFF },
        { _T("when"),              _T("when_statement.html"),               TokenCode::TOKWHEN },
        { _T("endwhen"),           _T("when_statement.html"),               TokenCode::TOKENDWHEN },
        { _T("SystemApp"),         _T("SystemApp_statement.html"),          TokenCode::TOKKWSYSTEMAPP },
        { _T("refused"),           _T("refused_value.html"),                TokenCode::TOKREFUSED },
        { _T("Audio"),             _T("Audio_statement.html"),              TokenCode::TOKKWAUDIO },
        { _T("ref"),               _T("function_arguments_ref.html"),       TokenCode::TOKREF },
        { _T("optional"),          _T("function_parameters_optional.html"), TokenCode::TOKOPTIONAL },
        { _T("HashMap"),           _T("HashMap_statement.html"),            TokenCode::TOKKWHASHMAP },
        // TODO_DISABLED_FOR_CSPRO77 { _T("DataSource"),        _T("ENGINECR_TODO.html"),                TokenCode::TOKKWDATASOURCE },
        { _T("Image"),             _T("Image_statement.html"),              TokenCode::TOKKWIMAGE },
        { _T("Document"),          _T("Document_statement.html"),           TokenCode::TOKKWDOCUMENT },
        { _T("Geometry"),          _T("Geometry_statement.html"),           TokenCode::TOKKWGEOMETRY },
        { _T("persistent"),        _T("persistent_modifier.html"),          TokenCode::TOKPERSISTENT },
    };
}


const ReservedWordsTable<KeywordDetails>& KeywordTable::GetKeywords()
{
    static const ReservedWordsTable<KeywordDetails> keyword_table(cs::span<const KeywordDetails>(Keywords, Keywords + _countof(Keywords)));
    return keyword_table;
}


bool KeywordTable::IsKeyword(const wstring_view text_sv, const KeywordDetails** keyword_details/* = nullptr*/)
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


const TCHAR* KeywordTable::GetKeywordName(const TokenCode token_code)
{
    return GetKeywords().GetName(
        [token_code](const KeywordDetails& keyword_details)
        {
            return ( keyword_details.token_code == token_code );
        });
}
