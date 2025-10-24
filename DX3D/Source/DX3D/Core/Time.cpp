#include <DX3D/Core/Time.h>
#include <algorithm>

namespace dx3d 
{
    void Time::Update(double realDeltaSeconds)
    {
        m_unscaledDeltaTime = realDeltaSeconds;
        m_deltaTime = realDeltaSeconds * m_timeScale;

        m_unscaledTotalTime += m_unscaledDeltaTime;
        m_totalTime += m_deltaTime;

        // Fixed-step accumulator
        m_fixedAccumulator += m_deltaTime;

        // Clamp to avoid spiral-of-death if frame took too long
        const double maxAccum = 0.25; // max 250ms of physics per frame
        if (m_fixedAccumulator > maxAccum)
            m_fixedAccumulator = maxAccum;

        // Compute interpolation alpha for rendering between fixed steps
        m_interpolationAlpha = m_fixedAccumulator / m_fixedDeltaTime;

        // Advance fixed physics steps
        while (m_fixedAccumulator >= m_fixedDeltaTime)
        {
            m_fixedTotalTime += m_fixedDeltaTime;
            m_fixedAccumulator -= m_fixedDeltaTime;

        }
    }
}
