/** @file
    @brief Implementation

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Internal Includes
#include <osvr/Client/DisplayConfig.h>
#include <osvr/Util/ProjectionMatrixFromFOV.h>
#include "DisplayDescriptorSchema1.h"

// Library/third-party includes
#include <osvr/Util/EigenExtras.h>

// Standard includes
#include <stdexcept>

namespace osvr {
namespace client {
    /// @param eye 0 for left, 1 for right
    inline static Viewport
    computeViewport(uint8_t eye,
                    display_schema_1::DisplayDescriptor &descriptor) {
        Viewport viewport = {0};
        // Set up the viewport based on the display resolution and the
        // display configuration.
        switch (descriptor.getDisplayMode()) {
        case display_schema_1::DisplayDescriptor::FULL_SCREEN:
            viewport.bottom = viewport.left = 0;
            viewport.width = descriptor.getDisplayWidth();
            viewport.height = descriptor.getDisplayHeight();
            break;
        case display_schema_1::DisplayDescriptor::HORIZONTAL_SIDE_BY_SIDE:
            viewport.bottom = 0;
            viewport.height = descriptor.getDisplayHeight();
            viewport.width = descriptor.getDisplayWidth() / 2;
            // Zeroeth eye at left, first eye starts in the middle.
            viewport.left = eye * viewport.width;
            break;
        case display_schema_1::DisplayDescriptor::VERTICAL_SIDE_BY_SIDE:
            viewport.left = 0;
            viewport.width = descriptor.getDisplayWidth();
            viewport.height = descriptor.getDisplayHeight() / 2;
            // Zeroeth eye in the top half, first eye at the bottom.
            if (eye == 0) {
                viewport.bottom = viewport.height;
            } else {
                viewport.bottom = 0;
            }
            break;
        default:
            throw std::logic_error("Unrecognized enum value for display mode");
        }
        return viewport;
    }

    inline static util::Rectd
    computeRect(display_schema_1::DisplayDescriptor &descriptor) {
        return util::computeSymmetricFOVRect(descriptor.getHorizontalFOV(),
                                             descriptor.getVerticalFOV());
    }

    static const char HEAD_PATH[] = "/me/head";
    DisplayConfigPtr DisplayConfigFactory::create(OSVR_ClientContext ctx) {
        DisplayConfigPtr cfg(new DisplayConfig);
        auto const descriptorString = ctx->getStringParameter("/display");
        auto desc = display_schema_1::DisplayDescriptor(descriptorString);
        cfg->m_viewers.emplace_back(Viewer(ctx, HEAD_PATH));
        auto &viewer = cfg->m_viewers.front();
        auto eyesDesc = desc.getEyes();
        std::vector<uint8_t> eyeIndices;
        Eigen::Vector3d offset;
        if (eyesDesc.size() == 2) {
            // stereo
            offset = desc.getIPDMeters() / 2. * Eigen::Vector3d::UnitX();
            eyeIndices = {0, 1};
        } else {
            // if (eyesDesc.size() == 1)
            // mono
            offset = Eigen::Vector3d::Zero();
            eyeIndices = {0};
        }
        for (auto eye : eyeIndices) {
            double offsetFactor =
                (2. * eye) - 1; // turns 0 into -1 and 1 into 1. Doesn't affect
                                // mono, which has a zero offset vector.

            viewer.m_eyes.emplace_back(ViewerEye(
                viewer, ctx, (offsetFactor * offset).eval(), HEAD_PATH,
                computeViewport(eye, desc), computeRect(desc),
                eyesDesc[eye].m_rotate180, desc.getPitchTilt().value()));
        }
        return cfg;
    }
    DisplayConfig::DisplayConfig() {}
} // namespace client
} // namespace osvr
