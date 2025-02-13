#pragma once

class LogicStackSaver
{
public:
    LogicStackSaver()
        :   m_nextStatementIndex(0)
    {
    }

    void PushStatement(int statement)
    {
        if( !m_suppressedStatements.empty() )
        {
            ASSERT(m_suppressedStatements.back() == statement);
            m_suppressedStatements.pop_back();
        }

        else
            m_statements.emplace_back(statement);
    }

    void SuppressNextPush(int statement)
    {
        m_suppressedStatements.emplace_back(statement);
    }

    bool Empty() const
    {
        return ( m_nextStatementIndex >= m_statements.size() );
    }

    int PopStatement()
    {
        ASSERT(!Empty());
        return m_statements[m_nextStatementIndex++];
    }

private:
    std::vector<int> m_statements;
    std::vector<int> m_suppressedStatements;
    size_t m_nextStatementIndex;
};
