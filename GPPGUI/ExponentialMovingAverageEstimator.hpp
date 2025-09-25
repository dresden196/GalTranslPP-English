#ifndef GALTRANSLPP_EXPONENTIALMOVINGAVESTIMATOR_HPP
#define GALTRANSLPP_EXPONENTIALMOVINGAVESTIMATOR_HPP

#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<double>; // 使用 double 类型的秒

class ExponentialMovingAverageEstimator {
private:
    double alpha;
    double avgSpeed;
    TimePoint lastTime;
    double lastProgress;
    bool isFirstUpdate;

public:
    explicit ExponentialMovingAverageEstimator(double smoothingFactor = 0.1)
        : alpha(smoothingFactor), avgSpeed(0.0), lastProgress(0.0), isFirstUpdate(true) {
    }

    std::pair<double, Duration> updateAndGetSpeedWithEta(double currentProgress, double totalProgress) {
        TimePoint currentTime = Clock::now();

        if (isFirstUpdate) {
            lastTime = currentTime;
            lastProgress = currentProgress;
            isFirstUpdate = false;
            return std::make_pair(0.0, Duration(std::numeric_limits<double>::infinity()));
        }

        Duration deltaTime = currentTime - lastTime;
        double deltaProgress = currentProgress - lastProgress;

        if (deltaTime.count() > 1e-9) { // 避免除以零
            double currentSpeed = deltaProgress / deltaTime.count();
            if (avgSpeed == 0.0) { // 第一次有效计算
                avgSpeed = currentSpeed;
            }
            else {
                avgSpeed = (alpha * currentSpeed) + ((1.0 - alpha) * avgSpeed);
            }
        }

        lastTime = currentTime;
        lastProgress = currentProgress;

        if (avgSpeed <= 1e-9) {
            return std::make_pair(0.0, Duration(std::numeric_limits<double>::infinity()));
        }

        double remainingWork = totalProgress - currentProgress;
        return std::make_pair(avgSpeed, Duration(remainingWork / avgSpeed));
    }

    void reset()
    {
        avgSpeed = 0.0;
        lastProgress = 0.0;
        isFirstUpdate = true;
    }
};

#endif //GALTRANSLPP_EXPONENTIALMOVINGAVESTIMATOR_HPP