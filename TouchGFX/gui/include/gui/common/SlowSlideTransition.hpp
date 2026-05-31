#ifndef SLOW_SLIDETRANSITION_HPP
#define SLOW_SLIDETRANSITION_HPP

#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/transitions/Transition.hpp>
#include <touchgfx/widgets/SnapshotWidget.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/EasingEquations.hpp>


namespace touchgfx
{

class Container;

/**
 * @class SlowSlideTransition SlowSlideTransition.hpp
 *
 * @brief A SlideTransition that slides from one screen to the next in a slow fashion (less CPU usage).
 *
 *        A Transition that slides from one screen to the next. It does so by moving a
 *        SnapShotWidget with a snapshot of the Screen transitioning away from, and by moving
 *        the contents of Screen transitioning to in a slow fashion.
 *
 * @tparam templateDirection Type of the template direction.
 *
 * @see Transition
 */
template <Direction templateDirection>
class SlowSlideTransition : public Transition
{
public:

    /**
     * @fn SlowSlideTransition::SlowSlideTransition(const uint8_t transitionSteps = 20) : Transition(), snapshot(), snapshotPtr(&snapshot), handleTickCallback(this, &SlowSlideTransition::tickMoveDrawable), direction(templateDirection), animationSteps(transitionSteps), animationCounter(0), calculatedValue(0)
     *
     * @brief Constructor.
     *
     *        Constructor.
     *
     * @param transitionSteps Number of steps in the transition animation.
     */
    SlowSlideTransition(const uint8_t transitionSteps = 15, const uint8_t transitionSkipSteps = 1)
        : Transition(),
          snapshot(),
          snapshotPtr(&snapshot),
          handleTickCallback(this, &SlowSlideTransition::tickMoveDrawable),
          direction(templateDirection),
          animationSteps(transitionSteps),
          animationCounter(0),
          calculatedValue(0),
          tickCounter(transitionSkipSteps),
          skipSteps(transitionSkipSteps)
    {
        if (HAL::USE_ANIMATION_STORAGE)
        {
            snapshot.setPosition(0, 0, HAL::DISPLAY_WIDTH, HAL::DISPLAY_HEIGHT);
            snapshot.makeSnapshot();

            switch (direction)
            {
            case EAST:
                targetValue = -HAL::DISPLAY_WIDTH;
                break;
            case WEST:
                targetValue = HAL::DISPLAY_WIDTH;
                break;
            case NORTH:
                targetValue = HAL::DISPLAY_HEIGHT;
                break;
            case SOUTH:
                targetValue = -HAL::DISPLAY_HEIGHT;
                break;
            default:
                done = true;
                // Nothing to do here
                break;
            }
        }
    }

    /**
     * @fn virtual SlowSlideTransition::~SlowSlideTransition()
     *
     * @brief Destructor.
     *
     *        Destructor.
     */
    virtual ~SlowSlideTransition()
    {
    }

    /**
     * @fn virtual void SlowSlideTransition::handleTickEvent()
     *
     * @brief Handles the tick event when transitioning.
     *
     *        Handles the tick event when transitioning. It moves the contents of the Screen's
     *        container and a SnapshotWidget with a snapshot of the previous Screen. The
     *        direction of the transition determines the direction the contents of the
     *        container and the SnapshotWidget moves.
     */
    virtual void handleTickEvent()
    {
        if (!HAL::USE_ANIMATION_STORAGE)
        {
            done = true;
            return;
        }

        if ((tickCounter % skipSteps == 0) || (skipSteps == 0))
        {
            Transition::handleTickEvent();

            // Calculate new position or stop animation
            animationCounter++;
            if (animationCounter <= animationSteps)
            {
                // Calculate value in [0;targetValue]
                calculatedValue = EasingEquations::cubicEaseOut(animationCounter, 0, targetValue, animationSteps);

                // Note: Result of "calculatedValue & 1" is compiler dependent for negative values of calculatedValue
                if (calculatedValue % 2)
                {
                    // Optimization: calculatedValue is odd, add 1/-1 to move drawables modulo 32 bits in framebuffer
                    calculatedValue += (calculatedValue > 0 ? 1 : -1);
                }
            }
            else
            {
                // Final step: stop the animation
                done = true;
                animationCounter = 0;
                tickCounter      = skipSteps;
                return;
            }

            // Move snapshot
            switch (direction)
            {
            case EAST:
            case WEST:
                // Convert to delta value relative to current X
                calculatedValue -= snapshot.getX();
                snapshot.moveRelative(calculatedValue, 0);
                break;
            case NORTH:
            case SOUTH:
                // Convert to delta value relative to current Y
                calculatedValue -= snapshot.getY();
                snapshot.moveRelative(0, calculatedValue);
                break;
            default:
                done = true;
                tickCounter      = skipSteps;
                // Nothing to do here
                break;
            }

            // Move children with delta value for X or Y
            screenContainer->forEachChild(&handleTickCallback);

        }

        if (!done)
        {
            tickCounter++;
        }
    }

    /**
     * @fn virtual void SlowSlideTransition::tearDown()
     *
     * @brief Tear down.
     *
     *        Tear down.
     *
     * @see Transition::teadDown()
     */
    virtual void tearDown()
    {
        if (HAL::USE_ANIMATION_STORAGE)
        {
            screenContainer->remove(snapshot);
        }
    }

    /**
     * @fn virtual void SlowSlideTransition::init()
     *
     * @brief Initializes this object.
     *
     *        Initializes this object.
     *
     * @see Transition::init()
     */
    virtual void init()
    {
        if (HAL::USE_ANIMATION_STORAGE)
        {
            Transition::init();

            Callback<SlowSlideTransition, Drawable&> initCallback(this, &SlowSlideTransition::initMoveDrawable);
            screenContainer->forEachChild(&initCallback);

            screenContainer->add(snapshot);
        }
    }

    SnapshotWidget  snapshot;    ///< The SnapshotWidget that is moved when transitioning.
    SnapshotWidget* snapshotPtr; ///< Pointer pointing to the snapshot used in this transition.The snapshot pointer

protected:

    /**
     * @fn virtual void SlowSlideTransition::initMoveDrawable(Drawable& d)
     *
     * @brief Moves the Drawable to its initial position.
     *
     *        Moves the Drawable to its initial position.
     *
     * @param [in] d The Drawable to move.
     */
    virtual void initMoveDrawable(Drawable& d)
    {
        switch (direction)
        {
        case EAST:
            d.moveRelative(HAL::DISPLAY_WIDTH, 0);
            break;
        case WEST:
            d.moveRelative(-HAL::DISPLAY_WIDTH, 0);
            break;
        case NORTH:
            d.moveRelative(0, -HAL::DISPLAY_HEIGHT);
            break;
        case SOUTH:
            d.moveRelative(0, HAL::DISPLAY_HEIGHT);
            break;
        default:
            // Nothing to do here
            break;
        }
    }

    /**
     * @fn virtual void SlowSlideTransition::tickMoveDrawable(Drawable& d)
     *
     * @brief Moves the Drawable.
     *
     *        Moves the Drawable.
     *
     * @param [in] d The Drawable to move.
     */
    virtual void tickMoveDrawable(Drawable& d)
    {
        if (&d == snapshotPtr)
        {
            return;
        }

        switch (direction)
        {
        case EAST:
        case WEST:
            d.moveRelative(calculatedValue, 0);
            break;
        case NORTH:
        case SOUTH:
            d.moveRelative(0, calculatedValue);
            break;
        default:
            // Special case, do not move. Class NoTransition can be used instead.
            done = true;
            break;
        }
    }

private:
    Callback<SlowSlideTransition, Drawable&> handleTickCallback;    ///< Callback used for tickMoveDrawable().

    Direction     direction;        ///< The direction of the transition.
    const uint8_t animationSteps;   ///< Number of steps the transition should move per complete animation.
    uint8_t       animationCounter; ///< Current step in the transition animation.
    int16_t       targetValue;      ///< The target value for the transition animation.
    int16_t       calculatedValue;  ///< The calculated X or Y value for the snapshot and the children.
    uint32_t      tickCounter;      ///< Current ticks number.
    uint8_t       skipSteps;        ///< Number of ticks to spkip for next move
};

} // namespace touchgfx
#endif // SLOW_SLIDETRANSITION_HPP
