#include "FormantCurveDisplay.h"
#include "../PluginProcessor.h"

namespace voxshred
{

void FormantCurveDisplay::timerCallback()
{
    envelope = processor.getFormantEnvelopeForUI();
    repaint();
}

} // namespace voxshred
