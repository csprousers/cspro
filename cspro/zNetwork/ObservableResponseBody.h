#pragma once

#include <external/rxcpp/rx-lite.hpp>
#include <ostream>
#include <sstream>


struct ObservableResponseBody
{
    rxcpp::observable<std::string> observable;

    std::ostream& ToStream(std::ostream& os) const;
    std::string ToString() const;
};


std::ostream& operator<<(std::ostream& os, const ObservableResponseBody& body);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::ostream& ObservableResponseBody::ToStream(std::ostream& os) const
{
    if( observable != rxcpp::observable<std::string>() )
    {
        observable.as_blocking().subscribe_with_rethrow(
            [&os](const std::string& s)
            {
                os << s;
            });
    }

    return os;
}


inline std::string ObservableResponseBody::ToString() const
{
    std::stringstream result;
    ToStream(result);
    return result.str();
}


inline std::ostream& operator<<(std::ostream& os, const ObservableResponseBody& body)
{
    return body.ToStream(os);
}
