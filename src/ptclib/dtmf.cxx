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

#ifdef __GNUC__
#pragma implementation "dtmf.h"
#endif

#include <ptlib.h>
#include <ptclib/dtmf.h>

#if P_DTMF

#include <math.h>

/* Integer math scaling factor */
#define FSC (1<<12)

/* This is the Q of the filter (pole radius) */
#define POLRAD .99

#define P2 ((int)(POLRAD*POLRAD*FSC))

PDTMFDecoder::PDTMFDecoder()
  : sampleCount(0)
  , tonesDetected(0)
  , inputAmplitude(0)
{
  // Initialise the class
  int i;
  for (i = 0; i < NumTones; i++)
    y[i] = h[i] = k[i] = 0;

  for (i = 0; i < 256; i++)
    key[i] = '?';

  /* We encode the tones in 8 bits, translate those to symbol */
  key[0x11] = '1'; key[0x12] = '4'; key[0x14] = '7'; key[0x18] = '*';
  key[0x21] = '2'; key[0x22] = '5'; key[0x24] = '8'; key[0x28] = '0';
  key[0x41] = '3'; key[0x42] = '6'; key[0x44] = '9'; key[0x48] = '#';
  key[0x81] = 'A'; key[0x82] = 'B'; key[0x84] = 'C'; key[0x88] = 'D';

  /* The frequencies we're trying to detect */
  /* These are precalculated to save processing power */
  /* static int dtmf[9] = {697, 770, 852, 941, 1209, 1336, 1477, 1633, 1100, 2100}; */
  /* p1[tone] = (-cos(2 * 3.141592 * dtmf[tone] / 8000.0) * FSC) */
  p1[0] = -3497; p1[1] = -3369; p1[2] = -3212; p1[3] = -3027;
  p1[4] = -2384; p1[5] = -2040; p1[6] = -1635; p1[7] = -1164;
  p1[8] = -2660; p1[9] = 321;
}

PString PDTMFDecoder::Decode(const short * sampleData, PINDEX numSamples, unsigned mult, unsigned div)
{
#if 0
  {
    static int fd = -1;
    if (fd < 0) {
      fd = ::_open("dtmf.pcm", _O_BINARY | _O_CREAT | O_RDWR, 0777);
      PTRACE(1, "DTMF\tdebug file opened");
    }
    if (fd >= 0) 
      ::_write(fd, sampleData, numSamples*2);
  }
#endif

  PString keyString;

  PINDEX pos;
  for (pos = 0; pos < numSamples; pos++) {

    /* Read (and scale) the next 16 bit sample */
    int x = (int)(mult * (*sampleData++)) / div;
    x = x / (32768/FSC);

    /* Input amplitude */
    if (x > 0)
      inputAmplitude += (x - inputAmplitude) / 128;
    else
      inputAmplitude += (-x - inputAmplitude) / 128;

    /* For each tone */
    int newTones = 0;
    for (int tone = 0; tone < NumTones; tone++) {

      /* Turn the crank */
      int c = (P2 * (x - k[tone])) / FSC;
      int d = x + c;
      int f = (p1[tone] * (d - h[tone])) / FSC;
      int n = x - k[tone] - c;
      k[tone] = h[tone] + f;
      h[tone] = f + d;

      /* Detect and Average */
      if (n > 0)
        y[tone] += (n - y[tone]) / 64;
      else
        y[tone] += (-n - y[tone]) / 64;

      /* Threshold */
      if (y[tone] > FSC/10 && y[tone] > inputAmplitude) 
        newTones |= 1 << tone;
    }

    /* Hysteresis and noise supressor */
    if (newTones != tonesDetected) {
      sampleCount = 0;
      tonesDetected = newTones;
    }
    else if (sampleCount++ == DetectSamples) {
      if (tonesDetected < 256) {
        if (key[tonesDetected] != '?') {
          PTRACE(3,"DTMF\tDetected '" << key[tonesDetected] << "' in PCM-16 stream");
          keyString += key[tonesDetected];
        }
      }
      else {
        char ch = 0;
        if ((tonesDetected & 0x100) != 0)
          ch = 'X';
        else if ((tonesDetected & 0x200) != 0)
          ch = 'Y';

        if (ch != 0) {
          PTRACE(3,"DTMF\tDetected tone '" << ch << "' in PCM-16 stream");
          keyString += ch;
        }
      }
    }
  }
  return keyString;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int sine(int angle, int freq)
{
  static int const sinArray[2000] = {
    0,0,1,2,3,3,4,5,6,7,7,8,9,10,10,11,12,13,14,14,15,16,17,18,18,19,20,21,21,22,
    23,24,25,25,26,27,28,29,29,30,31,32,32,33,34,35,36,36,37,38,39,40,40,41,42,43,43,44,45,46,
    47,47,48,49,50,51,51,52,53,54,54,55,56,57,58,58,59,60,61,62,62,63,64,65,65,66,67,68,69,69,
    70,71,72,72,73,74,75,76,76,77,78,79,80,80,81,82,83,83,84,85,86,87,87,88,89,90,90,91,92,93,
    94,94,95,96,97,98,98,99,100,101,101,102,103,104,105,105,106,107,108,108,109,110,111,112,112,113,114,115,115,116,
    117,118,119,119,120,121,122,122,123,124,125,126,126,127,128,129,130,130,131,132,133,133,134,135,136,137,137,138,139,140,
    140,141,142,143,144,144,145,146,147,147,148,149,150,151,151,152,153,154,154,155,156,157,157,158,159,160,161,161,162,163,
    164,164,165,166,167,168,168,169,170,171,171,172,173,174,175,175,176,177,178,178,179,180,181,181,182,183,184,185,185,186,
    187,188,188,189,190,191,192,192,193,194,195,195,196,197,198,198,199,200,201,202,202,203,204,205,205,206,207,208,208,209,
    210,211,212,212,213,214,215,215,216,217,218,218,219,220,221,221,222,223,224,225,225,226,227,228,228,229,230,231,231,232,
    233,234,234,235,236,237,238,238,239,240,241,241,242,243,244,244,245,246,247,247,248,249,250,250,251,252,253,254,254,255,
    256,257,257,258,259,260,260,261,262,263,263,264,265,266,266,267,268,269,269,270,271,272,272,273,274,275,275,276,277,278,
    278,279,280,281,282,282,283,284,285,285,286,287,288,288,289,290,291,291,292,293,294,294,295,296,297,297,298,299,300,300,
    301,302,303,303,304,305,306,306,307,308,309,309,310,311,312,312,313,314,314,315,316,317,317,318,319,320,320,321,322,323,
    323,324,325,326,326,327,328,329,329,330,331,332,332,333,334,335,335,336,337,337,338,339,340,340,341,342,343,343,344,345,
    346,346,347,348,349,349,350,351,352,352,353,354,354,355,356,357,357,358,359,360,360,361,362,363,363,364,365,365,366,367,
    368,368,369,370,371,371,372,373,373,374,375,376,376,377,378,379,379,380,381,381,382,383,384,384,385,386,387,387,388,389,
    389,390,391,392,392,393,394,394,395,396,397,397,398,399,400,400,401,402,402,403,404,405,405,406,407,407,408,409,410,410,
    411,412,412,413,414,415,415,416,417,417,418,419,420,420,421,422,422,423,424,425,425,426,427,427,428,429,430,430,431,432,
    432,433,434,434,435,436,437,437,438,439,439,440,441,442,442,443,444,444,445,446,446,447,448,449,449,450,451,451,452,453,
    453,454,455,456,456,457,458,458,459,460,460,461,462,463,463,464,465,465,466,467,467,468,469,470,470,471,472,472,473,474,
    474,475,476,476,477,478,478,479,480,481,481,482,483,483,484,485,485,486,487,487,488,489,489,490,491,492,492,493,494,494,
    495,496,496,497,498,498,499,500,500,501,502,502,503,504,504,505,506,507,507,508,509,509,510,511,511,512,513,513,514,515,
    515,516,517,517,518,519,519,520,521,521,522,523,523,524,525,525,526,527,527,528,529,529,530,531,531,532,533,533,534,535,
    535,536,537,537,538,539,539,540,541,541,542,543,543,544,545,545,546,547,547,548,549,549,550,550,551,552,552,553,554,554,
    555,556,556,557,558,558,559,560,560,561,562,562,563,564,564,565,565,566,567,567,568,569,569,570,571,571,572,573,573,574,
    575,575,576,576,577,578,578,579,580,580,581,582,582,583,583,584,585,585,586,587,587,588,589,589,590,590,591,592,592,593,
    594,594,595,596,596,597,597,598,599,599,600,601,601,602,602,603,604,604,605,606,606,607,607,608,609,609,610,611,611,612,
    612,613,614,614,615,616,616,617,617,618,619,619,620,620,621,622,622,623,624,624,625,625,626,627,627,628,628,629,630,630,
    631,631,632,633,633,634,635,635,636,636,637,638,638,639,639,640,641,641,642,642,643,644,644,645,645,646,647,647,648,648,
    649,650,650,651,651,652,653,653,654,654,655,655,656,657,657,658,658,659,660,660,661,661,662,663,663,664,664,665,666,666,
    667,667,668,668,669,670,670,671,671,672,673,673,674,674,675,675,676,677,677,678,678,679,679,680,681,681,682,682,683,683,
    684,685,685,686,686,687,687,688,689,689,690,690,691,691,692,693,693,694,694,695,695,696,697,697,698,698,699,699,700,700,
    701,702,702,703,703,704,704,705,705,706,707,707,708,708,709,709,710,710,711,712,712,713,713,714,714,715,715,716,717,717,
    718,718,719,719,720,720,721,721,722,723,723,724,724,725,725,726,726,727,727,728,728,729,730,730,731,731,732,732,733,733,
    734,734,735,735,736,736,737,738,738,739,739,740,740,741,741,742,742,743,743,744,744,745,745,746,746,747,748,748,749,749,
    750,750,751,751,752,752,753,753,754,754,755,755,756,756,757,757,758,758,759,759,760,760,761,761,762,762,763,763,764,764,
    765,765,766,766,767,768,768,769,769,770,770,771,771,772,772,773,773,774,774,774,775,775,776,776,777,777,778,778,779,779,
    780,780,781,781,782,782,783,783,784,784,785,785,786,786,787,787,788,788,789,789,790,790,791,791,792,792,793,793,793,794,
    794,795,795,796,796,797,797,798,798,799,799,800,800,801,801,802,802,802,803,803,804,804,805,805,806,806,807,807,808,808,
    809,809,809,810,810,811,811,812,812,813,813,814,814,814,815,815,816,816,817,817,818,818,819,819,819,820,820,821,821,822,
    822,823,823,823,824,824,825,825,826,826,827,827,827,828,828,829,829,830,830,831,831,831,832,832,833,833,834,834,834,835,
    835,836,836,837,837,837,838,838,839,839,840,840,840,841,841,842,842,843,843,843,844,844,845,845,846,846,846,847,847,848,
    848,848,849,849,850,850,850,851,851,852,852,853,853,853,854,854,855,855,855,856,856,857,857,857,858,858,859,859,859,860,
    860,861,861,861,862,862,863,863,863,864,864,865,865,865,866,866,867,867,867,868,868,869,869,869,870,870,870,871,871,872,
    872,872,873,873,874,874,874,875,875,875,876,876,877,877,877,878,878,878,879,879,880,880,880,881,881,881,882,882,883,883,
    883,884,884,884,885,885,885,886,886,887,887,887,888,888,888,889,889,889,890,890,891,891,891,892,892,892,893,893,893,894,
    894,894,895,895,895,896,896,896,897,897,898,898,898,899,899,899,900,900,900,901,901,901,902,902,902,903,903,903,904,904,
    904,905,905,905,906,906,906,907,907,907,908,908,908,909,909,909,910,910,910,911,911,911,912,912,912,913,913,913,913,914,
    914,914,915,915,915,916,916,916,917,917,917,918,918,918,918,919,919,919,920,920,920,921,921,921,922,922,922,922,923,923,
    923,924,924,924,925,925,925,925,926,926,926,927,927,927,928,928,928,928,929,929,929,930,930,930,930,931,931,931,932,932,
    932,932,933,933,933,934,934,934,934,935,935,935,935,936,936,936,937,937,937,937,938,938,938,939,939,939,939,940,940,940,
    940,941,941,941,941,942,942,942,942,943,943,943,944,944,944,944,945,945,945,945,946,946,946,946,947,947,947,947,948,948,
    948,948,949,949,949,949,950,950,950,950,951,951,951,951,952,952,952,952,952,953,953,953,953,954,954,954,954,955,955,955,
    955,956,956,956,956,956,957,957,957,957,958,958,958,958,958,959,959,959,959,960,960,960,960,960,961,961,961,961,962,962,
    962,962,962,963,963,963,963,963,964,964,964,964,964,965,965,965,965,965,966,966,966,966,967,967,967,967,967,967,968,968,
    968,968,968,969,969,969,969,969,970,970,970,970,970,971,971,971,971,971,972,972,972,972,972,972,973,973,973,973,973,973,
    974,974,974,974,974,975,975,975,975,975,975,976,976,976,976,976,976,977,977,977,977,977,977,978,978,978,978,978,978,979,
    979,979,979,979,979,980,980,980,980,980,980,980,981,981,981,981,981,981,981,982,982,982,982,982,982,983,983,983,983,983,
    983,983,984,984,984,984,984,984,984,984,985,985,985,985,985,985,985,986,986,986,986,986,986,986,986,987,987,987,987,987,
    987,987,987,988,988,988,988,988,988,988,988,989,989,989,989,989,989,989,989,989,990,990,990,990,990,990,990,990,990,990,
    991,991,991,991,991,991,991,991,991,992,992,992,992,992,992,992,992,992,992,992,993,993,993,993,993,993,993,993,993,993,
    993,994,994,994,994,994,994,994,994,994,994,994,994,995,995,995,995,995,995,995,995,995,995,995,995,995,995,996,996,996,
    996,996,996,996,996,996,996,996,996,996,996,996,997,997,997,997,997,997,997,997,997,997,997,997,997,997,997,997,997,997,
    998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,998,999,999,999,999,999,999,
    999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,
    999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999
  };
  static int const sinArraySize = sizeof(sinArray)/sizeof(sinArray[0]);

  int adjustedAngle = (int)(angle*sinArraySize*4LL/freq);
  int quadrant = adjustedAngle / sinArraySize;
  int offset   = adjustedAngle % sinArraySize;

  switch (quadrant) {
    case 0:
      return sinArray[offset];
    case 1:
      return sinArray[sinArraySize-1-offset];
    case 2: 
      return -sinArray[offset];
    default:
      return -sinArray[sinArraySize-1-offset];
  }
}


////////////////////////////////////////////////////////////////////////

    
PTones::PTones(unsigned volume, unsigned sampleRate)
  : m_sampleRate(sampleRate)
  , m_masterVolume(volume)
{
  Construct();
}


PTones::PTones(const PString & descriptor, unsigned volume, unsigned sampleRate)
  : m_sampleRate(sampleRate)
  , m_masterVolume(volume)
{
  Construct();

  if (!Generate(descriptor)) {
    PTRACE(1,"DTMF\tCannot encode tone \"" << descriptor << '"');
  }
}


void PTones::Construct()
{
  m_lastOperation = 0;
  m_lastFrequency1 = 0;
  m_lastFrequency2 = 0;
  m_angle1 = 0;
  m_angle2 = 0;

  if (m_sampleRate < 8000)
    m_sampleRate = 8000;
  else if (m_sampleRate > 96000)
    m_sampleRate = 96000;

  m_maxFrequency = m_sampleRate/4;

  if (m_masterVolume < 1)
    m_masterVolume = 1;
  else if (m_masterVolume > 100)
    m_masterVolume = 100;
}


bool PTones::Generate(const PString & descriptor)
{
  PStringArray toneChunks = descriptor.Tokenise('/');
  if (toneChunks.IsEmpty())
    return false;

  for (PINDEX chunk = 0; chunk < toneChunks.GetSize(); chunk++) {
    // split frequency and cadence
    PINDEX pos = toneChunks[chunk].Find(':');
    if (pos == P_MAX_INDEX)
      return false;

    PString frequencyStr = toneChunks[chunk].Left(pos).Trim();
    PString cadenceStr = toneChunks[chunk].Mid(pos+1).Trim();

    if (cadenceStr.IsEmpty())
      return false;

    // Do we have a volume?
    unsigned volume = 100;
    if ((pos = frequencyStr.Find('%')) != P_MAX_INDEX) {
        volume = frequencyStr.Left(pos).AsUnsigned();
        if (volume < 1 || volume > 100)
          return false;
        frequencyStr.Delete(0, pos+1);
    }

    if (frequencyStr.IsEmpty())
      return false;

    // Parse the frequencies
    unsigned frequency1, frequency2;
    char operation;
    if ((pos =  frequencyStr.FindOneOf("+-x")) != P_MAX_INDEX) {
      frequency1 = frequencyStr.Left(pos).AsUnsigned();
      frequency2 = frequencyStr.Mid(pos+1).AsUnsigned();
      operation = frequencyStr[pos];
    }
    else {
      frequency1 = frequency2 = frequencyStr.AsUnsigned();
      operation = '-';
    }

    // Parse the cadence
    double duration = cadenceStr.AsReal();

    // First one
    if (!Generate(operation, frequency1, frequency2, (unsigned)(duration*1000), volume))
      return false;

    char originalOperation = operation;
    operation = ' ';

    pos = 0;
    while ((pos = cadenceStr.Find('-', pos)) != P_MAX_INDEX) {
      duration = cadenceStr.Mid(++pos).AsReal();
      if (duration < 0 || duration > 60)
        return false;

      if (!Generate(operation, frequency1, frequency2, (unsigned)(duration*1000), volume))
        return false;

      // Alternate between the tone and silence
      operation = operation == ' ' ? originalOperation : ' ';
    }
  }

  return true;
}


bool PTones::Generate(char operation, unsigned frequency1, unsigned frequency2, unsigned milliseconds, unsigned volume)
{
  if (m_lastOperation  != operation  ||
      m_lastFrequency1 != frequency1 ||
      m_lastFrequency2 != frequency2) {
    m_lastOperation  = operation;
    m_lastFrequency1 = frequency1;
    m_lastFrequency2 = frequency2;

    m_angle1 = 0;
    m_angle2 = 0;
  }

  switch (operation) {
    case '+':
      return Juxtapose(frequency1, frequency2, milliseconds, volume);

    case 'x':
      return Modulate(frequency1, frequency2, milliseconds, volume);

    case '-':
      return PureTone(frequency1, milliseconds, volume);

    case ' ':
      return Silence(milliseconds);
  }

  return false;
}


bool PTones::Juxtapose(unsigned frequency1, unsigned frequency2, unsigned milliseconds, unsigned volume)
{
  if (frequency1 < MinFrequency || frequency1 > m_maxFrequency ||
      frequency2 < MinFrequency || frequency2 > m_maxFrequency)
    return false;

  // TODO this gived 8000 samples for 100 ms !!!
  //unsigned samples = CalcSamples(milliseconds, frequency1, frequency2);
  unsigned samples = milliseconds * m_sampleRate / 1000;
  while (samples-- > 0) {
    int a1 = sine(m_angle1, m_sampleRate);
    int a2 = sine(m_angle2, m_sampleRate);

    AddSample((a1 + a2) / 2, volume);

    m_angle1 += frequency1;
    if (m_angle1 >= (int)m_sampleRate) 
      m_angle1 -= m_sampleRate;

    m_angle2 += frequency2;
    if (m_angle2 >= (int)m_sampleRate) 
      m_angle2 -= m_sampleRate;
  }
  return true;
}


bool PTones::Modulate(unsigned frequency1, unsigned modulator, unsigned milliseconds, unsigned volume)
{
  if (frequency1 > m_maxFrequency || frequency1 > m_maxFrequency || modulator < MinModulation || modulator >= frequency1/2)
    return false;

  unsigned samples = CalcSamples(milliseconds, frequency1, modulator);

  while (samples-- > 0) {
    int a1 = sine(m_angle1, m_sampleRate);   // -999 to 999
    int a2 = sine(m_angle2, m_sampleRate);   // -999 to 999

    AddSample((a1 * (a2 + SineScale)) / SineScale / 2, volume);

    m_angle1 += frequency1;
    if (m_angle1 >= (int)m_sampleRate) 
      m_angle1 -= m_sampleRate;

    m_angle2 += modulator;
    if (m_angle2 >= (int)m_sampleRate) 
      m_angle2 -= m_sampleRate;
  }
  return true;
}

static unsigned char tone_2100[320] = {
	0x00, 0x00, 0xF4, 0x3F, 0xF7, 0xF5, 0xA0, 0xC1, 0xD2, 0x13, 0x44, 0x3B, 0xE1, 0xE2, 0x4E, 0xC9, 
	0xB5, 0x25, 0xC8, 0x30, 0xA3, 0xD2, 0x56, 0xD6, 0xE6, 0x33, 0x84, 0x21, 0xD7, 0xC6, 0x74, 0xE7, 
	0x03, 0x3D, 0xF9, 0x0E, 0xA4, 0xC0, 0xF8, 0xFA, 0x27, 0x40, 0xF7, 0xFA, 0xA4, 0xC0, 0xFA, 0x0E, 
	0x02, 0x3D, 0x74, 0xE7, 0xD7, 0xC6, 0x84, 0x21, 0xE6, 0x33, 0x57, 0xD6, 0xA4, 0xD2, 0xC8, 0x30, 
	0xB4, 0x25, 0x4D, 0xC9, 0xE1, 0xE2, 0x45, 0x3B, 0xD3, 0x13, 0x9F, 0xC1, 0xF7, 0xF5, 0xF4, 0x3F, 
	0x01, 0x00, 0x0D, 0xC0, 0x09, 0x0A, 0x60, 0x3E, 0x2D, 0xEC, 0xBB, 0xC4, 0x20, 0x1D, 0xB3, 0x36, 
	0x4B, 0xDA, 0x38, 0xCF, 0x5C, 0x2D, 0xAA, 0x29, 0x19, 0xCC, 0x7C, 0xDE, 0x28, 0x39, 0x8C, 0x18, 
	0xFE, 0xC2, 0x06, 0xF1, 0x5C, 0x3F, 0x08, 0x05, 0xDA, 0xBF, 0x08, 0x05, 0x5D, 0x3F, 0x07, 0xF1, 
	0xFE, 0xC2, 0x8C, 0x18, 0x29, 0x39, 0x7C, 0xDE, 0x1A, 0xCC, 0xAA, 0x29, 0x5C, 0x2D, 0x38, 0xCF, 
	0x4C, 0xDA, 0xB3, 0x36, 0x20, 0x1D, 0xBC, 0xC4, 0x2D, 0xEC, 0x62, 0x3E, 0x09, 0x0A, 0x0D, 0xC0, 
	0x01, 0x00, 0xF4, 0x3F, 0xF7, 0xF5, 0x9F, 0xC1, 0xD2, 0x13, 0x45, 0x3B, 0xE0, 0xE2, 0x4D, 0xC9, 
	0xB4, 0x25, 0xC7, 0x30, 0xA3, 0xD2, 0x56, 0xD6, 0xE6, 0x33, 0x85, 0x21, 0xD8, 0xC6, 0x74, 0xE7, 
	0x03, 0x3D, 0xFA, 0x0E, 0xA4, 0xC0, 0xF8, 0xFA, 0x26, 0x40, 0xF7, 0xFA, 0xA3, 0xC0, 0xFA, 0x0E, 
	0x03, 0x3D, 0x73, 0xE7, 0xD8, 0xC6, 0x85, 0x21, 0xE6, 0x33, 0x57, 0xD6, 0xA3, 0xD2, 0xC7, 0x30, 
	0xB5, 0x25, 0x4E, 0xC9, 0xE0, 0xE2, 0x44, 0x3B, 0xD3, 0x13, 0x9F, 0xC1, 0xF7, 0xF5, 0xF3, 0x3F, 
	0x00, 0x00, 0x0D, 0xC0, 0x0A, 0x0A, 0x61, 0x3E, 0x2D, 0xEC, 0xBC, 0xC4, 0x1F, 0x1D, 0xB2, 0x36, 
	0x4B, 0xDA, 0x38, 0xCF, 0x5C, 0x2D, 0xA9, 0x29, 0x1A, 0xCC, 0x7B, 0xDE, 0x29, 0x39, 0x8D, 0x18, 
	0xFD, 0xC2, 0x07, 0xF1, 0x5C, 0x3F, 0x08, 0x05, 0xDA, 0xBF, 0x08, 0x05, 0x5B, 0x3F, 0x07, 0xF1, 
	0xFE, 0xC2, 0x8D, 0x18, 0x28, 0x39, 0x7B, 0xDE, 0x19, 0xCC, 0xA9, 0x29, 0x5D, 0x2D, 0x38, 0xCF, 
	0x4B, 0xDA, 0xB2, 0x36, 0x1F, 0x1D, 0xBB, 0xC4, 0x2D, 0xEC, 0x61, 0x3E, 0x09, 0x0A, 0x0C, 0xC0
};


bool PTones::PureTone(unsigned frequency1, unsigned milliseconds, unsigned volume)
{
  if (frequency1 == 2100) {
    int samples = milliseconds * 8;
    short * tone = (short *)tone_2100;
    unsigned int toneLen  = sizeof(tone_2100) / 2;
    PINDEX length;
    for (length = 0; length < samples; ++length) {
      PINDEX length = GetSize();
      SetSize(length + 1);
      short sample = tone[length % toneLen];
      //sample *= volume;
      //sample *= masterVolume;
      //sample /= SineScale*100*100/SHRT_MAX;
      SetAt(length, (short)sample);
    }
    return true;
  }

  if (frequency1 < MinFrequency || frequency1 > m_maxFrequency)
    return false;

  unsigned samples = CalcSamples(milliseconds, frequency1);
  while (samples-- > 0) {
    AddSample(sine(m_angle1, m_sampleRate), volume);

    m_angle1 += frequency1;
    if (m_angle1 >= (int)m_sampleRate) 
      m_angle1 -= m_sampleRate;
  }
  return true;
}


bool PTones::Silence(unsigned milliseconds)
{
  unsigned samples = milliseconds * m_sampleRate/1000;
  while (samples-- > 0) 
    AddSample(0, 0);
  return true;
}


unsigned PTones::CalcSamples(unsigned ms, unsigned f1, unsigned f2)
{
  // firstly, find the minimum time to repeat the waveform
  unsigned v1 = 1;
  unsigned v2 = 1;

  if (f2 > 0)
  {
      while (v1*f2 != v2*f1) {
        if (v1*f2 < v2*f1)
          v1++;
        else
          v2++;
      }
  }

  // v1 repetitions of f1 == v2 repetitions of f2
  //cout << v1 << " cycles of " << f1 << "hz = " << v2 << " samples of " << f2 << "hz" << endl;

  // now find the number of times we need to repeat this to match the sampling rate
  unsigned n1 = 1;
  unsigned n2 = 1;
  while (n1*m_sampleRate*v1 != n2*f1) {
    if (n1*m_sampleRate*v1 < n2*f1) 
      n1++;
    else
      n2++;
  }

  // v1 repetitions of t == v2 repetitions sample frequency
  //cout << n1*v1 << " cycles at " << f1 << "hz = "
  //     << n1*v2 << " cycles at " << f2 << "hz = "
  //     << n2    << " samples at " << m_sampleRate << "hz" << endl;

  // Make sure we round up the number of milliseconds to even multiple of cycles
  return ms == 0 ? n2 : ((ms * m_sampleRate/1000 + n2 - 1)/n2*n2);
}


void PTones::AddSample(int sample, unsigned volume)
{
  // Sample added is value from -1000 to 1000, rescale to short range -32767 to +32767
  PINDEX length = GetSize();
  SetSize(length + 1);
  sample *= volume;
  sample *= m_masterVolume;
  sample /= SineScale*100*100/SHRT_MAX;
  SetAt(length, (short)sample);
}


////////////////////////////////////////////////////////////////////////

PDTMFEncoder::PDTMFEncoder(const char * dtmf, unsigned milliseconds) :
   PTones() 
{
  AddTone(dtmf, milliseconds);
}

PDTMFEncoder::PDTMFEncoder(char digit, unsigned milliseconds) :
   PTones() 
{
  AddTone(digit, milliseconds);
}

void PDTMFEncoder::AddTone(const char * str, unsigned milliseconds)
{
  if (str == NULL)
      return;

  while (*str != '\0')
    AddTone(*str++, milliseconds);
}


void PDTMFEncoder::AddTone(char digit, unsigned milliseconds)
{
  // DTMF frequencies as per http://www.commlinx.com.au/DTMF_frequencies.htm

  static struct {
    char code;
    char operation;
    unsigned frequency1;
    unsigned frequency2;
  } const dtmfData[] = {
    { '0', '+', 941,1336 }, 
    { '1', '+', 697,1209 }, 
    { '2', '+', 697,1336 }, 
    { '3', '+', 697,1477 }, 
    { '4', '+', 770,1209 }, 
    { '5', '+', 770,1336 }, 
    { '6', '+', 770,1477 }, 
    { '7', '+', 852,1209 }, 
    { '8', '+', 852,1336 }, 
    { '9', '+', 852,1477 }, 
    { '*', '+', 941,1209 }, 
    { '#', '+', 941,1477 }, 
    { 'A', '+', 697,1633 }, 
    { 'B', '+', 770,1633 }, 
    { 'C', '+', 852,1633 }, 
    { 'D', '+', 941,1633 }, 
    { 'a', '+', 697,1633 }, 
    { 'b', '+', 770,1633 }, 
    { 'c', '+', 852,1633 }, 
    { 'd', '+', 941,1633 }, 
    { 'X', '-', 1100     }, // CNG  1100 hz - sent by originating fax machine after dialing
    { 'x', '-', 1100     }, // CNG
    { 'Y', '-', 2100     }, // CED  2100 hz - sent by terminating fax machine after answer to disable echo cancellers
    { 'y', '-', 2100     }  // CED
  };

  for (PINDEX i = 0; i < PARRAYSIZE(dtmfData); i++) {
    if (dtmfData[i].code == digit) {
      Generate(dtmfData[i].operation, dtmfData[i].frequency1, dtmfData[i].frequency2, milliseconds);
      break;
    }
  }
}


void PDTMFEncoder::AddTone(double f1, double f2, unsigned milliseconds)
{
  if (f1 > 0 && f1 < m_maxFrequency && f2 > 0 && f2 < m_maxFrequency){
    Generate('+', (unsigned)f1, (unsigned)f2, milliseconds);
  } else {
    PAssertAlways(PInvalidParameter);
  }  
}


char PDTMFEncoder::DtmfChar(PINDEX i)
{
  PAssert(i < 16, "Only 16 dtmf symbols. Index too large");

  static char dtmfSymbols[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','*','#' };
  return dtmfSymbols[i];
}


#endif // P_DTMF

////////////////////////////////////////////////////////////////////////////
