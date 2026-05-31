#ifndef LIVEDATADISPLAY_PRESENTER_HPP
#define LIVEDATADISPLAY_PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class LiveDataDisplayView;

class LiveDataDisplayPresenter : public Presenter, public ModelListener
{
public:
    LiveDataDisplayPresenter(LiveDataDisplayView& v);

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

    virtual ~LiveDataDisplayPresenter() {};

private:
    LiveDataDisplayPresenter();

    LiveDataDisplayView& view;
};


#endif // LIVEDATADISPLAY_PRESENTER_HPP
