#include "stdafx.h"
#include "IncludesCC.h"
#include "CompilerHelper.h"
#include "CommonStoreCompilerHelper.h"
#include "ImputeAutomaticStatCompilerHelper.h"
#include "LoopStackCompilerHelper.h"
#include "PublishDateCompilerHelper.h"
#include <zToolsO/ObjectTransporter.h>


template<typename T>
T& LogicCompiler::GetCompilerHelper()
{
    std::shared_ptr<CompilerHelper> compiler_helper;

    auto search_cache = [&](const std::vector<std::shared_ptr<CompilerHelper>>& compiler_helpers)
    {
        for( const std::shared_ptr<CompilerHelper>& cached_compiler_helper : compiler_helpers )
        {
            if( dynamic_cast<T*>(cached_compiler_helper.get()) != nullptr )
            {
                compiler_helper = cached_compiler_helper;
                return true;
            }
        }

        return false;
    };

    // reuse an existing one if already created by the compiler
    if( search_cache(m_compilerHelpers) )
    {
        ASSERT(compiler_helper->m_compiler == this);
    }

    else
    {
        // if none is found, see if one has been cached at the application level
        std::vector<std::shared_ptr<CompilerHelper>>* application_level_compiler_helpers = ObjectTransporter::GetCompilerHelperCache();

        if( application_level_compiler_helpers != nullptr && search_cache(*application_level_compiler_helpers) )
        {
            // update the compiler and cache it in the current compiler's cache
            compiler_helper->m_compiler = this;
            m_compilerHelpers.emplace_back(compiler_helper);
        }

        else
        {
            // if there is no application level cache, or if one was not found in the cache, create a new one
            compiler_helper = m_compilerHelpers.emplace_back(std::make_shared<T>(*this));

            // cache it at the application level (if applicable)
            if( application_level_compiler_helpers != nullptr && compiler_helper->IsCacheable() )
                application_level_compiler_helpers->emplace_back(compiler_helper);
        }
    }

    return assert_cast<T&>(*compiler_helper);
}

template CommonStoreCompilerHelper& LogicCompiler::GetCompilerHelper<CommonStoreCompilerHelper>();
template ImputeAutomaticStatCompilerHelper& LogicCompiler::GetCompilerHelper<ImputeAutomaticStatCompilerHelper>();
template PublishDateCompilerHelper& LogicCompiler::GetCompilerHelper<PublishDateCompilerHelper>();


LoopStack& LogicCompiler::GetLoopStack()
{
    return GetCompilerHelper<LoopStackCompilerHelper>();
}
