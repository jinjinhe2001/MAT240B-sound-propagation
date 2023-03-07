#pragma once

#include <vector>
#include "al/graphics/al_Mesh.hpp"
#include "al/sound/al_SoundFile.hpp"

using namespace al;

class Boundry;

struct Line
{
    int index;
    Vec2f start;
    Vec2f end;
    Line() {}
    Line(Vec2f s, Vec2f e) : start(s), end(e) {}
};

struct Ray
{
    Vec2f ori;
    Vec2f dir;
    Ray() {}
    Ray(Vec2f o, Vec2f d) : ori(o), dir(d) {}
    Vec2f operator()(float t) {
        return ori + dir * t;
    }

    float lineDetect(Line line) {
        Vec2f v1 = line.start - ori;
        Vec2f lineDir = (line.end - line.start).normalize();
        float t1 = v1.dot(dir);
        Vec2f point = ori + dir * t1;
        float theta = acosf(lineDir.dot(dir));
        if (theta < 1e-5) return -1;
        float t2 = (line.start - point).norm2() / tanf(theta);
        return t1 + t2;
    }

    float circleDetect(Vec2f pos, float radius) {
        Vec2f v1 = pos - ori;
        float t1 = v1.dot(dir);
        Vec2f point = ori + dir * t1;
        Vec2f v2 = point - pos;
        float value = radius * radius - v2.magSqr();
        if (value < 0) return value; 
        float discriminant = sqrtf(value);
        t1 = t1 - discriminant > 0 ? t1 - discriminant : t1 + discriminant;
        return t1;
    }
};

struct Source
{
    Vec2f pos;
    SoundFilePlayerTS playerTS;
    std::vector<float> buffer;
    float absorption;
};

struct Path
{
    Vec2f start;
    Vec2f end;
    int depth;
    int receiveRadius;
    std::vector<int> indexArray;
};


struct Listener
{
    Vec2f pos;
    int depth;
    std::vector<Path> paths;

    void scatterRay(int num, Boundry& Boundry) {
        float offset = M_2PI / (float)num;
        float start = (float)random() / RAND_MAX;
        for (int i = 0; i < num; i++) {
            float theta = start + i * offset;
            Ray r(pos, Vec2f(cosf(theta), sinf(theta)));

        }
    }
};

class Boundry {
public:
    Mesh mesh;
    std::vector<Line> lines;
    int currentIndex = 0;

    void resizeRect(float width, float height, Vec2f center) {
        mesh.reset();
        lines.resize(4);

        Vec2f points[4] = {center - Vec2f(width / 2, height / 2), 
                           center - Vec2f(width / 2, -height / 2),
                           center - Vec2f(-width / 2, -height / 2),
                           center - Vec2f(-width / 2, height / 2)};

        addLine(points[0], points[1]);
        addLine(points[1], points[2]);
        addLine(points[2], points[3]);
        addLine(points[3], points[0]);
        line2Mesh();
    }

    void addLine(Vec2f start, Vec2f end) {
        Line l(start, end);
        l.index = currentIndex;
        lines.push_back(l);
        currentIndex++;
    }

    void line2Mesh() {
        mesh.primitive(Mesh::LINES);
        for (auto& line : lines) {
            mesh.vertex(Vec3f(line.start, 0.0f));
            mesh.vertex(Vec3f(line.end, 0.0f));
        }
    }

    float rayCollision(Ray& r) {
        float t = -1.0;
        for (auto& line : lines) {

        }
    }
};