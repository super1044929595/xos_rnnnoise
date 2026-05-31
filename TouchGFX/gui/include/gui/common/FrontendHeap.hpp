#ifndef FRONTENDHEAP_HPP
#define FRONTENDHEAP_HPP

#include <gui_generated/common/FrontendHeapBase.hpp>
#include <gui/common/SlowSlideTransition.hpp>
#include <gui/homeautomation_screen/HomeAutomationView.hpp>
#include <gui/homeautomation_screen/HomeAutomationPresenter.hpp>
#include <gui/animatedgraphics_screen/AnimatedGraphicsView.hpp>
#include <gui/animatedgraphics_screen/AnimatedGraphicsPresenter.hpp>
#include <gui/livedatadisplay_screen/LiveDataDisplayView.hpp>
#include <gui/livedatadisplay_screen/LiveDataDisplayPresenter.hpp>
#include <gui/chromart_screen/ChromArtView.hpp>
#include <gui/chromart_screen/ChromArtPresenter.hpp>

class FrontendHeap : public FrontendHeapBase
{
public:
    /**
     * A list of all view types. Must end with meta::Nil.
     * @note All view types used in the application MUST be added to this list!
     */
    typedef meta::TypeList< MenuView,
            meta::TypeList< HomeAutomationView,
            meta::TypeList< AnimatedGraphicsView,
            meta::TypeList< LiveDataDisplayView,
            meta::TypeList< ChromArtView,
            meta::Nil
            > > > > > UserDefinedViewTypes;

    /**
     * A list of all presenter types. Must end with meta::Nil.
     * @note All presenter types used in the application MUST be added to this list!
     */
    typedef meta::TypeList< MenuPresenter,
            meta::TypeList< HomeAutomationPresenter,
            meta::TypeList< AnimatedGraphicsPresenter,
            meta::TypeList< LiveDataDisplayPresenter,
            meta::TypeList< ChromArtPresenter,
            meta::Nil
            > > > > > UserDefinedPresenterTypes;

    /**
     * A list of all transition types. Must end with meta::Nil.
     * @note All transition types used in the application MUST be added to this list!
     */
    typedef meta::TypeList< NoTransition,
            meta::TypeList< SlowSlideTransition<EAST>,
            meta::TypeList< SlowSlideTransition<WEST>,
            meta::TypeList< SlowSlideTransition<NORTH>,
            meta::TypeList< SlowSlideTransition<SOUTH>,
            meta::Nil
            > > > > > UserDefinedTransitionTypes;

    /* Calculate largest view, both from generated and user-defined typelists */
    typedef meta::select_type_maxsize< UserDefinedViewTypes >::type MaxUserViewType;

    typedef meta::TypeList< MaxGeneratedViewType,
            meta::TypeList< MaxUserViewType,
            meta::Nil
            > > CombinedViewTypes;

    typedef meta::select_type_maxsize< CombinedViewTypes >::type MaxViewType;

    /* Calculate largest presenter, both from generated and user-defined typelists */
    typedef meta::select_type_maxsize< UserDefinedPresenterTypes >::type MaxUserPresenterType;

    typedef meta::TypeList< MaxGeneratedPresenterType,
            meta::TypeList< MaxUserPresenterType,
            meta::Nil
            > > CombinedPresenterTypes;
    typedef meta::select_type_maxsize< CombinedPresenterTypes >::type MaxPresenterType;

    /* Calculate largest transition, both from generated and user-defined typelists */
    typedef meta::select_type_maxsize< UserDefinedTransitionTypes >::type MaxUserTransitionType;

    typedef meta::TypeList< MaxGeneratedTransitionType,
            meta::TypeList< MaxUserTransitionType,
            meta::Nil
            > > CombinedTransitionTypes;
    typedef meta::select_type_maxsize< CombinedTransitionTypes >::type MaxTransitionType;

    static FrontendHeap& getInstance()
    {
        static FrontendHeap instance;
        return instance;
    }

    Partition< CombinedPresenterTypes, 5 > presenters;
    Partition< CombinedViewTypes, 5 > views;
    Partition< CombinedTransitionTypes, 5 > transitions;
    FrontendApplication app;
    Model model;

private:
    FrontendHeap() : FrontendHeapBase(presenters, views, transitions, app),
        app(model, *this)
    {
        gotoStartScreen(app);
    }
};

#endif // FRONTENDHEAP_HPP
