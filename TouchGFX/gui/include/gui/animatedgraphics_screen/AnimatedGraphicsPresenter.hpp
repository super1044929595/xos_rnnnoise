#ifndef ANIMATEDGRAPHICS_PRESENTER_HPP
#define ANIMATEDGRAPHICS_PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class AnimatedGraphicsView;

class AnimatedGraphicsPresenter : public Presenter, public ModelListener
{
public:
    AnimatedGraphicsPresenter(AnimatedGraphicsView& v);

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

    virtual ~AnimatedGraphicsPresenter() {};

private:
    AnimatedGraphicsPresenter();

    AnimatedGraphicsView& view;
};


#endif // ANIMATEDGRAPHICS_PRESENTER_HPP
