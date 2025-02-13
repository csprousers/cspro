//-----------------------------------------------------------------------
//
// CDECLARE.cpp   compiler for declarations in Application procedure
//
//-----------------------------------------------------------------------

#include "StandardSystemIncludes.h"

#ifdef GENCODE
#include "Exappl.h"
#else
#include "Tables.h"
#endif
#include "COMPILAD.H"
#include "COMPUTIL.H"
#include <zEngineO/Compiler/TokenHelper.h>
#include <zAppO/Application.h>
#include <zDictO/DDClass.h>
#include <zListingO/ErrorLister.h>
#include <zLogicO/Preprocessor.h>
#include <zLogicO/SourceBuffer.h>


// RHC INIC Sep 20, 2001
// Relation RelName MultVar/Sec [  TO  MultVar/Sec  PARALLEL
//                                                  LINKED BY exprlog
//                                                  WHERE  exprlog [MULTIPLE]
//                                    [
//                                                    ......
//                                                    ......
//                                     TO  MultVar/Sec  PARALLEL
//                                                      LINKED BY exprlog
//                                                      WHERE  exprlog [MULTIPLE]
//                                    ]
//                                ]

void CEngineCompFunc::CompileRelation()
{
    ASSERT(Tkn == TOKKWRELATION);

    NextTokenOrNewSymbolName();

    if( Tkn != TOKNEWSYMBOL )
        IssueError(102, Tokstr.c_str());

    auto pRelT = std::make_unique<RELT>(Tokstr, GetSymbolTable());

    // read the base symbol
    Symbol* base_symbol = nullptr;
    int base_level = 0;
    NextTokenWithPreference(SymbolType::Section);

    if( IsCurrentTokenVART(*this) )
    {
        VART* pVarT = VPT(Tokstindex);

        if( pVarT->IsArray() )
        {
            base_symbol = pVarT;
            base_level = pVarT->GetLevel();
        }
    }

    else if( Tkn == TOKSECT )
    {
        SECT* pSecT = SPT(Tokstindex);
        base_symbol = pSecT;
        base_level = pSecT->GetLevel();
    }

    // invalid base object
    if( base_symbol == nullptr )
        IssueError(33102);

    pRelT->AddBaseSymbol(base_symbol, MakeRelationWorkVar());

    NextToken();

    // read the related objects
    while( Tkn == TOKTO )
    {
        NextTokenWithPreference(SymbolType::Section);

        Symbol* related_symbol = nullptr;
        int related_level = 0;

        if( IsCurrentTokenVART(*this) )
        {
            VART* pVarT = VPT(Tokstindex);

            if( pVarT->IsArray() )
            {
                related_symbol = pVarT;
                related_level = pVarT->GetLevel();
            }
        }

        else if( Tkn == TOKSECT )
        {
            SECT* pSecT = SPT(Tokstindex);
            related_symbol = pSecT;
            related_level = pSecT->GetLevel();
        }

        // invalid related object
        if( related_symbol == nullptr )
            IssueError(33103);

        // cannot relate to object in lower level
        if( base_level > related_level )
            IssueError(33104);

        int relation_type = 0;
        int relation_expression = 1;

        switch( NextKeyword({ _T("PARALLEL"), _T("LINKED"), _T("WHERE") }) )
        {
            case 0:
            {
                IssueError(33107);
                break;
            }

            case 1: // PARALLEL
            {
                relation_type = USE_INDEX_RELATION;
                NextToken();
                break;
            }

            case 2: // LINKED
            {
                relation_type = USE_LINK_RELATION;
                NextToken();
                IssueErrorOnTokenMismatch(TOKBY, 33105); // BY keyword expected
                NextToken();
                relation_expression = exprlog();
                break;
            }

            case 3: // WHERE
            {
                relation_type = USE_WHERE_RELATION_SINGLE;
                NextToken();
                relation_expression = exprlog();

                if( Tkn == TOKMULTIPLE )
                {
                    relation_type = USE_WHERE_RELATION_MULTIPLE;
                    NextToken();
                }

                break;
            }
        }

        pRelT->AddToSymbol(related_symbol, relation_type, relation_expression, MakeRelationWorkVar());
    }

    m_engineData->AddSymbol(std::move(pRelT));
}


bool CEngineCompFunc::CompileDeclarations()
{
    bool compilation_errors = false;

    for( size_t i = 0; i < MaxNumberLevels; i++ )
        QidVars[i][0] = -1;

    while( Tkn != TOKEOP && Tkn != TOKERROR && GetSyntErr() == 0 )
    {
        try
        {
            if( Tkn == TOKCONFIG || Tkn == TOKPERSISTENT )
            {
                CompileSymbolWithModifiers();
            }

            else if( Tkn == TOKALIAS )
            {
                CompileAlias();
            }

            else if( Tkn == TOKENSURE )
            {
                CompileEnsure();
            }

            else if( Tkn == TOKKWFUNCTION )
            {
                CompileUserFunction();
            }

            else if( Tkn == TOKKWRELATION )
            {
                CompileRelation();
            }

            else if( Tkn == TOKSET )
            {
                CompileSetDeclarations();
            }

            else if( Tkn == TOKKWCTAB )
            {
                compctab(0, CTableDef::Ctab_Crosstab);  // TABLE/CROSSTAB in app
            }

            else if( Tkn == TOKMEAN )
            {
                compctab(0, CTableDef::Ctab_Mean);      // MEAN in app
            }

            else if( Tkn == TOKSMEAN )
            {
                compctab(0, CTableDef::Ctab_SMean);     // SMEAN in app
            }

            else if( Tkn == TOKSTABLE )                 // STABLE
            {
                compctab(0, CTableDef::Ctab_STable);
            }

            else if( Tkn == TOKHOTDECK )
            {
                compctab(0, CTableDef::Ctab_Hotdeck);
            }

            // preprocesor handling
            else if( Tkn == TOKHASH )
            {
                ASSERT(m_preprocessor != nullptr);
                m_preprocessor->ProcessLineDuringCompilation();

                // ProcessLineDuringCompilation will process all tokens on the line but not move to the next token, so we do that here
                NextToken();
            }

            else
            {
                CompileSymbolRouter();
            }
        }

        catch( const Logic::ParserMessage& )
        {
            compilation_errors = true;

            if( SkipBasicTokensUntil(TOKSEMICOLON) )
                NextToken();
        }

        if( Tkn == TOKSEMICOLON )
            NextToken();
    }

    return ( !compilation_errors && GetSyntErr() == 0 );
}


void CEngineCompFunc::CompileSetDeclarations()
{
    size_t set_type = NextKeyword({ _T("EXPLICIT"), _T("IMPLICIT"), _T("TRACE"), _T("ARRAY"), _T("IMPUTE") });

    // invalid option
    if( set_type == 0 )
    {
        IssueError(7006);
    }

    // EXPLICIT
    else if( set_type == 1 )
    {
        IssueWarning(Logic::ParserMessage::Type::DeprecationMajor, 95017);
        NextToken();
    }

    // IMPLICIT
    else if( set_type == 2 )
    {
        IssueError(95018);
    }

    // TRACE
    else if( set_type == 3 )
    {
        CompileSetTrace();
    }

    // SAVE [the deprecated syntax for save arrays: set array save(filename)]
    else if( set_type == 4 )
    {
        IssueError(19099);
    }

    // IMPUTE
    else if( set_type == 5 )
    {
        CompileSetImpute();
    }
}


void CEngineCompFunc::CompileDictRelations()
{
    clearSyntaxErrorStatus();

    for( const DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetSubType() == SymbolSubType::Work )
            continue;

        const CDataDict* dictionary = pDicT->GetDataDict();

        for( const DictRelation& dict_relation : dictionary->GetRelations() )
        {
            std::wstring relation_code = dict_relation.GenerateCode(pDicT->GetName());
            SetSourceBuffer(std::make_shared<Logic::SourceBuffer>(std::move(relation_code)));

            CString csExtraMsg = FormatText(_T(" (check relation '%s' declared in the dictionary)"), dict_relation.GetName().GetString());

            try
            {
                if( rutasync(Appl.GetSymbolIndex()) )
                    ReportError(GetSyntErr(), csExtraMsg.GetString());
            }
            catch(...) { ASSERT(false); }

            ClearSourceBuffer();
            
            if( GetSyntErr() != 0 || ( m_pEngineDriver->GetCompilerErrorLister() != nullptr && m_pEngineDriver->GetCompilerErrorLister()->HasErrors() ) )
            {
                incrementErrors();
                return;
            }
        }
    }
}
