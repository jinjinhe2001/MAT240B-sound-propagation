#pragma once

#include <iostream>
#include <vector>
#include <set>
#include "al/graphics/al_Mesh.hpp"

using namespace al;

Vec2f reflectPoint(Vec2f startPoint, Vec2f endPoint, Vec2f needRefectPoint) {
    Vec2f lineDir = (endPoint - startPoint).normalize();
    Vec2f v1 = (needRefectPoint - startPoint);
    float verticalValue = v1.dot(lineDir);
    Vec2f verticalPoint = startPoint + verticalValue * lineDir;
    Vec2f v2 = verticalPoint - needRefectPoint;
    return needRefectPoint + 2.0f * v2;
}