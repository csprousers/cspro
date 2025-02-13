#pragma once


template<typename T>
void safe_delete(T*& pSomething)
{
    delete pSomething;
    pSomething = nullptr;
}


template<typename T>
void safe_delete_array(T*& pSomething)
{
    delete[] pSomething;
    pSomething = nullptr;
}


template<typename T>
size_t emplace_back_get_index(std::vector<T>& vector, T object)
{
    size_t index = vector.size();
    vector.emplace_back(std::move(object));
    return index;
}


template<typename T>
void safe_delete_vector_contents(std::vector<T*>& vector)
{
    for( T* t : vector )
        delete t;

    vector.clear();
}


template<typename T>
void safe_delete_vector(std::vector<T*>*& pVector)
{
    if( pVector != nullptr )
    {
        safe_delete_vector_contents(*pVector);
        safe_delete(pVector);
    }
}
