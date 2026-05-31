#include <gui/containers/CityInfo.hpp>
#include <gui/common/Utils.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>
#include <stdlib.h>

CityInfo::CityInfo()
{

}

void CityInfo::initialize()
{
    CityInfoBase::initialize();
    textColor = 0x0;

    cityNameDropShadow.setPosition(cityName.getX() + 1, cityName.getY() + 1, cityName.getWidth(), cityName.getHeight());
    cityNameDropShadow.invalidate();

    timeAndDateDropShadow.setPosition(timeAndDate.getX() + 1, timeAndDate.getY() + 1, timeAndDate.getWidth(), timeAndDate.getHeight());
    timeAndDateDropShadow.invalidate();
}

void CityInfo::setBitmap(BitmapId bg)
{
    background.setBitmap(Bitmap(bg));

    setWidth(background.getWidth());
    setHeight(background.getHeight());
}

/* Setup the CityInfo with some static city information
 * In a real world example this data would be live updated
 * and come from the model, but in this demo it is just hard coded.
 */
void CityInfo::setCity(Cities c)
{
    city = c;

    switch (city)
    {
    case COPENHAGEN:
        cityName.setTypedText(TypedText(T_WEATHER_CITY_0));
        cityNameDropShadow.setTypedText(TypedText(T_WEATHER_CITY_0));
        timeAndDate.setTypedText(TypedText(T_WEATHER_TIME_INFO_0));
        timeAndDateDropShadow.setTypedText(TypedText(T_WEATHER_TIME_INFO_0));
        startTemperature = 12;
        break;

    case MUMBAI:
        cityName.setTypedText(TypedText(T_WEATHER_CITY_1));
        cityNameDropShadow.setTypedText(TypedText(T_WEATHER_CITY_1));
        timeAndDate.setTypedText(TypedText(T_WEATHER_TIME_INFO_1));
        timeAndDateDropShadow.setTypedText(TypedText(T_WEATHER_TIME_INFO_1));
        startTemperature = 32;
        break;

    case HONG_KONG:
        cityName.setTypedText(TypedText(T_WEATHER_CITY_2));
        cityNameDropShadow.setTypedText(TypedText(T_WEATHER_CITY_2));
        timeAndDate.setTypedText(TypedText(T_WEATHER_TIME_INFO_2));
        timeAndDateDropShadow.setTypedText(TypedText(T_WEATHER_TIME_INFO_2));
        startTemperature = 24;
        break;

    case NEW_YORK:
        cityName.setTypedText(TypedText(T_WEATHER_CITY_3));
        cityNameDropShadow.setTypedText(TypedText(T_WEATHER_CITY_3));
        timeAndDate.setTypedText(TypedText(T_WEATHER_TIME_INFO_3));
        timeAndDateDropShadow.setTypedText(TypedText(T_WEATHER_TIME_INFO_3));
        startTemperature = 16;
        break;

    default:
        break;
    }

    setTemperature(startTemperature);

    cityName.invalidate();
    cityNameDropShadow.invalidate();
    timeAndDate.invalidate();
    timeAndDateDropShadow.invalidate();
}

void CityInfo::setTemperature(int16_t newTemperature)
{
    currentTemperature = newTemperature;
    Unicode::snprintf(largeTemperatureBuffer, LARGETEMPERATURE_SIZE, "%d", currentTemperature);
    Unicode::snprintf(largeTemperatureDropShadowBuffer, LARGETEMPERATUREDROPSHADOW_SIZE, "%d", currentTemperature);
    largeTemperature.invalidate();
    largeTemperatureDropShadow.invalidate();
}

void CityInfo::adjustTemperature()
{
    // Make sure that the temperature does not drift too far from the starting point
    if (currentTemperature - 2 == startTemperature)
    {
        setTemperature(currentTemperature - 1);
    }
    else if (currentTemperature + 2 == startTemperature)
    {
        setTemperature(currentTemperature + 1);
    }
    else
    {
        setTemperature(currentTemperature + ((Utils::randomNumberBetween(0, 1)) ? 1 : -1));
    }


}
