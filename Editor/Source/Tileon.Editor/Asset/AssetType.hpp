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

#include "AssetInspector.hpp"
#include <Zyphryon.Content/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Checks whether an asset type has an inspection handler.
    ///
    /// \tparam Type The resource type to check for an `Inspect` overload.
    template<typename Type>
    concept IsAssetInspectable = requires (Ref<Toolkit::Previewer> Preview, ConstRetainer<Type> Asset)
    {
        Inspect(Preview, Asset);
    };

    /// \brief Checks whether an asset type has a thumbnail handler.
    ///
    /// \tparam Type The resource type to check for a `Thumbnail` overload.
    template<typename Type>
    concept IsAssetThumbnailable = requires (ConstRetainer<Type> Asset, Real32 Size, UInt32 Step)
    {
        Thumbnail(Asset, Size, Step);
    };

    /// \brief Type-erased description of one kind of file the project holds.
    ///
    /// \remark A descriptor is what lets the panel list, reload and describe a file without knowing what it
    ///         is, so a new kind of asset arrives as one entry in the catalogue rather than as a case in
    ///         every switch that ever asks.
    class AssetType final
    {
    public:

        /// \brief Enumerates what the editor can do with a kind of asset, beyond listing it.
        enum class Trait : UInt8
        {
            None   = 0,             ///< The editor only lists it.
            Create = 1 << 0,        ///< A blank one can be made from the folder's menu.
            Edit   = 1 << 1,        ///< An editor of its own opens it.
            All    = Create | Edit, ///< TODO_DOC
        };
        ZY_DEFINE_BITWISE_FRIEND_ENUM(Trait)

        /// \brief Signature of the handler that rereads an asset the editor has written over.
        using Reloader  = void (*)(Ref<Content::Service>, ConstRef<Content::Uri>);

        /// \brief Signature of the handler that drops an asset the editor has removed from disk.
        using Unloader  = void (*)(Ref<Content::Service>, ConstRef<Content::Uri>);

        /// \brief Signature of the handler that draws what an asset holds.
        using Inspector = void (*)(Ref<Toolkit::Previewer>, Ref<Content::Service>, ConstRef<Content::Uri>);

        /// \brief Signature of the handler that draws an asset small enough to sit in a tooltip.
        using Thumbnailer = UInt32 (*)(Ref<Content::Service>, ConstRef<Content::Uri>, Real32, UInt32);

    public:

        /// \brief Constructs a descriptor standing for no kind of asset at all.
        ZY_INLINE AssetType()
            : mReloader    { nullptr },
              mUnloader    { nullptr },
              mInspector   { nullptr },
              mThumbnailer { nullptr },
              mTraits      { Trait::None }
        {
        }

        /// \brief Constructs a descriptor with explicit handlers.
        ///
        /// \param Reloader  The handler that rereads the asset, or `nullptr` when nothing loads it.
        /// \param Unloader  The handler that drops the asset, or `nullptr` when nothing loads it.
        /// \param Inspector The handler that draws what it holds, or `nullptr` when it has nothing to show.
        /// \param Extension The suffix the kind is recognised by, including its dot.
        /// \param Label     The human-readable name displayed for the kind.
        /// \param Icon      The glyph displayed alongside the label.
        /// \param Traits    What the editor can do with the kind, beyond listing it.
        ZY_INLINE AssetType(
            Reloader Reloader, Unloader Unloader, Inspector Inspector, Thumbnailer Thumbnailer,
            Text Extension, Text Label, Text Icon, Trait Traits)
            : mReloader    { Reloader },
              mUnloader    { Unloader },
              mInspector   { Inspector },
              mThumbnailer { Thumbnailer },
              mExtension   { Extension },
              mLabel       { Label },
              mIcon        { Icon },
              mTraits      { Traits }
        {
        }

        /// \brief Gets the suffix the kind is recognised by.
        ///
        /// \return The extension, including its dot.
        ZY_INLINE Text GetExtension() const
        {
            return mExtension;
        }

        /// \brief Gets the human-readable name displayed for the kind.
        ///
        /// \return The label.
        ZY_INLINE Text GetLabel() const
        {
            return mLabel;
        }

        /// \brief Gets the glyph displayed alongside the label.
        ///
        /// \return The icon.
        ZY_INLINE Text GetIcon() const
        {
            return mIcon;
        }

        /// \brief Checks whether the editor can do something with the kind beyond listing it.
        ///
        /// \param Trait The capability to test for.
        /// \return `true` when the kind carries the trait, `false` otherwise.
        ZY_INLINE Bool HasTrait(Trait Trait) const
        {
            return (mTraits & Trait) == Trait;
        }

        /// \brief Checks whether anything knows how to reread the kind.
        ///
        /// \return `true` when the kind loads as a resource, `false` when it is only listed.
        ZY_INLINE Bool CanReload() const
        {
            return mReloader != nullptr;
        }

        /// \brief Rereads the asset, so an edit made outside the editor is picked up.
        ///
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        /// \return `true` when the kind knew how to reread itself, `false` otherwise.
        ZY_INLINE Bool Reload(Ref<Content::Service> Service, ConstRef<Content::Uri> Key) const
        {
            if (mReloader)
            {
                mReloader(Service, Key);
            }
            return mReloader != nullptr;
        }

        /// \brief Drops the asset from memory, so a file removed from disk leaves nothing behind.
        ///
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        /// \return `true` when the kind knew how to drop itself, `false` otherwise.
        ZY_INLINE Bool Unload(Ref<Content::Service> Service, ConstRef<Content::Uri> Key) const
        {
            if (mUnloader)
            {
                mUnloader(Service, Key);
            }
            return mUnloader != nullptr;
        }

        /// \brief Draws what the asset holds.
        ///
        /// \param Preview The previewer the view draws images through, which holds its zoom and pan.
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        /// \return `true` when the kind had something to show, `false` otherwise.
        ZY_INLINE Bool Inspect(Ref<Toolkit::Previewer> Preview, Ref<Content::Service> Service, ConstRef<Content::Uri> Key) const
        {
            if (mInspector)
            {
                mInspector(Preview, Service, Key);
            }
            return mInspector != nullptr;
        }

        /// \brief Draws the asset small, for a tooltip that answers what it is at a glance.
        ///
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        /// \param Size    The longest side the drawing may take, in pixels.
        /// \param Step    Which of the kind's faces to show, clamped to how many it has.
        /// \return How many faces can be stepped through, or zero when the kind had none.
        ZY_INLINE UInt32 Thumbnail(Ref<Content::Service> Service, ConstRef<Content::Uri> Key, Real32 Size, UInt32 Step) const
        {
            return mThumbnailer ? mThumbnailer(Service, Key, Size, Step) : 0;
        }

        /// \brief Builds a descriptor for a kind the content service knows how to load.
        ///
        /// \tparam Type      The resource type the kind loads as.
        /// \param  Extension The suffix the kind is recognised by, including its dot.
        /// \param  Label     The human-readable name displayed for the kind.
        /// \param  Icon      The glyph displayed alongside the label.
        /// \param  Traits    What the editor can do with the kind, beyond listing it.
        /// \return The descriptor.
        template<typename Type>
        ZY_INLINE static AssetType Create(Text Extension, Text Label, Text Icon, Trait Traits)
        {
            constexpr Inspector   Handler = (IsAssetInspectable<Type> ? OnInspect<Type> : nullptr);
            constexpr Thumbnailer Face    = (IsAssetThumbnailable<Type> ? OnThumbnail<Type> : nullptr);

            return AssetType(OnReload<Type>, OnUnload<Type>, Handler, Face, Extension, Label, Icon, Traits);
        }

        /// \brief Builds a descriptor for a kind nothing loads, such as the art an import bakes from.
        ///
        /// \param Extension The suffix the kind is recognised by, including its dot.
        /// \param Label     The human-readable name displayed for the kind.
        /// \param Icon      The glyph displayed alongside the label.
        /// \return The descriptor.
        ZY_INLINE static AssetType Create(Text Extension, Text Label, Text Icon)
        {
            return AssetType(nullptr, nullptr, nullptr, nullptr, Extension, Label, Icon, Trait::None);
        }

    private:

        /// \brief Reload handler generated for `Type` by \ref Create.
        ///
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        template<typename Type>
        ZY_INLINE static void OnReload(Ref<Content::Service> Service, ConstRef<Content::Uri> Key)
        {
            Service.Reload(Service.Load<Type>(Key));
        }

        /// \brief Unload handler generated for `Type` by \ref Create.
        ///
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        template<typename Type>
        ZY_INLINE static void OnUnload(Ref<Content::Service> Service, ConstRef<Content::Uri> Key)
        {
            if (Retainer<Type> Asset = Type::GetCache().GetOrCreate(Content::Uri(Key), false))
            {
                Service.Unload(Asset);
            }
        }

        /// \brief Inspection handler generated for `Type` by \ref Create.
        ///
        /// \param Preview The previewer the view draws images through, which holds its zoom and pan.
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        template<typename Type>
        ZY_INLINE static void OnInspect(Ref<Toolkit::Previewer> Preview, Ref<Content::Service> Service, ConstRef<Content::Uri> Key)
        {
            if constexpr (IsAssetInspectable<Type>)
            {
                Editor::Inspect(Preview, Service.Load<Type>(Key));
            }
        }

        /// \brief Thumbnail handler generated for `Type` by \ref Create.
        ///
        /// \param Service The content service the asset is loaded through.
        /// \param Key     The url the asset is loaded under.
        /// \param Size    The longest side the drawing may take, in pixels.
        /// \param Step    Which of the kind's faces to show, clamped to how many it has.
        /// \return How many faces can be stepped through, or zero when there were none.
        template<typename Type>
        ZY_INLINE static UInt32 OnThumbnail(
            Ref<Content::Service> Service, ConstRef<Content::Uri> Key, Real32 Size, UInt32 Step)
        {
            if constexpr (IsAssetThumbnailable<Type>)
            {
                // Qualified, or the name would find this class's own member before the free overload.
                return Editor::Thumbnail(Service.Load<Type>(Key), Size, Step);
            }
            return 0;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Reloader    mReloader;
        Unloader    mUnloader;
        Inspector   mInspector;
        Thumbnailer mThumbnailer;
        Text        mExtension;
        Text        mLabel;
        Text        mIcon;
        Trait       mTraits;
    };
}