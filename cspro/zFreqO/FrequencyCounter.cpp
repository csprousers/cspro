#include "stdafx.h"
#include "FrequencyCounter.h"


namespace
{
    template<typename ValueType, typename CountType>
    class SimpleFrequencyCounter : public FrequencyCounter<ValueType, CountType>
    {
    public:
        void Add(const ValueType& value, CountType count) override
        {
            auto count_lookup = m_counts.find(value);

            if( count_lookup == m_counts.end() )
            {
                m_counts.try_emplace(value, count);
            }

            else
            {
                count_lookup->second += count;
            }
        }

        CountType GetCount(const ValueType& value) const override
        {
            const auto& count_lookup = m_counts.find(value);
            return ( count_lookup == m_counts.end() ) ? 0 : count_lookup->second;
        }

        void SetCount(const ValueType& value, CountType count) override
        {
            m_counts[value] = count;
        }

        void ClearCounts() override
        {
            m_counts.clear();
        }

        std::map<ValueType, CountType> GetCounts() const override
        {
            return m_counts;
        }

    protected:
        std::map<ValueType, CountType> m_counts;
    };



    template<typename CountType>
    class SmallNumericItemFrequencyCounter : public SimpleFrequencyCounter<double, CountType>
    {
    public:
        static constexpr unsigned MaxNumberDigits = 4;

        SmallNumericItemFrequencyCounter(unsigned number_digits)
        {
            // though negative values could be included
            // with a calculation like this: -1 * (int)std::pow(10, number_digits - 1) + 1
            // these values are so rare in CSPro applications that
            // it's not worth using memory for such values

            m_maxValue = static_cast<unsigned>(std::pow(10, number_digits) - 1);
            m_integerCounts.resize(m_maxValue + 1);
        }

    private:
        inline bool GetIndex(const double& value, unsigned* out_vector_index) const
        {
            *out_vector_index = static_cast<unsigned>(value);

            // check if this is a valid integer in the correct range
            return ( *out_vector_index <= m_maxValue && *out_vector_index == value );
        }

    public:
        void Add(const double& value, CountType count) override
        {
            unsigned vector_index;

            if( GetIndex(value, &vector_index) )
            {
                m_integerCounts[vector_index] += count;
            }

            else
            {
                SimpleFrequencyCounter<double, CountType>::Add(value, count);
            }
        }

        CountType GetCount(const double& value) const override
        {
            unsigned vector_index;

            if( GetIndex(value, &vector_index) )
            {
                return m_integerCounts[vector_index];
            }

            else
            {
                return SimpleFrequencyCounter<double, CountType>::GetCount(value);
            }
        }

        void SetCount(const double& value, CountType count) override
        {
            unsigned vector_index;

            if( GetIndex(value, &vector_index) )
            {
                m_integerCounts[vector_index] = count;
            }

            else
            {
                SimpleFrequencyCounter<double, CountType>::SetCount(value, count);
            }
        }

        void ClearCounts() override
        {
            std::fill(m_integerCounts.begin(), m_integerCounts.end(), 0);

            SimpleFrequencyCounter<double, CountType>::ClearCounts();
        }

        std::map<double, CountType> GetCounts() const override
        {
            std::map<double, CountType> combined_counts = SimpleFrequencyCounter<double, CountType>::m_counts;

            unsigned value = 0;

            for( const CountType& count : m_integerCounts )
            {
                if( count != 0 )
                    combined_counts[value] = count;

                ++value;
            }

            return combined_counts;
        }

    private:
        unsigned m_maxValue;
        std::vector<CountType> m_integerCounts;
    };
}


template<typename ValueType, typename CountType>
std::unique_ptr<FrequencyCounter<ValueType, CountType>> FrequencyCounter<ValueType, CountType>::Create(const CDictItem* dict_item/* = nullptr*/)
{
    // see if the frequency counter is a candidate for small integers
    if constexpr(std::is_same_v<ValueType, double>)
    {
        if( dict_item != nullptr && dict_item->GetContentType() == ContentType::Numeric && dict_item->GetDecimal() == 0 &&
            dict_item->GetLen() <= SmallNumericItemFrequencyCounter<CountType>::MaxNumberDigits )
        {
            // note that SmallNumericItemFrequencyCounter's GetCounts, unlike SimpleFrequencyCounter's,
            // will not return values where the count was 0
            return std::make_unique<SmallNumericItemFrequencyCounter<CountType>>(dict_item->GetLen());
        }
    }

    return std::make_unique<SimpleFrequencyCounter<ValueType, CountType>>();
}


template class FrequencyCounter<double, size_t>;
template class FrequencyCounter<double, double>;
template class FrequencyCounter<std::wstring, size_t>;
template class FrequencyCounter<std::wstring, double>;
