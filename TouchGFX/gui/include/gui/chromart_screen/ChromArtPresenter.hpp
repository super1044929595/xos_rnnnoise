#ifndef CHROMART_PRESENTER_HPP
#define CHROMART_PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ChromArtView;

class ChromArtPresenter : public Presenter, public ModelListener
{
public:
    ChromArtPresenter(ChromArtView& v);

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

    virtual ~ChromArtPresenter() {};

    virtual void mcuLoadUpdated(uint8_t mcuLoad);
private:
    ChromArtPresenter();

    ChromArtView& view;
};


#endif // CHROMART_PRESENTER_HPP
