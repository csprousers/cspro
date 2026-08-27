#include "stdafx.h"
#include "IncludesRT.h"


Engine::Value LogicInterpreter::ex_inadvance(int /*program_index*/)
{
    return Engine::Value::Bool(
        ( GetEngineAppType() == EngineAppType::Entry &&
          InAdvance_INTERPRETER_DLL_TODO() )
    );
}
