#ifndef ANIMATEDGRAPHICSTILE_HPP_
#define ANIMATEDGRAPHICSTILE_HPP_

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/TouchArea.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <gui/common/FrontendApplication.hpp>

using namespace touchgfx;

class AnimatedGraphicsTile : public Container
{
public:
    AnimatedGraphicsTile();
    virtual ~AnimatedGraphicsTile();

    int16_t getWidth()
    {
        return Container::getWidth();
    }
    int16_t getHeight()
    {
        return Container::getHeight();
    }

    void gotoAssociatedScreen()
    {
        static_cast<FrontendApplication*>(Application::getInstance())->gotoAnimatedGraphicsScreen();
    }
private:
    TextArea tileId;
};

#endif /* ANIMATEDGRAPHICSTILE_HPP_ */
