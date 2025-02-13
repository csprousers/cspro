#pragma once

#include <zEngineF/zEngineF.h>
#include <zUtilO/Interapp.h>
#include <ZBRIDGEO/PifDlg.h>

class Application;
class CSymbolDict;
struct EngineData;
class EngineDictionary;


class CLASS_DECL_ZENGINEF PifInfoPopulator
{
public:
    PifInfoPopulator(const EngineData& engine_data, PFF& pff);

    std::vector<std::shared_ptr<PIFINFO>> GetPifInfo();

private:
    std::shared_ptr<PIFINFO> AddInfo(const FILETYPE& type, const CString& name, const UINT& options);
    void AddDictionary(const EngineDictionary& engine_dictionary, const TCHAR* external_form_name = nullptr);
    void AddDictionary(const CSymbolDict* pDicT, const TCHAR* external_form_name = nullptr);

    template<typename ExtensionCallback>
    CString GetFilenameOrDefaultFilename(const CString& filename, ExtensionCallback extension_callback) const;

private:
    const EngineData* m_engineData;
    PFF& m_pff;
    const Application& m_application;
    std::vector<std::shared_ptr<PIFINFO>> m_pifInfo;
};
