#include <gui/menu_screen/AnimatedGraphicsTile.hpp>
#include <BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/widgets/Button.hpp>
#include <touchgfx/EasingEquations.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Color.hpp>

AnimatedGraphicsTile::AnimatedGraphicsTile()
{
    setWidth(90);
    setHeight(70);

    tileId.setColor(Color::getColorFromRGB(0xFF, 0xFF, 0xFF));
    tileId.setTypedText(TypedText(T_ANIMATED_GRAPHICS_TILE));
    tileId.setPosition(3, getHeight() - 30, getWidth(), 20);

    add(tileId);
}

AnimatedGraphicsTile::~AnimatedGraphicsTile()
{
}
