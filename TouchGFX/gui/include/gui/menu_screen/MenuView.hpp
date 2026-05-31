#ifndef MENU_VIEW_HPP
#define MENU_VIEW_HPP

#include <gui_generated/menu_screen/MenuViewBase.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>

#include <gui/menu_screen/Tile.hpp>
#include <gui/menu_screen/LiveDataDisplayTile.hpp>
#include <gui/menu_screen/HomeAutomationTile.hpp>
#include <gui/menu_screen/AnimatedGraphicsTile.hpp>
#include <gui/menu_screen/ChromArtTile.hpp>

class MenuView : public MenuViewBase
{
    static const int TILE_OFFSET_X = 17;
    static const int TILE_OFFSET_Y = 13;

    Tile<LiveDataDisplayTile> liveDataDisplayTile;
    Tile<HomeAutomationTile> homeAutomationTile;
    Tile<AnimatedGraphicsTile> animatedGraphicsTile;
    Tile<ChromArtTile> chromArtTile;
public:
    MenuView();
    virtual ~MenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void addRoomToHomeAutomationTile(RoomTemperatureInfo& room);
    void initializeTiles();

    Callback<MenuView, const AbstractButton&> onButtonPressed;
    void buttonPressedhandler(const AbstractButton& button);
protected:
};

#endif // MENU_VIEW_HPP
