#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>

class CDataDict;
class PFF;
class PffExecutor;


class ZENGINEO_API LogicPff : public Symbol
{
private:
    LogicPff(const LogicPff& logic_pff);

public:
    LogicPff(std::wstring pff_name);
	
    LogicPff& operator=(const LogicPff& rhs);

    std::shared_ptr<const PFF> GetSharedPff();

    bool Load(std::wstring filename);
    bool Save(std::wstring filename);

    bool IsModified() const { return m_modified; }

    std::wstring GetRunnableFilename();

    std::vector<std::wstring> GetProperties(const std::wstring& property_name);

    void SetProperties(const std::wstring& property_name, const std::vector<std::wstring>& values,
                       std::wstring filename_for_relative_path_evaluation);

    PffExecutor* GetPffExecutor()                                  { return m_pffExecutor.get(); }
    std::shared_ptr<PffExecutor> GetSharedPffExecutor()            { return m_pffExecutor; }
    void SetPffExecutor(std::shared_ptr<PffExecutor> pff_executor) { m_pffExecutor = std::move(pff_executor); }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

private:
    void EnsurePffExists();

private:
    std::shared_ptr<PFF> m_pff;
    bool m_modified;

    std::shared_ptr<PffExecutor> m_pffExecutor;
};
