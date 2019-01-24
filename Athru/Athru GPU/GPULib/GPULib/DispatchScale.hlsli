#include "RenderUtility.hlsli"
#include "ScenePost.hlsli"

#define DISP_FACTOR_NDX 21
void DispatchScale(uint scaleBy,
                   uint ndx)
{
    if (ndx > DISP_FACTOR_NDX) { return; } // Mask off excess threads
    else if (ndx < DISP_FACTOR_NDX)
    {
        // Record one-based dispatch sizes inside the 23rd-29th counters
        uint val = counters[ndx];
        if (counters[DISP_FACTOR_NDX] == 1)
        {
            switch (ndx)
            {
                case 0:
                    counters[24] = val;
                    break;
                case 3:
                    counters[25] = val;
                    break;
                case 6:
                    counters[26] = val;
                    break;
                case 9:
                    counters[27] = val;
                    break;
                case 12:
                    counters[28] = val;
                    break;
                case 15:
                    counters[29] = val;
                    break;
                case 18:
                    counters[23] = val;
                    break;
                default:
                    break; // No counter backups outside threads [0, 3, 6, 9, 12, 15, 18]
            }
        }
        counters[ndx] = ceil(float(val) / scaleBy);
    }
    else { counters[ndx] = scaleBy; } // Record scaling factor
}