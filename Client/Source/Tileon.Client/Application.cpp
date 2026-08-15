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

#include "Application.hpp"
#include "Tileon_Client.Modules.hpp"
#include <Zyphryon.Content/Mount/Disk.hpp>
#include <Zyphryon.Input/Service.hpp>
#include <Zyphryon.Platform/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Client
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Filesystem::Path Scan(Text Directory)
    {
        Filesystem::Path Result;

        Filesystem::Enumerate(Directory, [&](ConstRef<Filesystem::Record> Record) -> Bool
        {
            if (Record.Type == Filesystem::Type::File && StrEndsWith(Record.Name, "tileon"))
            {
                Result = Filesystem::Path::Join(Directory, '/', Record.Name);
                return false;
            }
            return true;
        });
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Filesystem::Path Locate(ConstRef<Environment> Options)
    {
        Text Target = Options.GetText("project", Text::Empty());

        if (Target.IsEmpty() && !Options.GetOperands().IsEmpty())
        {
            Target = Options.GetOperands().GetFront();
        }

        if (Target.IsEmpty())
        {
            return Scan(".");
        }

        Filesystem::Path Path(Target);
        Path.Replace('\\', '/');

        return StrEndsWith(Path, "tileon") ? Path : Scan(Path);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Placement Restore(ConstRef<Filesystem::Path> Folder)
    {
        Blob File;

        if (Filesystem::Read(Folder + "/Session.json", File) != Filesystem::Result::Success)
        {
            return Placement();
        }

        JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

        if (!Document.IsObject())
        {
            return Placement();
        }

        const JsonObject Root(Document);
        return Placement::FromAbsolute(Root.GetNumber<Real64>("Camera.X"), Root.GetNumber<Real64>("Camera.Y"));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Pan(Ref<Director> Director, Vector2 Pixels)
    {
        const Vector2   Origin = Director.GetViewport() * (0.5f * Director.GetDensity());
        const Placement From   = Director.GetWorldCoordinates(Origin);
        const Placement To     = Director.GetWorldCoordinates(Origin + Pixels);

        Director.SetPosition(Placement::Normalize(Director.GetPosition() + To - From));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Application::Application()
        : mState   { State::Loading },
          mDensity { 32 },
          mSample  { 0.0 },
          mFrames  { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnConfigure(Ref<Runtime::Startup> Startup)
    {
        Startup.SetWindowTitle("Tileon Client (v0.1)");
        Startup.SetGraphicsColorFormat(Graphic::TextureFormat::RGBA8UIntNorm_sRGB);

#if   defined(ZY_PLATFORM_WEB)
        Startup.SetWindowBorderless(true);
#endif

        ConstRef<Environment> Options = GetEnvironment();
        Startup.SetWindowWidth(Options.GetNumber<UInt32>("width", Startup.GetWindowWidth()));
        Startup.SetWindowHeight(Options.GetNumber<UInt32>("height", Startup.GetWindowHeight()));
        Startup.SetWindowFullscreen(Options.GetBool("fullscreen", Startup.IsWindowFullscreen()));
        Startup.SetGraphicsTearless(Options.GetBool("tearless", false));

        const Filesystem::Path Path = Locate(Options);

        if (Path.IsEmpty())
        {
            LOG_E("Client: Found no 'tileon' project to run");
            return;
        }

        Blob File;

        if (Filesystem::Read(Path, File) != Filesystem::Result::Success)
        {
            LOG_E("Client: Failed to read project '{0}'", Path);
            return;
        }

        JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

        if (!Document.IsObject())
        {
            LOG_E("Client: Project '{0}' is malformed", Path);
            return;
        }

        // The world lives beside its project file, so that folder is what the content service is pointed at.
        const JsonObject Root(Document);
        mDensity = Root.GetObject("Configuration").GetNumber<UInt16>("density", mDensity);
        mFolder  = StrBeforeLast(Path, '/');

        if (mFolder.IsEmpty())
        {
            mFolder = ".";
        }

        Startup.SetWindowTitle(Root.GetObject("Metadata").GetString("name", Startup.GetWindowTitle()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Application::OnInitialize()
    {
        if (mFolder.IsEmpty())
        {
            return false;
        }

        // Serve the world's data, techniques and tilesets straight out of the project folder.
        ConstRetainer<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Resources", Retainer<Content::Disk>::Create(mFolder));

        ConstRef<Platform::Window> Window = GetService<Platform::Service>()->GetWindow();
        mTitle = Window.GetTitle();

        // Compose straight into the display, as the client has no interface to hand an off-screen image to.
        mPresenter = Unique<Presenter>::Create(* this, true);
        mPresenter->Init(Window.GetWidth(), Window.GetHeight(), mDensity);
        mPresenter->Load();

        // A run has to land on content, so the view opens where the world was last authored from.
        ConstRef<Environment> Options = GetEnvironment();
        const Placement       Anchor  = Restore(mFolder);

        Ref<Director> Director = mPresenter->GetDirector();
        Director.SetPosition(Placement::FromAbsolute(
            Options.GetNumber<Real64>("x", Anchor.GetAbsoluteX()),
            Options.GetNumber<Real64>("y", Anchor.GetAbsoluteY())));
        Director.SetZoom(Options.GetNumber<Real32>("zoom", Director.GetZoom()));

        // The kernel resets the swap chain on its own, this keeps the camera and the pipeline's targets in step.
        GetService<Input::Service>()->OnWindowResize.AddMethod<& Application::OnWindowResize>(this);

        LOG_I("Client: Running world '{0}' at {1} px per tile", mFolder, mDensity);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTick(Real64 Delta)
    {
        switch (mState)
        {
        case State::Loading:
        {
            // Nothing can be drawn until the techniques are on the GPU, so the display is only cleared.
            ConstRef<Platform::Window> Window = GetService<Platform::Service>()->GetWindow();
            const Graphic::Viewport    View(0.0f, 0.0f, Window.GetWidth(), Window.GetHeight());

            ConstRetainer<Graphic::Service> Graphics = GetService<Graphic::Service>();
            Graphics->Prepare(Graphic::kDisplay, View, Color::Black(), 1.0f, 0);
            Graphics->Commit();

            if (GetService<Content::Service>()->GetPending() == 0)
            {
                mState = State::Running;
            }
            break;
        }
        case State::Running:
        {
            Navigate(Delta);

            mPresenter->Present(Delta);
            break;
        }
        }

        Measure(Delta);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTerminate()
    {
        if (mPresenter)
        {
            mPresenter->Teardown();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Application::OnWindowResize(UInt32 Width, UInt32 Height)
    {
        if (mPresenter && Width > 0 && Height > 0)
        {
            mPresenter->Resize(Width, Height);
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::Navigate(Real64 Delta)
    {
        ConstRetainer<Input::Service> Input    = GetService<Input::Service>();
        Ref<Director>                 Director = mPresenter->GetDirector();

        // The service reports where the pointer is and not how far it travelled, so the delta is tracked by hand.
        const Vector2 Cursor(Input->GetMouseX(), Input->GetMouseY());
        const Vector2 Motion = Cursor - mCursor;
        mCursor = Cursor;

        const Real32 AxisX =
            (Input->IsKeyHeld(Input::Key::D) || Input->IsKeyHeld(Input::Key::Right) ? 1.0f : 0.0f) -
            (Input->IsKeyHeld(Input::Key::A) || Input->IsKeyHeld(Input::Key::Left)  ? 1.0f : 0.0f);
        const Real32 AxisY =
            (Input->IsKeyHeld(Input::Key::S) || Input->IsKeyHeld(Input::Key::Down)  ? 1.0f : 0.0f) -
            (Input->IsKeyHeld(Input::Key::W) || Input->IsKeyHeld(Input::Key::Up)    ? 1.0f : 0.0f);

        const Bool   Boost = Input->IsKeyHeld(Input::Key::LeftShift) || Input->IsKeyHeld(Input::Key::RightShift);
        const Real32 Speed = kPanSpeed * static_cast<Real32>(Delta) * (Boost ? kPanBoost : 1.0f);

        // Keys push the view at a fixed screen speed, a drag takes the ground under the cursor along with it.
        Vector2 Shift = Vector2(AxisX, AxisY) * Speed;

        if (Input->IsMouseButtonHeld(Input::Button::Middle) || Input->IsMouseButtonHeld(Input::Button::Right))
        {
            Shift -= Motion;
        }

        if (!Shift.IsAlmostZero())
        {
            Pan(Director, Shift);
        }

        // The wheel zooms about the cursor, so the tile it rests on stays where it is.
        if (const Real32 Wheel = Input->GetMouseScrollY(); Wheel != 0.0f)
        {
            const Real32 Magnitude = Director.GetZoom() * (Wheel > 0.0f ? 1.0f / kZoomFactor : kZoomFactor);

            Director.Focus(Director.GetWorldCoordinates(Cursor), Magnitude);
        }

        if (Input->IsKeyPressed(Input::Key::Escape))
        {
            Quit();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::Measure(Real64 Delta)
    {
        mSample += Delta;
        ++mFrames;

        if (mSample < kReportRate)
        {
            return;
        }

        // The client draws no interface, so the window title is where a benchmark run reads its numbers off.
        const Real64 Rate  = mFrames / mSample;
        const Real64 Frame = 1000.0 * mSample / mFrames;

        GetService<Platform::Service>()->GetWindow().SetTitle(
            String<192>::Print<"{0} - {1:.1} fps ({2:.2} ms)">(mTitle, Rate, Frame));

        mSample = 0.0;
        mFrames = 0;
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   MAIN   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ZY_APPLICATION(Tileon::Client::Application)