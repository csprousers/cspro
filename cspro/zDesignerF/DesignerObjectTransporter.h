#pragma once

#include <zToolsO/CommonObjectTransporter.h>


class DesignerObjectTransporter : public CommonObjectTransporter
{
protected:
    std::vector<std::shared_ptr<CompilerHelper>>* OnGetCompilerHelperCache() override
    {
        return &m_compilerHelpers;
    }

private:
    std::vector<std::shared_ptr<CompilerHelper>> m_compilerHelpers;
};
