#include <gui/livedatadisplay_screen/LiveDataDisplayView.hpp>
#include <touchgfx/EasingEquations.hpp>
#include "BitmapDatabase.hpp"
#include <touchgfx/Color.hpp>
#include <stdlib.h>
#include <touchgfx/hal/HAL.hpp>
#include <gui/common/Utils.hpp>

LiveDataDisplayView::LiveDataDisplayView() :
    currentState(NO_ANIMATION),
    animationCounter(0),
    tickCounter(0),
    dragX(0)
{
    // Initialize CityInfo Screens
    infoScreen[0] = &infoScreen0;
    infoScreen[1] = &infoScreen1;
    infoScreen[2] = &infoScreen2;
    infoScreen[3] = &infoScreen3;
}

void LiveDataDisplayView::setupScreen()
{
    LiveDataDisplayViewBase::setupScreen();

    infoScreen[0]->setBitmap(BITMAP_WEATHER_BACKGROUND_1_ID);
    infoScreen[0]->setCity(CityInfo::COPENHAGEN);
    infoScreen[0]->setTextColor(Color::getColorFromRGB(0xFA, 0xF0, 0xB4));
    infoScreen[1]->setBitmap(BITMAP_WEATHER_BACKGROUND_2_ID);
    infoScreen[1]->setCity(CityInfo::HONG_KONG);
    infoScreen[1]->setTextColor(Color::getColorFromRGB(0x0A, 0x45, 0x064));
    infoScreen[2]->setBitmap(BITMAP_WEATHER_BACKGROUND_3_ID);
    infoScreen[2]->setCity(CityInfo::MUMBAI);
    infoScreen[2]->setTextColor(Color::getColorFromRGB(0x27, 0x5F, 0x7D));
    infoScreen[3]->setBitmap(BITMAP_WEATHER_BACKGROUND_1_ID);
    infoScreen[3]->setCity(CityInfo::NEW_YORK);
    infoScreen[3]->setTextColor(Color::getColorFromRGB(0xBA, 0xD2, 0xDA));

    currentScreen = 0;
    weekInfoBar.setPosition(0, exitButton.getY() - 72, HAL::DISPLAY_WIDTH, 50);
    weekInfoBar.setTextColor(infoScreen[currentScreen]->getTextColor());
    weekInfoBar.setInfo(infoScreen[currentScreen]->getCity());

    dotIndicator.setNumberOfDots(NUMBER_OF_SCREENS);
    dotIndicator.setBitmaps(Bitmap(BITMAP_WEATHER_DOT_NORMAL_ID), Bitmap(BITMAP_WEATHER_DOT_SELECTED_ID));
    dotIndicator.setXY(((HAL::DISPLAY_WIDTH - dotIndicator.getWidth()) / 2), exitButton.getY() - 16);

    add(dotIndicator);
}

void LiveDataDisplayView::tearDownScreen()
{
    LiveDataDisplayViewBase::tearDownScreen();
}

void LiveDataDisplayView::handleTickEvent()
{
    tickCounter++;

    if (currentState == ANIMATE_SWIPE_CANCELLED_LEFT)
    {
        animateSwipeCancelledLeft();
    }
    else if (currentState == ANIMATE_SWIPE_CANCELLED_RIGHT)
    {
        animateSwipeCancelledRight();
    }
    else if (currentState == ANIMATE_LEFT)
    {
        animateLeft();
    }
    else if (currentState == ANIMATE_RIGHT)
    {
        animateRight();
    }

    // Sometimes change current temperature to make
    // demo screen look alive
    if (tickCounter % 100 == 0)
    {
        for (int16_t i = 0; i < NUMBER_OF_SCREENS; i++)
        {
            if (Utils::randomNumberBetween(0, 3) == 0)
            {
                infoScreen[i]->adjustTemperature();
            }
        }
    }

}

void LiveDataDisplayView::handleClickEvent(const ClickEvent& evt)
{
    // If an animation is already in progress do not
    // react to clicks
    if (currentState != NO_ANIMATION)
    {
        // Make sure that click events outside the scroll area are propagated
        if (evt.getY() > infoScreensViewPort.getY())
        {
            View<LiveDataDisplayPresenter>::handleClickEvent(evt);
        }
        return;
    }

    if (evt.getType() == ClickEvent::RELEASED)
    {
        // Save current position for use during animation
        animateDistance = dragX;
        startX = infoScreens.getX();

        if (dragX < 0)
        {
            if (currentScreen == NUMBER_OF_SCREENS - 1 || dragX > -120)
            {
                currentState = ANIMATE_SWIPE_CANCELLED_LEFT;
            }
            else
            {
                currentState = ANIMATE_LEFT;
            }
        }
        else if (dragX > 0)
        {
            if (currentScreen == 0 || dragX < 120)
            {
                currentState = ANIMATE_SWIPE_CANCELLED_RIGHT;
            }
            else
            {
                currentState = ANIMATE_RIGHT;
            }
        }

        //adjustInfoScreens();
    }

    // Make sure that click events outside the scroll area are propagated
    if (evt.getY() > infoScreensViewPort.getY())
    {
        View<LiveDataDisplayPresenter>::handleClickEvent(evt);
        return;
    }
}

void LiveDataDisplayView::handleDragEvent(const DragEvent& evt)
{
    // If an animation is already in progress do not
    // react to drags
    if (currentState != NO_ANIMATION)
    {
        return;
    }

    dragX += evt.getDeltaX();

    // Do not show too much background next to end screens
    if (currentScreen == 0 && dragX > backgroundSwipeAreaLeft.getWidth())
    {
        dragX = backgroundSwipeAreaLeft.getWidth();
    }
    else if (currentScreen == NUMBER_OF_SCREENS - 1 && dragX < -backgroundSwipeAreaRight.getWidth())
    {
        dragX = -backgroundSwipeAreaRight.getWidth();
    }

    adjustInfoScreens();
}

void LiveDataDisplayView::handleGestureEvent(const GestureEvent& evt)
{
    // Do not accept gestures while animating
    if (currentState != NO_ANIMATION)
    {
        return;
    }

    if (evt.getType() == evt.SWIPE_HORIZONTAL)
    {
        // Save current position for use during animation
        animateDistance = dragX;
        startX = infoScreens.getX();

        if (evt.getVelocity() < 0  && currentScreen < NUMBER_OF_SCREENS - 1)
        {
            currentState = ANIMATE_LEFT;
        }
        else if (evt.getVelocity() > 0  && currentScreen > 0)
        {
            currentState = ANIMATE_RIGHT;
        }
    }
}

void LiveDataDisplayView::adjustInfoScreens()
{
    infoScreens.moveTo(- (currentScreen * HAL::DISPLAY_WIDTH) + dragX, infoScreens.getY());

    int alphaAdjustment = (dragX < 0) ? -dragX : dragX;
    weekInfoBar.setAlpha(255 - alphaAdjustment);
}

void LiveDataDisplayView::animateSwipeCancelledLeft()
{
    uint8_t duration = 14;

    if (animationCounter <= duration)
    {
        int16_t delta = EasingEquations::backEaseOut(animationCounter, 0, -animateDistance, duration);
        dragX = animateDistance + delta;

        adjustInfoScreens();
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        dragX = 0;
        adjustInfoScreens();
    }
    animationCounter++;
}

void LiveDataDisplayView::animateSwipeCancelledRight()
{
    uint8_t duration = 14;

    if (animationCounter <= duration)
    {
        int16_t delta = EasingEquations::backEaseOut(animationCounter, 0, animateDistance, duration);
        dragX = animateDistance - delta;

        adjustInfoScreens();
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        dragX = 0;
        adjustInfoScreens();
    }
    animationCounter++;
}

void LiveDataDisplayView::animateLeft()
{
    uint8_t duration = 10;

    if (animationCounter <= duration)
    {
        int16_t delta = EasingEquations::cubicEaseOut(animationCounter, 0, infoScreensViewPort.getWidth() + animateDistance, duration);
        dragX = animateDistance - delta;

        adjustInfoScreens();
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        currentScreen++;
        dragX = 0;
        weekInfoBar.setTextColor(infoScreen[currentScreen]->getTextColor());
        weekInfoBar.setInfo(infoScreen[currentScreen]->getCity());
        adjustInfoScreens();
        dotIndicator.goRight();
    }
    animationCounter++;
}

void LiveDataDisplayView::animateRight()
{
    uint8_t duration = 10;

    if (animationCounter <= duration)
    {
        int16_t delta = EasingEquations::cubicEaseOut(animationCounter, 0, infoScreensViewPort.getWidth() - animateDistance, duration);
        dragX = animateDistance + delta;

        adjustInfoScreens();
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        currentScreen--;
        dragX = 0;
        weekInfoBar.setTextColor(infoScreen[currentScreen]->getTextColor());
        weekInfoBar.setInfo(infoScreen[currentScreen]->getCity());
        adjustInfoScreens();
        dotIndicator.goLeft();
    }
    animationCounter++;
}
