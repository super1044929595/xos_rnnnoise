#ifndef ANIMATEDGRAPHICS_VIEW_HPP
#define ANIMATEDGRAPHICS_VIEW_HPP

#include <gui_generated/animatedgraphics_screen/AnimatedGraphicsViewBase.hpp>
#include <gui/animatedgraphics_screen/AnimatedGraphicsPresenter.hpp>

#include <gui/animatedgraphics_screen/BumpMapImage.hpp>

class AnimatedGraphicsView : public AnimatedGraphicsViewBase
{
public:
    AnimatedGraphicsView();
    virtual ~AnimatedGraphicsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();
    virtual void handleDragEvent(const DragEvent& evt);
protected:
private:

    enum States
    {
        ANIMATE_SHADE_UP,
        NO_ANIMATION
    } currentState;

    struct
    {
        BitmapId image;
        const unsigned int* bump_map;
    } bumpMapInfo;

    uint32_t animationCounter;

    BumpMapImage bumpMapImage;

    Callback<AnimatedGraphicsView, const AbstractButton&> onButtonPressed;

    void buttonPressedhandler(const AbstractButton& button);
    void updateBumpMapImage();

    void animateShadeUp();
};

#endif // ANIMATEDGRAPHICS_VIEW_HPP
