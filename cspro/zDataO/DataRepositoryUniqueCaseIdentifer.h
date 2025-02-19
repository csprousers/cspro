#pragma once

#include <zDataO/DataRepository.h>


class DataRepositoryUniqueCaseIdentifer
{
public:
    enum class Type { Position, Uuid, Key };

    DataRepositoryUniqueCaseIdentifer(double position_in_repository);
    DataRepositoryUniqueCaseIdentifer(Type type, std::string uuid_or_key);

    // Returns the current position of the case.
    // If the case does not exist, DataRepositoryException::CaseNotFound will be thrown.
    double GetPosition(DataRepository& data_repository) const;

private:
    Type m_type;
    std::variant<double, std::string> m_identifier;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline DataRepositoryUniqueCaseIdentifer::DataRepositoryUniqueCaseIdentifer(const double position_in_repository)
    :   m_type(Type::Position),
        m_identifier(position_in_repository)
{
}


inline DataRepositoryUniqueCaseIdentifer::DataRepositoryUniqueCaseIdentifer(const Type type, std::string uuid_or_key)
    :   m_type(type),
        m_identifier(std::move(uuid_or_key))
{
    ASSERT(!std::get<std::string>(m_identifier).empty());
}



inline double DataRepositoryUniqueCaseIdentifer::GetPosition(DataRepository& data_repository) const
{
    if( std::holds_alternative<double>(m_identifier) )
    {
        ASSERT(m_type == Type::Position);
        return std::get<double>(m_identifier);
    }

    else
    {
        std::string key = ( m_type == Type::Key ) ? std::get<std::string>(m_identifier) : std::string();
        std::string uuid = ( m_type == Type::Uuid ) ? std::get<std::string>(m_identifier) : std::string();
        double position_in_repository;

        data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);

        return position_in_repository;
    }
}
