#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/INTERPRE.H>


template<typename CF>
void CIntDriver::IterateOverScopeChangeNodes(CF callback_function)
{
    auto scope_change_node_indices_crend = m_scopeChangeNodeIndices.crend();

    for( auto scope_change_node_indices_itr = m_scopeChangeNodeIndices.crbegin();
         scope_change_node_indices_itr != scope_change_node_indices_crend;
         ++scope_change_node_indices_itr )
    {
            if( !callback_function(*(*scope_change_node_indices_itr)) )
                return;
    }
}
