#pragma once

#include <zRuntimeO/WindowsRuntimeView.h>


class RuntimeView : public WindowsRuntimeView
{
    DECLARE_DYNCREATE(RuntimeView)

protected:
    RuntimeView() noexcept = default; // create from serialization only

protected:
    // WindowsRuntimeView overrides
    void SetRuntimeTitle(SharableString description) override;
    void AllRuntimesClosed() override;

protected:
    void OnInitialUpdate() override;
};
