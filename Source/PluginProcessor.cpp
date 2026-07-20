// =============================================================================
//  PluginProcessor.cpp — full integration.
// =============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IO/PatternState.h"
#include "Playback/PlaybackSequence.h"

using namespace acai;

AmbientComposerAudioProcessor::AmbientComposerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    // Seed the master RNG from a non-deterministic source (message thread only).
    std::random_device rd;
    seedRng_.seed (((uint64_t) rd() << 32) ^ (uint64_t) rd()
                   ^ (uint64_t) juce::Time::getHighResolutionTicks());
    currentSeeds_ = Seeds::fromMaster (seedRng_());
}

// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    previewPlayer_.prepare (sampleRate, samplesPerBlock);
    previewPlayer_.setBpm (apvts.getRawParameterValue (ParamIDs::bpm)->load());
}

void AmbientComposerAudioProcessor::releaseResources()
{
    previewPlayer_.releaseResources();
}

bool AmbientComposerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

void AmbientComposerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    // MIDI delivery is via drag/export, not the host bus — never emit live MIDI.
    midi.clear();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    buffer.clear();

    // Keep the preview tempo in sync with the parameter.
    previewPlayer_.setBpm (apvts.getRawParameterValue (ParamIDs::bpm)->load());
    previewPlayer_.process (buffer);
}

// -----------------------------------------------------------------------------
//  Settings snapshot
// -----------------------------------------------------------------------------
GenerationSettings AmbientComposerAudioProcessor::readSettings() const
{
    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    GenerationSettings s;
    s.keyChoice      = (int) raw (ParamIDs::key);
    s.scaleChoice    = (int) raw (ParamIDs::scale);
    s.moodIndex      = (int) raw (ParamIDs::mood);
    s.bpm            = raw (ParamIDs::bpm);
    s.barsPerChord   = ((int) raw (ParamIDs::barsPerChord) == 0) ? 1 : 2;
    s.timeSigIndex   = (int) raw (ParamIDs::timeSig);

    s.chordOctave    = (int) raw (ParamIDs::chordOctave);
    s.voicingWidth   = (int) raw (ParamIDs::voicingWidth);
    s.octaveDoubling = raw (ParamIDs::octaveDoubling) > 0.5f;
    s.sustainOverlap = raw (ParamIDs::sustainOverlap) / 100.0;

    s.melodyStyle    = (int) raw (ParamIDs::melodyStyle);
    s.rhythmStyle    = (int) raw (ParamIDs::rhythmStyle);
    s.complexity     = raw (ParamIDs::complexity) / 100.0;
    s.melodyOctave   = (int) raw (ParamIDs::melodyOctave);

    s.velHumanize    = raw (ParamIDs::velHumanize) / 100.0;
    s.timeHumanize   = raw (ParamIDs::timeHumanize) / 100.0;
    s.gateHumanize   = raw (ParamIDs::gateHumanize) / 100.0;
    return s;
}

bool AmbientComposerAudioProcessor::isSeedLocked() const
{
    return apvts.getRawParameterValue (ParamIDs::seedLock)->load() > 0.5f;
}

uint64_t AmbientComposerAudioProcessor::nextRandomSeed()
{
    return seedRng_();
}

// -----------------------------------------------------------------------------
//  Generation
// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessor::generateBoth()
{
    if (! isSeedLocked())
        currentSeeds_ = Seeds::fromMaster (nextRandomSeed());
    doGenerate (GenerateTarget::Both);
}

void AmbientComposerAudioProcessor::rollNewSeed()
{
    // The dice button explicitly means "new random seed", so it ignores the
    // Lock toggle (which only governs the plain Generate buttons).
    currentSeeds_ = Seeds::fromMaster (nextRandomSeed());
    doGenerate (GenerateTarget::Both);
}

void AmbientComposerAudioProcessor::generateChords()
{
    if (! isSeedLocked())
        currentSeeds_.chord = nextRandomSeed() & 0xFFFFFFFFull;
    // Need a previous pattern to keep the melody; fall back to Both if none.
    doGenerate (getCurrentPattern() ? GenerateTarget::ChordsOnly : GenerateTarget::Both);
}

void AmbientComposerAudioProcessor::generateMelody()
{
    if (! isSeedLocked())
        currentSeeds_.melody = nextRandomSeed() & 0xFFFFFFFFull;
    doGenerate (getCurrentPattern() ? GenerateTarget::MelodyOnly : GenerateTarget::Both);
}

void AmbientComposerAudioProcessor::doGenerate (GenerateTarget target)
{
    const auto settings = readSettings();
    auto prev = getCurrentPattern();

    auto pattern = std::make_shared<GeneratedPattern> (
        generatePattern (settings, currentSeeds_, target, prev.get()));

    currentSeeds_ = pattern->seeds;   // (unchanged, but keep in lockstep)

    pushHistory (pattern);
    publish (pattern);

    if (apvts.getRawParameterValue (ParamIDs::autoPlay)->load() > 0.5f)
        previewPlay();
}

void AmbientComposerAudioProcessor::publish (std::shared_ptr<const GeneratedPattern> pattern)
{
    {
        const juce::ScopedLock sl (patternLock_);
        currentPattern_ = pattern;
    }
    previewPlayer_.setBpm (pattern->bpm);
    previewPlayer_.setSequence (PlaybackSequence::build (*pattern));
    sendChangeMessage();   // repaint the editor (piano roll, seed field, ...)
}

void AmbientComposerAudioProcessor::pushHistory (std::shared_ptr<const GeneratedPattern> pattern)
{
    // drop any redo tail
    if (historyIndex_ + 1 < (int) history_.size())
        history_.erase (history_.begin() + historyIndex_ + 1, history_.end());

    history_.push_back (pattern);
    if ((int) history_.size() > kMaxHistory)
        history_.erase (history_.begin());
    historyIndex_ = (int) history_.size() - 1;
}

void AmbientComposerAudioProcessor::undo()
{
    if (! canUndo()) return;
    --historyIndex_;
    publish (history_[(size_t) historyIndex_]);
    currentSeeds_ = history_[(size_t) historyIndex_]->seeds;
}

void AmbientComposerAudioProcessor::redo()
{
    if (! canRedo()) return;
    ++historyIndex_;
    publish (history_[(size_t) historyIndex_]);
    currentSeeds_ = history_[(size_t) historyIndex_]->seeds;
}

std::shared_ptr<const GeneratedPattern> AmbientComposerAudioProcessor::getCurrentPattern() const
{
    const juce::ScopedLock sl (patternLock_);
    return currentPattern_;
}

bool AmbientComposerAudioProcessor::hasPattern() const
{
    return getCurrentPattern() != nullptr;
}

// -----------------------------------------------------------------------------
//  Preview transport
// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessor::previewPlay()
{
    if (getCurrentPattern()) previewPlayer_.play();
}
void AmbientComposerAudioProcessor::previewStop()          { previewPlayer_.stop(); }
void AmbientComposerAudioProcessor::previewSetLooping (bool b) { previewPlayer_.setLooping (b); }

// -----------------------------------------------------------------------------
//  Seed
// -----------------------------------------------------------------------------
juce::String AmbientComposerAudioProcessor::getSeedString() const
{
    return currentSeeds_.toString();
}

void AmbientComposerAudioProcessor::setSeedString (const juce::String& text)
{
    if (auto parsed = Seeds::parse (text.toStdString()))
    {
        currentSeeds_ = *parsed;
        doGenerate (GenerateTarget::Both);   // reproduce the pasted vibe exactly
    }
}

// -----------------------------------------------------------------------------
//  Favorites
// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessor::addFavorite (const juce::String& name)
{
    auto pattern = getCurrentPattern();
    if (! pattern) return;

    juce::ValueTree fav ("FAVORITE");
    fav.setProperty ("name", name.isNotEmpty() ? name : pattern->progressionText(), nullptr);
    fav.appendChild (apvts.copyState(), nullptr);            // full parameter snapshot
    fav.appendChild (patternState::toValueTree (*pattern), nullptr);
    favorites_.push_back (fav);
}

juce::StringArray AmbientComposerAudioProcessor::getFavoriteNames() const
{
    juce::StringArray names;
    for (const auto& f : favorites_)
        names.add (f.getProperty ("name", "Favorite").toString());
    return names;
}

void AmbientComposerAudioProcessor::recallFavorite (int index)
{
    if (index < 0 || index >= (int) favorites_.size()) return;
    const auto& fav = favorites_[(size_t) index];

    if (auto params = fav.getChildWithName (apvts.state.getType()); params.isValid())
        apvts.replaceState (params.createCopy());

    if (auto pt = fav.getChildWithName (patternState::kPatternType); pt.isValid())
    {
        auto pattern = std::make_shared<GeneratedPattern> (patternState::fromValueTree (pt));
        currentSeeds_ = pattern->seeds;
        pushHistory (pattern);
        publish (pattern);
    }
}

void AmbientComposerAudioProcessor::removeFavorite (int index)
{
    if (index >= 0 && index < (int) favorites_.size())
        favorites_.erase (favorites_.begin() + index);
}

// -----------------------------------------------------------------------------
//  Export
// -----------------------------------------------------------------------------
bool AmbientComposerAudioProcessor::exportToFile (ExportMode mode, const juce::File& file)
{
    auto pattern = getCurrentPattern();
    if (! pattern) return false;
    const int ch = (int) apvts.getRawParameterValue (ParamIDs::midiChannel)->load();
    return MidiExporter::writeToFile (*pattern, mode, ch, file);
}

juce::File AmbientComposerAudioProcessor::writeTempFileForDrag (ExportMode mode)
{
    auto pattern = getCurrentPattern();
    if (! pattern) return {};
    const int ch = (int) apvts.getRawParameterValue (ParamIDs::midiChannel)->load();
    return MidiExporter::writeTempFileForDrag (*pattern, mode, ch);
}

juce::String AmbientComposerAudioProcessor::suggestedFileName (ExportMode mode) const
{
    auto pattern = getCurrentPattern();
    if (! pattern) return "AmbientComposer.mid";
    return MidiExporter::suggestedFileName (*pattern, mode);
}

// -----------------------------------------------------------------------------
//  State save / restore  (parameters + last pattern + seed + favorites)
// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree root ("ACAI_STATE");
    root.appendChild (apvts.copyState(), nullptr);

    // seed
    juce::ValueTree seed ("SEED");
    seed.setProperty ("chord",    (juce::int64) currentSeeds_.chord, nullptr);
    seed.setProperty ("melody",   (juce::int64) currentSeeds_.melody, nullptr);
    seed.setProperty ("humanize", (juce::int64) currentSeeds_.humanize, nullptr);
    root.appendChild (seed, nullptr);

    // last-generated pattern
    if (auto pattern = getCurrentPattern())
        root.appendChild (patternState::toValueTree (*pattern), nullptr);

    // favorites
    juce::ValueTree favs ("FAVORITES");
    for (const auto& f : favorites_)
        favs.appendChild (f.createCopy(), nullptr);
    root.appendChild (favs, nullptr);

    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void AmbientComposerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml) return;

    auto root = juce::ValueTree::fromXml (*xml);

    // Back-compat: an older state was just the bare PARAMS tree.
    if (root.hasType (apvts.state.getType()))
    {
        apvts.replaceState (root);
        return;
    }
    if (! root.hasType ("ACAI_STATE")) return;

    if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid())
        apvts.replaceState (params.createCopy());

    if (auto seed = root.getChildWithName ("SEED"); seed.isValid())
    {
        currentSeeds_.chord    = (uint64_t) (juce::int64) seed.getProperty ("chord", 0);
        currentSeeds_.melody   = (uint64_t) (juce::int64) seed.getProperty ("melody", 0);
        currentSeeds_.humanize = (uint64_t) (juce::int64) seed.getProperty ("humanize", 0);
    }

    favorites_.clear();
    if (auto favs = root.getChildWithName ("FAVORITES"); favs.isValid())
        for (int i = 0; i < favs.getNumChildren(); ++i)
            favorites_.push_back (favs.getChild (i).createCopy());

    if (auto pt = root.getChildWithName (patternState::kPatternType); pt.isValid())
    {
        auto pattern = std::make_shared<GeneratedPattern> (patternState::fromValueTree (pt));
        if (! pattern->empty())
        {
            history_.clear();
            historyIndex_ = -1;
            pushHistory (pattern);
            publish (pattern);
        }
    }
}

// -----------------------------------------------------------------------------
juce::AudioProcessorEditor* AmbientComposerAudioProcessor::createEditor()
{
    return new AmbientComposerAudioProcessorEditor (*this);
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmbientComposerAudioProcessor();
}
