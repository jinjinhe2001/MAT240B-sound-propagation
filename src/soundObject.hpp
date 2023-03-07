#pragma once

#include <iostream>
#include <vector>
#include <set>
#include "al/graphics/al_Mesh.hpp"
#include "al/sound/al_SoundFile.hpp"

using namespace al;

#define alpha 1e-5

struct Line
{
    int index;
    Vec2f start;
    Vec2f end;
    Line() {}
    Line(Vec2f s, Vec2f e) : start(s), end(e) {}
    friend std::ostream & operator << (std::ostream &, Line &l) {
        std::cout <<"{"<< l.start << ", " << l.end << "," << l.index << "}";
        return std::cout;
    }
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
        //if (fabs(theta) < 1e-5) return -1;
        float t2 = (line.start - point).norm2() / tanf(theta);
        float t = t1 + t2;
        if (fabs(((ori + dir * t) - line.start).normalize().dot(lineDir) - 1) < alpha) return t;
        return t1 - t2;
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

    friend std::ostream & operator << (std::ostream &, Ray &r) {
        std::cout << r.ori << ", " << r.dir;
        return std::cout;
    }
};

struct Source
{
    Vec2f pos;
    float receiveRadius = 0.5f;
    SoundFilePlayerTS playerTS;
    std::vector<float> buffer;
    float absorption;
    void init(std::string fileStr) {
        if (!playerTS.open(fileStr.c_str())) {
            std::cerr << "File not found: " << fileStr.c_str() << std::endl;
            exit(0);
        }
        std::cout << "sampleRate: " << playerTS.soundFile.sampleRate << std::endl;
        std::cout << "channels: " << playerTS.soundFile.channels << std::endl;
        std::cout << "frameCount: " << playerTS.soundFile.frameCount << std::endl;
    }
    
};

struct Path
{
    Vec2f start;
    Vec2f end;
    std::vector<int> indexArray;
    bool operator<(const Path& p) const {
        return indexArray.size() < p.indexArray.size();
    }
};


class Boundry {
public:
    Mesh mesh;
    std::vector<Line> lines;
    int currentIndex = 0;

    void resizeRect(float width, float height, Vec2f center) {
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
};


struct Listener
{
    Vec2f pos;
    int depth = 3;
    std::vector<Path> paths;

    void reflectRay(float t, Ray ray, Line* _line, Boundry& boundry, Source& source, Path& p) {
        Vec2f hitPoint = ray(t);
        Vec2f lineDir = (hitPoint - _line->start).normalize();
        float cosTheta = ray.dir.dot(lineDir);
        Vec2f vertical = cosTheta * lineDir - ray.dir;
        Vec2f newRayDir = cosTheta * lineDir + vertical;
        //std::cout<<"newRay" <<"cos"<<cosTheta<<"lineDir"<<lineDir<< newRayDir<< "vertical"<<vertical;
        Ray r(hitPoint, newRayDir);

        Line* hitLine;
        t = -1;
        std::cout<<"ray"<<r<<"\n";
        for (auto& line : boundry.lines) {
            float temp = r.lineDetect(line);
            if (temp > alpha && (temp < t || t < alpha)) {
                t = temp;
                hitLine = &line;
            }
        }
        float temp = r.circleDetect(source.pos, source.receiveRadius);
        if (temp > alpha && (temp < t || t < alpha)) {
            p.start = source.pos;
            p.end = pos;
            paths.push_back(p);
        } else if (temp < alpha && t > alpha) {
            p.start = source.pos;
            if (p.indexArray.size() < depth) {
                p.indexArray.push_back(hitLine->index);
                reflectRay(t, r, hitLine, boundry, source, p);
            }
        }
    }

    void scatterRay(int num, Boundry& boundry, Source& source) {
        float offset = M_2PI / (float)num;
        float start = 0;//(float)random() / RAND_MAX;
        for (int i = 0; i < num; i++) {
            float t = -1;
            float theta = start + i * offset;
            Ray r(pos, Vec2f(cosf(theta), sinf(theta)));
            Path p;
            Line* hitLine;
            std::cout<<"\n"<<"ray"<<r<<"\n";
            //std::cout<<"line"<< std::endl;
            for (auto& line : boundry.lines) {
                float temp = r.lineDetect(line);
                if (temp > alpha && (temp < t || t < alpha)) {
                    t = temp;
                    hitLine = &line;
                } 
                //std::cout << r(t) << "+" << line << "+" << temp << "\n";
            }
            float temp = r.circleDetect(source.pos, source.receiveRadius);

            
            if (temp > alpha && (temp < t || t < alpha)) {
                p.start = source.pos;
                p.end = pos;
                paths.push_back(p);
                //std::cout<< r(temp) << std::endl;
            } else if (t > alpha) {
                p.start = source.pos;
                p.indexArray.push_back(hitLine->index);
                reflectRay(t, r, hitLine, boundry, source, p);
                //std::cout<< temp << r(t) << std::endl;
            }
        }
    }
};
