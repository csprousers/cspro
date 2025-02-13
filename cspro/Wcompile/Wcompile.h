#pragma once

class Application;
class CEngineDriver;
class CEngineArea;
class CEngineCompFunc;
class CompilerCreator;
class CSettings;
struct EngineData;

namespace Logic { class SourceBuffer; class SymbolTable; }

// SERPRO_CALC
class CLinkTable;
// SERPRO_CALC


class CCompIFaz
{
public:
    CEngineDriver*   m_pEngineDriver;
    CEngineArea*     m_pEngineArea;
    EngineData*      m_engineData;
    CSettings*       m_pEngineSettings;
    CEngineCompFunc* m_pEngineCompFunc;

    CCompIFaz(Application* pApplication, CompilerCreator* compiler_creator = nullptr);
    ~CCompIFaz();

    const Logic::SymbolTable& GetSymbolTable() const;

    bool C_CompilerInit( CString* pcsLines, bool& bSomeError );// RHF Jun 12, 2003 Add pcsLines
    void C_CompilerEnd();
    bool C_CompilerCompile(const TCHAR* buffer_text);
    bool C_CompilerCompile(std::shared_ptr<Logic::SourceBuffer> source_buffer);

// SERPRO_CALC
    int   C_GetLinkTables( CArray<CLinkTable*,CLinkTable*>& aLinkTables );
// SERPRO_CALC
};
