#ifndef CITYINFO_HPP
#define CITYINFO_HPP

#include <gui_generated/containers/CityInfoBase.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class CityInfo : public CityInfoBase
{
public:
    CityInfo();
    virtual ~CityInfo() {}

    virtual void initialize();

    enum Cities
    {
        COPENHAGEN,
        HONG_KONG,
        MUMBAI,
        NEW_YORK,
        NUMBER_OF_CITIES
    } city;

    void setBitmap(BitmapId bg);

    // Set the text color of background related text
    void setTextColor(colortype color)
    {
        textColor = color;
    }
    colortype getTextColor()
    {
        return textColor;
    }

    void setCity(Cities c);
    Cities getCity()
    {
        return city;
    }

    void setTemperature(int16_t newTemperature);
    int16_t getTemperature()
    {
        return currentTemperature;
    }

    // Randomly adjust temperature.
    void adjustTemperature();

private:
    colortype textColor;

    int16_t startTemperature;
    int16_t currentTemperature;
};

#endif // CITYINFO_HPP
