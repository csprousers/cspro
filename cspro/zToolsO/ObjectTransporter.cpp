#include "StdAfx.h"
#include "ObjectTransporter.h"
#include <zPlatformO/PlatformInterface.h>


ObjectTransporter* ObjectTransporter::GetInstance()
{
#ifdef WIN_DESKTOP
    return reinterpret_cast<ObjectTransporter*>(WindowsDesktopMessage::Send(UWM::ToolsO::GetObjectTransporter));

#else
    BaseApplicationInterface* app_interface = PlatformInterface::GetInstance()->GetApplicationInterface();

    return ( app_interface != nullptr ) ? app_interface->GetObjectTransporter() : 
                                          nullptr;
#endif
}


ObjectTransporter& ObjectTransporter::GetInstanceOrDefault()
{
    ObjectTransporter* object_transporter = GetInstance();

    if( object_transporter != nullptr )
    {
        return *object_transporter;
    }

    else
    {
        static ObjectTransporter dummy_object_transporter;
        return dummy_object_transporter;
    }
}


ObjectCacher ObjectTransporter::GetObjectCacher()
{
    ObjectTransporter* object_transporter = GetInstance();

    return ( object_transporter != nullptr ) ? object_transporter->m_objectCacher :
                                               ObjectCacher();
}


std::shared_ptr<CommonStore> ObjectTransporter::GetCommonStore()
{
    std::shared_ptr<CommonStore> common_store = GetInstanceOrDefault().OnGetCommonStore();

    if( common_store == nullptr )
        throw CSProException("The Common Store is not available in this environment.");

    return common_store;
}


std::vector<std::shared_ptr<CompilerHelper>>* ObjectTransporter::GetCompilerHelperCache()
{
    return GetInstanceOrDefault().OnGetCompilerHelperCache();
}


std::shared_ptr<InterpreterAccessor> ObjectTransporter::GetInterpreterAccessor()
{
    std::shared_ptr<InterpreterAccessor> interpreter_accessor = GetInstanceOrDefault().OnGetInterpreterAccessor();

    if( interpreter_accessor == nullptr )
        throw CSProException("The CSPro logic interpreter is not available in this environment.");

    return interpreter_accessor;
}


std::shared_ptr<ActionInvoker::Runtime> ObjectTransporter::GetActionInvokerRuntime()
{
    std::shared_ptr<ActionInvoker::Runtime> action_invoker_runtime = GetInstanceOrDefault().OnGetActionInvokerRuntime();

    if( action_invoker_runtime == nullptr )
        throw CSProException("The Action Invoker runtime is not available in this environment.");

    return action_invoker_runtime;
}


// --------------------------------------------------------------------------
// dummy implementations
// --------------------------------------------------------------------------

std::shared_ptr<CommonStore> ObjectTransporter::OnGetCommonStore()
{
    return nullptr;
}

std::vector<std::shared_ptr<CompilerHelper>>* ObjectTransporter::OnGetCompilerHelperCache()
{
    return nullptr;
}

std::shared_ptr<InterpreterAccessor> ObjectTransporter::OnGetInterpreterAccessor()
{
    return nullptr;
}

std::shared_ptr<ActionInvoker::Runtime> ObjectTransporter::OnGetActionInvokerRuntime()
{
    return nullptr;
}
