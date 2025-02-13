#pragma once
#include <zToolsO/CSProException.h>

namespace Paradata
{
    class Exception : public CSProException
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
            :   CSProException(_T("Paradata problem: %s"), TypeToString(type))
        {
        }

    private:
        static const TCHAR* TypeToString(Type type)
        {
            return ( type == Type::OpenDatabase )            ? _T("Could not open SQLite file") :
                   ( type == Type::BeginTransaction )        ? _T("Could not begin a transaction") :
                   ( type == Type::EndTransaction )          ? _T("Could not end a transaction") :
                   ( type == Type::CreateTable )             ? _T("Could not create a table") :
                   ( type == Type::UpdateTable )             ? _T("Could not update the table's columns") :
                   ( type == Type::CreateIndex )             ? _T("Could not create an index") :
                   ( type == Type::CreatePreparedStatement ) ? _T("Could not create a prepared statement") :
                   ( type == Type::Insert )                  ? _T("Could not insert a row") :
                   ( type == Type::Version )                 ? _T("Could not read or set the version") :
                                                               _T("");
        }
    };
}
