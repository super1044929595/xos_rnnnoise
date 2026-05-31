#include <gui/menu_screen/ChromArtTile.hpp>
#include <BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/widgets/Button.hpp>
#include <touchgfx/EasingEquations.hpp>
#include <touchgfx/Color.hpp>

ChromArtTile::ChromArtTile()
{
    setWidth(90);
    setHeight(70);

    tileId.setColor(Color::getColorFromRGB(0xFF, 0xFF, 0xFF));
    tileId.setTypedText(TypedText(T_CHROM_ART_TILE));

#ifndef SIMULATOR
    // find out if platform has ChromArt hardware
    BlitOperations BlitOps = HAL::getInstance()->getBlitCaps();
    if (!(BlitOps != static_cast<BlitOperations>(0)))
    {
        tileId.setTypedText(TypedText(T_CHROM_ART_TILE_NO_CHROM_ART));
    }
#endif

    tileId.setPosition(0, getHeight() - 30, getWidth(), 20);

    add(tileId);
}

ChromArtTile::~ChromArtTile()
{
}
