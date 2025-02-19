#pragma once

#include <rxcpp/rx-lite.hpp>

class Case;


class CaseObservable : public rxcpp::observable<std::shared_ptr<Case>>
{
public:
    CaseObservable(const rxcpp::observable<std::shared_ptr<Case>>& o)
        :   rxcpp::observable<std::shared_ptr<Case>>(o)
    {
    }

    CaseObservable(rxcpp::observable<std::shared_ptr<Case>>&& o)
        :   rxcpp::observable<std::shared_ptr<Case>>(o)
    {
    }
};
