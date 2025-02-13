#pragma once
#include <ostream>
#include <sstream>
#include <rxcpp/rx-lite.hpp>

struct ObservableResponseBody {
    rxcpp::observable<std::string> observable;

    std::ostream& ToStream(std::ostream& os) {
        if (observable != rxcpp::observable<std::string>())
            observable.as_blocking().subscribe_with_rethrow(
                [&os](std::string s) {
                    os << s;
                });
        return os;
    }

    std::string ToString() {
        std::stringstream result;
        ToStream(result);
        return result.str();
    }
};

inline std::ostream& operator<<(std::ostream& os, ObservableResponseBody& body)
{
    return body.ToStream(os);
}
