#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/FunctionTable.h>
#include <zLogicO/ReservedWordsTable.h>
#include <zLogicO/Symbol.h>


namespace Logic
{
    struct AdditionalReservedWordDetails
    {
        const TCHAR* const name;
        const TCHAR* const help_filename;
    };


    class ZLOGICO_API ReservedWords
    {
    public:
        static bool IsReservedWord(wstring_view text_sv);

        enum class ReservedWordType { Keyword, FunctionNamespace, FunctionNamespaceChild, Function, FunctionDotNotation, AdditionalReservedWord };

        static void ForeachReservedWord(const std::function<void(ReservedWordType, const std::wstring&, const void*)>& callback_function);

        // returns reserved words of all types except for FunctionNamespaceChild + FunctionDotNotation
        static const std::vector<std::wstring>& GetAllReservedWords();

        static const ReservedWordsTable<AdditionalReservedWordDetails>& GetAdditionalReservedWords();

        // special function names are not technically reserved but are defined in this
        // namespace so that the defined case can be returned by the below function
        static const ReservedWordsTable<AdditionalReservedWordDetails>& GetSpecialFunctions();

        // returns nullptr if the text is not a reserved word
        static const TCHAR* GetDefinedCase(wstring_view text_sv, const FunctionDomain& function_domain);

    private:
        template<typename T>
        static const TCHAR* GetDefinedCaseWorker(wstring_view text_sv, const T& function_domain);
    };
}
