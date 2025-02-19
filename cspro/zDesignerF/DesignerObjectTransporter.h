#pragma once

#include <zToolsO/CommonObjectTransporter.h>
#include <zSyncO/SyncRunnerActionInvoker.h>


class DesignerObjectTransporter : public CommonObjectTransporter
{
protected:
    std::vector<std::shared_ptr<CompilerHelper>>* OnGetCompilerHelperCache() override
    {
        return &m_compilerHelpers;
    }

    std::unique_ptr<ActionInvokerSyncRunner> OnCreateActionInvokerSyncRunner() const override 
    {
        return ActionInvokerSyncRunner::Instantiate();
    }

private:
    std::vector<std::shared_ptr<CompilerHelper>> m_compilerHelpers;
};
