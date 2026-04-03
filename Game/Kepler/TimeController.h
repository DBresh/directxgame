#pragma once
#include <vector>
#include <algorithm>

namespace Simulator
{
    class TimeController
    {
    public:
        double Epoch = 0.0;
        double TimeScale = 1.0;
        int CurrentWarpLevel = 0;

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

        void Update(double realDeltaTime)
        {
            Epoch += realDeltaTime * TimeScale;
        }

        double GetScaledDeltaTime(double realDeltaTime) const
        {
            return realDeltaTime * TimeScale;
        }

        void IncreaseWarp()
        {
            CurrentWarpLevel = std::min(CurrentWarpLevel + 1, static_cast<int>(WarpLevels.size() - 1));
            TimeScale = WarpLevels[CurrentWarpLevel];
        }

        void DecreaseWarp()
        {
            CurrentWarpLevel = std::max(CurrentWarpLevel - 1, 0);
            TimeScale = WarpLevels[CurrentWarpLevel];
        }

        void StopTimeWarp()
        {
            CurrentWarpLevel = 0;
            TimeScale = WarpLevels[0];
        }
    };
}