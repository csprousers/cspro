#pragma once

#include <zUtilO/zUtilO.h>

struct sqlite3;
struct sqlite3_context;
struct sqlite3_value;


class SqlLogicFunctions
{
public:
    // registers the callback functions if they have not already been registered;
    // the function throws exceptions on error
    CLASS_DECL_ZUTILO static void RegisterCallbackFunctions(sqlite3* db, std::function<void()> additional_function_registrar = std::function<void()>());

private:
    // creates the cspro_... functions
    static void RegisterCallbackFunctions_cspro(sqlite3* db);

    static void cspro_timestring(sqlite3_context* context, int iArgC, sqlite3_value** ppArgV);
};
