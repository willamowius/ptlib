/*
 * sound.cxx
 *
 * Sound driver implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): Derek Smithies - from a modification of oss module.
 *
 * $Revision$
 * $Author$
 * $Date$
 */
   

#pragma implementation "sound_pulse.h"

#include <pulse/context.h>
#include <pulse/error.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>
#include <pulse/volume.h>
//#include <pulse/gccmacro.h>
#include <pulse/sample.h>
#include <ptlib.h>
#include <ptclib/random.h>

#include "sound_pulse.h"



PCREATE_SOUND_PLUGIN(Pulse, PSoundChannelPulse);

static pa_threaded_mainloop* paloop;
static pa_context* context;

class PulseContext {
private:
  static void notify_cb(pa_context *c,void *userdata) {
    pa_threaded_mainloop_signal(paloop,0);
  }
public:
  PulseContext() {
    paloop=pa_threaded_mainloop_new();
    pa_threaded_mainloop_start(paloop);
    pa_threaded_mainloop_lock(paloop);
    pa_proplist *proplist=pa_proplist_new();
    pa_proplist_sets(proplist,"media.role","phone");
    /* TODO: I wasn't able to make module-cork-music-on-phone do what I expected */
    context=pa_context_new_with_proplist(pa_threaded_mainloop_get_api(paloop),"ptlib",proplist);
    pa_proplist_free(proplist);
    pa_context_connect(context,NULL,PA_CONTEXT_NOFLAGS ,NULL);
    pa_context_set_state_callback(context,notify_cb,NULL);
    while (pa_context_get_state(context)<PA_CONTEXT_READY) {
      pa_threaded_mainloop_wait(paloop);
    }
    pa_context_set_state_callback(context,NULL,NULL);
    pa_threaded_mainloop_unlock(paloop);
  }
  ~PulseContext() {
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_threaded_mainloop_stop(paloop);
    pa_threaded_mainloop_free(paloop);
  }
  static void signal() {
    pa_threaded_mainloop_signal(paloop,0);
  }

};

static PulseContext pamain;

class PulseLock {
public:

  PulseLock() {
    pa_threaded_mainloop_lock(paloop);
  }

  ~PulseLock() {
    pa_threaded_mainloop_unlock(paloop);
  }

  void wait() {
    pa_threaded_mainloop_wait(paloop);
  }

  bool waitFor(pa_operation* operation) {
    if (!operation)
      return false;

    while (pa_operation_get_state(operation)==PA_OPERATION_RUNNING) {
      pa_threaded_mainloop_wait(paloop);
    }
    bool toReturn=pa_operation_get_state(operation)==PA_OPERATION_DONE;
    pa_operation_unref(operation);
    return toReturn;
  }

};

///////////////////////////////////////////////////////////////////////////////
PSoundChannelPulse::PSoundChannelPulse()
{
  PTRACE(6, "Pulse\tConstructor for no args");
  PSoundChannelPulse::Construct();
  setenv ("PULSE_PROP_media.role", "phone", true);
}


PSoundChannelPulse::PSoundChannelPulse(const PString & device,
				       Directions dir,
				       unsigned numChannels,
				       unsigned sampleRate,
				       unsigned bitsPerSample)
{
  PTRACE(6, "Pulse\tConstructor with many args\n");

  PAssert((bitsPerSample == 16), PInvalidParameter);
  Construct();
  ss.rate = sampleRate;
  ss.channels = numChannels;
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannelPulse::Construct()
{
  PTRACE(6, "Pulse\tConstruct ");
  os_handle = -1;
  s = NULL;
  ss.format =  PA_SAMPLE_S16LE;
}


PSoundChannelPulse::~PSoundChannelPulse()
{
  PTRACE(6, "Pulse\tDestructor ");
  Close();
}



static void sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
  if (eol) {
    PulseContext::signal();
  } else {
    ((PStringArray*) userdata)->AppendString(i->name);
  }
}

static void source_info_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata)
{
  if (eol) {
    PulseContext::signal();
  } else {
    /* Ignore monitor sources */
    if (i->monitor_of_sink==PA_INVALID_INDEX)
      ((PStringArray*) userdata)->AppendString(i->name);
  }
}

PStringArray PSoundChannelPulse::GetDeviceNames(Directions dir)
{
  PTRACE(6, "Pulse\tReport devicenames as \"PulseAudio\"");
  PulseLock lock;
  PStringArray devices;
  devices.AppendString("PulseAudio"); // Default device
  pa_operation* operation;
  if (dir==Player) {
    operation=
      pa_context_get_sink_info_list(context,sink_info_cb,&devices);
  } else {
    operation=
      pa_context_get_source_info_list(context,source_info_cb,&devices);
  }
  lock.waitFor(operation);
  return devices;
}


PString PSoundChannelPulse::GetDefaultDevice(Directions dir)
{
  PTRACE(6, "Pulse\t report default device as \"PulseAudio\"");
  PStringArray devicenames;
  devicenames = PSoundChannelPulse::GetDeviceNames(dir);

  return devicenames[0];
}

static void stream_notify_cb(pa_stream *s, void *userdata) {
  PulseContext::signal();
}

static void stream_write_cb(pa_stream* s,size_t nbytes,void *userdata) {
  PulseContext::signal();
}

PBoolean PSoundChannelPulse::Open(const PString & _device,
				  Directions _dir,
				  unsigned _numChannels,
				  unsigned _sampleRate,
				  unsigned _bitsPerSample)
{
  PWaitAndSignal m(deviceMutex);
  PTRACE(6, "Pulse\t Open on device name of " << _device);
  Close();
  direction = _dir;
  mNumChannels = _numChannels;
  mSampleRate = _sampleRate;
  mBitsPerSample = _bitsPerSample;
  Construct();

  PulseLock lock;
  char *app = getenv ("PULSE_PROP_application.name");
  PStringStream appName, streamName;
  if (app != NULL)
    appName << app;
  else
    appName << "PTLib plugin ";
  if (_dir == Player) 
    streamName << ::hex << PRandom::Number();
  else
    streamName << ::hex << PRandom::Number();

  ss.rate = _sampleRate;
  ss.channels = _numChannels;
  ss.format =  PA_SAMPLE_S16LE;  

  const char* dev;
  if (_device=="PulseAudio") {
    /* Default device */
    dev=NULL;
  } else {
    dev=_device;
  }
  s=pa_stream_new(context,appName.GetPointer(),&ss,NULL);
  pa_stream_set_state_callback(s,stream_notify_cb,NULL);

  if (s == NULL) {
    PTRACE(2, ": pa_stream_new() failed: " << pa_strerror(pa_context_errno(context)));
    PTRACE(2, ": pa_stream_new() uses stream " << streamName);
    PTRACE(2, ": pa_stream_new() uses rate " << PINDEX(ss.rate));
    PTRACE(2, ": pa_stream_new() uses channels " << PINDEX(ss.channels));
    return PFalse;
  }

  if (_dir == Player) {
    int err=pa_stream_connect_playback(s,dev,NULL,PA_STREAM_NOFLAGS,NULL,NULL);
    if (err) {
      PTRACE(2, ": pa_connect_playback() failed: " << pa_strerror(err));
      pa_stream_unref(s);
      s=NULL;
      return PFalse;
    }
    pa_stream_set_write_callback(s,stream_write_cb,NULL);
  } else {
    int err=pa_stream_connect_record(s,dev,NULL,PA_STREAM_NOFLAGS);
    if (err) {
      PTRACE(2, ": pa_connect_record() failed: " << pa_strerror(pa_context_errno(context)));
      pa_stream_unref(s);
      s=NULL;
      return PFalse;
    }
    pa_stream_set_read_callback(s,stream_write_cb,NULL);
    /* No input yet */
    record_len=0;
    record_data=NULL;
  }

  /* Wait for stream to become ready */
  while (pa_stream_get_state(s)<PA_STREAM_READY) lock.wait();
  if (pa_stream_get_state(s)!=PA_STREAM_READY) {
    PTRACE(2, "stream state is " << pa_stream_get_state(s));
    pa_stream_unref(s);
    s=NULL;
    return PFalse;
  }

  os_handle = 1;
  return PTrue;
}

PBoolean PSoundChannelPulse::Close()
{
  PWaitAndSignal m(deviceMutex);
  PTRACE(6, "Pulse\tClose");
  PulseLock lock;

  if (s == NULL)
    return PTrue;

  /* Remove the reference. The main loop keeps going and will drain the output */
  pa_stream_disconnect(s);
  pa_stream_unref(s);
  s = NULL;
  os_handle = -1;

  return PTrue;
}

PBoolean PSoundChannelPulse::IsOpen() const
{
  PTRACE(6, "Pulse\t report is open as " << (os_handle >= 0));
  PulseLock lock;
  return os_handle >= 0;
}

PBoolean PSoundChannelPulse::Write(const void * buf, PINDEX len)
{
  PWaitAndSignal m(deviceMutex);
  PTRACE(6, "Pulse\tWrite " << len << " bytes");
  PulseLock lock;
  char* buff=(char*) buf;

  if (!os_handle) {
    PTRACE(4, ": Pulse audio Write() failed as device closed");
    return PFalse;
  }

  size_t toWrite=len;
  while (toWrite) {
    size_t ws;
    while ((ws=pa_stream_writable_size(s))<=0) lock.wait();
    if (ws>toWrite) ws=toWrite;
    int err=pa_stream_write(s,buff,ws,NULL,0,PA_SEEK_RELATIVE);
    if (err) {
      PTRACE(4, ": pa_stream_write() failed: " << pa_strerror(err));
      return PFalse;
    }
    toWrite-=ws;
    buff+=ws;
  }

  lastWriteCount = len;

  PTRACE(6, "Pulse\tWrite completed");
  return PTrue;
}

PBoolean PSoundChannelPulse::Read(void * buf, PINDEX len)
{
  PWaitAndSignal m(deviceMutex);
  PTRACE(6, "Pulse\tRead " << len << " bytes");
  PulseLock lock;
  char* buff=(char*) buf;

  if (!os_handle) {
    PTRACE(4, ": Pulse audio Read() failed as device closed");
    return PFalse;
  }

  size_t toRead=len;
  while (toRead) {
    while (!record_len) {
      /* Fill the record buffer first */
      pa_stream_peek(s,&record_data,&record_len);
      if (!record_len) lock.wait();
    }
    size_t toCopy=toRead<record_len ? toRead : record_len;
    memcpy(buff,record_data,toCopy);
    toRead-=toCopy;
    buff+=toCopy;
    record_data=((char*) record_data)+toCopy;
    record_len-=toCopy;
    /* Buffer empty? */
    if (!record_len) pa_stream_drop(s);
  }

  lastReadCount = len;

  PTRACE(6, "Pulse\tRead completed of " <<len << " bytes");
  return PTrue;
}


PBoolean PSoundChannelPulse::SetFormat(unsigned numChannels,
                              unsigned sampleRate,
                              unsigned bitsPerSample)
{
  PTRACE(6, "Pulse\tSet format");

  ss.rate = sampleRate;
  ss.channels = numChannels;
  PAssert((bitsPerSample == 16), PInvalidParameter);

  return PTrue;
}

// Get  the number of channels (mono/stereo) in the sound.
unsigned PSoundChannelPulse::GetChannels()   const
{
  PTRACE(6, "Pulse\tGetChannels return " 
	 << ss.channels << " channel(s)");
  return ss.channels;
}

// Get the sample rate in samples per second.
unsigned PSoundChannelPulse::GetSampleRate() const
{
  PTRACE(6, "Pulse\tGet sample rate return " 
	 << ss.rate << " samples per second");
  return ss.rate;
}

// Get the sample size in bits per sample.
unsigned PSoundChannelPulse::GetSampleSize() const
{
  return 16;
}

PBoolean PSoundChannelPulse::SetBuffers(PINDEX size, PINDEX count)
{
  PTRACE(6, "Pulse\tSet buffers to " << size << " and " << count);
  bufferSize = size;
  bufferCount = count;

  return PTrue;
}


PBoolean PSoundChannelPulse::GetBuffers(PINDEX & size, PINDEX & count)
{
  size = bufferSize;
  count = bufferCount;
  PTRACE(6, "Pulse\t report buffers as " << size << " and " << count);
  return PTrue;
}


PBoolean PSoundChannelPulse::PlaySound(const PSound & sound, PBoolean wait)
{

  return PFalse;
}


PBoolean PSoundChannelPulse::PlayFile(const PFilePath & filename, PBoolean wait)
{
  return PFalse;
}


PBoolean PSoundChannelPulse::HasPlayCompleted()
{
  return PTrue;
}


PBoolean PSoundChannelPulse::WaitForPlayCompletion()
{
  return PTrue;
}


PBoolean PSoundChannelPulse::RecordSound(PSound & sound)
{
  return PFalse;
}


PBoolean PSoundChannelPulse::RecordFile(const PFilePath & filename)
{
  return PFalse;
}


PBoolean PSoundChannelPulse::StartRecording()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::IsRecordBufferFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::AreAllRecordBuffersFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::WaitForRecordBufferFull()
{
  return PFalse;
}


PBoolean PSoundChannelPulse::WaitForAllRecordBuffersFull()
{
  return PFalse;
}

static void sink_volume_cb(pa_context* context,const pa_sink_info* i,int eol,void* userdata) {
  if (!eol) {
    *((pa_cvolume*) userdata)=i->volume;
    PulseContext::signal();
  }
}

static void source_volume_cb(pa_context* context,const pa_source_info* i,int eol,void* userdata) {
  if (!eol) {
    *((pa_cvolume*) userdata)=i->volume;
    PulseContext::signal();
  }
}

PBoolean PSoundChannelPulse::SetVolume(unsigned newVal)
{
  if (s) {
    PulseLock lock;
    int dev=pa_stream_get_device_index(s);
    pa_operation* operation;
    pa_cvolume volume;
    if (direction==Player) {
      operation=pa_context_get_sink_info_by_index(context,dev,sink_volume_cb,&volume);
    } else {
      operation=pa_context_get_source_info_by_index(context,dev,source_volume_cb,&volume);
    }
    if (!lock.waitFor(operation)) return PFalse;
    pa_cvolume_scale(&volume,newVal*PA_VOLUME_NORM/100);
    if (direction==Player) {
      pa_context_set_sink_volume_by_index(context,dev,&volume,NULL,NULL);
    } else {
      pa_context_set_source_volume_by_index(context,dev,&volume,NULL,NULL);
    }
  }
  return PTrue;
}

PBoolean  PSoundChannelPulse::GetVolume(unsigned &devVol)
{
  if (s) {
    PulseLock lock;
    int dev=pa_stream_get_device_index(s);
    pa_operation* operation;
    pa_cvolume volume;
    if (direction==Player) {
      operation=pa_context_get_sink_info_by_index(context,dev,sink_volume_cb,&volume);
    } else {
      operation=pa_context_get_source_info_by_index(context,dev,source_volume_cb,&volume);
    }
    if (!lock.waitFor(operation)) return PFalse;
    devVol=100*pa_cvolume_avg(&volume)/PA_VOLUME_NORM;
  }
  return PTrue;
}
  


// End of file

