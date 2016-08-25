/*
 * vsdl.cxx
 *
 * Classes to support video output via SDL
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "vsdl.h"
#endif

#include <ptlib.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/vsdl.h>

#define new PNEW

#if P_SDL

extern "C" {
  #include <SDL.h>
};

#ifdef _MSC_VER
  #pragma comment(lib, P_SDL_LIBRARY)
  #pragma message("SDL video support enabled")
#endif


class PVideoOutputDevice_SDL_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const
    {
      return new PVideoOutputDevice_SDL;
    }

    virtual PStringArray GetDeviceNames(int /*userData*/) const
    {
      return PString("SDL");
    }

    virtual bool         ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
      return deviceName.NumCompare("SDL") == PObject::EqualTo;
    }
} PVideoOutputDevice_SDL_descriptor;

PCREATE_PLUGIN(SDL, PVideoOutputDevice, &PVideoOutputDevice_SDL_descriptor);


///////////////////////////////////////////////////////////////////////

class PSDL_Window : public PMutex
{
  public:
    static PSDL_Window & GetInstance()
    {
      static PSDL_Window instance;
      return instance;
    }


    enum UserEvents {
      e_AddDevice,
      e_RemoveDevice,
      e_SizeChanged,
      e_ContentChanged
    };


    void Run()
    {
      if (m_thread == NULL) {
        m_thread = new PThreadObj<PSDL_Window>(*this, &PSDL_Window::MainLoop, true, "SDL");
        m_started.Wait();
      }
    }


  private:
    SDL_Surface * m_surface;
    PThread     * m_thread;
    PSyncPoint    m_started;

    typedef std::list<PVideoOutputDevice_SDL *> DeviceList;
    DeviceList m_devices;

    PSDL_Window()
      : m_surface(NULL)
      , m_thread(NULL)
    {
#if PTRACING
      SDL_version v1;
      SDL_VERSION(&v1);
      const SDL_version * v2 = SDL_Linked_Version();
      PTRACE(3, "VSDL\tCompiled version: "
             << (unsigned)v1.major << '.' << (unsigned)v1.minor << '.' << (unsigned)v1.patch
             << "  Run-Time version: "
             << (unsigned)v2->major << '.' << (unsigned)v2->minor << '.' << (unsigned)v2->patch);
#endif
    }


    virtual void MainLoop()
    {
      PTRACE(4, "VSDL\tStart of event thread");

      // initialise the SDL library
      if (::SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) < 0) {
        PTRACE(1, "VSDL\tCouldn't initialize SDL: " << ::SDL_GetError());
        return;
      }

#ifdef _WIN32
      SDL_SetModuleHandle(GetModuleHandle(NULL));
#endif

      m_started.Signal();

      while (HandleEvent())
        ;

      ::SDL_Quit();
      m_surface = NULL;
      m_thread = NULL;

      PTRACE(4, "VSDL\tEnd of event thread");
    }


    bool HandleEvent()
    {
      SDL_Event sdlEvent;
      if (!::SDL_WaitEvent(&sdlEvent)) {
        PTRACE(1, "VSDL\tError getting event: " << ::SDL_GetError());
        return false;
      }

      PWaitAndSignal mutex(*this);

      switch (sdlEvent.type) {
        case SDL_USEREVENT :
          switch (sdlEvent.user.code) {
            case e_AddDevice :
              AddDevice((PVideoOutputDevice_SDL *)sdlEvent.user.data1);
              break;

            case e_RemoveDevice :
              RemoveDevice((PVideoOutputDevice_SDL *)sdlEvent.user.data1);
              return !m_devices.empty();

            case e_SizeChanged :
              AdjustOverlays();
              ((PVideoOutputDevice_SDL *)sdlEvent.user.data1)->m_operationComplete.Signal();
              break;

            case e_ContentChanged :
              ((PVideoOutputDevice_SDL *)sdlEvent.user.data1)->UpdateContent();
              break;

            default :
              PTRACE(5, "SDL\tUnhandled user event " << sdlEvent.user.code);
          }
          break;

        case SDL_QUIT :
          PTRACE(3, "SDL\tUser closed window");
          for (DeviceList::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
            (*it)->FreeOverlay();

          m_devices.clear();
          return false;

        case SDL_VIDEORESIZE :
          PTRACE(4, "SDL\tResize window to " << sdlEvent.resize.w << " x " << sdlEvent.resize.h);
          AdjustOverlays();
          break;

        default :
          PTRACE(5, "SDL\tUnhandled event " << (unsigned)sdlEvent.type);
      }

      return true;
    }


    void AddDevice(PVideoOutputDevice_SDL * device)
    {
      m_devices.push_back(device);

      if (m_surface == NULL) {
        PString deviceName = device->GetDeviceName();

        PINDEX x_pos = deviceName.Find("X=");
        PINDEX y_pos = deviceName.Find("Y=");
        if (x_pos != P_MAX_INDEX && y_pos != P_MAX_INDEX) {
          PString str(PString::Printf, "SDL_VIDEO_WINDOW_POS=%i,%i",
                      atoi(&deviceName[x_pos+2]), atoi(&deviceName[y_pos+2]));
          ::SDL_putenv((char *)(const char *)str);
        }

        ::SDL_WM_SetCaption(device->GetTitle(), NULL);

        m_surface = ::SDL_SetVideoMode(device->GetFrameWidth(),
                                       device->GetFrameHeight(),
                                       0, SDL_SWSURFACE /* | SDL_RESIZABLE */);
        PTRACE_IF(1, m_surface == NULL, "SDL\tCouldn't create SDL surface: " << ::SDL_GetError());
      }

      AdjustOverlays();

      device->m_operationComplete.Signal();
    }


    void RemoveDevice(PVideoOutputDevice_SDL * device)
    {
      m_devices.remove(device);

      if (PAssertNULL(m_surface) != NULL) {
        device->FreeOverlay();
        AdjustOverlays();
      }

      device->m_operationComplete.Signal();
    }


    void AdjustOverlays()
    {
      if (m_surface == NULL)
        return;

      PString title;
      unsigned x = 0;
      unsigned y = 0;
      unsigned fullWidth = 0;
      unsigned fullHeight = 0;

      for (DeviceList::iterator it = m_devices.begin(); it != m_devices.end(); ++it) {
        PVideoOutputDevice_SDL & device = **it;

        if (!title.IsEmpty())
          title += " / ";
        title += device.GetTitle();

        device.m_x = x;
        device.m_y = y;
        if (device.m_overlay == NULL)
          device.CreateOverlay(m_surface);
        else if (device.GetFrameWidth() != (unsigned)device.m_overlay->w ||
                 device.GetFrameHeight() != (unsigned)device.m_overlay->h) {
          device.FreeOverlay();
          device.CreateOverlay(m_surface);
        }

        if (fullWidth < x+device.GetFrameWidth())
          fullWidth = x+device.GetFrameWidth();
        if (fullHeight < y+device.GetFrameHeight())
          fullHeight = y+device.GetFrameHeight();

        x += device.GetFrameWidth();
        if (x > 2*(y+fullHeight)) {
          x = 0;
          y += fullHeight;
        }
      }

      ::SDL_WM_SetCaption(title, NULL);

      if (::SDL_SetVideoMode(fullWidth, fullHeight, 0, SDL_SWSURFACE /* | SDL_RESIZABLE */) != m_surface) {
        PTRACE(1, "SDL\tCouldn't resize surface: " << ::SDL_GetError());
      }

      for (DeviceList::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
        (*it)->UpdateContent();
    }
};


///////////////////////////////////////////////////////////////////////

PVideoOutputDevice_SDL::PVideoOutputDevice_SDL()
  : m_overlay(NULL)
  , m_x(0)
  , m_y(0)
{
  colourFormat = "YUV420P";
}


PVideoOutputDevice_SDL::~PVideoOutputDevice_SDL()
{ 
  Close();
}


PStringArray PVideoOutputDevice_SDL::GetDeviceNames() const
{
  return PString("SDL");
}


PBoolean PVideoOutputDevice_SDL::Open(const PString & name, PBoolean /*startImmediate*/)
{
  Close();

  deviceName = name;

  PSDL_Window::GetInstance().Run();
  PostEvent(PSDL_Window::e_AddDevice, true);

  return IsOpen();
}


PBoolean PVideoOutputDevice_SDL::IsOpen()
{
  return m_overlay != NULL;
}


PBoolean PVideoOutputDevice_SDL::Close()
{
  if (!IsOpen())
    return false;

  PostEvent(PSDL_Window::e_RemoveDevice, true);

  return true;
}


PBoolean PVideoOutputDevice_SDL::SetColourFormat(const PString & colourFormat)
{
  if (colourFormat *= "YUV420P")
    return PVideoOutputDevice::SetColourFormat(colourFormat);

  return false;
}


PBoolean PVideoOutputDevice_SDL::SetFrameSize(unsigned width, unsigned height)
{
  if (width == frameWidth && height == frameHeight)
    return true;

  if (!PVideoOutputDevice::SetFrameSize(width, height))
    return false;

  if (IsOpen())
    PostEvent(PSDL_Window::e_SizeChanged, true);

  return true;
}


PINDEX PVideoOutputDevice_SDL::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(frameWidth, frameHeight, colourFormat));
}


PBoolean PVideoOutputDevice_SDL::SetFrameData(unsigned x, unsigned y,
                                          unsigned width, unsigned height,
                                          const BYTE * data,
                                          PBoolean endFrame) 
{
  if (!IsOpen())
    return false;

  if (x != 0 || y != 0 || width != frameWidth || height != frameHeight || data == NULL || !endFrame)
    return false;

  PWaitAndSignal mutex(PSDL_Window::GetInstance());

  ::SDL_LockYUVOverlay(m_overlay);

  PAssert(frameWidth == (unsigned)m_overlay->w && frameHeight == (unsigned)m_overlay->h, PLogicError);
  PINDEX pixelsFrame = frameWidth * frameHeight;
  PINDEX pixelsQuartFrame = pixelsFrame >> 2;

  const BYTE * dataPtr = data;

  PBYTEArray tempStore;
  if (converter != NULL) {
    converter->Convert(data, tempStore.GetPointer(pixelsFrame+2*pixelsQuartFrame));
    dataPtr = tempStore;
  }

  memcpy(m_overlay->pixels[0], dataPtr,                                  pixelsFrame);
  memcpy(m_overlay->pixels[1], dataPtr + pixelsFrame,                    pixelsQuartFrame);
  memcpy(m_overlay->pixels[2], dataPtr + pixelsFrame + pixelsQuartFrame, pixelsQuartFrame);

  ::SDL_UnlockYUVOverlay(m_overlay);

  PostEvent(PSDL_Window::e_ContentChanged, false);
  return true;
}


PString PVideoOutputDevice_SDL::GetTitle() const
{
  PINDEX pos = deviceName.Find("TITLE=\"");
  if (pos != P_MAX_INDEX) {
    pos += 6;
    PINDEX quote = deviceName.FindLast('"');
    return PString(PString::Literal, deviceName(pos, quote > pos ? quote : P_MAX_INDEX));
  }

  return "Video Output";
}


void PVideoOutputDevice_SDL::UpdateContent()
{
  SDL_Rect rect;
  rect.x = (Uint16)m_x;
  rect.y = (Uint16)m_y;
  rect.w = (Uint16)frameWidth;
  rect.h = (Uint16)frameHeight;
  ::SDL_DisplayYUVOverlay(PAssertNULL(m_overlay), &rect);
}


void PVideoOutputDevice_SDL::CreateOverlay(struct SDL_Surface * surface)
{
  if (m_overlay != NULL)
    return;

  m_overlay = ::SDL_CreateYUVOverlay(frameWidth, frameHeight, SDL_IYUV_OVERLAY, surface);
  if (m_overlay == NULL) {
    PTRACE(1, "SDL\tCouldn't create SDL overlay: " << ::SDL_GetError());
    return;
  }

  PINDEX sz = frameWidth*frameHeight;
  memset(m_overlay->pixels[0], 0, sz);
  sz /= 4;
  memset(m_overlay->pixels[1], 0x80, sz);
  memset(m_overlay->pixels[2], 0x80, sz);
}


void PVideoOutputDevice_SDL::FreeOverlay()
{
  if (m_overlay == NULL)
    return;

  ::SDL_FreeYUVOverlay(m_overlay);
  m_overlay = NULL;
}


void PVideoOutputDevice_SDL::PostEvent(unsigned code, bool wait)
{
  SDL_Event sdlEvent;
  sdlEvent.type = SDL_USEREVENT;
  sdlEvent.user.code = code;
  sdlEvent.user.data1 = this;
  sdlEvent.user.data2 = NULL;
  if (::SDL_PushEvent(&sdlEvent) < 0) {
    PTRACE(1, "SDL\tCouldn't post user event " << (unsigned)sdlEvent.user.code << ": " << ::SDL_GetError());
  }
  else {
    PTRACE(5, "SDL\tPosted user event " << (unsigned)sdlEvent.user.code);
    if (wait)
      m_operationComplete.Wait();
  }
}


#else

  #ifdef _MSC_VER
    #pragma message("SDL video support DISABLED")
  #endif

#endif // P_SDL


// End of file ////////////////////////////////////////////////////////////////
