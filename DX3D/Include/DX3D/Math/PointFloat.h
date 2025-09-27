#pragma once

class PointFloat
{
public:
    float x;
    float y;

    PointFloat() : x(0.0f), y(0.0f) {}
    PointFloat(float xVal, float yVal) : x(xVal), y(yVal) {}

    PointFloat(const PointFloat& other) : x(other.x), y(other.y) {}

    PointFloat(PointFloat&& other) noexcept : x(other.x), y(other.y) {
        other.x = 0;
        other.y = 0;
    }

    PointFloat& operator=(const PointFloat& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
        }
        return *this;
    }

    explicit PointFloat(float value) : x(value), y(value) {}
};
