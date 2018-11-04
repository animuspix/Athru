#include "AthruGlobals.h"

// Anything that can't be evaluated at compile time goes here...

std::chrono::steady_clock::time_point TimeStuff::timeAtLastFrame = std::chrono::steady_clock::now();
fourByteUnsigned TimeStuff::frameCtr = 0;

const float GraphicsStuff::VERT_FIELD_OF_VIEW_RADS = (MathsStuff::PI / 2);
const float GraphicsStuff::HORI_FIELD_OF_VIEW_RADS = 2 * atan(tan(GraphicsStuff::VERT_FIELD_OF_VIEW_RADS / 2) * GraphicsStuff::DISPLAY_ASPECT_RATIO);
const float GraphicsStuff::FRUSTUM_WIDTH_AT_NEAR = ((2 * GraphicsStuff::SCREEN_NEAR) * tan(GraphicsStuff::VERT_FIELD_OF_VIEW_RADS * 0.5f));
const float GraphicsStuff::FRUSTUM_HEIGHT_AT_NEAR = GraphicsStuff::FRUSTUM_WIDTH_AT_NEAR / GraphicsStuff::DISPLAY_ASPECT_RATIO;