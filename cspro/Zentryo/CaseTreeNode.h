#pragma once

#include <Zentryo/zEntryO.h>
#include <array>

class CDEFormBase;

/// <summary>
/// Node in the case tree.
/// Standard tree structure with levels that contain groups, groups that contain
/// group occurrences and fields.
/// </summary>

class CLASS_DECL_ZENTRYO CaseTreeNode {
public:

    enum class Type { LEVEL, GROUP_SINGLE, GROUP_WITH_OCCURRENCES, GROUP_OCC, FIELD };
    enum class Color { PARENT, NEVERVISITED, SKIPPED, VISITED, CURRENT, PROTECTED };

    ///<summary>
    ///Create a new node
    ///</summary>
    /// <param name="id">Unique id of tree node.</param>
    /// <param name="formItem">Item/group pointer from CDEForm</param>
    /// <param name="label">Label from form item or occurrence label if set</param>
    /// <param name="value">Value from data file formatted as string. Only displayed for fields.</param>
    /// <param name="type">Node type, determines layout used to display node</param>
    /// <param name="color">State of field, used to determine background color</param>
    /// <param name="index">Full index of field occurrence</param>
    /// <param name="visible">Whether or not to display node and children on screen</param>

    CaseTreeNode(int id, CDEFormBase* formItem, CString label, CString value,
        Type type, Color color, const std::array<int, 3>& index,
        bool visible);

    int getId() const
    {
        return m_id;
    }

    const CString& getName() const;

    CString getLabel() const
    {
        return m_label;
    }

    void setLabel(CString label)
    {
        m_label = label;
    }

    CString getValue() const
    {
        return m_value;
    }

    void setValue(CString value)
    {
        m_value = value;
    }

    Type getType() const
    {
        return m_type;
    }

    Color getColor() const
    {
        return m_color;
    }

    void setColor(Color color)
    {
        m_color = color;
    }

    CDEFormBase* getFormItem() const
    {
        return m_formItem;
    }

    int getFieldSymbol() const;

    const std::array<int, 3>& getIndex() const
    {
        return m_index;
    }

    bool getVisible() const
    {
        return m_visible;
    }

    void setVisible(bool visible)
    {
        m_visible = visible;
    }

    const std::vector<std::shared_ptr<CaseTreeNode>>& getChildren() const
    {
        return m_children;
    }

    std::vector<std::shared_ptr<CaseTreeNode>>& getChildren()
    {
        return m_children;
    }

private:
    const int m_id;
    CDEFormBase* m_formItem;
    CString m_label;
    CString m_value;
    bool m_visible;
    Type m_type;
    Color m_color;
    std::array<int, 3> m_index;
    std::vector<std::shared_ptr<CaseTreeNode>> m_children;
};