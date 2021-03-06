//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Original shader: http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
// Modified source: https://github.com/hughsk/glsl-hsv2rgb
// Formatting: Copyright (c) 2017, David Hirvonen (this file)

#ifndef OGLES_GPGPU_COMMON_HSV2RGB_PROC
#define OGLES_GPGPU_COMMON_HSV2RGB_PROC

#include "../common_includes.h"
#include "ogles_gpgpu/common/proc/base/filterprocbase.h"

BEGIN_OGLES_GPGPU

class Hsv2RgbProc : public ogles_gpgpu::FilterProcBase {
public:
    Hsv2RgbProc() {}
    virtual const char* getProcName() {
        return "Hsv2RgbProc";
    }

private:
    virtual const char* getFragmentShaderSource() {
        return fshaderHsv2RgbSrc;
    }

    static const char* fshaderHsv2RgbSrc; // fragment shader source
};

END_OGLES_GPGPU

#endif
