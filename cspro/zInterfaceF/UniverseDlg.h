#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zInterfaceF/DictionaryTreeActionResponder.h>

class CDataDict;
class CLogicCtrl;
class DictionaryTreeCtrl;


class CLASS_DECL_ZINTERFACEF UniverseDlg : public CDialog, public DictionaryTreeActionResponder
{
    DECLARE_DYNAMIC(UniverseDlg)

public:
    class ActionResponder
    {
    public:
        virtual ~ActionResponder() { }
        virtual bool CheckSyntax(const std::wstring& universe) = 0;
        virtual void ToggleNamesInTree() = 0;
    };

    UniverseDlg(std::shared_ptr<const CDataDict> dictionary, std::wstring universe, ActionResponder& action_responder, CWnd* pParent = nullptr);
    ~UniverseDlg();

    const std::wstring& GetUniverse() const { return m_universe; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    BOOL PreTranslateMessage(MSG* pMsg) override;
    void OnOK() override;

    LRESULT OnFocusOnUniverse(WPARAM wParam, LPARAM lParam);

    void OnDictionaryTreeDoubleClick(const DictionaryTreeNode& dictionary_tree_node) override;

    void OnDictionaryTreeSelectionChanged(const DictionaryTreeNode& dictionary_tree_node) override;

    void OnItemValuesDoubleClick();

    void OnClearButtonClicked();
    void OnOperatorButtonClicked(UINT nID);
    void OnParenthesesButtonClicked();

private:
    void InsertTextToUniverse(std::wstring text);

private:
    std::shared_ptr<const CDataDict> m_dictionary;
    std::wstring m_universe;
    ActionResponder& m_actionResponder;

    std::unique_ptr<DictionaryTreeCtrl> m_dictionaryTreeCtrl;
    CListBox m_itemValues;
    std::vector<std::wstring> m_logicForItemValues;
    std::unique_ptr<CLogicCtrl> m_universeLogicCtrl;
};
