#include <gui/chromart_screen/ChromArtView.hpp>
#include <gui/chromart_screen/ChromArtPresenter.hpp>

ChromArtPresenter::ChromArtPresenter(ChromArtView& v)
    : view(v)
{
}

void ChromArtPresenter::activate()
{

}

void ChromArtPresenter::deactivate()
{

}

void ChromArtPresenter::mcuLoadUpdated(uint8_t mcuLoad)
{
    view.updateMCULoad(mcuLoad);
}
