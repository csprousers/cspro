#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/PointerClasses.h>
#include <zToolsO/RaiiHelpers.h>


class CLASS_DECL_ZTOOLSO SerializerHelper
{
public:
    struct Helper
    {
        virtual ~Helper() { }
    };

    class HelperAndTypeInfo;

    // registers a helper, returning an RAII object controlling the lifetime of the helper
    template<typename T>
    RAII::PushOnVectorAndPopOnDestruction<HelperAndTypeInfo> Register(T helper);

    // gets a helper of the certain type (searching from most-recently-added to last added),
    // returning null if no such helper exists
    template<typename T>
    T* Get();

public:
    class HelperAndTypeInfo
    {
        friend class SerializerHelper;

    private:
        template<typename T>
        HelperAndTypeInfo(T helper);

    private:
        cs::non_null_shared_or_raw_ptr<Helper> m_helper;
        const std::type_info* m_lastCastType;
    };

private:
    std::vector<HelperAndTypeInfo> m_helpers;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
SerializerHelper::HelperAndTypeInfo::HelperAndTypeInfo(T helper)
    :   m_helper(std::move(helper)),
        m_lastCastType(nullptr)
{
}


template<typename T>
RAII::PushOnVectorAndPopOnDestruction<SerializerHelper::HelperAndTypeInfo> SerializerHelper::Register(T helper)
{
    return RAII::PushOnVectorAndPopOnDestruction<HelperAndTypeInfo>(m_helpers, HelperAndTypeInfo(std::move(helper)));
}


template<typename T>
T* SerializerHelper::Get()
{
    const std::type_info* this_type = &typeid(T);
    const auto& itr_crend = m_helpers.crend();

    // to reduce the numeric of dynamic casts, first searched based on the last cast
    for( auto itr = m_helpers.crbegin(); itr != itr_crend; ++itr )
    {
        if( itr->m_lastCastType == this_type )
            return assert_cast<T*>(itr->m_helper.get());
    }

    for( auto itr = m_helpers.rbegin(); itr != itr_crend; ++itr )
    {
        T* cast_helper = dynamic_cast<T*>(itr->m_helper.get());

        if( cast_helper != nullptr )
        {
            itr->m_lastCastType = this_type;
            return cast_helper;
        }
    }

    return nullptr;
}
