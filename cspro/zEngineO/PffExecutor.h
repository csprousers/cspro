#pragma once

#include <zEngineO/zEngineO.h>
#include <zAppO/PFF.h>

class CDataDict;


class ZENGINEO_API PffExecutor
{
public:
    static bool IsValidEmbeddedProperty(const std::wstring& property_name)
    {
        return SO::EqualsOneOfNoCase(property_name, PFF_COMMAND_INPUT_DICT,
                                                    PFF_COMMAND_OUTPUT_DICT);
    }

    bool SetEmbeddedDictionary(const std::wstring& property_name, std::shared_ptr<const CDataDict> dictionary);


    static bool CanExecute(APPTYPE app_type);

    bool Execute(const PFF& pff);

private:
    bool ExecuteCSConcat(const PFF& pff);
    bool ExecuteCSDiff(const PFF& pff);
    bool ExecuteCSIndex(const PFF& pff);
    bool ExecuteCSReFmt(const PFF& pff);
    bool ExecuteCSSort(const PFF& pff);
    bool ExecuteParadataConcat(const PFF& pff);
    bool ExecuteCSPack(const PFF& pff);
    bool ExecuteCSView(const PFF& pff);

private:
    std::shared_ptr<const CDataDict> m_inputDictionary;
    std::shared_ptr<const CDataDict> m_outputDictionary;
};
