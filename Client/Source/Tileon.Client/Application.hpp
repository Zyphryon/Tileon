// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Tileon.Runtime/Controller.hpp"
#include <Zyphryon.Runtime/Kernel.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Client
{
    /// \brief Represents the main application kernel for the Tileon client.
    class Application final : public Runtime::Kernel
    {
    public:

        /// \brief Constructs a new instance of the application.
        Application();

    protected:

        /// \see Kernel::OnConfigure(Ref<Runtime::Startup>)
        void OnConfigure(Ref<Runtime::Startup> Startup) override;

        /// \see Kernel::OnInitialize()
        Bool OnInitialize() override;

        /// \see Kernel::OnTerminate()
        void OnTerminate() override;

        /// \see Kernel::OnTick(Real64)
        void OnTick(Real64 Delta) override;

    private:

        /// \brief How far holding a movement key drags the view, in screen pixels per second.
        static constexpr Real32 kPanSpeed   = 640.0f;

        /// \brief How much a movement key covers while the boost modifier is held.
        static constexpr Real32 kPanBoost   = 4.0f;

        /// \brief How much a wheel notch multiplies (or divides) the zoom by.
        static constexpr Real32 kZoomFactor = 1.25f;

        /// \brief How often the measured frame rate is reported, in seconds.
        static constexpr Real64 kReportRate = 0.5;

        /// \brief Enumerates the different states of the client application.
        enum class State : UInt8
        {
            Loading,    ///< The client is waiting for the content service to finish streaming the world in.
            Running,    ///< The client is executing the world and drawing it to the display.
        };

        /// \brief Adopts the display size for the camera and the render targets.
        ///
        /// \param Width  The new width of the display in pixels.
        /// \param Height The new height of the display in pixels.
        /// \return `false` so the event keeps propagating to any other listener.
        Bool OnWindowResize(UInt32 Width, UInt32 Height);

        /// \brief Applies this frame's input to the camera, panning and zooming the view.
        ///
        /// \param Delta The time elapsed since the last tick.
        void Navigate(Real64 Delta);

        /// \brief Accumulates frame timings and reports them through the window title.
        ///
        /// \param Delta The time elapsed since the last tick.
        void Measure(Real64 Delta);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        State              mState;
        Str                mTitle;
        Filesystem::Path   mFolder;
        UInt16             mDensity;
        Unique<Controller> mController;
        Vector2            mCursor;
        Real64             mSample;
        UInt32             mFrames;
    };
}