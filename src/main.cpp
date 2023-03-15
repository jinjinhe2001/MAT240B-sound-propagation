#include <iostream>
#include <memory>
#include <mutex>

// for master branch
// #include "al/core.hpp"

// for devel branch
#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/io/al_Imgui.hpp"
#include "al/math/al_Ray.hpp"
#include "soundObject.hpp"

// reference: http://gamma.cs.unc.edu/GSOUND/gsound_aes41st.pdf, http://gamma.cs.unc.edu/SOUND09/
// Next Step: Paths de-duplication, generate image source from paths, then apply to sound
// further step: Spatial division(octree), doppler shifting, GUI
using namespace al;

struct MyApp : App
{
  Boundry boundry;
  Source source;
  Listener listener;
  std::vector<Mesh> rays;
  Vec2f listenerDir;
  Vec2f lineDir;
  float lineLength;
  float absorbFactor = 0.95f;
  float scale = 10.0f;

  std::mutex mLock;
  bool enableAddLine = false;

  bool enableReflect = true;

  void onCreate() override
  {
    addScene(boundry);
    nav().pos(Vec3f(0, 0, 25));
    Ray2d r(Vec2f(0, 0), Vec2f(1, 0));

    source.init("./data/pno-cs.wav");
    source.pos = Vec2f(0, 0);
    listener.pos = Vec2f(1, -1);
    listener.scatterRay(500, boundry, source);
    for (auto p : listener.paths)
    {
      Mesh m;
      m.primitive(Mesh::LINE_STRIP);
      m.vertex(listener.pos);
      m.color(RGB(1, 0, 0));
      for (auto point : p.hitPoint)
      {
        m.vertex(Vec3f(point, 0.0f));
        m.color(RGB(0.5f, 0.5f, 1));
      }
      m.vertex(source.pos);
      m.color(RGB(0, 1, 0));
      rays.push_back(m);
    }
    navControl().disable();
  }

  bool onKeyDown(Keyboard const &k) override
  {
    switch (k.key())
    {
    case 'w':
    {
      listenerDir.y = 0.01f;
    }
    break;
    case 's':
    {
      listenerDir.y = -0.01f;
    }
    break;
    case 'a':
    {
      listenerDir.x = -0.01f;
    }
    break;
    case 'd':
    {
      listenerDir.x = 0.01f;
    }
    break;
    }
    return true;
  }

  bool onKeyUp(Keyboard const &k) override
  {
    switch (k.key())
    {
    case 'w':
    {
      listenerDir.y = 0.0f;
    }
    break;
    case 's':
    {
      listenerDir.y = 0.0f;
    }
    break;
    case 'a':
    {
      listenerDir.x = 0.0f;
    }
    break;
    case 'd':
    {
      listenerDir.x = 0.0f;
    }
    break;
    }
    return true;
  }

  void onAnimate(double dt) override
  {
    listener.pos += listenerDir;
    nav().pos(Vec3f(0, 0, 12));
    nav().faceToward(Vec3f(0, 0, 0));
    if (listenerDir.mag() > alpha)
    {
      mLock.lock();
      listener.paths.clear();
      listener.scatterRay(500, boundry, source);
      mLock.unlock();
    }
    rays.clear();
    for (auto p : listener.paths)
    {
      Mesh m;
      m.primitive(Mesh::LINE_STRIP);
      m.vertex(listener.pos);
      m.color(RGB(1, 0, 0));
      for (auto point : p.hitPoint)
      {
        m.vertex(Vec3f(point, 0.0f));
        m.color(RGB(0.5f, 0.5f, 1));
      }
      m.vertex(source.pos);
      m.color(RGB(0, 1, 0));
      rays.push_back(m);
    }
  }

  void onDraw(Graphics &g) override
  {
    g.depthTesting(true);
    g.clear(0.2);
    g.polygonLine();
    g.pushMatrix();
    g.draw(boundry.mesh);
    g.popMatrix();
    for (auto &ray : rays)
    {
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

    drawImGUI(g);
  }

  void onSound(AudioIOData &io) override
  {

    int frames = (int)io.framesPerBuffer();
    int channels = source.playerTS.soundFile.channels;
    int bufferLength = frames * channels;
    if ((int)source.buffer.size() < bufferLength)
    {
      source.buffer.resize(bufferLength);
    }
    int second = (channels < 2) ? 0 : 1;
    while (io())
    {
      int frame = (int)io.frame();
      int idx = frame * channels;
      if (enableReflect)
      {
        float ds = 0;
        mLock.lock();
        io.out(0) = 0;
        io.out(1) = 0;
        for (auto path : listener.paths)
        {
          long long int index = source.playerTS.player.frame + idx;
          long long int offset = source.playerTS.soundFile.sampleRate * path.delay;
          index -= offset;
          if (index < 0)
            continue;
          float s = (source.playerTS.soundFile.data[index] + source.playerTS.soundFile.data[index + second]) * 0.5f;
          //ds += source.playerTS.soundFile.data[index] * path.absorb * path.reflectAbsorb;
          Vec2f dir = path.dir;
          float cosTheta = dir.dot(listener.leftDirection);
          if (cosTheta > 0) {
            io.out(0) += s * path.absorb * path.reflectAbsorb * (0.5 + 0.5 * cosTheta);
            io.out(1) += s * path.absorb * path.reflectAbsorb * (0.5);
          } else {
            io.out(0) += s * path.absorb * path.reflectAbsorb * 0.5f;
            io.out(1) += s * path.absorb * path.reflectAbsorb * (0.5 + 0.5 * -cosTheta);
          }
        }
        mLock.unlock();
      }
      else
      {
        io.out(0) = source.playerTS.soundFile.data[source.playerTS.player.frame + idx];
        io.out(1) = source.playerTS.soundFile.data[source.playerTS.player.frame + idx + second];
      }
    }
    source.playerTS.player.frame = (source.playerTS.player.frame + bufferLength) % (source.playerTS.soundFile.frameCount * 2);
  }

  void onInit() override
  {
    imguiInit();
  }

  void onExit() override { imguiShutdown(); }

  void drawImGUI(Graphics &g)
  {
    imguiBeginFrame();

    int anythingChange = 0;

    ImGui::Begin("GUI");
    static bool _enableReflect = true;
    ImGui::Checkbox("Enable reflect", &_enableReflect);
    enableReflect = _enableReflect;

    static float _scale = 10.0f;
    ImGui::SliderFloat("scale", &_scale, 0.0f, 30.0f);
    anythingChange += fabs(listener.scale - _scale) < alpha ? 0 : 1;
    listener.scale = _scale;

    static float _absorb = 0.95f;
    ImGui::SliderFloat("absorb(reflect)", &_absorb, 0.0f, 1.0f);
    anythingChange += fabs(_absorb - listener.absorbFactor) < alpha ? 0 : 1;
    listener.absorbFactor = _absorb;
    if (anythingChange) {
      nav().pos(Vec3f(0, 0, 12));
      nav().faceToward(Vec3f(0, 0, 0));
      {
        mLock.lock();
        listener.paths.clear();
        listener.scatterRay(500, boundry, source);
        mLock.unlock();
      }
      rays.clear();
      for (auto p : listener.paths)
      {
        Mesh m;
        m.primitive(Mesh::LINE_STRIP);
        m.vertex(listener.pos);
        m.color(RGB(1, 0, 0));
        for (auto point : p.hitPoint)
        {
          m.vertex(Vec3f(point, 0.0f));
          m.color(RGB(0.5f, 0.5f, 1));
        }
        m.vertex(source.pos);
        m.color(RGB(0, 1, 0));
        rays.push_back(m);
      }
    }

    static float vec2a[2] = {-1.0f, 0.0f};
    ImGui::SliderFloat2("Listener Direction", vec2a, -1.0f, 1.0f);
    listener.leftDirection = vec2a;
    listener.leftDirection = listener.leftDirection.normalize();

    static bool _addLine = false;
    ImGui::Checkbox("Add Line", &_addLine);
    enableAddLine = _addLine;

    static float _lineDir[2] = {-1.0f, 0.0f};
    ImGui::SliderFloat2("Line Direction", _lineDir, -1.0f, 1.0f);
    lineDir = _lineDir;
    lineDir = lineDir.normalize();

    static float _lineLength = 1.0f;
    ImGui::SliderFloat("Line Length", &_lineLength, 0.0f, 5.0f);
    lineLength = _lineLength;

    ImGui::End();
    imguiEndFrame();
    imguiDraw();
  }
  
  Vec3d unproject(Vec3d screenPos)
  {
    auto &g = graphics();
    auto mvp = g.projMatrix() * g.viewMatrix() * g.modelMatrix();
    Matrix4d invprojview = Matrix4d::inverse(mvp);
    Vec4d worldPos4 = invprojview.transform(screenPos);
    return worldPos4.sub<3>(0) / worldPos4.w;
  }

  Rayd getPickRay(int screenX, int screenY)
  {
    Rayd r;
    Vec3d screenPos;
    screenPos.x = (screenX * 1. / width()) * 2. - 1.;
    screenPos.y = ((height() - screenY) * 1. / height()) * 2. - 1.;
    screenPos.z = -1.;
    Vec3d worldPos = unproject(screenPos);
    r.origin().set(worldPos);

    screenPos.z = 1.;
    worldPos = unproject(screenPos);
    r.direction().set(worldPos);
    r.direction() -= r.origin();
    r.direction().normalize();
    return r;
  }
  
  bool onMouseDown(const Mouse &m) override
  {
    if (enableAddLine) {
      Rayd r = getPickRay(m.x(), m.y());
      if (fabs(r.direction().z) < alpha) return false;
      double a = -r.origin().z / r.direction().z;
      Vec2f hitPoint = Vec2f((a * r.direction() + r.origin()).x, (a * r.direction() + r.origin()).y);
      //std::cout<<"1"<<std::endl;
      Vec2f start = hitPoint - lineDir.normalize() * lineLength / 2;
      Vec2f end = hitPoint + lineDir.normalize() * lineLength / 2;
      boundry.addLine(start, end);
      boundry.Line2Mesh(Line(start, end));
      nav().pos(Vec3f(0, 0, 12));
      nav().faceToward(Vec3f(0, 0, 0));
      {
        mLock.lock();
        listener.paths.clear();
        listener.scatterRay(500, boundry, source);
        mLock.unlock();
      }
      rays.clear();
      for (auto p : listener.paths)
      {
        Mesh m;
        m.primitive(Mesh::LINE_STRIP);
        m.vertex(listener.pos);
        m.color(RGB(1, 0, 0));
        for (auto point : p.hitPoint)
        {
          m.vertex(Vec3f(point, 0.0f));
          m.color(RGB(0.5f, 0.5f, 1));
        }
        m.vertex(source.pos);
        m.color(RGB(0, 1, 0));
        rays.push_back(m);
      }
    }
    return true;
  }
};

int main()
{
  MyApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(600, 400);
  app.start();
}
