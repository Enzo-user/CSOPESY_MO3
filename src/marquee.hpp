// marquee.hpp - the renderer thread ("refresh rate").
#pragma once

#include "marquee_state.hpp"

namespace marquee {

// Body of the renderer thread. Draws one frame every `speed_ms` while the
// marquee runs, plus one frame whenever request_redraw() is called. Returns
// when `state.quit` becomes true.
void run(MarqueeState& state);

} // namespace marquee
