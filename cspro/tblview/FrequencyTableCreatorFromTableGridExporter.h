#pragma once

struct FrequencyRow;
struct FrequencyTable;


class FrequencyTableCreatorFromTableGridExporter : public CTableGridExporter
{
public:
    struct CreationException { };

    FrequencyTableCreatorFromTableGridExporter();
    ~FrequencyTableCreatorFromTableGridExporter();

    // creates the frequency JSON for a table; throws CreationException on error
    std::wstring CreateFrequencyJson(CTblGrid& table_grid);

    // CTableGridExporter overrides
protected:
    void StartFile(_tostream& os) override;
    void EndFile(_tostream& os) override;
    
    void StartFormats(_tostream& os) override;
    void WriteFormat(_tostream& os, const CFmt& fmt) override;
    void EndFormats(_tostream& os) override;
    
    void StartTable(_tostream& os, int iNumCols) override;
    void StartHeaderRows(_tostream& os) override;
    void EndHeaderRows(_tostream& os) override;
    void EndTable(_tostream& os) override;
    
    void WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle) override;
    void WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle) override;
    
    void StartRow(_tostream& os, int iRow, const CDWordArray& aRowHeaders) override;
    void EndRow(_tostream& os) override;
    
    bool IgnoreFormatting() override;
    
    void WriteCell(_tostream& os, int iCol, int iRow, const CFmt& fmt, const CString& sCellData, const CJoinRegion& join, const CArray<CJoinRegion>& aColHeaders) override;

private:
    std::unique_ptr<FrequencyTable> m_frequencyTable;
    std::unique_ptr<std::vector<std::wstring>> m_titles;
    FrequencyRow* m_currentFrequencyRow;
};
