#pragma once


// in conjunction with the ObjectTransporter, objects can easily be cached by deriving from CacheableObject

struct CacheableObject
{
    virtual ~CacheableObject() { }
};



class ObjectCacher
{
public:
    ObjectCacher()
        :   m_cachedObjects(std::make_shared<std::map<size_t, std::shared_ptr<CacheableObject>>>())
    {
    }

    template<typename T>
    T* Get()
    {
        auto lookup = m_cachedObjects->find(typeid(T).hash_code());
        return ( lookup != m_cachedObjects->cend() ) ? assert_cast<T*>(lookup->second.get()) :
                                                       nullptr;
    }

    template<typename T, typename... Args>
    T& GetOrCreate(Args&&... args)
    {
        T* object = Get<T>();

        if( object != nullptr )
            return *object;

        return assert_cast<T&>(*m_cachedObjects->try_emplace(typeid(T).hash_code(), std::make_shared<T>(std::forward<Args>(args)...)).first->second);
    }

private:
    std::shared_ptr<std::map<size_t, std::shared_ptr<CacheableObject>>> m_cachedObjects;
};
