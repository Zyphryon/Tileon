// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Animator.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Advance(Real64 Time, ConstRef<Animation> Sequence)
    {
        if (mStatus == Status::Idle)
        {
            return;
        }

        ConstRef<Animation::Flipbook> Flipbook = Sequence.GetFlipbook();

        if (const UInt32 Count = Flipbook.GetCount(); Count > 1)
        {
            const Real64 Delta    = Time - mTimestamp;
            const Real64 Duration = Flipbook.GetDuration();

            switch (mStatus)
            {
            case Status::Idle:
            {
                break;
            }
            case Status::Forward:
            {
                if (Delta < Duration)
                {
                    const Real64 Frame = Ease(Delta, Duration);
                    mKeyframe = static_cast<UInt8>(Flipbook.Locate(Frame));
                }
                else
                {
                    SetKeyframe(Count - 1);
                    SetStatus(Status::Idle);
                }
                break;
            }
            case Status::Backward:
            {
                if (Delta < Duration)
                {
                    const Real64 Frame = Ease(Duration - Delta, Duration);
                    mKeyframe = static_cast<UInt8>(Flipbook.Locate(Frame));
                }
                else
                {
                    SetKeyframe(0);
                    SetStatus(Status::Idle);
                }
                break;
            }
            case Status::Repeat:
            {
                const Real64 Absolute = Mod(Delta, Duration);
                const Real64 Frame    = Ease(Absolute, Duration);

                mKeyframe = static_cast<UInt8>(Flipbook.Locate(Frame));

                if (Delta >= Duration)
                {
                    mTimestamp = Time - Absolute;
                }
                break;
            }
            case Status::Mirror:
            {
                const Real64 Cycle    = Duration * 2.0;
                const Real64 Absolute = Mod(Delta, Cycle);

                const Real64 Frame = Ease(Absolute > Duration ? Cycle - Absolute : Absolute, Duration);
                mKeyframe = static_cast<UInt8>(Flipbook.Locate(Frame));

                if (Delta >= Cycle)
                {
                    mTimestamp = Time - Absolute;
                }
                break;
            }
            }
        }
    }
}