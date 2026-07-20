// =============================================================================
//  IGenerator.h
//  Future-proofing seam (§19): every future track generator (bassline,
//  counter-melody, arpeggiator, generative pads, probability engine...)
//  implements this interface and registers itself. GeneratedPattern already
//  carries `extraTracks` for their output — no refactor needed to extend.
//
//  Nothing is registered in v1; the seam exists so v2 can plug in cleanly.
// =============================================================================
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../Model/GeneratedPattern.h"
#include "MoodEngine.h"
#include "RandomEngine.h"
#include "ScaleEngine.h"

namespace acai
{
    struct GeneratorContext
    {
        const MusicalContext& music;
        const MoodProfile&    mood;
        const TimeSignature&  timeSig;
        int barsPerChord = 2;
        int lengthTicks  = 0;
    };

    class IGenerator
    {
    public:
        virtual ~IGenerator() = default;

        /// Stable identifier, also used as the extraTracks key ("bass", ...).
        virtual std::string name() const = 0;

        /// Generate a track against the already-generated pattern.
        virtual std::vector<Note> generateTrack (const GeneratedPattern& soFar,
                                                 const GeneratorContext&,
                                                 RandomEngine& rng) = 0;
    };

    /// Minimal registry: v2 generators self-register at static-init time.
    class GeneratorRegistry
    {
    public:
        static GeneratorRegistry& instance()
        {
            static GeneratorRegistry r;
            return r;
        }

        void add (std::unique_ptr<IGenerator> gen)
        {
            generators_.push_back (std::move (gen));
        }

        const std::vector<std::unique_ptr<IGenerator>>& all() const
        {
            return generators_;
        }

    private:
        std::vector<std::unique_ptr<IGenerator>> generators_;
    };
}
