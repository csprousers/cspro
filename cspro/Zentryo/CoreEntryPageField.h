#pragma once

#include <Zentryo/zEntryO.h>
#include <Zentryo/CapiContentVirtualFileMapping.h>
#include <engine/DEFLD.H>

class CRunAplEntry;
class CDEField;
class CEntryDriver;
class CEngineArea;
class CIntDriver;
class CoreEntryEngineInterface;
class ResponseProcessor;
class ValueSetResponse;


class CLASS_DECL_ZENTRYO CoreEntryPageField
{
public:
    CoreEntryPageField(CoreEntryEngineInterface* core_entry_engine_interface, CDEField* pField);

    CDEField* GetField() const { return m_pOriginalField; }

    bool IsReadOnly() const         { return m_readOnly; }
    const CString& GetLabel() const { return m_label; }

    std::optional<std::wstring> GetQuestionTextUrl() const { return m_capiContentVirtualFileMapping.GetQuestionTextUrl(); }
    std::optional<std::wstring> GetHelpTextUrl() const     { return m_capiContentVirtualFileMapping.GetHelpTextUrl(); }

    CString GetNote() const;

    const CString& GetName() const;
    bool IsMirror() const;

    int GetSymbol() const;
    const C3DIndexes& GetIndexes() const;

    bool IsNumeric() const;
    bool IsAlpha() const { return !IsNumeric(); }

    int GetIntegerPartLength() const;
    int GetFractionalPartLength() const;
    int GetAlphaLength() const;

    const CaptureInfo& GetEvaluatedCaptureInfo() const { return m_evaluatedCaptureInfo; }
    void SetCaptureTypeForSingleFieldDisplay();
    bool IsUpperCase() const;
    bool IsMultiline() const;
    int GetMaxCheckboxSelections() const;
    void GetSliderProperties(double& min_value, double& max_value, double& step) const;
    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const;

    double GetNumericValue() const;
    void SetNumericValue(double value);

    CString GetAlphaValue() const;
    void SetAlphaValue(const CString& value);

    const std::vector<size_t>& GetSelectedResponses() const { return m_selectedIndices; }
    void SetSelectedResponses(const std::vector<size_t>& indices, bool clear_existing_indices = true);
    void SetSelectedResponses(size_t index) { SetSelectedResponses(std::vector<size_t> { index }); }

    void SetNote(CString note);
    void EditNote();

    bool IsFieldFilled() const;

#ifdef _CONSOLE
    std::vector<CString> GetVerboseFieldInformation() const;
    CString GetValueSetName() const;

    CString GetDataBuffer() const { return m_dataBuffer; }
    double GetNumericEngineValue() const { return m_numericEngineValue; }
    double GetNumericDisplayValue() const { return m_numericDisplayValue; }
#endif

private:
    const Logic::SymbolTable& GetSymbolTable() const;

    void RefreshValues();
    void RefreshSelectedResponses();

private:
    CoreEntryEngineInterface* m_coreEntryEngineInterface;
    CRunAplEntry* m_pRunAplEntry;
    CDEField* m_pField;
    CDEField* m_pOriginalField;
    DEFLD m_DeFld;
    VART* m_pVarT;
    const ValueProcessor* m_valueProcessor;

    bool m_readOnly;
    CString m_name;
    CString m_label;
    CapiContentVirtualFileMapping m_capiContentVirtualFileMapping;

    const ValueSet* m_valueSet;
    CaptureInfo m_evaluatedCaptureInfo;
    CString m_captureDateFormat;
    ResponseProcessor* m_responseProcessor;

    CString m_dataBuffer;
    double m_numericEngineValue;
    double m_numericDisplayValue;
    std::vector<size_t> m_selectedIndices;

    // the following are here for convenience reasons and link to objects within m_pRunAplEntry
    CEntryDriver* m_pEntryDriver;
    CEngineArea* m_pEngineArea;
    CIntDriver* m_pIntDriver;
};
