#include <gui/menu_screen/MenuView.hpp>

MenuView::MenuView() :
    onButtonPressed(this, &MenuView::buttonPressedhandler)
{

}

void MenuView::setupScreen()
{
    MenuViewBase::setupScreen();

    liveDataDisplayTile.setXY(TILE_OFFSET_X, container.getHeight() - liveDataDisplayTile.getHeight() - TILE_OFFSET_Y + 2);

    homeAutomationTile.setXY(TILE_OFFSET_X, TILE_OFFSET_Y);

    animatedGraphicsTile.setXY(TILE_OFFSET_X, homeAutomationTile.getY() + homeAutomationTile.getHeight() + TILE_OFFSET_Y - 4);

    chromArtTile.setXY(animatedGraphicsTile.getX() + animatedGraphicsTile.getWidth() + TILE_OFFSET_X - 3, animatedGraphicsTile.getY());

    add(liveDataDisplayTile);
    add(homeAutomationTile);
    add(animatedGraphicsTile);
    add(chromArtTile);
}

void MenuView::tearDownScreen()
{
    MenuViewBase::tearDownScreen();
}

void MenuView::buttonPressedhandler(const AbstractButton& button)
{

}

void MenuView::addRoomToHomeAutomationTile(RoomTemperatureInfo& room)
{
    homeAutomationTile.getTile().addRoom(room);
}

void MenuView::initializeTiles()
{
    homeAutomationTile.getTile().initialize();
}
