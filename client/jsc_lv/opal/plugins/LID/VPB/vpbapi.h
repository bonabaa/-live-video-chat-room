/*---------------------------------------------------------------------------*\

    FILE....: VPBAPI.H
    TYPE....: C Header File
    AUTHOR..: Voicetronix
    DATE....: 26/11/97

    Header file for:
  
    "Voice Processing Board Applications Programmer Interface" functions.

\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\

         Voicetronix Voice Processing Board (VPB) Software

         Copyright (C) 1999-2001 Voicetronix www.voicetronix.com.au

         This library is free software; you can redistribute it and/or
         modify it under the terms of the GNU Lesser General Public
         License as published by the Free Software Foundation; either
         version 2.1 of the License, or (at your option) any later version.

         This library is distributed in the hope that it will be useful,
         but WITHOUT ANY WARRANTY; without even the implied warranty of
         MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
         Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
         License along with this library; if not, write to the Free Software
         Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
	 USA

\*---------------------------------------------------------------------------*/

#ifndef __VPBAPI__
#define __VPBAPI__

#ifdef _WIN32
#define	WINAPI	__stdcall
#else
#define	WINAPI
#endif

/*-----------------------------------------------------------------------*\

				   GENERAL

\*-----------------------------------------------------------------------*/

// Return codes

#define	VPB_OK		 0		
#define	VPB_NO_EVENTS	-1		
#define	VPB_TIME_OUT	-2		

// string length

#define	VPB_MAX_STR	256

// vpb_sethook() states

#define	VPB_ONHOOK	0
#define	VPB_OFFHOOK	1

// VPB card model flags

#define VPB_VPB4        0      // VPB4 4 port ISA CT card
#define VPB_VPB8L       1      // VPB8L 8 port logging card
#define VPB_V4PCI       2      // V4PCI 4 port PCI CT card
#define VPB_V4LOG       3      // V4LOG 4 port PCI logging card
#define VPB_V12PCI      4      // V12PCI 12 port PCI CT/PBX card
#define VPB_V6PCI       5      // V6PCI 6 port PCI CT/PBX card

// vpb_bridge() modes

#define VPB_BRIDGE_ON   1     // start bridging
#define VPB_BRIDGE_OFF  0     // stop bridging

// vpb_conf() modes

#define VPB_CONF_JOIN   1     // Join conference
#define VPB_CONF_LEAVE  0     // Leave conference

// vpb_set_station() modes

#define VPB_STATION_ON   1     // turn station port on
#define VPB_STATION_OFF  0     // turn station port off

// vpb_ring_station() modes

#define VPB_RING_STATION_ON   1     // start station port ringing
#define VPB_RING_STATION_OFF  0     // stop station port ringing

/*-------------------------------------------------------------------------*\

			     MISC FUNCTIONS

\*-------------------------------------------------------------------------*/


typedef struct {
	char	model[10];	// Model
	char	date[16];	// Manufacture Date
	char	rev[8];		// Revision
	char	sn[10];		// Serial Number
} VPB_CARD_INFO;


int WINAPI vpb_open(unsigned int board, unsigned int channel);
int WINAPI vpb_close(int handle);
void WINAPI vpb_sleep(long time_ms);
int WINAPI vpb_sethook_async(int handle, int hookstate);
int WINAPI vpb_sethook_sync(int handle, int hookstate);
int WINAPI vpb_get_model(char *s);
int WINAPI vpb_reset_play_fifo_alarm(int handle);
int WINAPI vpb_reset_record_fifo_alarm(int handle);
int WINAPI vpb_config(int num_cards, int *bases, char *firmware_file, int model);
int WINAPI vpb_start(int num_cards, int *bases, char *firmware_file, int model);
int WINAPI vpb_stop();
int WINAPI vpb_get_num_cards();
int WINAPI vpb_get_ports_per_card();
int WINAPI vpb_get_card_info(int board, VPB_CARD_INFO *detail);

void WINAPI vpb_set_codec_reg(int chdev,  unsigned short addr, unsigned short data);
int WINAPI vpb_bridge(int h1, int h2, int mode, int resource);
int WINAPI vpb_watchdog_enable(int h, int enable);
int WINAPI vpb_watchdog_reset(int h);
int WINAPI vpb_conf(int h1,int resource, int mode);
int WINAPI vpb_sbridge( int h1, int h2, int mode);

/*-------------------------------------------------------------------------*\

			     STATION PORT FUNCTIONS

\*-------------------------------------------------------------------------*/

int WINAPI vpb_set_station_sync(int handle, int state);
int WINAPI vpb_ring_station_sync(int handle, int state);
int WINAPI vpb_ring_station_async(int handle, int state);
int WINAPI vpb_user_ring_station_sync(int handle, int state);

/*-----------------------------------------------------------------------*\

			     EVENT HANDLING	

\*-----------------------------------------------------------------------*/

typedef struct {
	int	type;		// event type (see below)
	int	handle;		// channel that generated event
	int	data;		// optional data
	unsigned long data1;
} VPB_EVENT;

// unsolicited events (maskable)

#define VPB_RING		0
#define VPB_DIGIT		1
#define	VPB_TONEDETECT		2
#define	VPB_TIMEREXP		3
#define	VPB_VOXON		4
#define	VPB_VOXOFF		5
#define	VPB_PLAY_UNDERFLOW	6
#define	VPB_RECORD_OVERFLOW	7
#define VPB_DTMF		8
#define VPB_STATION_OFFHOOK	9
#define VPB_STATION_ONHOOK	10
#define VPB_RING_OFF		11
#define VPB_DROP		12
#define VPB_STATION_FLASH	13

// solicited events (not maskable)

#define	VPB_PLAYEND		100
#define	VPB_RECORDEND		101
#define	VPB_DIALEND		102
#define	VPB_TONE_DEBUG_END	103
#define	VPB_CALLEND		104

// Event mask values

#define VPB_MRING		(1<<VPB_RING)
#define VPB_MDIGIT     		(1<<VPB_DIGIT)
#define VPB_MDTMF     		(1<<VPB_DTMF)
#define	VPB_MTONEDETECT		(1<<VPB_TONEDETECT)
#define VPB_MTIMEREXP		(1<<VPB_TIMEREXP)
#define VPB_MVOXON		(1<<VPB_VOXON)
#define VPB_MVOXOFF		(1<<VPB_VOXOFF)
#define	VPB_MPLAY_UNDERFLOW	(1<<VPB_PLAY_UNDERFLOW)
#define	VPB_MRECORD_OVERFLOW	(1<<VPB_RECORD_OVERFLOW)
#define VPB_MSTATION_OFFHOOK	(1<<VPB_STATION_OFFHOOK)
#define VPB_MSTATION_ONHOOK	(1<<VPB_STATION_ONHOOK)
#define VPB_MRING_OFF		(1<<VPB_RING_OFF)
#define VPB_MDROP		(1<<VPB_DROP)
#define VPB_MSTATION_FLASH	(1<<VPB_STATION_FLASH)

int WINAPI vpb_enable_event(int handle, unsigned short mask);
int WINAPI vpb_disable_event(int handle, unsigned short mask);
int WINAPI vpb_get_event_mask(int handle);
int WINAPI vpb_set_event_mask(int handle, unsigned short mask);
int WINAPI vpb_get_event_async(VPB_EVENT *event);
int WINAPI vpb_get_event_async_bayonne(int board, VPB_EVENT *event);
int WINAPI vpb_get_event_sync(VPB_EVENT *event, unsigned int time_out);
int WINAPI vpb_get_event_ch_async(int handle, VPB_EVENT *e);
int WINAPI vpb_get_event_ch_sync(int handle, VPB_EVENT *e, 
				 unsigned int time_out);
int WINAPI vpb_put_event(VPB_EVENT *event);
void WINAPI vpb_translate_event(VPB_EVENT *e, char s[]);
int WINAPI vpb_set_event_callback(int handle, void (WINAPI *event_callback)(VPB_EVENT *e, void *context), void *context);

/*-----------------------------------------------------------------------*\

			      PLAY AND RECORD

\*-----------------------------------------------------------------------*/

// return code for vpb_play_buf_sync and vpb_record_buf_sync

#define	VPB_FINISH		1	

// compression modes

#define	VPB_LINEAR	0	// 128 kbit/s 16 bit linear
#define	VPB_ALAW	1	// 64 kbit/s A-law companded
#define	VPB_MULAW	2	// 64 kbit/s mu-law companded
#define	VPB_OKIADPCM	3	// 32 kbit/s OKI ADPCM
#define	VPB_OKIADPCM24	4	// 24 kbit/s OKI ADPCM

// structures for configuring record and play params

typedef struct {
	char *term_digits;	// string of digits to terminate collection
} VPB_PLAY;

typedef struct {
	char         *term_digits;  // string of digits to terminate collection
	unsigned int time_out;	    // record terminates after time_out ms
				    // if set to 0 (default) no time out
} VPB_RECORD;

// play functions

int WINAPI vpb_play_file_sync(int handle, char file_name[]);
int WINAPI vpb_play_file_async(int handle, char file_name[], int data);
int WINAPI vpb_play_voxfile_sync(int handle, char file_name[],unsigned short mode);
int WINAPI vpb_play_voxfile_async(int handle, char file_name[], unsigned short mode, int data);

int WINAPI vpb_play_buf_start(int handle, unsigned short mode);
int WINAPI vpb_play_buf_sync(int handle, char *buf, unsigned short length);
int WINAPI vpb_play_buf_finish(int handle);
int WINAPI vpb_play_terminate(int handle);
int WINAPI vpb_play_set(int handle, VPB_PLAY *vpb_play);
int WINAPI vpb_play_set_gain(int handle, float gain);
int WINAPI vpb_play_get_gain(int handle, float *gain);
int WINAPI vpb_play_set_hw_gain(int handle, float gain); 

// record functions

int WINAPI vpb_record_file_sync(int handle, char file_name[], unsigned short mode);
int WINAPI vpb_record_file_async(int handle, char file_name[], unsigned short mode);
int WINAPI vpb_record_voxfile_sync(int handle, char file_name[], unsigned short mode);
int WINAPI vpb_record_voxfile_async(int handle, char file_name[], unsigned short mode);
int WINAPI vpb_record_buf_start(int handle, unsigned short mode);
int WINAPI vpb_record_buf_sync(int handle, char *buf, unsigned short length);
int WINAPI vpb_record_buf_finish(int handle);
int WINAPI vpb_record_terminate(int handle);
int WINAPI vpb_record_set(int handle, VPB_RECORD *vpb_record);
int WINAPI vpb_record_set_gain(int handle, float gain);
int WINAPI vpb_record_get_gain(int handle, float *gain);
int WINAPI vpb_record_set_hw_gain(int handle, float gain); 

// record terminating reason outputed during a vpb_record_???_async() in 
// the data member of the VPBevents

#define VPB_RECORD_DIGIT       1    // terminated due to Digit String
#define VPB_RECORD_TIMEOUT     2    // terminated due to record timeout
#define VPB_RECORD_ENDOFDATA   3    // terminate due to end of data in buffer
#define VPB_RECORD_MAXDIGIT    4    // terminate due to Maximum digits

/*-----------------------------------------------------------------------*\

				ERROR HANDLING

\*-----------------------------------------------------------------------*/

// Run time error manager modes

#define	VPB_DEVELOPMENT	0	// API function error causes program abort
#define	VPB_ERROR_CODE	1	// API function error returns error code
#define	VPB_EXCEPTION	2	// API function error throw a exception

// Exception class

class VpbException {
public:
	int	code;				// error code
	char	s[VPB_MAX_STR];			// code translated into string
	char	api_function[VPB_MAX_STR];	// api func that threw exception
	
	VpbException(int c, char trans[], char api_function[]);
};

int WINAPI vpb_seterrormode(int mode);
char * WINAPI vpb_strerror(int code);
void WINAPI vpb_throw_exception(int c, char trans[], char api_function[]);

/*-----------------------------------------------------------------------*\

				DIALLING

\*-----------------------------------------------------------------------*/

// blind dialling functions

int WINAPI vpb_dial_sync(int handle, char *dialstr);
int WINAPI vpb_dial_async(int handle, char *dialstr);

// Call progress tone ids

#define	VPB_CALL_DISCONNECT	0	
#define	VPB_CALL_DIALTONE	1
#define	VPB_CALL_RINGBACK	2
#define	VPB_CALL_BUSY		3
#define	VPB_CALL_GRUNT		4

typedef struct {
	unsigned int	tone_id;	// tone detector tone id
	unsigned int	call_id;	// call progress tone id
	unsigned int	terminate;	// non zero to terminate list
} VPB_TONE_MAP;

#define	VPB_MAX_TONE_MAP	10	// maximum number of entries in tone map

// structure used to store call progress config information

typedef struct {
  unsigned int	dialtones;		// number of dialtones (eg internal line,
	                                // then outside line = 2 dialtones)
  unsigned int	dialtone_timeout;	// wait for dial tone timeout in ms
  unsigned int	ringback_timeout;	// wait for initial ringback in ms
  unsigned int	inter_ringback_timeout;	// if ringback stops for this time (ms),
	                                // call is considered connected
  unsigned int	answer_timeout;		// time to wait for answer after ringback
	                                // detected in ms
  VPB_TONE_MAP	tone_map[VPB_MAX_TONE_MAP];// maps tone_id to call progress tone
} VPB_CALL;

// getting and setting call progress configuration

int WINAPI vpb_get_call(int handle, VPB_CALL *vpb_call);
int WINAPI vpb_set_call(int handle, VPB_CALL *vpb_call);

// call progress return codes

#define	VPB_CALL_CONNECTED	0	 // call connected successfully
#define	VPB_CALL_NO_DIAL_TONE	1	 // dial tone time out
#define	VPB_CALL_NO_RING_BACK	2	 // ring back time out
#define	VPB_CALL_BUSY		3	 // busy tone 
#define	VPB_CALL_NO_ANSWER	4	 // no answer time out
#define	VPB_CALL_DISCONNECTED	5	 // no answer time out

// dialling with call progress

int WINAPI vpb_call_sync(int handle, char *dialstr);
int WINAPI vpb_call_async(int handle, char *dialstr);

/*-----------------------------------------------------------------------*\

			PROGRAMMABLE TONE GENERATOR

\*-----------------------------------------------------------------------*/

// Programmable tone generator structure ---------------------------------

typedef struct {
	unsigned short	freq1;	// frequency of first tone
	unsigned short	freq2;	// frequency of second tone
	unsigned short	freq3;	// frequency of third tone
	short		level1;	// first tone level in dB, 0dB maximum
	short		level2;	// second tone level in dB, 0dB maximum
	short		level3;	// third tone level in dB, 0dB maximum
	unsigned long	ton;	// on time ms 
	unsigned long	toff;	// off time ms 
} VPB_TONE;

int WINAPI vpb_settone(char ident, VPB_TONE *vpb_tone);
int WINAPI vpb_gettone(char ident, VPB_TONE *vpb_tone);
int WINAPI vpb_playtone_async(int handle, VPB_TONE *vpb_tone);
int WINAPI vpb_playtone_sync(int handle, VPB_TONE *vpb_tone);
int WINAPI vpb_tone_terminate(int handle);

/*-----------------------------------------------------------------------*\

			 PROGRAMMABLE TONE DETECTOR

\*-----------------------------------------------------------------------*/

// built in tone dectector IDs

#define	VPB_DIAL	  0   // dial tone detected (US/Aust/sudio308)
#define	VPB_RINGBACK	  1   // ringback detected  (US)
#define	VPB_BUSY	  2   // busy tone detected (US)
#define	VPB_GRUNT	  3   // grunt detected
#define	VPB_RINGBACK_308  4   // ringback detected (studio 308)
#define	VPB_BUSY_308	  5   // busy tone detected (studio 308)
#define VPB_FAX           6   // fax CNG tone/preamble

#define	VPB_MD	10	// maximum number of tone detectors per device
#define	VPB_MS	10	// maximum number of states per cadence state mach

// State transition table consists of one entry for each state transition.

#define	VPB_TIMER 	0
#define	VPB_RISING	1
#define	VPB_FALLING	2
#define	VPB_DELAY	3

typedef struct {
    unsigned short type;     // VPB_TIMER, VPB_RISING, or VPB_FALLING
    unsigned short tfire;    // timer mode only			
    unsigned short tmin;     // minimum tone on/off time (non timer) in ms
    unsigned short tmax;     // maximum tone on/off time (non timer) in ms
} VPB_STRAN;			// State TRANsition

typedef struct {
   unsigned short	nstates;     // number of cadence states	
   unsigned short	tone_id;     // unique ID number for this tone	
   unsigned short	ntones;	     // number of tones (1 or 2)	
   unsigned short	freq1;	     // freq of first tone (Hz)	
   unsigned short	bandwidth1;  // bandwidth of first tone (Hz)	
   unsigned short	freq2;	     // freq of first tone (Hz)	
   unsigned short	bandwidth2;  // bandwidth of second tone (Hz)
   short		minlevel1;   // min amp of 1st tone ref 0dBm0
   short		minlevel2;   // min amp of 2nd tone ref 0dbm0
   short		twist;	     // allowable difference in tone powers
				     // If (E1/E2 < twist) AND (E2/E1 < twist)
	                             // tone OK		
   short		snr;	     // min signal to noise ratio to accept 
	                             // tone			
   unsigned short	glitch;	     // short transitions of glitch ms ignored

   VPB_STRAN stran[VPB_MS];	     // cadence state transition table
} VPB_DETECT;

int WINAPI vpb_settonedet(int handle, VPB_DETECT *d);
int WINAPI vpb_gettonedet(int handle, int id, VPB_DETECT *d);
int WINAPI vpb_debug_tonedet(int handle, int id, char file_name[], int sec);
int WINAPI vpb_tonedet_make_default(VPB_DETECT *d);

/*-------------------------------------------------------------------------*\

				TIMER

\*-------------------------------------------------------------------------*/

int WINAPI vpb_timer_open(void **timer, int handle, int id, unsigned long period);
int WINAPI vpb_timer_close(void *timer);
int WINAPI vpb_timer_start(void *timer);
int WINAPI vpb_timer_stop(void *timer);
int WINAPI vpb_timer_restart(void *timer);
int WINAPI vpb_timer_get_unique_timer_id();
int WINAPI vpb_timer_change_period(void *timer, unsigned long newperiod);

/*-------------------------------------------------------------------------*\

				VOX

\*-------------------------------------------------------------------------*/

typedef struct {
   float		onlevel;  // switch on level in dB (0 dB maximum)
   float		offlevel; // switch off level in dB (0 dB maximum)
   unsigned short 	runon;	  // run on time in ms
} VPB_VOX;

int WINAPI vpb_setvox(int handle, VPB_VOX *vox);
int WINAPI vpb_getvox(int handle, VPB_VOX *vox);

/*-------------------------------------------------------------------------*\

				AGC

\*-------------------------------------------------------------------------*/

typedef struct {
    float setpoint;		// desired signal level			
    float attack;		// agc filter attack 		
    float decay;		// agc filter decay			
} VPB_AGC;

int WINAPI vpb_setagc(int handle, VPB_AGC *agc);
int WINAPI vpb_getagc(int handle, VPB_AGC *agc);

/*-------------------------------------------------------------------------*\

			ADPCM CONVERSION FUNCTIONS

\*-------------------------------------------------------------------------*/

int  WINAPI vpb_adpcm_open(void **adpcm);
void WINAPI vpb_adpcm_reset(void *adpcm);
void WINAPI vpb_adpcm_close(void *adpcm);
int  WINAPI vpb_adpcm_encode(void  *adpcm,
			    char  adpcmbuf[],   unsigned short *nadpcmbytes,
			    short linearbuf[],  unsigned short nlinear);
int  WINAPI vpb_adpcm_decode(void  *adpcm,
			    short linearbuf[],  unsigned short *nlinear,
			    char  adpcmbuf[] ,  unsigned short nadpcmbytes);

/*-------------------------------------------------------------------------*\

				WAVE FUNCTIONS

\*-------------------------------------------------------------------------*/

int WINAPI vpb_wave_open_write(void **ppv, char filename[], int mode);
int WINAPI vpb_wave_write(void *wv, char buf[], long n);
void WINAPI vpb_wave_close_write(void *wv);

int WINAPI vpb_wave_open_read(void **ppv, char filename[]);
int WINAPI vpb_wave_read(void *wv, char buf[], long n);
void WINAPI vpb_wave_close_read(void *wv);

void WINAPI vpb_wave_set_sample_rate(void *wv, unsigned short rate);
int WINAPI vpb_wave_seek(void *wv, long offset);
int WINAPI vpb_wave_get_mode(void *wv, unsigned short *mode);

/*-------------------------------------------------------------------------*\

				GET DIGIT FUNCTIONS

\*-------------------------------------------------------------------------*/

typedef struct {
  char		 *term_digits;	       // digits that may terminate collection
  unsigned short max_digits;	       // term. after this many digits collected
  unsigned long	 digit_time_out;       // max total time for digit collect (ms)
  unsigned long	 inter_digit_time_out; // max time between digits (ms)
} VPB_DIGITS;

// terminate reason passed in data field of VPB_EVENT when
// VPB_DIGIT event posted

#define	VPB_DIGIT_TERM			0
#define	VPB_DIGIT_MAX			1
#define	VPB_DIGIT_TIME_OUT		2
#define	VPB_DIGIT_INTER_DIGIT_TIME_OUT	3
#define VPB_DIGIT_BUFFER_FULL		4

int WINAPI vpb_flush_digits(int handle);
int WINAPI vpb_get_digits_async(int handle, VPB_DIGITS *digits, char *digbuf);
int WINAPI vpb_get_digits_sync(int handle, VPB_DIGITS *digits, char *digbuf);

/*-------------------------------------------------------------------------*\

				PIP FUNCTIONS

\*-------------------------------------------------------------------------*/

typedef struct {
	unsigned int	width;	// width of pip pulse in ms
	unsigned int	period;	// period between pip pulse in ms
} VPB_PIP;

int WINAPI vpb_set_pip(VPB_PIP *vpb_pip);
int WINAPI vpb_get_pip(VPB_PIP *vpb_pip);

int WINAPI vpb_pip_on(int handle);
int WINAPI vpb_pip_off(int handle);

/*-------------------------------------------------------------------------*\

				CID FUNCTIONS

\*-------------------------------------------------------------------------*/

int WINAPI vpb_cid_demodulate(char* number_str, int* nChars, short* data, int nData);
void WINAPI vpb_cid_debug_on(char debug_file_name[]);
int WINAPI vpb_cid_decode(char *number_str, short *in, int n);

/*-------------------------------------------------------------------------*\
 
 			CID GENERATION DEFINEs
 
\*-------------------------------------------------------------------------*/

#define		VPB_COUNTRY_AU		61
#define		VPB_COUNTRY_USA		1
#define		VPB_COUNTRY_UK		44
#define		VPB_CID_MAX_BUF		20400
#define		VPB_CID_MAX_DLP		256

//	Field type identifiers for CID functions
#define		VPB_CID_EMPTY			0
#define 	VPB_CID_DATE_TIME		1
#define		VPB_CID_CALLING_LINE_DN		2
#define 	VPB_CID_CALLED_DN		3
#define  	VPB_CID_RFA_CLDN		4
#define  	VPB_CID_CALLER_NAME		7
#define		VPB_CID_TEXT_PARAMETER		7000
#define  	VPB_CID_RFA_CN			8
#define		VPB_CID_CALL_TYPE		0x11
#define		VPB_CID_NMSS			0x13

//	call type's for CID_CALL_TYPE int
//	Voice Call
#define		VPB_CID_CT_VOICE		0x01 	
//	Ring Back When Free
#define		VPB_CID_CT_RBWF			0x02
//	Message Waiting
#define		VPB_CID_CT_MW			0x81

/*-----------------------------------------------------------------------\
 									
 			CID GENERATION STRUCTURE				
 									
\-----------------------------------------------------------------------*/

typedef struct {
	int	call_type ;		// 0-255 bin byte
	char	date_time[9];		// 8x ascii numerics
	char	cldn[19];		// 18x 0-9 +symbols & space
	char	cdn[19];		// 18x ...
	char	ra_cldn[2];		// 'O' or 'P'
	char	cn[21];			// 20x ascii text
	char	ra_cn[2];		// 'O' or 'P'
	int	nmss ;			// 0-255 bin byte
}VPB_CID;

/*-----------------------------------------------------------------------\
 									
		CID GENERATION FUNCTION PROTOTYPES				
 									
\-----------------------------------------------------------------------*/

int WINAPI vpb_cid_set(VPB_CID *, int, void *);

int WINAPI vpb_cid_compose_dlp(VPB_CID *, char *);
int WINAPI vpb_cid_compose_wav(char *, int , short *, int *);

int WINAPI vpb_cid_collect(int h, VPB_CID *);

/*-------------------------------------------------------------------------*\

				ECHO CANCELLER

\*-------------------------------------------------------------------------*/

int WINAPI vpb_echo_canc_force_adapt_on();
int WINAPI vpb_echo_canc_force_adapt_off();
int WINAPI vpb_echo_canc_enable();
int WINAPI vpb_echo_canc_disable();
int WINAPI vpb_echo_canc_get_sup_thresh(short *thresh);
int WINAPI vpb_echo_canc_set_sup_thresh(short *thresh);

#endif	// #ifndef __VPBAPI__	
