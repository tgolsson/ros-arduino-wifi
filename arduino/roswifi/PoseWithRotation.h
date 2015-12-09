#pragma once
struct PoseWithRotation
{
    double x, y, theta;


    PoseWithRotation(double x_, double y_, double th_) : x(x_), y(y_), theta(th_) {}
    PoseWithRotation() : x(0), y(0), theta(0) {}
};
