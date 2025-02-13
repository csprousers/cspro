#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/ObjectCacher.h>

class CommonStore;
class CompilerHelper;
class InterpreterAccessor;
namespace ActionInvoker { class Runtime; }


class CLASS_DECL_ZTOOLSO ObjectTransporter
{
public:
    virtual ~ObjectTransporter() { }

    // sends a message to the main window in an attempt to get an object transporter, returning null if none exists
    static ObjectTransporter* GetInstance();

    // returns the object cacher belonging to the current object transporter;
    // if no object transporter exists, a new object cacher is returned;
    // note that if a new object cacher is returned, its lifetime is controlled by the receiving
    // function, so be aware that if using that function to returns references to data in CacheableObject
    // subclasses, that those data objects should be shared pointers
    static ObjectCacher GetObjectCacher();

    // returns the Common Store, throwing an exception if none is available
    static std::shared_ptr<CommonStore> GetCommonStore();

    // returns the cache of compiler helpers, returning null if none exist
    static std::vector<std::shared_ptr<CompilerHelper>>* GetCompilerHelperCache();

    // returns an accessor to the interpreter, throwing an exception if none is available
    static std::shared_ptr<InterpreterAccessor> GetInterpreterAccessor();

    // returns the Action Invoker runtime, throwing an exception if none is available
    static std::shared_ptr<ActionInvoker::Runtime> GetActionInvokerRuntime();

protected:
    // subclasses should not return null if objects do not exist because the base implementations return null
    virtual std::shared_ptr<CommonStore> OnGetCommonStore();
    virtual std::vector<std::shared_ptr<CompilerHelper>>* OnGetCompilerHelperCache();
    virtual std::shared_ptr<InterpreterAccessor> OnGetInterpreterAccessor();
    virtual std::shared_ptr<ActionInvoker::Runtime> OnGetActionInvokerRuntime();

private:
    // calls GetInstance, and on failure, returns an instance of this base class
    static ObjectTransporter& GetInstanceOrDefault();

private:
    ObjectCacher m_objectCacher;
};
