#include <gui/homeautomation_screen/HomeAutomationView.hpp>
#include <gui/homeautomation_screen/HomeAutomationPresenter.hpp>

HomeAutomationPresenter::HomeAutomationPresenter(HomeAutomationView& v)
    : view(v)
{
}

void HomeAutomationPresenter::activate()
{
    for (uint8_t i = 0; i < model->getNumberOfRooms(); i++)
    {
        view.insertRoom(model->getRoomTemperatureInfo(i));
    }
    roomSelected(model->getSelectedRoom());
}

void HomeAutomationPresenter::deactivate()
{
}

void HomeAutomationPresenter::roomUpdated(RoomTemperatureInfo& room)
{
    view.updateRoom(room);
}

void HomeAutomationPresenter::roomSelected(uint8_t roomId)
{
    model->setSelectedRoom(roomId);
    view.setSelectedMenuItem(model->getRoomTemperatureInfoById(roomId));
}

void HomeAutomationPresenter::newRoomTemperature(int16_t temperature)
{
    model->setRoomTemperature(temperature);
}

void HomeAutomationPresenter::newScheduleAccepted()
{
    view.saveScheduleInfo(model->getRoomTemperatureInfoById(model->getSelectedRoom()));
}

RoomTemperatureInfo& HomeAutomationPresenter::getSelectedRoom()
{
    return model->getRoomTemperatureInfo(model->getSelectedRoom());
}

void HomeAutomationPresenter::exitScreen()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoMenuScreen();
}
