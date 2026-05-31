#include <gui/containers/WeekInfo.hpp>

#include "BitmapDatabase.hpp"
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

WeekInfo::WeekInfo()
{
    smallIcons[0] = &smallIcon0;
    smallIcons[1] = &smallIcon1;
    smallIcons[2] = &smallIcon2;

    days[0] = &day0;
    days[1] = &day1;
    days[2] = &day2;

    smallTemperatures[0] = &smallTemperature0;
    smallTemperatures[1] = &smallTemperature1;
    smallTemperatures[2] = &smallTemperature2;
    smallTemperatureBuffers[0] = smallTemperature0Buffer;
    smallTemperatureBuffers[1] = smallTemperature1Buffer;
    smallTemperatureBuffers[2] = smallTemperature2Buffer;

    smallTemperatureDropShadows[0] = &smallTemperatureDropShadow0;
    smallTemperatureDropShadows[1] = &smallTemperatureDropShadow1;
    smallTemperatureDropShadows[2] = &smallTemperatureDropShadow2;
    smallTemperatureDropShadowsBuffers[0] = smallTemperatureDropShadow0Buffer;
    smallTemperatureDropShadowsBuffers[1] = smallTemperatureDropShadow1Buffer;
    smallTemperatureDropShadowsBuffers[2] = smallTemperatureDropShadow2Buffer;
}

void WeekInfo::initialize()
{
    WeekInfoBase::initialize();
}

/* Setup the WeekInfo with some static city information
 * In a real world example this data would be live updated
 * and come from the model, but in this demo it is just hard coded.
 */
void WeekInfo::setInfo(CityInfo::Cities city)
{
    switch (city)
    {
    case CityInfo::COPENHAGEN:
        smallIcons[0]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_ID));
        smallIcons[1]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_BIGCLOUD_ID));
        smallIcons[2]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_ID));
        Unicode::snprintf(smallTemperatureBuffers[0], SMALLTEMPERATURE0_SIZE, "%d", 23);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[0], SMALLTEMPERATUREDROPSHADOW0_SIZE, "%d", 23);
        Unicode::snprintf(smallTemperatureBuffers[1], SMALLTEMPERATURE1_SIZE, "%d", 16);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[1], SMALLTEMPERATUREDROPSHADOW1_SIZE, "%d", 16);
        Unicode::snprintf(smallTemperatureBuffers[2], SMALLTEMPERATURE2_SIZE, "%d", 24);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[2], SMALLTEMPERATUREDROPSHADOW2_SIZE, "%d", 24);
        break;

    case CityInfo::HONG_KONG:
        smallIcons[0]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_ID));
        smallIcons[1]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_BIGCLOUD_ID));
        smallIcons[2]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SHOWER_ID));
        Unicode::snprintf(smallTemperatureBuffers[0], SMALLTEMPERATURE0_SIZE, "%d", 23);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[0], SMALLTEMPERATUREDROPSHADOW0_SIZE, "%d", 23);
        Unicode::snprintf(smallTemperatureBuffers[1], SMALLTEMPERATURE1_SIZE, "%d", 19);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[1], SMALLTEMPERATUREDROPSHADOW1_SIZE, "%d", 19);
        Unicode::snprintf(smallTemperatureBuffers[2], SMALLTEMPERATURE2_SIZE, "%d", 21);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[2], SMALLTEMPERATUREDROPSHADOW2_SIZE, "%d", 21);
        break;

    case CityInfo::MUMBAI:
        smallIcons[0]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_ID));
        smallIcons[1]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_BIGCLOUD_ID));
        smallIcons[2]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_ID));
        Unicode::snprintf(smallTemperatureBuffers[0], SMALLTEMPERATURE0_SIZE, "%d", 32);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[0], SMALLTEMPERATUREDROPSHADOW0_SIZE, "%d", 32);
        Unicode::snprintf(smallTemperatureBuffers[1], SMALLTEMPERATURE1_SIZE, "%d", 28);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[1], SMALLTEMPERATUREDROPSHADOW1_SIZE, "%d", 28);
        Unicode::snprintf(smallTemperatureBuffers[2], SMALLTEMPERATURE2_SIZE, "%d", 27);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[2], SMALLTEMPERATUREDROPSHADOW2_SIZE, "%d", 27);
        break;

    case CityInfo::NEW_YORK:
        smallIcons[0]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SUN_BIGCLOUD_ID));
        smallIcons[1]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SHOWER_ID));
        smallIcons[2]->setBitmap(Bitmap(BITMAP_WEATHER_SMALL_ICON_SHOWER_ID));
        Unicode::snprintf(smallTemperatureBuffers[0], SMALLTEMPERATURE0_SIZE, "%d", 20);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[0], SMALLTEMPERATUREDROPSHADOW0_SIZE, "%d", 20);
        Unicode::snprintf(smallTemperatureBuffers[1], SMALLTEMPERATURE1_SIZE, "%d", 19);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[1], SMALLTEMPERATUREDROPSHADOW1_SIZE, "%d", 20);
        Unicode::snprintf(smallTemperatureBuffers[2], SMALLTEMPERATURE2_SIZE, "%d", 19);
        Unicode::snprintf(smallTemperatureDropShadowsBuffers[2], SMALLTEMPERATUREDROPSHADOW2_SIZE, "%d", 20);
        break;

    default:
        break;
    }

    for (int i = 0; i < NUMBER_OF_DAYS; i++)
    {
        smallIcons[i]->invalidate();
        smallTemperatures[i]->invalidate();
        smallTemperatureDropShadows[i]->invalidate();
    }
}

void WeekInfo::setAlpha(int alpha)
{
    if (alpha < 0)
    {
        alpha = 0;
    }
    if (alpha > 255)
    {
        alpha = 255;
    }

    for (int i = 0; i < NUMBER_OF_DAYS; i++)
    {
        smallIcons[i]->setAlpha(alpha);
        smallIcons[i]->invalidate();

        smallTemperatures[i]->setAlpha(alpha);
        smallTemperatures[i]->invalidate();

        smallTemperatureDropShadows[i]->setAlpha(alpha);
        smallTemperatureDropShadows[i]->invalidate();

        days[i]->setAlpha(alpha);
        days[i]->invalidate();
    }
}

void WeekInfo::setTextColor(colortype color)
{
    for (int i = 0; i < NUMBER_OF_DAYS; i++)
    {
        days[i]->setColor(color);
        days[i]->invalidate();
    }
}
