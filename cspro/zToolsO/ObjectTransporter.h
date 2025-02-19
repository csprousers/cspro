#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/ObjectCacher.h>

namespace ActionInvoker { class Runtime; }
class ActionInvokerSyncRunner;
class CommonStore;
class CompilerHelper;
class InterpreterAccessor;


class CLASS_DECL_ZTOOLSO ObjectTransporter
{
public:
    virtual ~ObjectTransporter() { }

    // Sends a message to the main window in an attempt to get an object transporter,
    // returning null if none exists.
    static ObjectTransporter* GetInstance();

    // Returns the object cacher belonging to the current object transporter.
    // If no object transporter exists, a new object cacher is returned.
    // Note that if a new object cacher is returned, its lifetime is controlled
    // by the receiving function, so be aware that if using that function to return
    // references to data in CacheableObject subclasses, that those data objects
    // should be shared pointers.
    static ObjectCacher GetObjectCacher();

    // Returns the Common Store, throwing an exception if none is available.
    static std::shared_ptr<CommonStore> GetCommonStore();

    // Returns the cache of compiler helpers, returning null if none exist.
    static std::vector<std::shared_ptr<CompilerHelper>>* GetCompilerHelperCache();

    // Returns an accessor to the interpreter, throwing an exception if none is available.
    static std::shared_ptr<InterpreterAccessor> GetInterpreterAccessor();

    // Returns the Action Invoker runtime, throwing an exception if none is available.
    static std::shared_ptr<ActionInvoker::Runtime> GetActionInvokerRuntime();

    // Instantiates a SyncRunner for use by the Action Invoker, throwing an exception
    // if sychronization runtimes are unavailable.
    static std::unique_ptr<ActionInvokerSyncRunner> CreateActionInvokerSyncRunner();

protected:
    // Subclasses do not need to return null if objects do not exist because
    // the base class implementation returns null.
    virtual std::shared_ptr<CommonStore> OnGetCommonStore();
    virtual std::vector<std::shared_ptr<CompilerHelper>>* OnGetCompilerHelperCache();
    virtual std::shared_ptr<InterpreterAccessor> OnGetInterpreterAccessor();
    virtual std::shared_ptr<ActionInvoker::Runtime> OnGetActionInvokerRuntime();
    virtual std::unique_ptr<ActionInvokerSyncRunner> OnCreateActionInvokerSyncRunner() const;

private:
    // Calls GetInstance, and on failure, returns an instance of this base class.
    static ObjectTransporter& GetInstanceOrDefault();

private:
    ObjectCacher m_objectCacher;
};
