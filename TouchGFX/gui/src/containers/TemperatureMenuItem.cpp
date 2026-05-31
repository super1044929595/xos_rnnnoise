#include <gui/containers/TemperatureMenuItem.hpp>
#include <touchgfx/Color.hpp>

TemperatureMenuItem::TemperatureMenuItem() :
    onButtonPressed(this, &TemperatureMenuItem::buttonPressedhandler)
{

}

void TemperatureMenuItem::initialize()
{
    TemperatureMenuItemBase::initialize();

    itemSelectArea.setPosition(0, 0, background.getWidth(), background.getHeight());
    itemSelectArea.setAction(onButtonPressed);

    setSelected(false);

    add(itemSelectArea);

    setWidth(background.getWidth());
    setHeight(background.getHeight());
}

void TemperatureMenuItem::buttonPressedhandler(const AbstractButton& button)
{
    if (&button == &itemSelectArea)
    {
        if (menuItemSelectedCallback)
        {
            menuItemSelectedCallback->execute(*this);
        }
    }
}

void TemperatureMenuItem::setRoomTemperatureInfo(RoomTemperatureInfo& info)
{
    primaryText.setTypedText(TypedText(info.getRoomName()));
    primaryText.invalidate();

    Unicode::snprintf(currentTemperatureBuffer, CURRENTTEMPERATURE_SIZE, "%d", info.getCurrentTemperature());
    currentTemperature.invalidate();
}

void TemperatureMenuItem::setSelected(bool sel)
{
    selected = sel;

    if (selected)
    {
        background.setBitmap(Bitmap(BITMAP_TEMPERATURE_MENU_ITEM_SELECTED_ID));
        primaryText.setColor(Color::getColorFromRGB(0xFF, 0xFF, 0xFF));
        currentTemperature.setColor(Color::getColorFromRGB(0xFF, 0xFF, 0xFF));
    }
    else
    {
        background.setBitmap(Bitmap(BITMAP_TEMPERATURE_MENU_ITEM_ID));
        primaryText.setColor(Color::getColorFromRGB(0xA9, 0xAD, 0xB6));
        currentTemperature.setColor(Color::getColorFromRGB(0xA9, 0xAD, 0xB6));
    }
    background.invalidate();
    primaryText.invalidate();
    currentTemperature.invalidate();
}

void TemperatureMenuItem::setTextsAlpha(int16_t alpha)
{
    alpha = (alpha < 0) ? 0 : alpha;
    alpha = (alpha > 255) ? 255 : alpha;

    primaryText.setAlpha((uint8_t) alpha);
    currentTemperature.setAlpha((uint8_t) alpha);

    primaryText.invalidate();
    currentTemperature.invalidate();
}
