#pragma once

namespace CaseTreeIcons
{
    int GetIconIndex(int id);
    void ForeachIcon(const std::function<void(int)>& function);
    size_t GetNumberIcons();
}
