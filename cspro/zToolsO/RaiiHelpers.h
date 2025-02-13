#pragma once


namespace RAII
{
    // --------------------------------------------------------------------------
    // ModifyValueOnDestruction
    // --------------------------------------------------------------------------

    template<typename T>
    class ModifyValueOnDestruction
    {
    public:
        ModifyValueOnDestruction(T& value, T destruction_value)
            :   m_value(&value),
                m_destructionValue(std::move(destruction_value))
        {
        }

        ModifyValueOnDestruction(ModifyValueOnDestruction<T>&& rhs)
            :   m_value(rhs.m_value),
                m_destructionValue(std::move(rhs.m_destructionValue))
        {
            rhs.m_value = nullptr;
        }

        ~ModifyValueOnDestruction()
        {
            if( m_value != nullptr )
                *m_value = m_destructionValue;
        }

    private:
        T* m_value;
        T m_destructionValue;
    };



    // --------------------------------------------------------------------------
    // SetValueAndRestoreOnDestruction
    // --------------------------------------------------------------------------

    template<typename T>
    class SetValueAndRestoreOnDestruction
    {
    public:
        template<typename VT>
        SetValueAndRestoreOnDestruction(T& value, VT&& new_value)
            :   m_modifyValueOnDestruction(value, value)
        {
            value = std::forward<VT>(new_value);
        }

    private:
        ModifyValueOnDestruction<T> m_modifyValueOnDestruction;
    };



    // --------------------------------------------------------------------------
    // PushOnStackAndPopOnDestruction
    // --------------------------------------------------------------------------

    template<typename T>
    class PushOnStackAndPopOnDestruction
    {
    public:
        template<typename VT>
        PushOnStackAndPopOnDestruction(std::stack<T>& values, VT&& value)
            :   m_values(&values)
        {
            m_values->push(std::forward<VT>(value));
        }

        PushOnStackAndPopOnDestruction(const PushOnStackAndPopOnDestruction& rhs) = delete;

        PushOnStackAndPopOnDestruction(PushOnStackAndPopOnDestruction&& rhs)
            :   m_values(rhs.m_values)
        {
            rhs.m_values = nullptr;
        }

        ~PushOnStackAndPopOnDestruction()
        {
            if( m_values != nullptr )
                m_values->pop();
        }

    private:
        std::stack<T>* m_values;
    };



    // --------------------------------------------------------------------------
    // PushOnVectorAndPopOnDestruction
    // --------------------------------------------------------------------------

    template<typename T>
    class PushOnVectorAndPopOnDestruction
    {
    public:
        template<typename VT>
        PushOnVectorAndPopOnDestruction(std::vector<T>& values, VT&& value)
            :   m_values(values)
        {
            m_values.emplace_back(std::forward<VT>(value));
            m_valuesSize = m_values.size();
        }

        PushOnVectorAndPopOnDestruction(const PushOnVectorAndPopOnDestruction& rhs) = delete;

        PushOnVectorAndPopOnDestruction(PushOnVectorAndPopOnDestruction&& rhs) noexcept
            :   m_values(rhs.m_values),
                m_valuesSize(rhs.m_valuesSize)
        {
            rhs.m_valuesSize = 0;
        }

        ~PushOnVectorAndPopOnDestruction()
        {
            if( m_valuesSize != 0 )
            {
                ASSERT(m_valuesSize == m_values.size());
                m_values.pop_back();
            }
        }

        const T& GetValue() const
        {
            ASSERT(m_valuesSize != 0 && m_valuesSize <= m_values.size());
            return m_values[m_valuesSize - 1];
        }

        template<typename VT>
        void SetValue(VT&& value)
        {
            ASSERT(m_valuesSize != 0 && m_valuesSize <= m_values.size());
            m_values[m_valuesSize - 1] = std::forward<VT>(value);
        }

    private:
        std::vector<T>& m_values;
        size_t m_valuesSize;
    };



    // --------------------------------------------------------------------------
    // RunOnDestruction
    // --------------------------------------------------------------------------

    class RunOnDestruction
    {
    public:
        RunOnDestruction(std::function<void()> destruction_function)
            :   m_destructionFunction(std::move(destruction_function))
        {
            ASSERT(m_destructionFunction);
        }

        ~RunOnDestruction()
        {
            m_destructionFunction();
        }

    private:
        std::function<void()> m_destructionFunction;
    };
}
