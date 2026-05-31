#ifndef TEMPERATUREMENUITEM_HPP
#define TEMPERATUREMENUITEM_HPP

#include <gui_generated/containers/TemperatureMenuItemBase.hpp>

#include <touchgfx/widgets/TouchArea.hpp>
#include <gui/model/RoomTemperatureInfo.hpp>
#include <BitmapDatabase.hpp>

class TemperatureMenuItem : public TemperatureMenuItemBase
{
public:
    TemperatureMenuItem();
    virtual ~TemperatureMenuItem() {}

    virtual void initialize();
    void setMenuItemSelectedCallback(GenericCallback< const TemperatureMenuItem& >& callback)
    {
        menuItemSelectedCallback = &callback;
    }

    void setRoomTemperatureInfo(RoomTemperatureInfo& info);

    void setSelected(bool sel);

    void setRoomId(uint8_t id)
    {
        roomId = id;
    }
    uint8_t getRoomId() const
    {
        return roomId;
    }

    void setTextsAlpha(int16_t alpha);

private:
    uint8_t roomId;

    TouchArea itemSelectArea;

    bool selected;

    GenericCallback< const TemperatureMenuItem& >* menuItemSelectedCallback;

    Callback<TemperatureMenuItem, const AbstractButton&> onButtonPressed;

    void buttonPressedhandler(const AbstractButton& button);
};

#endif // TEMPERATUREMENUITEM_HPP
