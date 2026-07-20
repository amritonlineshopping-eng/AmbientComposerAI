// =============================================================================
//  RandomEngine.h
//  Seedable PRNG + the master-seed -> sub-seed architecture.
//
//  A master seed deterministically derives three sub-seeds (chords, melody,
//  humanize). "Regenerate melody, keep chords" simply re-rolls the melody
//  sub-seed while the chord sub-seed stays put — so every part is individually
//  reproducible and the whole pattern is reproducible from its Seeds triple.
//
//  Header-only, pure C++ (no JUCE).
// =============================================================================
#pragma once

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace acai
{
    /// splitmix64 — tiny, high-quality mixer used to derive sub-seeds from the
    /// master seed. (Recommended seeding scheme for mt19937_64.)
    inline uint64_t splitmix64 (uint64_t x) noexcept
    {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    class RandomEngine
    {
    public:
        explicit RandomEngine (uint64_t seedValue)
            : seed_ (seedValue), gen_ (splitmix64 (seedValue)) {}

        uint64_t seed() const noexcept { return seed_; }

        /// Derive an independent, deterministic child generator. Different
        /// salts give statistically independent streams from the same seed.
        RandomEngine derive (uint64_t salt) const
        {
            return RandomEngine (splitmix64 (seed_ ^ (salt * 0xD1B54A32D192ED03ull)));
        }

        /// Uniform integer in [lo, hi] (inclusive).
        int nextInt (int lo, int hi)
        {
            if (lo >= hi) return lo;
            return std::uniform_int_distribution<int> (lo, hi) (gen_);
        }

        /// Uniform double in [0, 1).
        double nextDouble()
        {
            return std::uniform_real_distribution<double> (0.0, 1.0) (gen_);
        }

        /// Gaussian, mean 0, given stddev — used by the humanizer.
        double nextGaussian (double stddev)
        {
            return std::normal_distribution<double> (0.0, stddev) (gen_);
        }

        /// True with probability p.
        bool chance (double p) { return nextDouble() < p; }

        /// Index drawn from a discrete weighted distribution.
        /// Returns 0 if the weights are empty/degenerate.
        int pickWeighted (const std::vector<double>& weights)
        {
            double total = 0.0;
            for (auto w : weights) total += (w > 0.0 ? w : 0.0);
            if (total <= 0.0 || weights.empty()) return 0;
            double r = nextDouble() * total;
            for (size_t i = 0; i < weights.size(); ++i)
            {
                r -= (weights[i] > 0.0 ? weights[i] : 0.0);
                if (r < 0.0) return (int) i;
            }
            return (int) weights.size() - 1;
        }

        template <typename T>
        const T& choose (const std::vector<T>& items)
        {
            return items[(size_t) nextInt (0, (int) items.size() - 1)];
        }

        uint64_t nextU64() { return gen_(); }

    private:
        uint64_t seed_;
        std::mt19937_64 gen_;
    };

    // -------------------------------------------------------------------------
    // Seeds — the reproducibility contract for one GeneratedPattern.
    //
    // Displayed / copied / pasted as "XXXXXXXX-XXXXXXXX-XXXXXXXX" (hex triple:
    // chord-melody-humanize). Pasting a plain number is also accepted and is
    // treated as a master seed (sub-seeds derived from it).
    // -------------------------------------------------------------------------
    struct Seeds
    {
        // Stored (and generated) as 32-bit values so the displayed string and
        // the internal state round-trip exactly through copy/paste.
        uint64_t chord    = 0;
        uint64_t melody   = 0;
        uint64_t humanize = 0;

        bool operator== (const Seeds&) const = default;

        static Seeds fromMaster (uint64_t master)
        {
            Seeds s;
            s.chord    = splitmix64 (master ^ 0xC1053EEDC1053EE1ull) & 0xFFFFFFFFull;
            s.melody   = splitmix64 (master ^ 0x3E10D15EEDA55EE2ull) & 0xFFFFFFFFull;
            s.humanize = splitmix64 (master ^ 0x4B3A2E5EED0AB003ull) & 0xFFFFFFFFull;
            return s;
        }

        std::string toString() const
        {
            char buf[64];
            std::snprintf (buf, sizeof (buf), "%08llX-%08llX-%08llX",
                           (unsigned long long) chord,
                           (unsigned long long) melody,
                           (unsigned long long) humanize);
            return buf;
        }

        /// Accepts either the triple format above or a single decimal/hex
        /// value (treated as a master seed). Whitespace tolerated.
        static std::optional<Seeds> parse (const std::string& text)
        {
            std::string clean;
            for (char c : text)
                if (! std::isspace ((unsigned char) c)) clean += c;
            if (clean.empty()) return std::nullopt;

            auto hexVal = [] (const std::string& s) -> std::optional<uint64_t>
            {
                if (s.empty() || s.size() > 16) return std::nullopt;
                uint64_t v = 0;
                for (char c : s)
                {
                    int d;
                    if (c >= '0' && c <= '9')      d = c - '0';
                    else if (c >= 'a' && c <= 'f') d = 10 + c - 'a';
                    else if (c >= 'A' && c <= 'F') d = 10 + c - 'A';
                    else return std::nullopt;
                    v = (v << 4) | (uint64_t) d;
                }
                return v;
            };

            // Triple form: CCCCCCCC-MMMMMMMM-HHHHHHHH
            auto dash1 = clean.find ('-');
            if (dash1 != std::string::npos)
            {
                auto dash2 = clean.find ('-', dash1 + 1);
                if (dash2 == std::string::npos) return std::nullopt;
                auto c = hexVal (clean.substr (0, dash1));
                auto m = hexVal (clean.substr (dash1 + 1, dash2 - dash1 - 1));
                auto h = hexVal (clean.substr (dash2 + 1));
                if (! (c && m && h)) return std::nullopt;
                return Seeds { *c & 0xFFFFFFFFull, *m & 0xFFFFFFFFull, *h & 0xFFFFFFFFull };
            }

            // Single value: treat as a master seed (hex with 0x prefix, else
            // decimal if all digits, else hex).
            uint64_t master = 0;
            if (clean.size() > 2 && (clean[0] == '0') && (clean[1] == 'x' || clean[1] == 'X'))
            {
                auto v = hexVal (clean.substr (2));
                if (! v) return std::nullopt;
                master = *v;
            }
            else if (clean.find_first_not_of ("0123456789") == std::string::npos)
            {
                for (char c : clean)
                {
                    if (master > (UINT64_MAX - 9) / 10) return std::nullopt; // overflow
                    master = master * 10 + (uint64_t) (c - '0');
                }
            }
            else
            {
                auto v = hexVal (clean);
                if (! v) return std::nullopt;
                master = *v;
            }
            return fromMaster (master);
        }
    };
}
