#include "StdAfx.h"
#include "SqlLogicFunctions.h"
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteHelpers.h>


void SqlLogicFunctions::RegisterCallbackFunctions(sqlite3* const db, std::function<void()> additional_function_registrar/* = std::function<void()>()*/)
{
    // no need to register the functions if they have already been registered
    constexpr const char* RegisterSqlCallbackFunctionsKey = "RegisterSqlCallbackFunctions";

    if( SQLiteHelpers::TemporaryKeyValuePairExists(db, RegisterSqlCallbackFunctionsKey) )
        return;

    // register the cspro_... functions
    RegisterCallbackFunctions_cspro(db);

    // register any additional functions
    if( additional_function_registrar )
        additional_function_registrar();

    // mark the functions as registered
    SQLiteHelpers::SetTemporaryKeyValuePair(db, RegisterSqlCallbackFunctionsKey, "");
}


void SqlLogicFunctions::RegisterCallbackFunctions_cspro(sqlite3* const db)
{
    auto create_function = [&](const char* const name, void (*function)(sqlite3_context*, int, sqlite3_value**))
    {
        if( sqlite3_create_function(db, name, -1, SQLITE_UTF8, nullptr, function, nullptr, nullptr) != SQLITE_OK )
            throw CSProException("There was an error adding CSPro functions as SQL callback functions.");
    };

    create_function("cspro_timestring", cspro_timestring);
}


void SqlLogicFunctions::cspro_timestring(sqlite3_context* const context, const int iArgC, sqlite3_value** const ppArgV)
{
    // return null if too many arguments
    if( iArgC > 2 )
    {
        sqlite3_result_null(context);
        return;
    }

    // the formatting string comes first, defaulting to "%c"
    const char* const formatter = ( iArgC > 0 ) ? reinterpret_cast<const char*>(sqlite3_value_text(ppArgV[0])) :
                                                  "%c";                                                       

    // followed by the timestamp
    const double timestamp = ( iArgC == 2 ) ? sqlite3_value_double(ppArgV[1]) :
                                              GetTimestamp();

    const std::string timestring = FormatTimestamp(timestamp, formatter);

    sqlite3_result_text(context, timestring.c_str(), timestring.length(), SQLITE_TRANSIENT);
}
