#undef NDEBUG // keep asserts active in Release builds
#include "../Source/HalftimeCore.h"
#include <cassert>
#include <cmath>
#include <cstdio>

// Runs one sample at a time so tests can see every output value.
static float step (HalftimeCore& core, float in, int hostPos = -1)
{
    float* ch[] = { &in };
    core.process (ch, 1, 1, hostPos, 1.0f);
    return in;
}

int main()
{
    // 1. half-speed read of a ramp gives a ramp with half the slope
    {
        HalftimeCore core;
        core.prepare (1, 16, 4);
        for (int i = 0; i < 16; ++i)
            assert (step (core, (float) i) == i * 0.5f);
    }

    // 2. reset at each cycle: the second cycle restarts from its own start, once the fade is over
    {
        HalftimeCore core;
        core.prepare (1, 64, 4);
        core.setCycleLength (16);
        for (int cycle = 0; cycle < 3; ++cycle)
            for (int i = 0; i < 16; ++i)
            {
                const float out = step (core, (float) (100 * cycle + i));
                if (i >= 4)
                    assert (std::fabs (out - (100.0f * cycle + i * 0.5f)) < 1e-4f);
            }
        assert (core.position() == 16);
    }

    // 3. no read past the cycle: buffer beyond len stays 0, constant input must stay exactly 1
    {
        HalftimeCore core;
        core.prepare (1, 100, 20); // fade longer than the cycle pushes the old reader past len
        core.setCycleLength (8);
        for (int i = 0; i < 200; ++i)
            assert (step (core, 1.0f) == 1.0f);
    }

    // 4. crossfade keeps the output continuous on cycle wrap and on host jumps
    {
        HalftimeCore core;
        core.prepare (1, 1000, 100);
        float prev = 0.0f, maxJump = 0.0f;
        int phase = 0; // input is a sawtooth locked to the cycle, like a looped bar
        for (int i = 0; i < 3000; ++i, phase = (phase + 1) % 1000)
        {
            int hostPos = -1;
            if (i == 1500)
                hostPos = phase = 200; // transport jump mid-cycle
            const float out = step (core, (float) phase, hostPos);
            if (i > 0)
                maxJump = std::max (maxJump, std::fabs (out - prev));
            prev = out;
        }
        assert (maxJump < 10.0f); // without the fade the wrap would jump by ~500
    }

    std::puts ("all tests passed");
    return 0;
}
