#ifndef CAMERAS_UTILS_H
#define CAMERAS_UTILS_H

namespace cameras {
    inline float scaleToRange(float value, float minInput, float maxInput) {
        // Clamp input to avoid extrapolation
        if (value < minInput) value = minInput;
        if (value > maxInput) value = maxInput;

        // Linear scale
        return 1.0f + (value - minInput) * (4.0f - 1.0f) / (maxInput - minInput);
    }
}

#endif // CAMERAS_UTILS_H