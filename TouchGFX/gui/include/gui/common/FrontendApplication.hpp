#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    void gotoMenuScreen();
    void gotoMenuScreenNoAnimation();
    void gotoHomeAutomationScreen();
    void gotoAnimatedGraphicsScreen();
    void gotoLiveDataDisplayScreen();
    void gotoChromArtScreen();
    void gotoChromArtScreenNoTr();

private:
    touchgfx::Callback<FrontendApplication> userTransitionCallback;

    void gotoMenuScreenImpl();
    void gotoMenuScreenNoAnimationImpl();
    void gotoHomeAutomationScreenImpl();
    void gotoAnimatedGraphicsScreenImpl();
    void gotoLiveDataDisplayScreenImpl();
    void gotoChromArtScreenImpl();
    void gotoChromArtScreenNoTrImpl();

};

#endif // FRONTENDAPPLICATION_HPP
