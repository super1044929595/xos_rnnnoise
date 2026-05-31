#include <new>
#include <gui/common/FrontendApplication.hpp>
#include <mvp/View.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/menu_screen/MenuView.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>
#include <gui/menu_screen/MenuView.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>
#include <gui/homeautomation_screen/HomeAutomationView.hpp>
#include <gui/homeautomation_screen/HomeAutomationPresenter.hpp>
#include <gui/animatedgraphics_screen/AnimatedGraphicsView.hpp>
#include <gui/animatedgraphics_screen/AnimatedGraphicsPresenter.hpp>
#include <gui/livedatadisplay_screen/LiveDataDisplayView.hpp>
#include <gui/livedatadisplay_screen/LiveDataDisplayPresenter.hpp>
#include <gui/chromart_screen/ChromArtView.hpp>
#include <gui/chromart_screen/ChromArtPresenter.hpp>
#include <gui/common/SlowSlideTransition.hpp>
#include <gui/common/FrontendHeap.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      userTransitionCallback()
{
}

void FrontendApplication::gotoMenuScreen()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoMenuScreenImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoMenuScreenImpl()
{
    makeTransition< MenuView, MenuPresenter, SlowSlideTransition<WEST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoMenuScreenNoAnimation()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoMenuScreenNoAnimationImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoMenuScreenNoAnimationImpl()
{
    makeTransition< MenuView, MenuPresenter, NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoHomeAutomationScreen()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoHomeAutomationScreenImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoHomeAutomationScreenImpl()
{
    makeTransition< HomeAutomationView, HomeAutomationPresenter, SlowSlideTransition<NORTH>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoAnimatedGraphicsScreen()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoAnimatedGraphicsScreenImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoAnimatedGraphicsScreenImpl()
{
    makeTransition< AnimatedGraphicsView, AnimatedGraphicsPresenter, SlowSlideTransition<WEST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoLiveDataDisplayScreen()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoLiveDataDisplayScreenImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoLiveDataDisplayScreenImpl()
{
    makeTransition< LiveDataDisplayView, LiveDataDisplayPresenter, SlowSlideTransition<SOUTH>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoChromArtScreen()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoChromArtScreenImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoChromArtScreenImpl()
{
    makeTransition< ChromArtView, ChromArtPresenter, SlowSlideTransition<EAST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoChromArtScreenNoTr()
{
    userTransitionCallback = Callback< FrontendApplication >(this, &FrontendApplication::gotoChromArtScreenNoTrImpl);
    pendingScreenTransitionCallback = &userTransitionCallback;
}

void FrontendApplication::gotoChromArtScreenNoTrImpl()
{
    makeTransition< ChromArtView, ChromArtPresenter, NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
