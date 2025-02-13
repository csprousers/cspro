#pragma once

#include <zFreqO/zFreqO.h>

class CDictItem;


template<typename ValueType, typename CountType>
class ZFREQO_API FrequencyCounter
{
protected:
    FrequencyCounter() { }

public:
    virtual ~FrequencyCounter() { }

    static std::unique_ptr<FrequencyCounter> Create(const CDictItem* dict_item = nullptr);

    virtual void Add(const ValueType& value, CountType count) = 0;

    virtual CountType GetCount(const ValueType& value) const = 0;

    virtual void SetCount(const ValueType& value, CountType count) = 0;

    virtual void ClearCounts() = 0;

    virtual std::map<ValueType, CountType> GetCounts() const = 0;
};
