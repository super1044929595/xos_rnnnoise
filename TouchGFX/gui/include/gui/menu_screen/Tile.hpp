#ifndef TILE_HPP_
#define TILE_HPP_

#include <touchgfx/mixins/ClickListener.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/Color.hpp>

using namespace touchgfx;

/**
 * Container wrapping tile classes. Adding pressed and clicked events to the tile.
 */
template<class T>
class Tile : public Container
{
public:
    Tile() :
        pressedActive(false),
        onTileClicked(this, &Tile::tileClickedhandler)
    {
        setWidth(tile.getWidth() + 6);
        setHeight(tile.getHeight() + 6);

        tile.setXY(3, 3);
        tile.setClickAction(onTileClicked);

        add(tile);
    }

    virtual ~Tile()
    {

    }

    T& getTile()
    {
        return tile;
    }

private:

    ClickListener<T> tile;

    bool pressedActive;

    Callback<Tile, const T&, const ClickEvent& > onTileClicked;

    void tileClickedhandler(const T& t, const ClickEvent& event)
    {
        if (&t == &tile)
        {
            if (event.getType() == ClickEvent::RELEASED)
            {
                if (pressedActive)
                {
                    tile.gotoAssociatedScreen();
                }
            }
            else if (event.getType() == ClickEvent::PRESSED)
            {
                pressedActive = true;
            }
            else if (event.getType() == ClickEvent::CANCEL)
            {
                pressedActive = false;
            }
        }
    }
};

#endif /* TILE_HPP_ */
