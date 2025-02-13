#pragma once

#include <random>


namespace VectorHelpers
{
    // returns true if the values pointed to by the shared pointer are equal
    template<typename T>
    bool ValueOfSharedPointersIsEqual(const std::vector<std::shared_ptr<T>>& lhs, const std::vector<std::shared_ptr<T>>& rhs);
    

    // appends one or more vectors to a destination vector
    template<typename T, typename... Args>
    void Append(std::vector<T>& destination, std::vector<T> source1, Args... sourceN);

    template<typename T>
    void Append(std::vector<T>& destination, std::vector<T> source);


    // concatenates the vectors, returning the combined vector
    template<typename T, typename... Args>
    std::vector<T> Concatenate(std::vector<T> source1, Args... sourceN);


    // removes duplicates, keeping the first instance of the duplicate
    template<typename T, typename Predicate>
    void RemoveDuplicates(std::vector<T>& values, Predicate predicate);

    template<typename T>
    void RemoveDuplicates(std::vector<T>& values);

    template<typename T>
    void RemoveDuplicateStringsNoCase(std::vector<T>& values);

    // randomizes the specified indices of the vector
    template<typename T, typename RandomEngine>
    void Randomize(std::vector<T>& values, std::vector<size_t> indices_to_randomize, RandomEngine&& random_engine);
}



template<typename T>
bool VectorHelpers::ValueOfSharedPointersIsEqual(const std::vector<std::shared_ptr<T>>& lhs, const std::vector<std::shared_ptr<T>>& rhs)
{
    if( lhs.size() != rhs.size() )
        return false;

    auto lhs_itr = lhs.cbegin();
    auto lhs_end = lhs.cend();
    auto rhs_itr = rhs.cbegin();

    for( ; lhs_itr != lhs_end; ++lhs_itr, ++rhs_itr )
    {
        ASSERT(lhs_itr->get() != nullptr && rhs_itr->get() != nullptr);

        if( !( *(*lhs_itr) == *(*rhs_itr) ) )
            return false;
    }

    return true;
}


template<typename T, typename... Args>
void VectorHelpers::Append(std::vector<T>& destination, std::vector<T> source1, Args... sourceN)
{
    Append(destination, source1);
    Append(destination, sourceN...);
}


template<typename T>
void VectorHelpers::Append(std::vector<T>& destination, std::vector<T> source)
{
    if( source.empty() )
        return;

    if( destination.empty() )
    {
        destination = std::move(source);
    }

    else
    {
        destination.insert(std::end(destination), std::make_move_iterator(std::begin(source)),
                                                  std::make_move_iterator(std::end(source)));
    }
}


template<typename T, typename... Args>
std::vector<T> VectorHelpers::Concatenate(std::vector<T> source1, Args... sourceN)
{
    Append(source1, sourceN...);
    return source1;
}


template<typename T, typename Predicate>
void VectorHelpers::RemoveDuplicates(std::vector<T>& values, Predicate predicate)
{
    if( values.size() < 2 )
        return;

    for( auto values_itr = values.end() - 1; values_itr > values.begin(); --values_itr )
    {
        if( values_itr != std::find_if(values.begin(), values_itr,
                                       [&](const auto& value) { return predicate(value, *values_itr); }) )
        {
            values_itr = values.erase(values_itr);
        }
    }
}


template<typename T>
void VectorHelpers::RemoveDuplicates(std::vector<T>& values)
{
    RemoveDuplicates(values, [](const auto& value1, const auto& value2) { return ( value1 == value2 ); });
}


template<typename T>
void VectorHelpers::RemoveDuplicateStringsNoCase(std::vector<T>& values)
{
    RemoveDuplicates(values, [&](wstring_view value1, wstring_view value2) { return SO::EqualsNoCase(value1, value2); });
}


template<typename T, typename RandomEngine>
void VectorHelpers::Randomize(std::vector<T>& values, std::vector<size_t> indices_to_randomize, RandomEngine&& random_engine)
{
    ASSERT(indices_to_randomize.size() < values.size());

    // randomize the indices
    std::vector<size_t> randomized_indices = indices_to_randomize;
    std::shuffle(randomized_indices.begin(), randomized_indices.end(), std::forward<RandomEngine>(random_engine));

    // reorder the values
    for( size_t i = 0; i < indices_to_randomize.size(); ++i )
    {
        size_t original_index = indices_to_randomize[i];
        size_t randomized_index = randomized_indices[i];

        if( original_index != randomized_index )
        {
            std::swap(values[original_index], values[randomized_index]);

            // modify the future index to reflect the swapped value's new position
            auto swapped_index = std::find(indices_to_randomize.begin() + i, indices_to_randomize.end(), randomized_index);
            ASSERT(swapped_index != indices_to_randomize.end());
            *swapped_index = original_index;
        }
    }
}
