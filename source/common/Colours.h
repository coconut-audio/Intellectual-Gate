#pragma once

#include <JuceHeader.h>

namespace Theme
{
    inline const Colour background    { 0xFFB5504A };
    inline const Colour surface       { 0xFFFFFFFF };
    inline const Colour foreground    { 0xFF3D2B1F };
    inline const Colour indicator     { 0xFFB5504A };
    inline const Colour text          { 0xFFFFFFFF };
    inline const Colour textDim       { 0xFF3D2B1F };

    inline Colour indicatorFill()     { return indicator.withAlpha(0.30f); }
    inline Colour gridMajorLine()     { return foreground.withAlpha(0.38f); }
    inline Colour gridMinorLine()     { return foreground.withAlpha(0.13f); }
    inline Colour regionFill()        { return indicator.withAlpha(0.10f); }
}
