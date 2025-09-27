#pragma once

class Point
{
public:
    int x;
    int y;

    Point() : x(0), y(0) {}
    Point(int xVal, int yVal) : x(xVal), y(yVal) {}

    Point(const Point& other) : x(other.x), y(other.y) {}

    Point(Point&& other) noexcept : x(other.x), y(other.y) {
        other.x = 0;
        other.y = 0;
    }

    Point& operator=(const Point& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
        }
        return *this;
    }

    explicit Point(int value) : x(value), y(value) {}
};
