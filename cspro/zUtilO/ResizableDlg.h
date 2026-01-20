#pragma once

#include <zUtilO/zUtilO.h>

class DynamicLayoutControlResizer;
class SettingsDb;
enum class SizingDirection;


// --------------------------------------------------------------------------
// ResizableDlg and ResizableDlgEx are subclasses of CDialog and CDialogEx
// that:
//
// - Prevents the sizing of a dialog to be smaller than its size
//   as constructed in the resource editor.
//
// - If SerializeDialogSize is called, the size is preserved across uses
//   of the dialog, with the size stored in %AppData%/CSPro/States.db.
//
// - The DynamicLayoutResizableDlg subclass combines ResizableDlg
//   with the ability to modify the size of controls that do not
//   respond to Dynamic Layout settings.
// --------------------------------------------------------------------------

template<typename DialogT>
class ResizableDlgBase : public DialogT
{
protected:
    using DialogT::DialogT;

    void SerializeDialogSize(std::string serialize_key);

protected:
    BOOL OnInitDialog() override;

    void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    void OnDestroy();

private:
    CSize m_minimumSize;
    std::shared_ptr<std::tuple<SettingsDb, std::string, std::string>> m_serializationSettings;
};


class CLASS_DECL_ZUTILO ResizableDlg : public ResizableDlgBase<CDialog>
{
public:
    using ParentType = ResizableDlgBase<CDialog>;
    using ParentType::ParentType;

protected:
    DECLARE_MESSAGE_MAP()
};


class CLASS_DECL_ZUTILO ResizableDlgEx : public ResizableDlgBase<CDialogEx>
{
public:
    using ParentType = ResizableDlgBase<CDialogEx>;
    using ParentType::ParentType;

protected:
    DECLARE_MESSAGE_MAP()
};


class CLASS_DECL_ZUTILO DynamicLayoutResizableDlg : public ResizableDlg
{
public:
    using ResizableDlg::ResizableDlg;
    ~DynamicLayoutResizableDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void OnSize(UINT nType, int cx, int cy);

protected:
    BOOL OnInitDialog() override;

    virtual std::vector<std::tuple<CWnd*, SizingDirection>> GetDynamicLayoutControls() = 0;

private:
    std::optional<CSize> m_initialClientSize;
    DynamicLayoutControlResizer* m_dynamicLayoutControlResizer = nullptr;
};
