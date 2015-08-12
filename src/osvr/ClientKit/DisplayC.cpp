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
#include <osvr/ClientKit/DisplayC.h>
#include <osvr/ClientKit/InterfaceC.h>
#include <osvr/Util/Verbosity.h>
#include <osvr/Common/ClientContext.h>
#include <osvr/Client/DisplayConfig.h>
#include <osvr/Util/MacroToolsC.h>

// Library/third-party includes
#include <boost/assert.hpp>

// Standard includes
#include <utility>

struct OSVR_DisplayConfigObject {
    OSVR_DisplayConfigObject(OSVR_ClientContext context)
        : ctx(context),
          cfg(osvr::client::DisplayConfigFactory::create(context)) {
        OSVR_DEV_VERBOSE("Created an OSVR_DisplayConfigObject!");
    }
    ~OSVR_DisplayConfigObject() {
        OSVR_DEV_VERBOSE("OSVR_DisplayConfigObject destructor");
    }
    OSVR_ClientContext ctx;
    osvr::client::DisplayConfigPtr cfg;
};

#define OSVR_VALIDATE_OUTPUT_PTR(X, DESC)                                      \
    OSVR_UTIL_MULTILINE_BEGIN                                                  \
    if (nullptr == X) {                                                        \
        OSVR_DEV_VERBOSE("Passed a null pointer for output parameter " #X      \
                         ", " DESC "!");                                       \
        return OSVR_RETURN_FAILURE;                                            \
    }                                                                          \
    OSVR_UTIL_MULTILINE_END

OSVR_ReturnCode osvrClientGetDisplay(OSVR_ClientContext ctx,
                                     OSVR_DisplayConfig *disp) {
    OSVR_VALIDATE_OUTPUT_PTR(disp, "display config");
    if (ctx == nullptr) {
        OSVR_DEV_VERBOSE("Passed a null client context!");
        *disp = nullptr;
        return OSVR_RETURN_FAILURE;
    }
    std::shared_ptr<OSVR_DisplayConfigObject> config;
    try {
        config = std::make_shared<OSVR_DisplayConfigObject>(ctx);
    } catch (std::exception &e) {
        OSVR_DEV_VERBOSE(
            "Error creating display config: constructor threw exception :"
            << e.what());
        return OSVR_RETURN_FAILURE;
    }
    if (!config) {
        OSVR_DEV_VERBOSE(
            "Error creating display config - null config returned");
        return OSVR_RETURN_FAILURE;
    }
    if (!config->cfg) {
        OSVR_DEV_VERBOSE("Error creating display config - null internal config "
                         "object returned");
        return OSVR_RETURN_FAILURE;
    }
    ctx->acquireObject(config);
    *disp = config.get();
    return OSVR_RETURN_SUCCESS;
}

#define OSVR_VALIDATE_DISPLAY_CONFIG                                           \
    OSVR_UTIL_MULTILINE_BEGIN                                                  \
    if (nullptr == disp) {                                                     \
        OSVR_DEV_VERBOSE("Passed a null display config!");                     \
        return OSVR_RETURN_FAILURE;                                            \
    }                                                                          \
    OSVR_UTIL_MULTILINE_END

/// @todo make these an "always" check? instead of an assert

/// @todo actually check the config for number of viewers
/// (viewer < disp->cfg->size())
#define OSVR_VALIDATE_VIEWER_ID                                                \
    BOOST_ASSERT_MSG(viewer == 0, "Must pass a valid viewer ID.")

#define OSVR_VALIDATE_EYE_ID                                                   \
    BOOST_ASSERT_MSG(eye < disp->cfg->getNumViewerEyes(viewer),                \
                     "Must pass a valid eye ID.")

/// @todo actually check the config for number of surfaces
#define OSVR_VALIDATE_SURFACE_ID                                               \
    BOOST_ASSERT_MSG(surface == 0, "Must pass a valid surface ID.")

OSVR_ReturnCode osvrClientFreeDisplay(OSVR_DisplayConfig disp) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_ClientContext ctx = disp->ctx;
    BOOST_ASSERT_MSG(
        ctx != nullptr,
        "Should never get a display config object with a null context in it.");
    if (nullptr == ctx) {
        return OSVR_RETURN_FAILURE;
    }
    auto freed = ctx->releaseObject(disp);
    return freed ? OSVR_RETURN_SUCCESS : OSVR_RETURN_FAILURE;
}

OSVR_ReturnCode osvrClientGetNumViewers(OSVR_DisplayConfig disp,
                                        OSVR_ViewerCount *viewers) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_OUTPUT_PTR(viewers, "viewer count");
    *viewers = disp->cfg->size();
    return OSVR_RETURN_SUCCESS;
}
OSVR_ReturnCode osvrClientGetViewerPose(OSVR_DisplayConfig disp,
                                        OSVR_ViewerCount viewer,
                                        OSVR_Pose3 *pose) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_VIEWER_ID;
    OSVR_VALIDATE_OUTPUT_PTR(pose, "viewer pose");

    /// @todo implement
    return OSVR_RETURN_FAILURE;
}
OSVR_ReturnCode osvrClientGetNumEyesForViewer(OSVR_DisplayConfig disp,
                                              OSVR_ViewerCount viewer,
                                              OSVR_EyeCount *eyes) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_VIEWER_ID;
    OSVR_VALIDATE_OUTPUT_PTR(eyes, "eye count");
    *eyes = disp->cfg->getViewer(viewer).size();
    return OSVR_RETURN_SUCCESS;
}

OSVR_ReturnCode osvrClientGetViewerEyePose(OSVR_DisplayConfig disp,
                                           OSVR_ViewerCount viewer,
                                           OSVR_EyeCount eye,
                                           OSVR_Pose3 *pose) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_VIEWER_ID;
    OSVR_VALIDATE_EYE_ID;
    OSVR_VALIDATE_OUTPUT_PTR(pose, "eye pose");
    try {
        *pose = disp->cfg->getViewerEye(viewer, eye).getPose();
        return OSVR_RETURN_SUCCESS;
    } catch (osvr::client::NoPoseYet &) {
        OSVR_DEV_VERBOSE(
            "Error getting viewer eye pose: no pose yet available");
        return OSVR_RETURN_FAILURE;
    } catch (std::exception &e) {

        OSVR_DEV_VERBOSE(
            "Error getting viewer eye pose - exception: " << e.what());
        return OSVR_RETURN_FAILURE;
    }
    return OSVR_RETURN_FAILURE;
}

OSVR_ReturnCode
osvrClientGetNumSurfacesForViewerEye(OSVR_DisplayConfig disp,
                                     OSVR_ViewerCount viewer, OSVR_EyeCount eye,
                                     OSVR_SurfaceCount *surfaces) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_VIEWER_ID;
    OSVR_VALIDATE_EYE_ID;
    OSVR_VALIDATE_OUTPUT_PTR(surfaces, "surface count");
    /// @todo implement
    return OSVR_RETURN_FAILURE;
}

OSVR_ReturnCode osvrClientGetRelativeViewportForViewerEyeSurface(
    OSVR_DisplayConfig disp, OSVR_ViewerCount viewer, OSVR_EyeCount eye,
    OSVR_SurfaceCount surface, OSVR_ViewportDimension *left,
    OSVR_ViewportDimension *bottom, OSVR_ViewportDimension *width,
    OSVR_ViewportDimension *height) {

    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_VIEWER_ID;
    OSVR_VALIDATE_EYE_ID;
    OSVR_VALIDATE_SURFACE_ID;
    OSVR_VALIDATE_OUTPUT_PTR(left, "viewport left bound");
    OSVR_VALIDATE_OUTPUT_PTR(bottom, "viewport bottom bound");
    OSVR_VALIDATE_OUTPUT_PTR(width, "viewport width");
    OSVR_VALIDATE_OUTPUT_PTR(height, "viewport height");
    /// @todo implement
    return OSVR_RETURN_FAILURE;
}
OSVR_ReturnCode osvrClientGetProjectionForViewerEyeSurface(
    OSVR_DisplayConfig disp, OSVR_ViewerCount viewer, OSVR_EyeCount eye,
    OSVR_SurfaceCount surface, double near, double far, OSVR_Matrix44 *matrix) {
    OSVR_VALIDATE_DISPLAY_CONFIG;
    OSVR_VALIDATE_VIEWER_ID;
    OSVR_VALIDATE_EYE_ID;
    OSVR_VALIDATE_SURFACE_ID;
    OSVR_VALIDATE_OUTPUT_PTR(matrix, "projection matrix");
    if (near == 0 || far == 0) {
        OSVR_DEV_VERBOSE("Can't specify a near or far distance as 0!");
        return OSVR_RETURN_FAILURE;
    }
    if (near < 0 || far < 0) {
        OSVR_DEV_VERBOSE("Can't specify a negative near or far distance!");
        return OSVR_RETURN_FAILURE;
    }
    /// @todo connect to implementation
    return OSVR_RETURN_FAILURE;
}
