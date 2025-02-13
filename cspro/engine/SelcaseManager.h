#pragma once

// manages selcase, nmembers, and for-dictionary operations

class SelcaseDictionaryManager
{
private:
    struct SelcaseNode
    {
        double position_in_repository;
        bool marked;
    };

public:
    SelcaseDictionaryManager()
    {
        Reset();
    }

    void Reset()
    {
        m_numberMarked = 0;
        m_numberUnmarked = 0;
        m_iterateMarked = false;
        m_iterateUnmarked = false;

        // this doesn't actually start an iterator but just resets the "iterator" values
        StartIterator(false, false);

        m_nodes.clear();
    }

    void Add(const std::vector<double>& positions_in_repository, const std::vector<bool>* marked_selections = nullptr)
    {
        ASSERT(marked_selections == nullptr || positions_in_repository.size() == marked_selections->size());

        for( size_t i = 0; i < positions_in_repository.size(); ++i )
        {
            bool marked = ( marked_selections == nullptr ) ? true : marked_selections->at(i);
            m_nodes.push_back({ positions_in_repository[i], marked });

            if( marked )
                ++m_numberMarked;

            else
                ++m_numberUnmarked;
        }
    }

    int GetNumMarked() const   { return m_numberMarked; }
    int GetNumUnmarked() const { return m_numberUnmarked; }
    int GetNumAll() const      { return m_numberMarked + m_numberUnmarked; }

    void StartIterator(bool marked, bool unmarked)
    {
        m_iterateMarked = marked;
        m_iterateUnmarked = unmarked;
        m_iteratorPos = SIZE_MAX;
    }

    bool GetNextPosition(double& position_in_repository)
    {
        while( ++m_iteratorPos < m_nodes.size() )
        {
            const SelcaseNode& node = m_nodes[m_iteratorPos];

            if( ( node.marked && m_iterateMarked ) || ( !node.marked && m_iterateUnmarked ) )
            {
                position_in_repository = node.position_in_repository;
                return true;
            }
        }

        return false;
    }

private:
    int m_numberMarked;
    int m_numberUnmarked;

    bool m_iterateMarked;
    bool m_iterateUnmarked;
    size_t m_iteratorPos;

    std::vector<SelcaseNode> m_nodes;
};


class SelcaseManager
{
public:
    ~SelcaseManager()
    {
        // delete all of the dictionary managers
        for( const auto& itr : m_dictionaryManagers )
            delete itr.second;
    }

    SelcaseDictionaryManager& GetDictionaryManager(int dictionary_symbol_index)
    {
        // search for the appropriate dictionary manager
        auto dictionary_manager_search = m_dictionaryManagers.find(dictionary_symbol_index);

        return ( dictionary_manager_search != m_dictionaryManagers.end() ) ? *dictionary_manager_search->second :
               *(m_dictionaryManagers.insert(std::make_pair(dictionary_symbol_index, new SelcaseDictionaryManager())).first->second);
    }

private:
    std::map<int, SelcaseDictionaryManager*> m_dictionaryManagers;
};
