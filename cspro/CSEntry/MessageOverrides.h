#pragma once

// 20100518 message overrides

struct MessageOverrides
{
    enum class Mode { NoOverride, SystemControlled, OperatorControlled };

    Mode mode = Mode::NoOverride;
    std::optional<std::wstring> clear_text;
    std::optional<WPARAM> clear_key_code;

    bool ForceSystemControlled() const   { return ( mode == Mode::SystemControlled ); }
    bool ForceOperatorControlled() const { return ( mode == Mode::OperatorControlled ); }
};
