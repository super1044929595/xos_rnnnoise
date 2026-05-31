#ifndef MODEL_HPP
#define MODEL_HPP

#include <gui/model/Time.hpp>
#include <gui/model/RoomTemperatureInfo.hpp>
#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    uint8_t getNumberOfRooms();
    RoomTemperatureInfo& getRoomTemperatureInfo(uint8_t roomIndex);
    RoomTemperatureInfo& getRoomTemperatureInfoById(uint8_t roomId);

    void setSelectedRoom(uint8_t roomId);
    uint8_t getSelectedRoom();
    void setRoomTemperature(int16_t temperature);

    bool getMCULoadActive() const
    {
        return mcuLoadActive;
    }
    void setMCULoadActive(bool active)
    {
        mcuLoadActive = active;
    }
protected:
    ModelListener* modelListener;

    bool mcuLoadActive;

    Time currentTime;

    RoomTemperatureInfo roomTemperatureInfo[10];
    uint8_t numberOfRooms;
    uint8_t selectedRoom;
};

#endif /* MODEL_HPP */
