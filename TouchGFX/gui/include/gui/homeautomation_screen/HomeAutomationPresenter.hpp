#ifndef HOMEAUTOMATION_PRESENTER_HPP
#define HOMEAUTOMATION_PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class HomeAutomationView;

class HomeAutomationPresenter : public Presenter, public ModelListener
{
public:
    HomeAutomationPresenter(HomeAutomationView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~HomeAutomationPresenter() {};

    void roomSelected(uint8_t roomId);
    void newRoomTemperature(int16_t temperature);

    virtual void roomUpdated(RoomTemperatureInfo& room);
    void newScheduleAccepted();
    void getNewSchedule(RoomTemperatureInfo& room);
    void exitScreen();

    RoomTemperatureInfo& getSelectedRoom();
private:
    HomeAutomationPresenter();

    HomeAutomationView& view;
};


#endif // HOMEAUTOMATION_PRESENTER_HPP
