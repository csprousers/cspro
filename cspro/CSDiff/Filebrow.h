#pragma once

#include <zUtilF/DialogValidators.h>

class CCSDiffDoc;


class CFilesBrow : public CDialog
{
public:
    CFilesBrow(CCSDiffDoc* pDoc, PFF& pff, CWnd* pParent = nullptr);   // standard constructor

    enum { IDD = IDD_FILEBROW };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK();
    void OnListbrow();
    void OnInpbrow();
    void OnRefbrow();
    void OnChangeInputfile();
    void OnChangeListfile();
    void OnChangeReferencefile();

private:
    void OnDataBrowse(ConnectionString& connection_string, const ConnectionString& other_connection_string);

    void EnableDisable();

private:
    CCSDiffDoc* m_pDoc;
    DiffSpec& m_diffSpec;
    PFF& m_pff;

    ConnectionString m_inputConnectionString;
    ConnectionString m_referenceConnectionString;
    std::string m_listingFilePath;
    DiffSpec::DiffMethod m_diffMethod;
    RadioEnumHelper<DiffSpec::DiffMethod> m_diffMethodRadioEnumHelper;
    DiffSpec::DiffOrder m_diffOrder;
    RadioEnumHelper<DiffSpec::DiffOrder> m_diffOrderRadioEnumHelper;
};
