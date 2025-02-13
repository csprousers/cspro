#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "ProgramControl.h"
#include <zEngineO/UserFunction.h>
#include <zLogicO/KeywordTable.h>


double CIntDriver::exendcase(int /*iExpr*/)
{
    ASSERT(Appl.ApplicationType == ModuleType::Batch);

    if( m_iExLevel == 0 )
    {
        issaerror(MessageType::Error, 822, Logic::KeywordTable::GetKeywordName(TOKENDCASE));
        return DEFAULT;
    }

    throw EndCaseProgramControlException();
}


double CIntDriver::exuniverse(int iExpr)
{
    const auto& statement_node = GetNode<STN_NODE>(iExpr);

    // if the condition is false...
    if( ConditionalValueIsFalse(evalexpr(statement_node.arguments[0])) )
    {
        // ... endcase
        if( statement_node.arguments[1] == 1 )
            return exendcase(iExpr);

        // ... or exit the procedure
        else
            throw ExitProgramControlException();
    }

    return 0;
}


double CIntDriver::exskipcase(int /*iExpr*/)
{
    ASSERT(Appl.ApplicationType == ModuleType::Batch);

    if( m_iExLevel == 0 )
    {
        issaerror(MessageType::Error, 822, _T("SKIP CASE"));
        return DEFAULT;
    }

    throw SkipCaseProgramControlException();
}


double CIntDriver::exexit(int iExpr)
{
    const auto& statement_node = GetNode<STN_NODE>(iExpr);

    if( statement_node.arguments[0] != -1 )
    {
        // set the user function's return value
        UserFunction& user_function = GetSymbolUserFunction(statement_node.arguments[0]);
        user_function.SetReturnValue(EvaluateVariantExpression(user_function.GetReturnDataType(), statement_node.arguments[1]));
    }

    throw ExitProgramControlException();
}
