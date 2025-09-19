#pragma once
#include <memory>

namespace dx3d
{
	class Time
	{
	public:
		static std::shared_ptr<Time> Instance()
		{
			static std::shared_ptr<Time> instance(new Time());
			return instance;
		}

		// Frame time (scaled)
		double deltaTime() const { return m_deltaTime; }
		double unscaledDeltaTime() const { return m_unscaledDeltaTime; }

		// Fixed step
		double fixedDeltaTime() const { return m_fixedDeltaTime; }
		double fixedTotalTime() const { return m_fixedTotalTime; }

		// Total scaled and unscaled time
		double totalTime() const { return m_totalTime; }
		double unscaledTotalTime() const { return m_unscaledTotalTime; }

		double timeScale() const { return m_timeScale; }
		void setTimeScale(double scale) { m_timeScale = scale; }

		// Interpolation factor between fixed steps (0..1)
		double interpolationAlpha() const { return m_interpolationAlpha; }

		// Call every frame
		void Update(double realDeltaSeconds);

	private:
		Time() = default;

		double m_deltaTime = 0.0;
		double m_unscaledDeltaTime = 0.0;
		double m_totalTime = 0.0;
		double m_unscaledTotalTime = 0.0;
		double m_fixedDeltaTime = 0.01; // default 10ms physics step
		double m_fixedTotalTime = 0.0;
		double m_timeScale = 1.0;
		double m_interpolationAlpha = 0.0;

		double m_fixedAccumulator = 0.0;
	};
}