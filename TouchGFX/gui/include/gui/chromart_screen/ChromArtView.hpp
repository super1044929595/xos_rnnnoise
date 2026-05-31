#ifndef CHROMART_VIEW_HPP
#define CHROMART_VIEW_HPP

#include <gui_generated/chromart_screen/ChromArtViewBase.hpp>
#include <gui/chromart_screen/ChromArtPresenter.hpp>

#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class ChromArtView : public ChromArtViewBase
{
public:
    ChromArtView();
    virtual ~ChromArtView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();

    void updateMCULoad(uint8_t mcuLoad);
protected:
private:
    static const uint8_t CLOUD_SPACING = 100;

    enum States
    {
        ANIMATE_STARTUP,
        ANIMATE_TO_NEXT_SETUP,
        NO_ANIMATION
    } currentState;

    uint32_t animationCounter;
    uint32_t tickCounter;
    bool ChromArtSupported;

    // The element that the next step animation should
    // use as previous element when inserting elements
    Drawable* nextInsertElement;

    Callback<ChromArtView, const AbstractButton&> onButtonPressed;
    void buttonPressedhandler(const AbstractButton& button);

    void animateStartUp();
    void animateToNextSetup();
    void animateClouds();
};

#endif // CHROMART_VIEW_HPP
