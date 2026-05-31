#include <gui/menu_screen/MenuView.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>

MenuPresenter::MenuPresenter(MenuView& v)
    : view(v)
{
}

void MenuPresenter::activate()
{
    for (int i = 0; i < model->getNumberOfRooms(); i++)
    {
        // Avoid "Master Bedroom" - name too long :)
        if (i != 1)
        {
            view.addRoomToHomeAutomationTile(model->getRoomTemperatureInfo(i));
        }
    }
    view.initializeTiles();
}

void MenuPresenter::deactivate()
{

}
