#pragma once

#include <iostream>
#include <vector>
#include <set>
#include "al/graphics/al_Mesh.hpp"
#include "al/sound/al_SoundFile.hpp"
#include "Gamma/Delay.h"
#include "geometry_helper.hpp"

using namespace al;
using namespace gam;

#define alpha 1e-5

struct Line
{
    int index;
    Vec2f start;
    Vec2f end;
    Line() {}
    Line(Vec2f s, Vec2f e) : start(s), end(e) {}
    friend std::ostream &operator<<(std::ostream &, Line &l)
    {
        std::cout << "{" << l.start << ", " << l.end << "," << l.index << "}";
        return std::cout;
    }
};

struct Ray2d
{
    Vec2f ori;
    Vec2f dir;
    Ray2d() {}
    Ray2d(Vec2f o, Vec2f d) : ori(o), dir(d) {}
    Vec2f operator()(float t)
    {
        return ori + dir * t;
    }

    float lineDetect(Line line)
    {
        // Mystery bug appears in a case that is extremely difficult to reproduce.
        /*Vec2f v1 = line.start - ori;
        Vec2f lineDir = (line.end - line.start).normalize();
        float t1 = v1.dot(dir);
        Vec2f point = ori + dir * t1;
        float theta = acosf(lineDir.dot(dir));
        // if (fabs(theta) < 1e-5) return -1;
        float t2 = (line.start - point).norm2() / tanf(theta);
        float t = t1 + t2;
        if (fabs(((ori + dir * t) - line.start).normalize().dot(lineDir) - 1) < alpha)
            return t;
        return t1 - t2;*/
        float bigY = dir.y * (line.start.x - line.end.x) - dir.x * (line.start.y - line.end.y);
        float bigX = dir.x * (line.end.y - ori.y) - dir.y * (line.end.x - ori.x);
        if (fabs(bigY) < alpha) return -1;
        float a = bigX / bigY;
        if (a < 0 || a > 1) return -1;
        Vec2f hitPoint = line.start * a + (1 - a) * line.end;
        float t = (hitPoint - ori).dot(dir);
        return t;
    }

    float circleDetect(Vec2f pos, float radius)
    {
        Vec2f v1 = pos - ori;
        float t1 = v1.dot(dir);
        Vec2f point = ori + dir * t1;
        Vec2f v2 = point - pos;
        float value = radius * radius - v2.magSqr();
        if (value < 0)
            return value;
        float discriminant = sqrtf(value);
        t1 = t1 - discriminant > 0 ? t1 - discriminant : t1 + discriminant;
        return t1;
    }

    friend std::ostream &operator<<(std::ostream &, Ray2d &r)
    {
        std::cout << r.ori << ", " << r.dir;
        return std::cout;
    }
};

struct Source
{
    Vec2f pos;
    float receiveRadius = 0.5f;
    Mesh circle;
    SoundFilePlayerTS playerTS;
    std::vector<float> buffer;
    float absorption;
    void init(std::string fileStr)
    {
        if (!playerTS.open(fileStr.c_str()))
        {
            std::cerr << "File not found: " << fileStr.c_str() << std::endl;
            exit(0);
        }
        std::cout << "sampleRate: " << playerTS.soundFile.sampleRate << std::endl;
        std::cout << "channels: " << playerTS.soundFile.channels << std::endl;
        std::cout << "frameCount: " << playerTS.soundFile.frameCount << std::endl;
        addCircle(circle, receiveRadius);
    }
};

struct Path
{
    Vec2f start; // listener
    Vec2f end; // source
    std::vector<int> indexArray;
    std::vector<Vec2f> hitPoint;
    Vec2f image; // listener's image source
    float dist;
    float delay;
    float absorb = 1.0f;
    float reflectAbsorb = 1.0f;
    float scale = 10.0f;
    float absorbFactor = 0.95f;
    Vec2f dir;
   //Delay<float, ipl::Trunc> delayFiliter;

    void calculateImageSource(std::vector<Line>& lines) {
        image = start;
        for (auto index : indexArray) {
            Line& line = lines.at(index);
            image = reflectPoint(line.start, line.end, image);
            reflectAbsorb *= absorbFactor;
        }
        delay = (end - image).mag() * scale / 340.0f;
        dist = (end - image).mag() * scale ;
        absorb = 1 / sqrt((end - image).mag() * scale);
        //delayFiliter.maxDelay(1.0f);
		//delayFiliter.delay((end - image).mag() * scale / 340.0f);	

        if (hitPoint.size() == 0) {
            dir = (end - start).normalize();
        } else {
            dir = (hitPoint[0] - start).normalize();
        }
    }

    bool operator<(const Path &p) const
    {
        if ((start - p.start).mag() < alpha)
        {
            if (indexArray.size() == p.indexArray.size())
            {
                for (int i = 0; i < indexArray.size(); i++)
                {
                    if (indexArray[i] == p.indexArray[i])
                    {
                        continue;
                    }
                    else
                    {
                        return indexArray[i] < p.indexArray[i] ? true : false;
                    }
                }
                return false;
            }
            else
            {
                return indexArray.size() < p.indexArray.size() ? true : false;
            }
        }
        else
        {
            return start.mag() < p.start.mag() ? true : false;
        }
    }

    friend std::ostream &operator<<(std::ostream &, Path &p)
    {
        std::cout << "start point:" << p.start << "end point" << p.end << std::endl;
        for (int i = 0; i < p.indexArray.size(); i++)
        {
            std::cout << "Line" << i << ": index " << p.indexArray[i] << std::endl;
        }
        return std::cout;
    }
};

class Boundry
{
public:
    Mesh mesh{Mesh::LINES};
    std::vector<Line> lines;
    int currentIndex = 0;

    void resizeRect(float width, float height, Vec2f center)
    {
        mesh.reset();
        lines.clear();

        Vec2f points[4] = {center - Vec2f(width / 2, height / 2),
                           center - Vec2f(width / 2, -height / 2),
                           center - Vec2f(-width / 2, -height / 2),
                           center - Vec2f(-width / 2, height / 2)};

        addLine(points[0], points[1]);
        addLine(points[1], points[2]);
        addLine(points[2], points[3]);
        addLine(points[3], points[0]);
        line2Meshs();
    }

    void addLine(Vec2f start, Vec2f end)
    {
        Line l(start, end);
        l.index = currentIndex;
        lines.push_back(l);
        currentIndex++;
    }

    void Line2Mesh(Line line) {
        mesh.vertex(Vec3f(line.start, 0.0f));
        mesh.vertex(Vec3f(line.end, 0.0f));
    }

    void line2Meshs()
    {
        mesh.primitive(Mesh::LINES);
        for (auto &line : lines)
        {
            mesh.vertex(Vec3f(line.start, 0.0f));
            mesh.vertex(Vec3f(line.end, 0.0f));
        }
    }
};

struct Listener
{
    Vec2f pos;
    int depth = 10;
    std::set<Path> paths;
    Vec2f leftDirection = Vec2f(-1, 0);
    float absorbFactor = 0.95f;
    float scale = 10.0f;

    void reflectRay(float t, Ray2d ray, Line *_line, Boundry &boundry, Source &source, Path &p)
    {
        Vec2f hitPoint = ray(t);
        Vec2f lineDir = (hitPoint - _line->start).normalize();
        float cosTheta = ray.dir.dot(lineDir);
        Vec2f vertical = cosTheta * lineDir - ray.dir;
        Vec2f newRayDir = cosTheta * lineDir + vertical;
        // std::cout<<"newRay" <<"cos"<<cosTheta<<"lineDir"<<lineDir<< newRayDir<< "vertical"<<vertical;
        Ray2d r(hitPoint, newRayDir);

        Line *hitLine;
        t = -1;
        // std::cout<<"ray"<<r<<"\n";
        for (auto &line : boundry.lines)
        {
            float temp = r.lineDetect(line);
            if (temp > alpha && (temp < t || t < alpha))
            {
                t = temp;
                hitLine = &line;
            }
        }
        float temp = r.circleDetect(source.pos, source.receiveRadius);
        if (temp > alpha && (temp < t || t < alpha))
        {
            p.start = pos;
            p.end = source.pos;
            p.absorbFactor = absorbFactor;
            p.scale = scale;
            p.calculateImageSource(boundry.lines);
            paths.insert(p);
        }
        else if (temp < alpha && t > alpha)
        {
            p.start = pos;
            if (p.indexArray.size() < depth)
            {
                p.indexArray.push_back(hitLine->index);
                p.hitPoint.push_back(r(t));
                reflectRay(t, r, hitLine, boundry, source, p);
            }
        }
    }

    void scatterRay(int num, Boundry &boundry, Source &source)
    {
        float offset = M_2PI / (float)num;
        float start = 0; //(float)random() / RAND_MAX;
        for (int i = 0; i < num; i++)
        {
            float t = -1;
            float theta = start + i * offset;
            Ray2d r(pos, Vec2f(cosf(theta), sinf(theta)));
            Path p;
            Line *hitLine;
            // std::cout<<"\n"<<"ray"<<r<<"\n";
            // std::cout<<"line"<< std::endl;
            for (auto &line : boundry.lines)
            {
                float temp = r.lineDetect(line);
                if (temp > alpha && (temp < t || t < alpha))
                {
                    t = temp;
                    hitLine = &line;
                }
                // std::cout << r(t) << "+" << line << "+" << temp << "\n";
            }
            float temp = r.circleDetect(source.pos, source.receiveRadius);

            if (temp > alpha && (temp < t || t < alpha))
            {
                p.start = pos;
                p.end = source.pos;
                p.absorbFactor = absorbFactor;
                p.scale = scale;
                p.calculateImageSource(boundry.lines);
                paths.insert(p);
                // std::cout<< r(temp) << std::endl;
            }
            else if (t > alpha)
            {
                p.start = pos;
                p.indexArray.push_back(hitLine->index);
                p.hitPoint.push_back(r(t));
                reflectRay(t, r, hitLine, boundry, source, p);
                // std::cout<< temp << r(t) << std::endl;
            }
        }
    }
};

void addScene(Boundry& boundry) {
    boundry.resizeRect(6.0f, 6.0f, Vec2f(0, 0));
    Vec2f points[6] = {Vec2f(1.65, 0), Vec2f(2, 0), Vec2f(2, 1), Vec2f(1, 1), 
                       Vec2f(1, 0), Vec2f(1.35, 0)};
    for (int i = 0; i < 5; i++) {
        boundry.addLine(points[i], points[i + 1]);
        boundry.Line2Mesh(Line(points[i], points[i + 1]));
    }
}