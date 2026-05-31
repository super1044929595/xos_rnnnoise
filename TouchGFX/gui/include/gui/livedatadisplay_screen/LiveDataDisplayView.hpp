#ifndef LIVEDATADISPLAY_VIEW_HPP
#define LIVEDATADISPLAY_VIEW_HPP

#include <gui_generated/livedatadisplay_screen/LiveDataDisplayViewBase.hpp>
#include <gui/livedatadisplay_screen/LiveDataDisplayPresenter.hpp>

#include <gui/common/DotIndicator.hpp>

class LiveDataDisplayView : public LiveDataDisplayViewBase
{
public:
    LiveDataDisplayView();
    virtual ~LiveDataDisplayView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();
    virtual void handleClickEvent(const ClickEvent& evt);
    virtual void handleDragEvent(const DragEvent& evt);
    virtual void handleGestureEvent(const GestureEvent& evt);

private:
    enum States
    {
        ANIMATE_SWIPE_CANCELLED_LEFT,
        ANIMATE_SWIPE_CANCELLED_RIGHT,
        ANIMATE_LEFT,
        ANIMATE_RIGHT,
        NO_ANIMATION
    } currentState;

    static const uint8_t NUMBER_OF_SCREENS = 4;

    uint8_t animationCounter;
    int32_t tickCounter;

    int16_t dragX;
    int16_t animateDistance;
    int16_t startX;
    uint8_t currentScreen;

    DotIndicator dotIndicator;

    CityInfo* infoScreen[NUMBER_OF_SCREENS];

    void adjustInfoScreens();

    void animateSwipeCancelledLeft();
    void animateSwipeCancelledRight();
    void animateLeft();
    void animateRight();
};

#endif // LIVEDATADISPLAY_VIEW_HPP
