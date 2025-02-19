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
        virtual bool CheckSyntax(const std::string& universe) = 0;
        virtual void ToggleNamesInTree() = 0;
    };

    UniverseDlg(std::shared_ptr<const CDataDict> dictionary, std::string universe, ActionResponder& action_responder, CWnd* pParent = nullptr);
    ~UniverseDlg();

    const std::string& GetUniverse() const { return m_universe; }

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
    void InsertTextToUniverse(std::string text);

private:
    std::shared_ptr<const CDataDict> m_dictionary;
    std::string m_universe;
    ActionResponder& m_actionResponder;

    std::unique_ptr<DictionaryTreeCtrl> m_dictionaryTreeCtrl;
    CListBox m_itemValues;
    std::vector<std::string> m_logicForItemValues;
    std::unique_ptr<CLogicCtrl> m_universeLogicCtrl;
};
