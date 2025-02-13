#pragma once

#include <zLogicO/Token.h>
#include <engine/CtDef.h>

#if defined(USE_BINARY) // IGNORE_CTAB
#else
class CtStatBase;
#endif

// const to be assigned to CTNODE's m_iNodeType field, see below
const int CTNODE_VARIABLE = 0;
const int CTNODE_OP_MUL = (-TOKMULOP);
const int CTNODE_OP_ADD = (-TOKADDOP);

// Nodes for new-tbd (tbd After Jul, 2001 )
struct CTNODE {
private:
    int     m_iNodeType;             // node type: Variable or Operator
public:
    int     m_iSymbol;               // if node type is variable, here it will be the index
                                     // if node type is operator, here it will be
                                     //   the value = (m_iNodeType - 1)
    int     m_iNumCells;             // # of cells in associated subtree
    struct {
        int m_iInfo1;                // m_iCtOcc,       m_iCtLeft
        int m_iInfo2;                // m_iCtNumRanges, m_iCtRight
    } m_NodeInfo;
    int     m_iParentIndex;     // RHF Jul 24, 2001
    int     m_iCoordNumber;     //  -1, 1,2,...n: Only for Variable nodes. Doesn't depend of dimension.
    int     m_iSeqNumber;       //  -1, 1,2,...n: Only for Variable nodes. Doesn't depend of dimension.
    int     m_iStatType;        // See CStatType

#if defined(USE_BINARY) // IGNORE_CTAB
#else
    CtStatBase* m_pStatBase;
#endif

public:
    int  m_iFlags;  // MASK to indicate special values in ranges ...
                    // will use ct_NOFLAGS, ct_MISSING, ct_DEFAULT, ct_NOTAPPL, ct_REFUSED, ct_UNDEFINED
    bool m_bSpecialFlagsInserted;
    CTNODE() { init(); }
    void init()
    {
        m_NodeInfo.m_iInfo1 = 0;
        m_NodeInfo.m_iInfo2 = 0;
        m_iParentIndex = -1; // taken from // RHF Jul 25, 2001
        m_iFlags = ct_NOFLAGS;
        m_bSpecialFlagsInserted = false;
#if defined(USE_BINARY) // IGNORE_CTAB
#else
        m_pStatBase = 0;
#endif
        m_iSeqNumber = 0;
    }
    void setOperNode( int iNodeType )
    {
        m_iNodeType = iNodeType;
        m_iSymbol   = (iNodeType - 1);
    }
    int getOperator()
    {
        ASSERT(isOperNode());
        return m_iNodeType;
    }
    void setVarNode( int iSymbol )
    {
        ASSERT( iSymbol > 0 );
        m_iNodeType = CTNODE_VARIABLE;
        m_iSymbol   = iSymbol;
    }
    bool isOperNode()
    {
        bool bRet = (m_iNodeType == CTNODE_OP_MUL || m_iNodeType == CTNODE_OP_ADD);
        if( bRet )
            bRet = (m_iSymbol == (m_iNodeType - 1));
        return bRet;
    }
    bool isOperNode(int iTheOper)
    {
        return isOperNode() && ( m_iNodeType == iTheOper );
    }
    bool isVarNode() { return m_iNodeType == CTNODE_VARIABLE && m_iSymbol > 0; }
    bool hasThisVariable( int iSymbol ) { return isVarNode() && iSymbol == m_iSymbol; }

    int getNumCells() { return m_iNumCells; }
    void setNumCells( int iNumCells ) { m_iNumCells = iNumCells; }
    void addCells( int iNumCells ) { m_iNumCells += iNumCells; }

    CTNODE& operator=( CTNODE& other ) // rcl, Jul 2005
    {
        if( &other != this )
        {
            this->m_iNodeType = other.m_iNodeType;
            this->m_iSymbol   = other.m_iSymbol;
            this->setNumCells(other.getNumCells());
            this->m_NodeInfo.m_iInfo1 = other.m_NodeInfo.m_iInfo1;
            this->m_NodeInfo.m_iInfo2 = other.m_NodeInfo.m_iInfo2;
            this->m_iParentIndex = other.m_iParentIndex;
            this->m_iCoordNumber = other.m_iCoordNumber;
            this->m_iSeqNumber = other.m_iSeqNumber;
            this->m_iStatType = other.m_iStatType;
#if defined(USE_BINARY) // IGNORE_CTAB
#else
            this->m_pStatBase = other.m_pStatBase;
#endif
            // Special flags management
            this->m_iFlags = other.m_iFlags;
            this->m_bSpecialFlagsInserted = other.m_bSpecialFlagsInserted;
        }

        return *this;
    }

};

const int NORMAL_RANGE  = 0;
const int SPECIAL_RANGE = 1;
const int TOTAL_RANGE   = 2;

struct CTRANGE {
    double   m_iRangeLow;
    double   m_iRangeHigh;
    int     m_iRangeCollapsed;  // 0 to n. 0 indicates no collapsed // RHF Jul 03, 2001

////// --- Some useful methods ---

    int     getNumCells();
    static
    int  getNumCells( double rLow, double rHigh, int iCollapsed );

    bool fitInside( double rValue );

    bool rangeContainsMe( double rLow, double rHigh );
};

// Stats

// CTNODE_STAT_PERCENT Bits
#define CTSTAT_PERCENT_CELL         1
#define CTSTAT_PERCENT_ROW          2
#define CTSTAT_PERCENT_COLUMN       4
#define CTSTAT_PERCENT_TOTAL        8
#define CTSTAT_PERCENT_LAYER       16
#define CTSTAT_PERCENT_SUBCLASS    32
#define CTSTAT_PERCENT_SUBTABLE    64
#define CTSTAT_PERCENT_TABLE       128
#define CTSTAT_PERCENT_USEASFREQ   256   // When there is no FREQ stat, the acumulators are used as freq for the table total sumation (see ctab.cpp)
#define CTSTAT_PERCENT_COLLAPSED   512 // RHF Jun 17, 2003

// CTNODE_STAT_PROP bits
#define CTSTAT_PROP_TOTAL           1
#define CTSTAT_PROP_PERCENT         2


// Change if CTNODE or CTRANGE are changed
// sizeof(CTNODE)/sizeof(int)
// sizeof(CTRANGE)/sizeof(int)
#define CTNODE_SLOTS    (sizeof(CTNODE)/sizeof(int))
#define CTRANGE_SLOTS   (sizeof(CTRANGE)/sizeof(int))

#define m_iCtLeft           m_NodeInfo.m_iInfo1
#define m_iCtRight          m_NodeInfo.m_iInfo2

#define m_iCtOcc            m_NodeInfo.m_iInfo1 // occurrence number
#define m_iCtNumRanges      m_NodeInfo.m_iInfo2 // No. of range pairs

// Some definitions
#define TBD_MAXDIM          3
#define TBD_MAXDEPTH        2

// See CTRANGE
// #define POS_HIGH       3 // Position in reverse where the high is located
// #define ctcell( __pCtNodeBase,  p ) ( *( __pCtNodeBase + p + POS_NUMCELLS ) )

// The cost of a cast is only being payed at C++ compile time
#define ctcell( __pCtNodeBase,  p )  (((CTNODE*)( __pCtNodeBase + p))->m_iNumCells ) // rcl, May 2005

// Use a map only when total cells is <= MAXCELL_REMAP
#define MAXCELL_REMAP 1000  // Above this amount of cells use GetTableCoordOnLine & GetSubTableCoordOnLine methods
