#include "AthruGlobals.h"

std::chrono::steady_clock::time_point TimeStuff::timeAtLastFrame = std::chrono::steady_clock::now();
const float MathsStuff::PI = 3.14159265359f;

const bool GraphicsStuff::FULL_SCREEN = false;
const bool GraphicsStuff::VSYNC_ENABLED = true;

const fourByteUnsigned GraphicsStuff::DISPLAY_WIDTH = 1024;
const fourByteUnsigned GraphicsStuff::DISPLAY_HEIGHT = 768;

const float GraphicsStuff::DISPLAY_ASPECT_RATIO = (GraphicsStuff::DISPLAY_WIDTH / GraphicsStuff::DISPLAY_HEIGHT);
const float GraphicsStuff::SCREEN_FAR = 1000.0f;
const float GraphicsStuff::SCREEN_NEAR = 0.1f;
const float GraphicsStuff::VERT_FIELD_OF_VIEW_RADS = (MathsStuff::PI / 2);
const float GraphicsStuff::HORI_FIELD_OF_VIEW_RADS = 2 * atan(tan(GraphicsStuff::VERT_FIELD_OF_VIEW_RADS / 2) * GraphicsStuff::DISPLAY_ASPECT_RATIO);
const float GraphicsStuff::FRUSTUM_WIDTH_AT_NEAR = ((2 * GraphicsStuff::SCREEN_NEAR) * tan(GraphicsStuff::VERT_FIELD_OF_VIEW_RADS * 0.5f));
const float GraphicsStuff::FRUSTUM_HEIGHT_AT_NEAR = GraphicsStuff::FRUSTUM_WIDTH_AT_NEAR / GraphicsStuff::DISPLAY_ASPECT_RATIO;
const fourByteUnsigned GraphicsStuff::PROG_PASS_COUNT = GraphicsStuff::DISPLAY_HEIGHT;
