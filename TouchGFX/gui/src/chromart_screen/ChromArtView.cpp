#include <gui/chromart_screen/ChromArtView.hpp>

#include <touchgfx/Color.hpp>
#include <touchgfx/EasingEquations.hpp>

#ifndef SIMULATOR
#include <touchgfx/hal/HAL.hpp>
#include "BitmapDatabase.hpp"
bool chromArtEnabeld;
#endif

ChromArtView::ChromArtView() :
    currentState(ANIMATE_STARTUP),
    animationCounter(0),
    tickCounter(0),
    ChromArtSupported(false),
    onButtonPressed(this, &ChromArtView::buttonPressedhandler)
{

}

void ChromArtView::setupScreen()
{
    ChromArtViewBase::setupScreen();

    clouds0.setAlpha(0);
    clouds1.setAlpha(0);
    tree.setAlpha(0);
    stLogo.setAlpha(0);
    touchGFXLogo.setAlpha(0);

    exitButton.setAction(onButtonPressed);
    nextButton.setAction(onButtonPressed);

#ifndef SIMULATOR
    // find out if platform has ChromArt hardware
    BlitOperations BlitOps = HAL::getInstance()->getBlitCaps();
    if (BlitOps != static_cast<BlitOperations>(0))
    {
        ChromArtSupported = true;
        chromArtOnOffButton.setBitmaps(Bitmap(BITMAP_CHROM_ART_ON_BUTTON_ID), Bitmap(BITMAP_CHROM_ART_OFF_BUTTON_ID));
        chromArtOnOffButton.setXY(16, background.getHeight() - chromArtOnOffButton.getHeight() - 10);
        chromArtOnOffButton.setAction(onButtonPressed);
        chromArtOnOffButton.setVisible(true);

        chromArtEnabeld = true;
        HAL::getInstance()->enableDMAAcceleration(chromArtEnabeld);
    }
#endif

    nextInsertElement = &tree;

#ifndef SIMULATOR
    if (!ChromArtSupported)
    {
        viewPort.remove(chromArtOnOffButton);
    }
#endif
}

void ChromArtView::tearDownScreen()
{
#ifndef SIMULATOR
    chromArtEnabeld = true;
    HAL::getInstance()->enableDMAAcceleration(chromArtEnabeld);
#endif
    viewPort.setVisible(false);
    ChromArtViewBase::tearDownScreen();
}


void ChromArtView::handleTickEvent()
{
    tickCounter++;

    if (currentState == ANIMATE_TO_NEXT_SETUP)
    {
        animateToNextSetup();
    }
    else if (currentState == ANIMATE_STARTUP)
    {
        animateStartUp();
    }

    animateClouds();
}

void ChromArtView::buttonPressedhandler(const AbstractButton& button)
{
    if (&button == &nextButton)
    {
        nextButton.setTouchable(false);

        currentState = ANIMATE_TO_NEXT_SETUP;
        animationCounter = 0;
    }
    else if (&button == &chromArtOnOffButton)
    {
        if (ChromArtSupported)
        {
#ifndef SIMULATOR
            chromArtEnabeld = !chromArtEnabeld;
            HAL::getInstance()->enableDMAAcceleration(chromArtEnabeld);
#endif
        }
    }
    else if (&button == &exitButton)
    {
        static_cast<FrontendApplication*>(Application::getInstance())->gotoMenuScreen();
    }
}

void ChromArtView::animateToNextSetup()
{
    uint8_t fadeDuration = 18;

    if (animationCounter < fadeDuration)
    {
        int16_t alpha = EasingEquations::quadEaseIn(animationCounter, 0, 255, fadeDuration - 1);
        stLogo.setAlpha((uint8_t)(255 - alpha));
        touchGFXLogo.setAlpha((uint8_t)(255 - alpha));
        stLogo.invalidate();
        touchGFXLogo.invalidate();
    }
    else if (animationCounter == fadeDuration)
    {
        viewPort.remove(stLogo);
        viewPort.insert(nextInsertElement, stLogo);

        viewPort.remove(touchGFXLogo);
        viewPort.insert(&stLogo, touchGFXLogo);
        viewPort.invalidate();

        // Prepare the next insertion
        if (nextInsertElement == &tree)
        {
            nextInsertElement = &background;
        }
        else if (nextInsertElement == &background)
        {
            nextInsertElement = &clouds1;
        }
        else
        {
            nextInsertElement = &tree;
        }
    }
    else if (animationCounter < (uint8_t)(fadeDuration * 2))
    {
        int16_t alpha = EasingEquations::quadEaseIn(animationCounter - fadeDuration, 0, 255, fadeDuration - 1);
        stLogo.setAlpha((uint8_t) alpha);
        touchGFXLogo.setAlpha((uint8_t) alpha);
        stLogo.invalidate();
        touchGFXLogo.invalidate();
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        nextButton.setTouchable(true);
    }

    animationCounter++;
}

void ChromArtView::animateStartUp()
{
    uint8_t fadeDuration = 34;

    if (animationCounter < fadeDuration)
    {
        int16_t alpha = EasingEquations::quadEaseIn(animationCounter, 0, 255, fadeDuration - 1);
        clouds0.setAlpha((uint8_t) alpha);
        clouds1.setAlpha((uint8_t) alpha);
        clouds0.invalidate();
        clouds1.invalidate();
    }
    else if (animationCounter < (uint8_t)(fadeDuration * 2))
    {
        int16_t alpha = EasingEquations::quadEaseIn(animationCounter - fadeDuration, 0, 255, fadeDuration - 1);
        stLogo.setAlpha((uint8_t) alpha);
        stLogo.invalidate();
    }
    else if (animationCounter < (uint8_t)(fadeDuration * 3))
    {
        int16_t alpha = EasingEquations::quadEaseIn(animationCounter - (fadeDuration * 2), 0, 255, fadeDuration - 1);
        touchGFXLogo.setAlpha((uint8_t) alpha);
        touchGFXLogo.invalidate();
    }
    else if (animationCounter < (uint8_t)(fadeDuration * 4))
    {
        int16_t alpha = EasingEquations::quadEaseIn(animationCounter - (fadeDuration * 3), 0, 255, fadeDuration - 1);
        tree.setAlpha((uint8_t) alpha);
        tree.invalidate();
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        nextButton.setTouchable(true);
    }

    animationCounter++;
}

void ChromArtView::animateClouds()
{
    clouds0.moveTo(clouds0.getX() - 1, clouds0.getY());
    clouds1.moveTo(clouds1.getX() - 1, clouds1.getY());

    if (clouds0.getX() + clouds0.getWidth() < 0)
    {
        clouds0.moveTo(clouds1.getX() + clouds1.getWidth() + CLOUD_SPACING, clouds0.getY());
    }
    else if (clouds1.getX() + clouds1.getWidth() < 0)
    {
        clouds1.moveTo(clouds0.getX() + clouds0.getWidth() + CLOUD_SPACING, clouds1.getY());
    }
}

void ChromArtView::updateMCULoad(uint8_t mcuLoad)
{
    Unicode::snprintf(mcuLoadTxtBuffer, MCULOADTXT_SIZE, "%d", mcuLoad);
    mcuLoadTxt.invalidate();
}
