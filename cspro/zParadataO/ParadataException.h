#pragma once

#include <zToolsO/CSProException.h>

namespace Paradata { class Exception; }


class Paradata::Exception : public CSProException
{
public:
    enum class Type
    {
        OpenDatabase,
        BeginTransaction,
        EndTransaction,
        CreateTable,
        UpdateTable,
        CreateIndex,
        CreatePreparedStatement,
        Insert,
        Version
    };

    Exception(Type type)
        :   CSProException("Paradata problem: %s", TypeToString(type))
    {
    }

private:
    static const char* TypeToString(Type type)
    {
        return ( type == Type::OpenDatabase )            ? "Could not open SQLite file" :
               ( type == Type::BeginTransaction )        ? "Could not begin a transaction" :
               ( type == Type::EndTransaction )          ? "Could not end a transaction" :
               ( type == Type::CreateTable )             ? "Could not create a table" :
               ( type == Type::UpdateTable )             ? "Could not update the table's columns" :
               ( type == Type::CreateIndex )             ? "Could not create an index" :
               ( type == Type::CreatePreparedStatement ) ? "Could not create a prepared statement" :
               ( type == Type::Insert )                  ? "Could not insert a row" :
               ( type == Type::Version )                 ? "Could not read or set the version" :
                                                           "";
    }
};
