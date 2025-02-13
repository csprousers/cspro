#pragma once

#include <zFormO/zFormO.h>
#include <zUtilO/PortableColor.h>


// enum classes
// --------------------------------------------------

enum class BoxType : int { Etched = 1, Raised, Thin, Thick };

enum class HorizontalAlignment : int { Left, Center, Right };

enum class FieldLabelType : int { DictionaryName, DictionaryLabel, Custom };

enum class FreeMovement : int { Disabled, Horizontal, Vertical };

enum class RosterOrientation : int { Horizontal = 1, Vertical = 2 };

enum class VerticalAlignment : int { Top, Middle, Bottom };



// form defaults
// --------------------------------------------------

namespace FormDefaults
{
    constexpr FieldLabelType FieldLabelType = FieldLabelType::Custom;

    constexpr LONG FormPadding = 25;

    constexpr FreeMovement FreeMovement = FreeMovement::Disabled;

    CLASS_DECL_ZFORMO extern const PortableColor FormBackgoundColor;

    constexpr unsigned QuestionTextHeightDefault = 60;
    constexpr unsigned QuestionTextHeightMax     = 800;

    constexpr RosterOrientation RosterOrientation = RosterOrientation::Horizontal;
}



// ToString functions
// --------------------------------------------------

const TCHAR* ToString(RosterOrientation roster_orientation);
