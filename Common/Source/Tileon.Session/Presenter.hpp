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

#include "Session.hpp"
#include "Tileon.Render/Director.hpp"
#include "Tileon.Render/Renderer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a session that presents the world to a viewer, streaming it from a camera.
    class Presenter final : public Session
    {
    public:

        /// \brief Constructs a Presenter instance with the specified service host.
        ///
        /// \param Host      The service host to associate with the presenter.
        /// \param Immediate `true`, renders directly to the display, otherwise renders to an off-screen texture.
        /// \param Density   The pixel density one world unit occupies, fixed for the session's lifetime.
        Presenter(Ref<Engine::Subsystem::Host> Host, Bool Immediate, Real32 Density);

        /// \brief Loads the world and the tileset the pipeline draws it with.
        void Load();

        /// \brief Saves the current state of the world and of the renderer.
        void Save();

        /// \brief Resizes the rendering viewport to the specified dimensions.
        ///
        /// \param Width  The new width of the viewport in pixels.
        /// \param Height The new height of the viewport in pixels.
        void Resize(UInt16 Width, UInt16 Height);

        /// \brief Advances the camera, streams the regions it now sees, and executes the pipeline.
        ///
        /// \param Delta The unscaled time delta since the last frame, in seconds.
        void Present(Real64 Delta);

        /// \brief Gets the director instance associated with the presenter.
        ///
        /// \return The director instance managed by the presenter.
        ZY_INLINE Ref<Director> GetDirector()
        {
            return mDirector;
        }

        /// \brief Gets the renderer instance associated with the presenter.
        ///
        /// \return The renderer instance managed by the presenter.
        ZY_INLINE Ref<Renderer> GetRenderer()
        {
            return mRenderer;
        }

    private:

        /// \brief Makes the regions the given rectangle covers resident, if it is not the one already covered.
        ///
        /// \param Boundaries The rectangle of regions the camera now sees.
        void Navigate(IntRect Boundaries);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Director             mDirector;
        Renderer             mRenderer;
        IntRect              mBoundaries;
        Sequence<IntVector2> mResident;
    };
}