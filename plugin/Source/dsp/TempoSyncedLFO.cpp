#include "TempoSyncedLFO.h"

// This DSP class is header-only (all methods are declared inline in the
// class definition for the real-time hot path). This translation unit only
// exists so build systems that expect a matching .cpp per header stay happy
// and to give the linker a stable place for the vtable-free type info.
