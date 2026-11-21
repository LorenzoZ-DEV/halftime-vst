// Halftime DSP core: header-only, no JUCE. Records each cycle and plays it back at half speed.
#pragma once
#include <algorithm>
#include <cstdlib>
#include <vector>

class HalftimeCore
{
public:
    void prepare (int numChannels, int maxLength, int fadeLength)
    {
        buf.assign ((size_t) numChannels, std::vector<float> ((size_t) std::max (2, maxLength), 0.0f));
        fadeLen = std::max (1, fadeLength);
        len = (int) buf[0].size();
        pos = 0;
        fade = 0;
    }

    // Cycle length in samples, clamped to the prepared buffer.
    void setCycleLength (int length) { len = std::clamp (length, 2, (int) buf[0].size()); }

    int position() const { return pos; }

    // hostPos: sample index inside the cycle reported by the host, or -1 to free-run on the internal counter.
    void process (float* const* ch, int numChannels, int numSamples, int hostPos, float mix)
    {
        const int nch = std::min (numChannels, (int) buf.size());

        // shortcut: drift up to kTolerance samples vs the host is ignored, raise it if a host jitters more.
        if (hostPos >= 0 && std::abs (hostPos % len - pos) > kTolerance)
            restart (hostPos % len);

        for (int s = 0; s < numSamples; ++s)
        {
            if (pos >= len)
                restart (0);

            const float g = fade > 0 ? (float) fade / (float) fadeLen : 0.0f; // weight of the old reader

            for (int c = 0; c < nch; ++c)
            {
                auto& b = buf[(size_t) c];
                const float dry = ch[c][s];
                b[(size_t) pos] = dry; // write first: read position pos/2 never passes the write head
                float wet = read (b, pos * 0.5);
                if (fade > 0)
                    wet = wet * (1.0f - g) + read (b, oldRead) * g;
                ch[c][s] = dry * (1.0f - mix) + wet * mix;
            }

            if (fade > 0)
            {
                oldRead += 0.5;
                --fade;
            }
            ++pos;
        }
    }

private:
    static constexpr int kTolerance = 32;

    // Linear interpolation, indices clamped to the current cycle.
    float read (const std::vector<float>& b, double p) const
    {
        const int i0 = std::min ((int) p, len - 1);
        const int i1 = std::min (i0 + 1, len - 1);
        const float f = (float) (p - (double) (int) p);
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * f;
    }

    void restart (int newPos)
    {
        oldRead = pos * 0.5; // old reader keeps going from where it was, faded out
        pos = newPos;
        fade = fadeLen;
    }

    std::vector<std::vector<float>> buf;
    int len = 2, pos = 0, fade = 0, fadeLen = 1;
    double oldRead = 0.0;
};
