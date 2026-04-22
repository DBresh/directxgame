#pragma once
#include <vector>
#include <algorithm>

namespace Simulator
{
	class TimeController
	{
	private:
		int m_timeWarpIndex = 0;
		bool m_paused = false;
		bool m_reverse = false;

	public:
		double Epoch = 0.0;
		double TimeScale = 1.0;
		double TargetTimeScale = 1.0;

		std::vector<double> WarpLevels = {
			1.0,
			5.0,
			10.0,
			50.0,
			100.0,
			1000.0,
			10000.0,
			100000.0
		};

		int GetTimeWarpIndex() const { return m_timeWarpIndex; }
		double GetCurrentWarpScale() const { return WarpLevels[m_timeWarpIndex]; }
		bool IsPaused() const { return m_paused; }
		bool IsReversed() const { return m_reverse; }

		void SetTimeWarpByIndex(int index)
		{
			SetPaused(false);
			m_timeWarpIndex = std::clamp(index, 0, static_cast<int>(WarpLevels.size() - 1));
			TargetTimeScale = m_reverse ? -WarpLevels[m_timeWarpIndex] : WarpLevels[m_timeWarpIndex];
		}

		void SetPaused(bool paused)
		{
			m_paused = paused;
			if (m_paused) {
				TargetTimeScale = 0.0;
				TimeScale = 0.0;
			}
			else {
				TargetTimeScale = m_reverse ? -WarpLevels[m_timeWarpIndex] : WarpLevels[m_timeWarpIndex];
				TimeScale = TargetTimeScale;
			}
		}

		void Update(double realDeltaTime)
		{
			if (TimeScale != TargetTimeScale)
			{
				TimeScale += (TargetTimeScale - TimeScale) * 5.0 * realDeltaTime;
				if (std::abs(TargetTimeScale - TimeScale) < 0.001) TimeScale = TargetTimeScale;
			}
			Epoch += realDeltaTime * TimeScale;
		}

		double GetScaledDeltaTime(double realDeltaTime) const
		{
			return realDeltaTime * TimeScale;
		}

		void IncreaseWarp()
		{
			if (m_paused) {
				SetPaused(false);
			}
			else
			{
				SetTimeWarpByIndex(m_timeWarpIndex + 1);
			}
		}

		void DecreaseWarp()
		{
			if (m_paused) return;
			if (m_timeWarpIndex == 0) {
				SetPaused(true);
			}
			else {
				SetTimeWarpByIndex(m_timeWarpIndex - 1);
			}
		}

		void StopTimeWarp()
		{
			SetTimeWarpByIndex(0);
		}

		void Reverse()
		{
			m_reverse = !m_reverse;
			TimeScale = -TimeScale;
			TargetTimeScale = -TargetTimeScale;
		}
	};
}