#pragma once
//***************************************************************************
//  File name: TllyStat.h
//
//  Description:
//       Stat classes for tally format.
//
//
//***************************************************************************

#include <zTableO/zTableO.h>
#include <zTableO/Table.h>

///////////////////////////
//
// CTallyVarStatFmt
//
// Base class for all stats.
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmt
{
public:

    // return name of stat as string ("Total", "Counts", ...)
    virtual LPCTSTR GetType() const = 0;

    // display string to show in dialog box (e.g. "Percent (Column)")
    virtual CIMSAString GetDisplayString() const
    {
        return CIMSAString(GetType());
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const = 0;

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        // by default use whatever users default is
        return NUM_DECIMALS_DEFAULT;
    }

    // Read from file.
    virtual bool Build(CSpecFile& specFile, bool bSilent);

    // Write to file.
    virtual void Save(CSpecFile& specFile);

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // reconcile stat ranges on vset change
    virtual bool ReconcileStatRanges(const DictValueSet* pVSet);

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats) = 0;

    virtual bool operator==(const CTallyVarStatFmt& rhs) const;

    bool operator!=(const CTallyVarStatFmt& rhs) const
    {
        return !this->operator==(rhs);
    }

    virtual ~CTallyVarStatFmt()
    {
    }

    virtual CTallyVarStatFmt* Clone() const = 0;
};

///////////////////////////
//
// CTallyVarStatFmtCounts
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtCounts : public CTallyVarStatFmt
{
public:

    virtual LPCTSTR GetType() const
    {
        return _T("Counts");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return DICT_TABVAL;
    }

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtCounts;
    }

};

///////////////////////////
//
// CTallyVarStatFmtTotal
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtTotal : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Total");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_TOTAL_TABVAL;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("TOTAL, "));
    }

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtTotal;
    }
};

// Type of percent
enum CLASS_DECL_ZTABLEO PctType {
    PT_ROW,
    PT_COL,
    PT_TOTAL
};
CLASS_DECL_ZTABLEO CIMSAString PctTypeToString(PctType pt);

CLASS_DECL_ZTABLEO PctType PctTypeFromString(const CIMSAString& s);


///////////////////////////
//
// CTallyVarStatFmtPercent
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtPercent : public CTallyVarStatFmt
{
public:

    // constructor
    CTallyVarStatFmtPercent();

    // display string to show in dialog box (e.g. "Percent (Column)")
    virtual CIMSAString GetDisplayString() const;

    // copy constructor
    CTallyVarStatFmtPercent(const CTallyVarStatFmtPercent& rhs);

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Percents");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_PCT_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    // Read from file.
    virtual bool Build(CSpecFile& specFile, bool bSilent);

    // Write to file.
    virtual void Save(CSpecFile& specFile);

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // comparison
    virtual bool operator==(const CTallyVarStatFmt& rhs) const;

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtPercent(*this);
    }

    PctType GetPctType() const
    {
        return m_pctType;
    }

    CIMSAString GetPctTypeString() const
    {
        return PctTypeToString(m_pctType);
    }

    void SetPctType(PctType t)
    {
        m_pctType = t;
    }

    // whether or not interleaved with counts
    bool GetInterleaved() const
    {
        return m_bInterleaved;
    }

    void SetInterleaved(bool b)
    {
        m_bInterleaved = b;
    }

private:


    PctType m_pctType;
    bool m_bInterleaved;

};


///////////////////////////
//
// CTallyVarStatFmtTotalPercent
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtTotalPercent : public CTallyVarStatFmt
{
public:

    // constructor
    CTallyVarStatFmtTotalPercent();

    // copy constructor
    CTallyVarStatFmtTotalPercent(const CTallyVarStatFmtTotalPercent& rhs);

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Total Percent");
    }

    // display string to show in dialog box (e.g. "Percent (Column)")
    virtual CIMSAString GetDisplayString() const;

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_PCT_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    // Read from file.
    virtual bool Build(CSpecFile& specFile, bool bSilent);

    // Write to file.
    virtual void Save(CSpecFile& specFile);

    // comparison
    virtual bool operator==(const CTallyVarStatFmt& rhs) const;

    // Determine label for tab val based on neighbor stats
    CIMSAString GetTabValLabel(const CArray<CTallyVarStatFmt*>& aNeighborStats, const FOREIGN_KEYS* pForeignKeys);

    // Find index of closest percent, return none
    int FindClosestPercent(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtTotalPercent(*this);
    }

    PctType GetPctType() const
    {
        return m_pctType;
    }

    CIMSAString GetPctTypeString() const
    {
        return PctTypeToString(m_pctType);
    }

    void SetPctType(PctType t)
    {
        m_pctType = t;
    }

    bool GetSameTypeAsPercents() const
    {
        return m_bSameTypeAsPercents;
    }

    void SetSameTypeAsPercents(bool b)
    {
        m_bSameTypeAsPercents = b;
    }

private:

    bool m_bSameTypeAsPercents;
    PctType m_pctType;
};

///////////////////////////
//
// CTallyVarStatFmtMean
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtMean : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Mean");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_MEAN_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("MEAN, "));
    }

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtMean;
    }
};

///////////////////////////
//
// CStatRangeProps
//
// Range props shared between ntiles and median
///////////////////////////
class CLASS_DECL_ZTABLEO CStatRangeProps
{
public:
    // constructor
    CStatRangeProps(double dVarMin, double dVarMax);

    // constructor
    CStatRangeProps(const CStatRangeProps& rhs);

    // set defaults for min and max vals of a given variable
    void SetDefaults(double dVarMin, double dVarMax);
    double ComputeDefaultInterval(double dVarMin, double dVarMax);

    // comparison
    bool operator==(const CStatRangeProps& rhs) const;

    // use value set for intervals, otherwise generate intervals using min, max, interval
    bool GetUseValueSet() const
    {
        return m_bUseValueSet;
    }
    void SetUseValueSet(bool b)
    {
        m_bUseValueSet = b;
    }

    // min for calculating intervals if not using vset
    double GetIntervalsMin() const
    {
        return m_dMin;
    }
    void SetIntervalsMin(double d)
    {
        m_dMin = d;
    }

    // max for calculating intervals if not using vset
    double GetIntervalsMax() const
    {
        return m_dMax;
    }
    void SetIntervalsMax(double d)
    {
        m_dMax = d;
    }
    // interval for calculating intervals if not using vset
    double GetIntervalsStep() const
    {
        return m_dStep;
    }
    void SetIntervalsStep(double d)
    {
        m_dStep = d;
    }

    // get string to display in dlg box
    CIMSAString GetDisplayString() const;

    // return intervals string for code gen
    CString GetIntervalsString();

    // reconcile stat ranges on vset change
    bool Reconcile(const DictValueSet* pVSet);

    // Write to file
    void Save(CSpecFile& specFile);

    // Read line from file, return true if line is read
    bool BuildLine(const CIMSAString& sCmd, const CIMSAString& sArg, int lineNumber);

private:

protected:

    static CIMSAString DblToStringAsIntIfPossible(double d);

    bool m_bUseValueSet;
    double m_dMin; // min/max/step chosen by user
    double m_dMax;
    double m_dStep;
    double m_dVarMin; // min/max for vset chosen
    double m_dVarMax;
};

///////////////////////////
//
// CTallyVarStatFmtMedian
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtMedian : public CTallyVarStatFmt
{
public:

    // constructor - give min and max vals for variable to establish default ranges
    CTallyVarStatFmtMedian(double dVarMin, double dVarMax);

    // copy constructor
    CTallyVarStatFmtMedian(const CTallyVarStatFmtMedian& rhs);

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Median");
    }

     // display string to show in dialog box (e.g. "Percent (Column)")
    virtual CIMSAString GetDisplayString() const;

   // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_MEDIAN_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtMedian(*this);
    }

    // Read from file.
    virtual bool Build(CSpecFile& specFile, bool bSilent);

    // Write to file.
    virtual void Save(CSpecFile& specFile);

    // comparison
    virtual bool operator==(const CTallyVarStatFmt& rhs) const;

    // reconcile stat ranges on vset change
    virtual bool ReconcileStatRanges(const DictValueSet* pVSet)
    {
        return m_rangeProps.Reconcile(pVSet);
    }

    //
    // median specific properties
    //

    // range properties
    CStatRangeProps& GetRangeProps()
    {
        return m_rangeProps;
    }
    const CStatRangeProps& GetRangeProps() const
    {
        return m_rangeProps;
    }

    // continuous vs. discrete median
    bool GetContinuous() const
    {
        return m_bContinuous;
    }
    void SetContinuous(bool b)
    {
        m_bContinuous = b;
    }

private:
    CStatRangeProps m_rangeProps;
    bool m_bContinuous;
};

///////////////////////////
//
// CTallyVarStatFmtMode
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtMode : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Mode");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_MODE_TABVAL;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("MODE, "));
    }

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtMode;
    }
};

///////////////////////////
//
// CTallyVarStatFmtStdDeviation
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtStdDeviation : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Standard Deviation");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_STDDEV_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("STDDEV, "));
    }

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtStdDeviation;
    }
};


///////////////////////////
//
// CTallyVarStatFmtVariance
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtVariance : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Variance");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_VARIANCE_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("VARIANCE, "));
    }

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtVariance;
    }
};

///////////////////////////
//
// CTallyVarStatFmtMin
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtMin : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Minimum");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_MIN_TABVAL;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("MIN, "));
    }

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtMin;
    }
};

///////////////////////////
//
// CTallyVarStatFmtMax
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtMax : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Maximum");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_MAX_TABVAL;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("MAX, "));
    }

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtMax;
    }
};

///////////////////////////
//
// CTallyVarStatFmtNTiles
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtNTiles : public CTallyVarStatFmt
{
public:

    // constructor - pass in default min and max vals for variable to establish default ranges
    CTallyVarStatFmtNTiles(double dVarMin, double dVarMax);

    // copy constructor
    CTallyVarStatFmtNTiles(const CTallyVarStatFmtNTiles& rhs);

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("N-tiles");
    }

    // display string to show in dialog box (e.g. "Percent (Column)")
    virtual CIMSAString GetDisplayString() const;

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_NTILE_TABVAL;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    // Read from file.
    virtual bool Build(CSpecFile& specFile, bool bSilent);

    // Write to file.
    virtual void Save(CSpecFile& specFile);

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // comparison
    virtual bool operator==(const CTallyVarStatFmt& rhs) const;

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtNTiles(*this);
    }

    // reconcile stat ranges on vset change
    virtual bool ReconcileStatRanges(const DictValueSet* pVSet)
    {
        return m_rangeProps.Reconcile(pVSet);
    }

    //
    // ntile specific properties
    //

    // range properties
    CStatRangeProps& GetRangeProps()
    {
        return m_rangeProps;
    }
    const CStatRangeProps& GetRangeProps() const
    {
        return m_rangeProps;
    }

    // number of tiles
    int GetNumTiles() const
    {
        return m_numTiles;
    }
    void SetNumTiles(int n)
    {
        m_numTiles = n;
    }

private:

    CStatRangeProps m_rangeProps;
    int m_numTiles;
};


///////////////////////////
//
// CTallyVarStatFmtProportion
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtProportion : public CTallyVarStatFmt
{
public:

    // constructor
    CTallyVarStatFmtProportion();

    // copy constructor
    CTallyVarStatFmtProportion(const CTallyVarStatFmtProportion& rhs);

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("Proportion");
    }

    // display string to show in dialog box (e.g. "Percent (Column)")
    virtual CIMSAString GetDisplayString() const;

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        ASSERT(FALSE); // this one has multiple tab vals
        return (TABVAL_TYPE) -1;
    }

    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const;

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats);

    // Read from file.
    virtual bool Build(CSpecFile& specFile, bool bSilent);

    // Write to file.
    virtual void Save(CSpecFile& specFile);

    // comparison
    virtual bool operator==(const CTallyVarStatFmt& rhs) const;

    // copy categories for the stat into array of tab vars
    virtual void GetTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                            const DictValueSet* pVSet,
                            const CustSpecialValSettings* custSpecials,
                            const CArray<CTallyVarStatFmt*>& aNeighborStats,
                            const FOREIGN_KEYS* pForeignKeys);

    // reconcile tab vals on stat or dictionary change
    virtual bool ReconcileTabVals(CArray<CTabValue*,CTabValue*>& aTabValue, int iStatIndex,
                                  CArray<CTabValue*,CTabValue*>& aExistingTabValue, const DictValueSet* pVSet,
                                  const CustSpecialValSettings* custSpecials,
                                  const CArray<CTallyVarStatFmt*>& aNeighborStats,
                                  const FOREIGN_KEYS* pForeignKeys);

    // create copy
    virtual CTallyVarStatFmt* Clone() const
    {
        return new CTallyVarStatFmtProportion(*this);
    }

    enum PropType {
        PRT_PERCENT,
        PRT_PERCENT_AND_TOTAL,
        PRT_RATIO,
        PRT_RATIO_AND_TOTAL
    };

    PropType GetProportionType() const
    {
        return m_propType;
    }


    void SetProportionType(PropType t)
    {
        m_propType = t;
    }

    const CIMSAString& GetRange() const
    {
        return m_sRange;
    }

    void SetRange(LPCTSTR sRange)
    {
        m_sRange = sRange;
    }

private:

    void GetTabValLabelsAndTypes(CStringArray& aTabValLabels,
                                 CArray<TABVAL_TYPE>& aTabValTypes,
                                 const DictValueSet* pVSet,
                                 const FOREIGN_KEYS* pForeignKeys);

    PropType m_propType;
    CIMSAString m_sRange;
};

///////////////////////////
//
// CTallyVarStatFmtFactory
//
// Singleton factory for creating stats
//
///////////////////////////
class CLASS_DECL_ZTABLEO CTallyVarStatFmtFactory {
public:
    static CTallyVarStatFmtFactory* GetInstance();

    // create new stat object from TFT string
    CTallyVarStatFmt* Create(LPCTSTR sTFTCommand, double dVarMinVal = -1, double dVarMaxVal = -1);

private:
    CTallyVarStatFmtFactory()
    {
    }
    CTallyVarStatFmtFactory(const CTallyVarStatFmtFactory&);
};
//Savy (R) sampling app 20081208
class CLASS_DECL_ZTABLEO CSampStatFmtR : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("R");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_R_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("R, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtR;
    }
};

class CLASS_DECL_ZTABLEO CSampStatFmtSE : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("SE");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_SE_STABVAL;
    }

    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }
    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("SE, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtSE;
    }
};

//N-UNWE
class CLASS_DECL_ZTABLEO CSampStatFmtNUNWE : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("N-UNWE");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_N_UNWE_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        // by default use whatever users default is
        return NUM_DECIMALS_DEFAULT;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("N-UNWE, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtNUNWE;
    }
};
//N-WEIG
class CLASS_DECL_ZTABLEO CSampStatFmtNWEIG : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("N-WEIG");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_N_WEIG_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        // by default use whatever users default is
        return NUM_DECIMALS_DEFAULT;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("N-WEIG, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtNWEIG;
    }
};
//SER

class CLASS_DECL_ZTABLEO CSampStatFmtSER : public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("SER");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_SER_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("SER, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtSER;
    }
};
//SD
class CLASS_DECL_ZTABLEO CSampStatFmtSD: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("SD");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_SD_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("SD, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtSD;
    }
};
//DEFT
class CLASS_DECL_ZTABLEO CSampStatFmtDEFT: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("DEFT");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_DEFT_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("DEFT, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtDEFT;
    }
};
//ROH
class CLASS_DECL_ZTABLEO CSampStatFmtROH: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("ROH");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_ROH_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("ROH, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtROH;
    }
};
//SE/R
class CLASS_DECL_ZTABLEO CSampStatFmtSESR: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("SE/R");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_SE_R_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("SE/R, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtSESR;
    }
};
//R-2SE
class CLASS_DECL_ZTABLEO CSampStatFmtRNSE: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("R-2SE");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_R_N2SE_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("R-2SE, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtRNSE;
    }
};
//R+2SE
class CLASS_DECL_ZTABLEO CSampStatFmtRPSE: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("R+2SE");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_R_P2SE_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_THREE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("R+2SE, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtRPSE;
    }
};
//SAMP_BASE
class CLASS_DECL_ZTABLEO CSampStatFmtSampBase: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("SAMP_BASE");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_SAMP_BASE_STABVAL;
    }
    //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        // by default use whatever users default is
        return NUM_DECIMALS_DEFAULT;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("SAMP_BASE, "));
    }

    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtSampBase;
    }
};
//B
class CLASS_DECL_ZTABLEO CSampStatFmtB: public CTallyVarStatFmt
{
public:

    // Stat type as string
    virtual LPCTSTR GetType() const
    {
        return _T("B");
    }

    // return type to assign to generated tab vals
    virtual TABVAL_TYPE GetTabValType() const
    {
        return STAT_SAMPLING_B_STABVAL;
    }
        //Savy (R) sampling app 20081222
    // return default num decimal places for this type of stat
    virtual NUM_DECIMALS GetDefaultNumDecimals(CTabValue* pTabVal) const
    {
        UNREFERENCED_PARAMETER(pTabVal);
        return NUM_DECIMALS_ONE;
    }

    // gen code for crosstab statement (stat portion)
    virtual CIMSAString GetStatVarString(const CArray<CTallyVarStatFmt*>& aNeighborStats)
    {
        UNREFERENCED_PARAMETER(aNeighborStats);
        return CIMSAString(_T("B, "));
    }


    virtual CTallyVarStatFmt* Clone() const
    {
        return new CSampStatFmtB;
    }
};
