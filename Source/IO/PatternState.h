// =============================================================================
//  PatternState.h
//  (De)serialises a GeneratedPattern to/from a juce::ValueTree so the full
//  last-generated music (plus its seeds) is stored inside the DAW session and
//  restored exactly on reload. Plugin-only (needs juce_data_structures).
// =============================================================================
#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../Model/GeneratedPattern.h"

namespace acai::patternState
{
    juce::ValueTree toValueTree (const GeneratedPattern&);
    GeneratedPattern fromValueTree (const juce::ValueTree&);

    inline const juce::Identifier kPatternType { "PATTERN" };
}
