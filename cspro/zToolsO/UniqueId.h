#pragma once

#include <zToolsO/zToolsO.h>


// --------------------------------------------------------------------------
// UniqueId
//
// The UniqueId class creates an object with a value that will be unique
// across a program's execution.
// --------------------------------------------------------------------------

class UniqueId
{
public:
    UniqueId()
        :   m_id(CreateInt())
    {
    }

    bool operator==(const UniqueId& rhs)
    {
        return ( m_id == rhs.m_id );
    }

    bool operator!=(const UniqueId& rhs)
    {
        return ( m_id != rhs.m_id );
    }

    static int CreateInt()
    {
        return m_nextId++;
    }

private:
    int m_id;
    CLASS_DECL_ZTOOLSO static int m_nextId;
};
