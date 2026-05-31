#include <gui/animatedgraphics_screen/AnimatedGraphicsView.hpp>

#include <gui/animatedgraphics_screen/BumpMap.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/EasingEquations.hpp>

AnimatedGraphicsView::AnimatedGraphicsView() :
    animationCounter(0),
    bumpMapImage(BITMAP_LIGHT_EFFECT_IMAGE_ID, _bump_map_touchgfx_logo, _light_source),
    onButtonPressed(this, &AnimatedGraphicsView::buttonPressedhandler)
{
    bumpMapInfo.image = BITMAP_LIGHT_EFFECT_IMAGE_ID;
    bumpMapInfo.bump_map = _bump_map_touchgfx_logo;
}

void AnimatedGraphicsView::setupScreen()
{
    bumpMapImage.setPosition(0, 0, Bitmap(BITMAP_LIGHT_EFFECT_IMAGE_ID).getWidth(), Bitmap(BITMAP_LIGHT_EFFECT_IMAGE_ID).getHeight());
    bumpMapImage.setVisible(true);
    updateBumpMapImage();

    shade.setColor(0x0);
    shade.setPosition(0, 0, Bitmap(BITMAP_LIGHT_EFFECT_IMAGE_ID).getWidth(), Bitmap(BITMAP_LIGHT_EFFECT_IMAGE_ID).getHeight() - exitButton.getHeight());
    currentState = ANIMATE_SHADE_UP;

    add(bumpMapImage);

    AnimatedGraphicsViewBase::setupScreen();

    exitButton.setAction(onButtonPressed);

    // ExitButton will be inactive until shade animation has ended
    exitButton.setTouchable(false);
}

void AnimatedGraphicsView::tearDownScreen()
{
    AnimatedGraphicsViewBase::tearDownScreen();
}


void AnimatedGraphicsView::handleTickEvent()
{
    if (currentState == ANIMATE_SHADE_UP)
    {
        animateShadeUp();
    }
}

void AnimatedGraphicsView::handleDragEvent(const DragEvent& evt)
{
    // Drag is disabled due to the capability of the resistive touch on the ST Discovery board
    //bumpMapImage.setLightPosition(evt.getNewX(), evt.getNewY());
}

void AnimatedGraphicsView::buttonPressedhandler(const AbstractButton& button)
{
    if (&button == &exitButton)
    {
        static_cast<FrontendApplication*>(Application::getInstance())->gotoMenuScreen();
    }
}

void AnimatedGraphicsView::updateBumpMapImage()
{
    bumpMapImage.setImage(Bitmap(bumpMapInfo.image));
    bumpMapImage.setBumpMap(bumpMapInfo.bump_map);
    bumpMapImage.invalidate();
}

void AnimatedGraphicsView::animateShadeUp()
{
    uint8_t animateShadeUpDuration = 18;

    if (animationCounter <= animateShadeUpDuration)
    {
        int16_t deltaBackground = EasingEquations::quadEaseIn(animationCounter, 0, shade.getHeight(), animateShadeUpDuration);
        shade.moveTo(0, -deltaBackground);
    }
    else
    {
        // Final step: stop the animation
        currentState = NO_ANIMATION;
        animationCounter = 0;
        exitButton.setTouchable(true);
    }

    animationCounter++;
}
