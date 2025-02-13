#pragma once

// Additional structures to support table arithmetic & others
typedef struct {
    int     m_iNodeId;          // For variables SYMTindex, For * -index
    int     m_iCtNode;          // ctnode index
    int     m_iPrevCells;       // No. of preceeding cells
} CTTERM;
