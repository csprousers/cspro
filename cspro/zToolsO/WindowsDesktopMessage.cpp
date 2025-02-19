#include "StdAfx.h"
#include "WindowsDesktopMessage.h"
#include "ObjectCacher.h"
#include <mutex>


struct WindowsDesktopMessage::Data
{
    std::vector<std::variant<std::monostate, SharableString, std::shared_ptr<CacheableObject>>> objects;
    std::mutex objects_mutex;
};


WindowsDesktopMessage::Data WindowsDesktopMessage::m_data;


WPARAM WindowsDesktopMessage::CacheObject(Object object)
{
    std::lock_guard<std::mutex> lock(m_data.objects_mutex);

    // try to reuse a slot
    WPARAM cache_key = 0;

    for( Object& previously_cached_object : m_data.objects )
    {
        if( std::holds_alternative<std::monostate>(previously_cached_object) )
        {
            previously_cached_object = std::move(object);
            return cache_key;
        }

        ++cache_key;
    }

    ASSERT(cache_key == m_data.objects.size());

    m_data.objects.emplace_back(std::move(object));

    return cache_key;
}


template<typename T>
T WindowsDesktopMessage::GetCachedObject(const size_t cache_key)
{
    std::lock_guard<std::mutex> lock(m_data.objects_mutex);

    if( cache_key < m_data.objects.size() )
    {
        std::variant<std::monostate, SharableString, std::shared_ptr<CacheableObject>> object = std::exchange(m_data.objects[cache_key], std::monostate());

        if( std::holds_alternative<T>(object) )
            return std::move(std::get<T>(object));
    }

    return ReturnProgrammingError(T());
}

template CLASS_DECL_ZTOOLSO SharableString WindowsDesktopMessage::GetCachedObject(size_t cache_key);
template CLASS_DECL_ZTOOLSO std::shared_ptr<CacheableObject> WindowsDesktopMessage::GetCachedObject(size_t cache_key);
