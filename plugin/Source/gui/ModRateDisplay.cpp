#include "ModRateDisplay.h"
#include "../PluginProcessor.h"

namespace voxshred
{

void ModRateDisplay::timerCallback()
{
    phase = processor.getModRatePhaseForUI();

    history.erase (history.begin());
    history.push_back (processor.getModRateOutputForUI());

    repaint();
}

} // namespace voxshred
