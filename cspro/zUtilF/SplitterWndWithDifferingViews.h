#pragma once


class SplitterWndWithDifferingViews : public CSplitterWnd
{
public:
    BOOL SplitColumn(int cxBefore, CRuntimeClass* pViewClass)
    {
        std::swap(m_pDynamicViewClass, pViewClass);
        BOOL result = __super::SplitColumn(cxBefore);
        std::swap(m_pDynamicViewClass, pViewClass);
        return result;
    }

    BOOL SplitRow(int cyBefore, CRuntimeClass* pViewClass)
    {
        std::swap(m_pDynamicViewClass, pViewClass);
        BOOL result = __super::SplitRow(cyBefore);
        std::swap(m_pDynamicViewClass, pViewClass);
        return result;
    }

    // resizing helpers
    int GetSplitterGap() const { return m_cySplitterGap; }
};
