#ifndef HOMEAUTOMATION_VIEW_HPP
#define HOMEAUTOMATION_VIEW_HPP

#include <gui_generated/homeautomation_screen/HomeAutomationViewBase.hpp>
#include <gui/homeautomation_screen/HomeAutomationPresenter.hpp>

#include <gui/homeautomation_screen/JogWheel.hpp>
//#include <gui/homeautomation_screen/TemperatureMenuItem.hpp>
#include <gui/containers/TemperatureMenuItem.hpp>
#include <gui/homeautomation_screen/TemperatureScheduleMenuItem.hpp>
#include <gui/homeautomation_screen/TemperatureSlider.hpp>

class HomeAutomationView : public HomeAutomationViewBase
{
public:
    HomeAutomationView();
    virtual ~HomeAutomationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();

    // New Info from the presenter
    void insertRoom(RoomTemperatureInfo& room);
    void updateRoom(RoomTemperatureInfo& room);

    void setSelectedMenuItem(RoomTemperatureInfo& room);
    void saveScheduleInfo(RoomTemperatureInfo& room);
protected:
private:
    enum States
    {
        ANIMATE_JOG_WHEEL_IN,
        ANIMATE_JOG_WHEEL_OUT,
        ANIMATE_SCHEDULE_IN,
        ANIMATE_SCHEDULE_OUT,
        JOG_WHEEL_STATE,
        SCHEDULE_STATE,
        ROOM_STATE
    } currentState;

    uint8_t animationCounter;

    static const int16_t INITIAL_TEMPERATURE = 6;
    static const int16_t JOGWHEEL_DELTA = 6;

    static const uint8_t NUMBER_OF_ROOMS = 8;
    TemperatureMenuItem* menuItems[NUMBER_OF_ROOMS];
    uint8_t currentNumberOfRooms;
    TEXTS selectedRoomName;

    // Jog wheel widgets
    JogWheel jogWheel;

    // Scrollmenu in schedule mode
    TemperatureScheduleMenuItem scheduleMenuItems[RoomTemperatureInfo::NUMBER_OF_DAYS];

    TemperatureSlider slider;

    Callback<HomeAutomationView, const AbstractButton&> onButtonPressed;
    Callback<HomeAutomationView, int16_t> onJogWheelValueChanged;
    Callback<HomeAutomationView, int16_t> onJogWheelEndDragEvent;
    Callback<HomeAutomationView, const TemperatureMenuItem&> onMenuItemSelected;
    Callback<HomeAutomationView, const TemperatureScheduleMenuItem&> onScheduleMenuItemSelected;
    Callback<HomeAutomationView, const TemperatureScheduleMenuItem&> onScheduleMenuItemValueUpdated;

    void buttonPressedhandler(const AbstractButton& button);
    void jogWheelValueChanged(int16_t value);
    void jogWheelEndDragEvent(int16_t value);
    void menuItemSelected(const TemperatureMenuItem&);
    void scheduleMenuItemSelected(const TemperatureScheduleMenuItem&);
    void scheduleMenuItemValueUpdated(const TemperatureScheduleMenuItem&);

    void setCurrentState(States newState);
    void animateJogWheelIn();
    void animateJogWheelOut();
    void animateScheduleIn();
    void animateScheduleOut();

    void showRoomElements(uint8_t startAlpha);
    void showScheduleElements(uint8_t startAlpha);
    void setActiveStateForRoomElements(bool active);
    void setActiveStateForScheduleElements(bool active);
    void setAlphaForRoomElements(uint8_t alpha);
    void setAlphaForScheduleElements(uint8_t alpha);
    void hideAllElements();

    void setOkButtonState(bool valuesUpdated);
};

#endif // HOMEAUTOMATION_VIEW_HPP
