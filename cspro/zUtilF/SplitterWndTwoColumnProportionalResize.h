#pragma once


class SplitterWndTwoColumnProportionalResize : public CSplitterWnd
{
public:
    double GetWidthProportion() const          { return m_proportion; }
    void SetWidthProportion(double proportion) { m_proportion = proportion; }

    void SizeProportionally(int cx);

protected:
    void StopTracking(BOOL bAccept) override;

private:
    double m_proportion = 0.15;
};



inline void SplitterWndTwoColumnProportionalResize::SizeProportionally(int cx)
{
    ASSERT(GetColumnCount() == 2 && m_pColInfo != nullptr);
    ASSERT(m_proportion > 0 && m_proportion < 1);

    const int view1_width_new = static_cast<int>(( cx - m_cxSplitterGap ) * m_proportion);
    constexpr int view1_width_min = 10;

    if( view1_width_new > view1_width_min )
    {
        SetColumnInfo(0, view1_width_new, view1_width_min);
        RecalcLayout();
    }
}


inline void SplitterWndTwoColumnProportionalResize::StopTracking(BOOL bAccept)
{
    __super::StopTracking(bAccept);

    // when the splitter is moved, calculate the new proportion
    if( bAccept )
    {
        CRect rect;
        GetClientRect(rect);

        int view1_width_current;
        int view1_width_min;
        GetColumnInfo(0, view1_width_current, view1_width_min);

        int non_view1_width_current = rect.Width() - m_cxSplitterGap;

        double view1_proportion_current = static_cast<double>(view1_width_current) / non_view1_width_current;

        if( view1_proportion_current > 0 && view1_proportion_current < 1)
            m_proportion = view1_proportion_current;
    }
}
