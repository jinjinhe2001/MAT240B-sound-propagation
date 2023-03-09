#include <iostream>
#include <memory>

// for master branch
// #include "al/core.hpp"

// for devel branch
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Image.hpp"
#include "soundObject.hpp"

// reference: http://gamma.cs.unc.edu/GSOUND/gsound_aes41st.pdf, http://gamma.cs.unc.edu/SOUND09/
// Next Step: Paths de-duplication, generate image source from paths, then apply to sound
// further step: Spatial division(octree), doppler shifting, GUI
using namespace al;

struct MyApp : App {
  Boundry boundry;
  Source source;
  Listener listener;
  std::vector<Mesh> rays;
  ParameterBool enableReflect{"enable reflect", "", 1.0};

  void onCreate() override {
    boundry.resizeRect(4.0f, 4.0f, Vec2f(0, 0));
    nav().pos(Vec3f(0, 0, 8));
    Ray r(Vec2f(0, 0), Vec2f(1, 0));
    std::cout<< r.lineDetect(Line(Vec2f(-1, 1), Vec2f(-2, -2))) <<std::endl;
    std::cout<< r.circleDetect(Vec2f(1, 0), 2.0f) <<std::endl;
    
    source.init("./data/pno-cs.wav");
    source.pos = Vec2f(0, 0);
    listener.pos = Vec2f(-1, -1);
    listener.scatterRay(500, boundry, source);
    for (auto p : listener.paths) {
      Mesh m;
      m.primitive(Mesh::LINE_STRIP);
      m.vertex(listener.pos);
      m.color(RGB(1, 0, 0));
      std::cout << p << std::endl;
      float randomColor = (float)random() / RAND_MAX;
      for (auto point : p.hitPoint) {
        m.vertex(Vec3f(point, 0.0f));
        m.color(RGB(randomColor, randomColor, 1));
      }
      m.vertex(source.pos);
      m.color(RGB(0, 1, 0));
      rays.push_back(m);
    }
  }

  bool onKeyDown(Keyboard const& k) override {
    return true;
  }
  void onAnimate(double dt) override {
    nav().pos(Vec3f(0, 0, 8));
    nav().faceToward(Vec3f(0, 0, 0));
  }

  void onDraw(Graphics& g) override {
    g.depthTesting(true);
    g.clear(0.2);
    g.polygonLine();
    g.pushMatrix();
    g.draw(boundry.mesh);
    g.popMatrix();
    for (auto& ray : rays) {
      g.polygonLine();
      g.pushMatrix();
      g.meshColor();
      g.draw(ray);
      g.popMatrix();
    }
    g.pushMatrix();
    g.color(RGB(0, 1, 0));
    g.draw(source.circle);
    g.popMatrix();
  }

  void onSound(AudioIOData& io) override {
  
    int frames = (int)io.framesPerBuffer();
    int channels = source.playerTS.soundFile.channels;
    int bufferLength = frames * channels;
    if ((int)source.buffer.size() < bufferLength) {
      source.buffer.resize(bufferLength);
    }
    int second = (channels < 2) ? 0 : 1;
    while (io()) {
      int frame = (int)io.frame();
      int idx = frame * channels;
      io.out(0) = source.playerTS.soundFile.data[source.playerTS.player.frame + idx];
      io.out(1) = source.playerTS.soundFile.data[source.playerTS.player.frame + idx + second];
      if (enableReflect) {
        float ds = 0;
        for (auto path : listener.paths) {
          long long int index = source.playerTS.player.frame + idx;
          long long int offset = source.playerTS.soundFile.sampleRate * path.delay;
          index -= offset;
          if (index < 0) continue;
          ds += source.playerTS.soundFile.data[index] * path.absorb * path.reflectAbsorb;
          io.out(0) += source.playerTS.soundFile.data[index] * path.absorb * path.reflectAbsorb;
          io.out(1) += source.playerTS.soundFile.data[index + second] * path.absorb * path.reflectAbsorb;
        }
      }
    }
    source.playerTS.player.frame = (source.playerTS.player.frame + bufferLength);
  }

  void onInit() override {
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto& gui = GUIdomain->newGUI();
    gui.add(enableReflect);
  }
};



int main() {
  MyApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(600, 400);
  app.start();
}
