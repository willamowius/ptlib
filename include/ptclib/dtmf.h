/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.org> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * Extract DTMF signals from 16 bit PCM audio
 *
 * Originally written by Poul-Henning Kamp <phk@freebsd.org>
 * Made into a C++ class by Roger Hardiman <roger@freebsd.org>, January 2002
 *
 * $Revision$
 * $Author$
 * $Date$
 */
 
#ifndef PTLIB_DTMF_H
#define PTLIB_DTMF_H

#if P_DTMF

#ifdef P_USE_PRAGMA
#pragma interface
#endif


class PDTMFDecoder : public PObject
{
  PCLASSINFO(PDTMFDecoder, PObject)

  public:
    enum {
      DetectSamples = 520,
      DetectTime = DetectSamples/8  // Milliseconds
    };

    PDTMFDecoder();
    PString Decode(const short * sampleData, PINDEX numSamples, unsigned mult = 1, unsigned div = 1);

  protected:
    enum {
      NumTones = 10
    };

    // key lookup table (initialised once)
    char key[256];

    // frequency table (initialised once)
    int p1[NumTones];

    // variables to be retained on each cycle of the decode function
    int h[NumTones], k[NumTones], y[NumTones];
    int sampleCount, tonesDetected, inputAmplitude;
};


/** This class can be used to generate PCM data for tones (such as telephone
    calling tones and DTMF) at a sample rate of 8khz.

    The class contains a  master volume which is applied as well as the
    individual tone volumes. Thus a master volume ot 50% and a tone voluem
    of 50%  would result in a net volume of 25%.

    Tones may be described via a list of descriptor strings based on an
    ITU-T "semi-standard", one used within various standard documents but not
    a standard in itself. This format was enhanced to allow for multiple
    tones and volume indications.

    The basic format is:

          [volume % ] frequency ':' cadence [ '/' ... ]

      where frequency is one of
          frequency         single frequency tone
          freq1 '+' freq2   two frequency juxtaposed (simple mixing)
          freq1 'x' freq2   first frequency modulated by second frequency
          freq1 '-' freq2   Alternate frequencies, generated tone is freq1
                            used for compatibility with tone filters
      and cadence is
          mintime
          ontime '-' offtime [ '-' ontime '-' offtime [ ... ] ]

      and volume is a percentage of full volume

      examples:
          300:0.25              300Hz for minimum 250ms
          1100:0.4-0.4          1100Hz with cadence 400ms on, 400ms off
          900-1300:1.5          900Hz for 1.5 seconds
          350+440:1             350Hz superimposed with 440Hz (US dial tone) for 1 second
          425x15:0.4-0.2-0.4-2  425Hz modulated with 15Hz (Aus ring back tone)
                                with cadence 400ms on, 200ms off, 400ms on, 2s off
          425:0.4-0.1/50%425:0.4-0.1   425Hz with cadence 400ms on, 100ms off,
                                       400ms on, 100ms off, where second tone is
                                       reduced in volume by 50%

      A database of tones for all contries in the worls is available at:
          http://www.3amsystems.com/wireline/tone-search.htm

  */
class PTones : public PShortArray
{
  PCLASSINFO(PTones, PShortArray)

  public:
    enum {
      MaxVolume = 100,
      DefaultSampleRate = 8000,
      MinFrequency = 30,
      MinModulation = 5,
      SineScale = 1000
    };

    /** Create an empty tone buffer. Tones added will use the specified
        master volume.
      */
    PTones(
      unsigned masterVolume = MaxVolume,      ///< Percentage volume
      unsigned sampleRate = DefaultSampleRate ///< Sample rate of generated data
    );

    /** Create a filled tone buffer using the specified descriptor.
      */
    PTones(
      const PString & descriptor,             ///< Descriptor string for tone(s). See class notes.
      unsigned masterVolume = MaxVolume,      ///< Percentage volume
      unsigned sampleRate = DefaultSampleRate ///< Sample rate of generated data
    );

    /** Generate a tone using the specified descriptor.
        See class general notes for format of the descriptor string.
      */
    bool Generate(
      const PString & descriptor    ///< Descriptor string for tone(s). See class notes.
    );

    /** Generate a tone using the specified values.
        The operation parameter may be '+', 'x', '-' or ' ' for summing, modulation,
        pure tone or silence resepctively.
        The tones duration is always rounded up to the nearest even multiple of the
        tone cycle to assure correct zero crossing when tones change.
      */
    bool Generate(
      char operation,             ///< Operation for mixing frequency
      unsigned frequency1,        ///< Primary frequency for tone
      unsigned frequency2,        ///< Secondary frequency for summing or modulation
      unsigned milliseconds,      ///< Duration of tone
      unsigned volume = MaxVolume ///< Percentage volume
    );

  protected:
    void Construct();

    bool Juxtapose(unsigned frequency1, unsigned frequency2, unsigned milliseconds, unsigned volume);
    bool Modulate (unsigned frequency, unsigned modulate, unsigned milliseconds, unsigned volume);
    bool PureTone (unsigned frequency, unsigned milliseconds, unsigned volume);
    bool Silence  (unsigned milliseconds);

    unsigned CalcSamples(unsigned milliseconds, unsigned frequency1, unsigned frequency2 = 0);

    void AddSample(int sample, unsigned volume);

    unsigned m_sampleRate;
    unsigned m_maxFrequency;
    unsigned m_masterVolume;
    char     m_lastOperation;
    unsigned m_lastFrequency1, m_lastFrequency2;
    int      m_angle1, m_angle2;
};


/**
  * this class can be used to generate PCM data for DTMF tones
  * at a sample rate of 8khz
  */
class PDTMFEncoder : public PTones
{
  PCLASSINFO(PDTMFEncoder, PTones)

  public:
    enum { DefaultToneLen = 100 };

    /**
      * Create PCM data for the specified DTMF sequence 
      */
    PDTMFEncoder(
        const char * dtmf = NULL,      ///< character string to encode
        unsigned milliseconds = DefaultToneLen  ///< length of each DTMF tone in milliseconds
    );

    /**
      * Create PCM data for the specified dtmf key
      */
    PDTMFEncoder(
        char key,      ///< character string to encode
        unsigned milliseconds = DefaultToneLen  ///< length of each DTMF tone in milliseconds
    );    

    /**
      * Add the PCM data for the specified tone sequence to the buffer
      */
    void AddTone(
        const char * str,              ///< string to encode
        unsigned milliseconds = DefaultToneLen  ///< length of DTMF tone in milliseconds
    );

    /**
      * Add the PCM data for the specified tone to the buffer
      */
    void AddTone(
        char ch,                       ///< character to encode
        unsigned milliseconds = DefaultToneLen  ///< length of DTMF tone in milliseconds
    );

    /**
      * Add the PCM data for the specified dual-frequency tone to the buffer
      * frequency2 can be zero, which will generate a single frequency tone
      */
    void AddTone(
        double frequency1,                  // primary frequency
        double frequency2 = 0,              // secondary frequency, or 0 if no secondary frequency
        unsigned milliseconds = DefaultToneLen  // length of DTMF tone in milliseconds
    );

    /**
      * Generate PCM data for a single cadence of the US standard ring tone
      * of 440/480hz for 2 seconds, followed by 5 seconds of silence
      */
    void GenerateRingBackTone()
    {
      Generate("440+480:2-4");
    }

    /**
      * Generate PCM data for 1 second of US standard dial tone 
      * of 350/440hz 
      */
    void GenerateDialTone()
    {
      Generate("350+440:1");
    }

    /**
      * Generate PCM data for a single cadence of the US standard busy tone
      * of 480/620hz for 1/2 second, 1/2 second of silence
      */
    void GenerateBusyTone()
    {
      Generate("480+620:0.5-0.5");
    }

    /**
     * Convenience function to get the ASCII character for a DTMF index, 
     * where the index varies from 0 to 15
     *
     * @returns ASCII value
     */

    char DtmfChar(
        PINDEX i    ///< index of tone
    );
    // Overiding GetSize() screws up the SetSize()
};


#endif // P_DTMF

#endif // PTLIB_DTMF_H


// End Of File ///////////////////////////////////////////////////////////////
