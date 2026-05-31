#ifndef WEEKINFO_HPP
#define WEEKINFO_HPP

#include <gui_generated/containers/WeekInfoBase.hpp>

#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <gui/containers/CityInfo.hpp>

class WeekInfo : public WeekInfoBase
{
public:
    WeekInfo();
    virtual ~WeekInfo() {}

    virtual void initialize();

    void setAlpha(int alpha);
    void setTextColor(colortype color);

    void setInfo(CityInfo::Cities city);

private:
    static const int NUMBER_OF_DAYS = 3;

    Image* smallIcons[NUMBER_OF_DAYS];
    TextArea* days[NUMBER_OF_DAYS];
    TextAreaWithOneWildcard* smallTemperatures[NUMBER_OF_DAYS];
    TextAreaWithOneWildcard* smallTemperatureDropShadows[NUMBER_OF_DAYS];
    Unicode::UnicodeChar* smallTemperatureBuffers[NUMBER_OF_DAYS];
    Unicode::UnicodeChar* smallTemperatureDropShadowsBuffers[NUMBER_OF_DAYS];
};

#endif // WEEKINFO_HPP
