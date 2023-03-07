#include <iostream>
#include <memory>

// for master branch
// #include "al/core.hpp"

// for devel branch
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Image.hpp"
#include "soundObject.hpp"

using namespace al;

struct MyApp : App {
  Boundry boundry;

  void onCreate() override {
    boundry.resizeRect(2.0f, 2.0f, Vec2f(0, 0));
    nav().pos(Vec3f(0, 0, 4));
    Ray r(Vec2f(0, 0), Vec2f(1, 0));
    std::cout<< r.lineDetect(Line(Vec2f(-1, 1), Vec2f(-2, -2))) <<std::endl;
    std::cout<< r.circleDetect(Vec2f(1, 0), 2.0f) <<std::endl;
  }

  bool onKeyDown(Keyboard const& k) override {
    return true;
  }
  void onAnimate(double dt) override {
  }

  void onDraw(Graphics& g) override {
    g.depthTesting(true);
    g.clear(0.2);
    g.polygonLine();
    g.pushMatrix();
    g.draw(boundry.mesh);
    g.popMatrix();
  }
};

int main() {
  MyApp app;
  app.dimensions(600, 400);
  app.start();
}
