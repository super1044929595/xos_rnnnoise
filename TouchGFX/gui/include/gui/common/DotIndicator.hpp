#ifndef DOTINDICATOR_HPP_
#define DOTINDICATOR_HPP_

#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/containers/ListLayout.hpp>

using namespace touchgfx;

class DotIndicator : public Container
{
public:
    DotIndicator();
    virtual ~DotIndicator();

    void setNumberOfDots(uint8_t size);
    void setBitmaps(const Bitmap& unselected, const Bitmap& selected);

    void goRight();
    void goLeft();
    void setHighlightPosition(uint8_t index);
private:
    static const uint8_t MAX_SIZE = 10;

    ListLayout unselectedDots;
    Image dotHighlighted;
    Image dotNormal[MAX_SIZE];

    uint8_t numberOfDots;
    uint8_t currentDot;
};

#endif /* DOTINDICATOR_HPP_ */
