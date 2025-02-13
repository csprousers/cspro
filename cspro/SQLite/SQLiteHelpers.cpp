#ifdef WASM
#include <engine/StandardSystemIncludes.h>
#endif
#include "SQLite.h"
#include "SQLiteHelpers.h"
#include <stddef.h>
#include <assert.h>
#include <ctype.h>


namespace SQLiteHelpers
{
    constexpr const char* CreateTable = "CREATE TEMPORARY TABLE IF NOT EXISTS `cspro_sqlite_helpers` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Value` TEXT NOT NULL) WITHOUT ROWID;";
    constexpr const char* InsertRow = "INSERT OR REPLACE INTO `cspro_sqlite_helpers` (`Key`, `Value`) VALUES ( ?, ? );";
    constexpr const char* ExistsRow = "SELECT 1 FROM `cspro_sqlite_helpers` WHERE `Key` = ? LIMIT 1;";

    void SetTemporaryKeyValuePair(sqlite3* db,const char* lpszKey,const char* lpszValue)
    {
        if( sqlite3_exec(db,CreateTable,NULL,NULL,NULL) == SQLITE_OK )
        {
            sqlite3_stmt* stmt = nullptr;

            if( sqlite3_prepare_v2(db,InsertRow,-1,&stmt,nullptr) == SQLITE_OK )
            {
                sqlite3_bind_text(stmt,1,lpszKey,-1,SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt,2,lpszValue,-1,SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }

    bool TemporaryKeyValuePairExists(sqlite3* db,const char* lpszKey)
    {
        bool bExists = false;
        sqlite3_stmt* stmt = nullptr;

        if( sqlite3_prepare_v2(db,ExistsRow,-1,&stmt,nullptr) == SQLITE_OK )
        {
            sqlite3_bind_text(stmt,1,lpszKey,-1,SQLITE_TRANSIENT);

            bExists = ( sqlite3_step(stmt) == SQLITE_ROW );

            sqlite3_finalize(stmt);
        }

        return bExists;
    }


    void AddSqlStatement(std::vector<std::string>& aSqlStatements,const std::string& sSql,size_t iStatementStart,size_t iStatementEnd)
    {
        assert(iStatementStart <= iStatementEnd);
        assert(iStatementEnd < sSql.length());

        std::string sSqlStatement = sSql.substr(iStatementStart,iStatementEnd - iStatementStart + 1);

        // only add the string if it isn't blank
        for( auto ch : sSqlStatement )
        {
            if( !isspace(ch) )
            {
                aSqlStatements.push_back(sSqlStatement);
                break;
            }
        }
    }

    std::vector<std::string> SplitSqlStatement(const std::string& sSql)
    {
        const char SingleQuoteChar = '\'';
        const char DoubleQuoteChar = '"';
        const char SemicolonChar = ';';

        size_t iStatementStart = 0;
        const char* pDelimCharacter = nullptr;
        std::vector<std::string> aSqlStatements;

        for( size_t i = 0; i < sSql.length(); i++ )
        {
            char ch = sSql[i];

            if( ( pDelimCharacter != nullptr ) && ( ch == *pDelimCharacter ) )
            {
                if( ( ( i + 1 ) < sSql.length() ) && ( sSql[i + 1] ) == *pDelimCharacter )
                {
                    // an escaped quote
                }

                else
                {
                    pDelimCharacter = nullptr; // the end of the quote
                }
            }

            else if( ( pDelimCharacter == nullptr ) && ( ch == SemicolonChar ) )
            {
                AddSqlStatement(aSqlStatements,sSql,iStatementStart,i);
                iStatementStart = i + 1;
            }

            else if( ch == SingleQuoteChar )
            {
                pDelimCharacter = &SingleQuoteChar;
            }

            else if( ch == DoubleQuoteChar )
            {
                pDelimCharacter = &DoubleQuoteChar;
            }
        }

        if( iStatementStart < sSql.length() )
            AddSqlStatement(aSqlStatements,sSql,iStatementStart,sSql.length() - 1);

        return aSqlStatements;
    }


    std::string GetTextPrefixBoundary(std::string text)
    {
        std::string& first_non_match = text;

        // instead of using LIKE ...%, we will do the following so as to not worry about having to escape characters;
        // this could potentially fail if the last character + 1 would go to 0 or become an invalid unicode character
        if( !first_non_match.empty() )
            ++first_non_match[first_non_match.length() - 1];

        return first_non_match;
    }
}
