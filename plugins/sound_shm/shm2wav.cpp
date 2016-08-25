/***************************************************************************
 * shm2wav - consumer sample code to retrieve frames from SHM
 *
 *   Copyright (C) 2009 by mattes   *
 *
 * requires  libsndfile    (rpms: libsndfile-1.0+ libsndfile-devel)
 *
 ***************************************************************************/

#define VERSION "shm2wav 0905"
#define CVSREVID "$Id$"

//The headers
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"


void
usage()
{
   printf("%s\r\n", VERSION );
   printf("\r\n"
          "        shm2wav wavfile\r\n"
   );
}


#include <sndfile.h>
SNDFILE *sFxx = NULL;
char sErr[1024];

int
openWAVfile( char *filepath, int samplerate, int channels )
{
   SF_INFO sfinfo;
   memset (&sfinfo, 0, sizeof(sfinfo)) ;

   sfinfo.format = SF_FORMAT_WAV| SF_FORMAT_PCM_16 |SF_ENDIAN_LITTLE;
   sfinfo.samplerate = 48000;
   sfinfo.channels = 2;
sfinfo.samplerate = samplerate;
sfinfo.channels = channels;

   sFxx = sf_open( filepath, SFM_WRITE , &sfinfo) ;
   if ( sFxx == NULL ) {
      sf_error_str( NULL, sErr,sizeof(sErr) );
      printf("openWAVfile(): open error '%s': %s",
           filepath, sErr );
      //sf_perror(NULL);
      return -1;
   }

   printf("openWAVfile(): open ok '%s':", filepath );
   return 0;
}

int
closeWAVfile()
{
   if ( sFxx ) sf_close( sFxx );
   sFxx = NULL;
   return 0;
}

int
writeWAVfile( unsigned char *buf, int len )
{
   if ( sFxx == NULL ) return 0;   //file not open

   int rc = sf_write_raw( sFxx, buf, len );

   if ( rc != len ) {
      sf_error_str( NULL, sErr,sizeof(sErr) );
      printf("saveWAVfile() ERROR rc=%d!=%d: %s:",
         rc, len, sErr );
      return -1;
   }
   return 0;
}

#include "signal.h"

static int ctrlcHits = 0;
void
crtlc_handler( int sig ) /* SIGINT handler */
{
   printf("\n\nCaught SIGINT\n");
   ctrlcHits++;

   if ( ctrlcHits > 5 ) exit(-99);
   return;
}


#include "shmif.h"

int main( int argc, char* args[] )
{
   char *wavFile = "audio.wav";

   if ( argc >= 2 ) {
      if ( strcasecmp(args[1], "-h") == 0 ) {
         usage();
         return 1;
      }
   }

   if (signal(SIGINT, crtlc_handler) == SIG_ERR)
      printf("ERROR installing signal 'INT' handling");

   int sampleRate = 0;
   int audioChannels = 0;
   int bitsPerSample = 0;
   int frameFormat;

   if ( argc >=2 )
      wavFile = args[1];

   printf("%s\n", VERSION );
   printf("  wavfile = %s\n", wavFile );

   int rc = -1;
   void *abuf = NULL;
   int alen = 0;
   SHMIFmap msMap;

   // --- get audio vi SHM map
   memset( &msMap,0,sizeof(msMap) );     // initialize map data structure
   msMap.semName = "audioOPAL.shm";
   printf("shm sem=%s\n", msMap.semName );

   rc = shmifOpen( &msMap, SHMT_CONSUMER, 0 );
   if ( rc < 0 ) return rc;

   // read one frame to get audio properties
   rc = shmifRead( &msMap, &abuf, &alen );
   if ( rc < 0 ) goto Ende;

      // set frame properties AUDIO
      frameFormat   = msMap.format;
      sampleRate    = msMap.samplerate;
      audioChannels = msMap.channels;
      bitsPerSample = msMap.bitspersample;
      printf("shm prop AUDIO: samplerate=%d bps=%d channels=%d\n", sampleRate,bitsPerSample,audioChannels );

      if ( sampleRate <= 0 || audioChannels <= 0 ) {
         printf("missing AUDIO parameter: samplerate= %d channels=%d\n",
               sampleRate,audioChannels );
      }

   // --- open wav file
   if ( wavFile != NULL ) {
      rc = openWAVfile( wavFile, sampleRate, audioChannels );
      if ( rc < 0 ) return rc;
   }

   while ( 1 ) {

      if ( ctrlcHits ) break;

      // get next frame
      rc = shmifRead( &msMap, &abuf, &alen );
      //printf(".");

      // we have a new frame
      rc = writeWAVfile( (unsigned char*)abuf, alen );
      if ( rc < 0 ) break;

   }   // while

Ende:
    sleep(1);
    shmifClose( &msMap );

    closeWAVfile();
    return 0;
}


