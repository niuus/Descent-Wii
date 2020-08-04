/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "texmap.h"		// For Lighting_on
#include "songs.h"
#include "screens.h"
#include "game.h"
#include "player.h"
#include "cfile.h"
#include "segpoint.h"
#include "ai.h"
#include "piggy.h"
#include "wall.h"
#include "byteswap.h"
#include "mem.h"
#include "sounds.h"
#include "mono.h"
#include "error.h"
#include "kconfig.h"
#include "newdemo.h"

#include <ogcsys.h>
#include <asndlib.h>
#include <mp3player.h>
#include "timidity/timidity.h"

#define _DIGI_SAMPLE_FLAGS (_VOLUME | _PANNING )
#define _DIGI_MAX_VOLUME			MAX_VOLUME
#define MAX_SOUND_CHANNELS			24
#define MAX_SND_VOICES				16

#define MUSIC

static ubyte use_sounds = 1;
int digi_lomem = 0;
int digi_initialized = 0;
ubyte digi_paused = 0;
ubyte digi_cd_started = 0;			// have we started a disc that is not the Descent cd?
int midi_volume = 255;
int digi_volume = _DIGI_MAX_VOLUME;
int digi_midi_song_playing = 0;
int digi_max_channels = MAX_SOUND_CHANNELS;
ushort num_sounds;

#define AUDIO_BUFFER_SAMPLES 4096
static short audio_out[2][2*AUDIO_BUFFER_SAMPLES] ATTRIBUTE_ALIGN(32);
static int which = 0;
static int midi_state;
static int digi_midi_looping;
MidiSong *Song;
// endlevel has its own MidiSong which remains in memory at all times to avoid loading lag
#define ENDLEVEL_FILE "endlevel.hmp"
MidiSong *EndLevelSong;
FILE *mp3_file;

// we have to run music in a separate thread because callbacks can trash
// the fpu registers, and it interferes with the bitblt routine.
lwp_t midi_thread = LWP_THREAD_NULL;
sem_t midi_sema = LWP_SEM_NULL;

extern ubyte Config_master_volume;

// sound object stuff -- used for fans and the boss only??
#define SOF_USED			1 		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving object. If object dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to segment, pos
#define SOF_PLAY_FOREVER	16		// Play forever (or until level is stopped), otherwise plays once

#define SOUND_PLAYING			1
#define SOUND_OBJECT_PLAYING	2

typedef struct song_resource {
	short		midi_id;
	ubyte		lead_inst;
	ubyte		buffer_ahead;
	ushort		tempo;
	ushort		pitch_shift;
	ubyte		sound_voices;
	ubyte		max_notes;
	ushort		norm_voices;
} song_resource;

static struct {
	s32 voice;					// the ASND voice
	s32 active;					// if this voice is in use
	u32 pan;					// left/right pan, 255 = left, 0 = right
	s32 volume;					// volume for this clip before panning and master adjust
	int looping;				// belongs to a SoundObject, don't reuse when StatusVoice==SND_UNUSED
} Voices[MAX_SND_VOICES];

typedef struct sound_object {
	short			signature;		// A unique signature to this sound
	ubyte			flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	fix				max_volume;		// Max volume that this sound is playing at
	fix				max_distance;	// The max distance that this sound can be heard at...
	int				volume;			// Volume that this sound is playing at
	int				pan;			// Pan value that this sound is playing at
	int				soundnum;		// The translated soundnum being played by the Voice
	int				voicenum;		// The Voice index being used
	union {
		struct {
			short			segnum;				// Used if SOF_LINK_TO_POS field is used
			short			sidenum;
			vms_vector		position;
		} pos;
		struct {
			short			objnum;				// Used if SOF_LINK_TO_OBJ field is used
			short			objsignature;
		} obj;
	} link_type;
} sound_object;

#define MAX_SOUND_OBJECTS 5
sound_object SoundObjects[MAX_SOUND_OBJECTS];
short next_signature=0;

#define SOUND_RATE_11k		11025

void PauseMusic()
{
	ASND_PauseVoice(0, 1);
	midi_state = 2; // paused
}

static void EndMidi()
{
	midi_state = 0;
	MP3Player_Stop();
	ASND_PauseVoice(0, 1);

	if (midi_thread != LWP_THREAD_NULL) {
		void *ret;
		LWP_SemPost(midi_sema);
		LWP_JoinThread(midi_thread, &ret);
		midi_thread = LWP_THREAD_NULL;
	}

	if (midi_sema != LWP_SEM_NULL) {
		LWP_SemDestroy(midi_sema);
		midi_sema = LWP_SEM_NULL;
	}
}

void EndAllSound()
{
	int i;
	for (i=0; i < MAX_SND_VOICES; i++) {
		if (Voices[i].active)
			ASND_StopVoice(Voices[i].voice);
		Voices[i].active = 0;
	}
	EndMidi();
}

int GetNewVoice()
{
	int i;
	for (i=0; i < MAX_SND_VOICES; i++) {
		if (!Voices[i].active || (!Voices[i].looping && ASND_StatusVoice(Voices[i].voice)==SND_UNUSED)) {
			Voices[i].looping = 0;
			Voices[i].pan = 0;
			Voices[i].volume = MAX_VOLUME;
			return i;
		}
	}
	return -1;
}

void TranslateVolumes(int index, s32 *vol_out)
{
	int pan;
	if (index<0 || index >= MAX_SND_VOICES)
		return;

	fix new_vol = (Voices[index].volume*(Config_master_volume<<5))>>8;
	pan = (Voices[index].pan+256)>>1;
	vol_out[1] = MAX(0, MIN(MAX_VOLUME, (new_vol*pan)>>8));
	vol_out[0] = MAX(0, MIN(MAX_VOLUME, (new_vol*(256-pan))>>8));
}

void ChangeSoundVolume(int index, int volume)
{
	s32 new_vol[2];
	if (index<0 || index >= MAX_SND_VOICES)
		return;

	Voices[index].volume = volume;
	if (Voices[index].active && ASND_StatusVoice(Voices[index].voice)!=SND_UNUSED) {
		TranslateVolumes(index, new_vol);
		ASND_ChangeVolumeVoice(Voices[index].voice, new_vol[0], new_vol[1]);
	}
}

void ChangeMidiVolume()
{
	int new_vol = (midi_volume*(Config_master_volume<<5))>>8;
	new_vol = MAX(0, MIN(255, new_vol));
	MP3Player_Volume(new_vol);
}

void SetMasterVolume()
{
	int i;
	// apply to all current voices
	for (i=0; i < MAX_SND_VOICES; i++)
		ChangeSoundVolume(i, Voices[i].volume);
	ChangeMidiVolume();
}

void EndSound(int index)
{
	if (index<0 || index >= MAX_SND_VOICES) return;

	ASND_StopVoice(Voices[index].voice);
	Voices[index].active = 0;
}

static int IsThisSoundFXFinished(int index)
{
	if (index<0 || index >= MAX_SND_VOICES)
		return 1;

	if (!Voices[index].active || ASND_StatusVoice(Voices[index].voice)==SND_UNUSED) {
		Voices[index].active = 0;
		return 1;
	}
	return 0;
}

static void ChangeSoundStereoPosition(int index, int angle)
{
	if (index<0 || index >= MAX_SND_VOICES)
		return;

	Voices[index].pan = angle;
	ChangeSoundVolume(index, Voices[index].volume);
}

static void BeginSound(int index, int sndnum, int loop)
{
	s32 vols[2];
	if (index<0 || index >= MAX_SND_VOICES)
		return;

	s32 voice = Voices[index].voice = ASND_GetFirstUnusedVoice();
	void *data = GameSounds[sndnum].data;
	s32 length = GameSounds[sndnum].length;
	Voices[index].looping = loop;

	TranslateVolumes(index, vols);
	if (!loop)
		ASND_SetVoice(voice, VOICE_MONO_8BIT_U, SOUND_RATE_11k, 0, data, length, vols[0], vols[1], NULL);
	else
		ASND_SetInfiniteVoice(voice, VOICE_MONO_8BIT_U, SOUND_RATE_11k, 0, data, length, vols[0], vols[1]);

	Voices[index].active = 1;
}

static void* playmidi_func(void* arg)
{
	int timid_ret = -1;

	while (midi_state) {
		do {
			LWP_SemWait(midi_sema);
		} while (midi_state==2);

		timid_ret = Timidity_PlaySome(audio_out[which], AUDIO_BUFFER_SAMPLES);
		if (timid_ret==14/*RC_TUNE_END*/ && digi_midi_looping) {
			Timidity_Start(Song);
			LWP_SemPost(midi_sema);
			continue;
		}
		else if (timid_ret)
			break;

		// time to do something
		if (midi_state==1) {
			ASND_AddVoice(0, audio_out[which], AUDIO_BUFFER_SAMPLES*2*sizeof(short));
			which ^= 1;
		}
	}

	return NULL;
}

static void voice_cb(s32 voice)
{
	if (midi_thread != LWP_THREAD_NULL)
		LWP_SemPost(midi_sema);
}

static s32 mp3_cb(void *_file, void *buf, s32 size)
{
	FILE *fp = (FILE*)_file;

	s32 ret = (s32)fread(buf, 1, size, fp);
	if (feof(fp) && digi_midi_looping) {
		rewind(fp);
		if (ret<=0)
			ret = (s32)fread(buf, 1, size, fp);
	}

	return ret;
}

void digi_reset_digi_sounds()
{
	if ( !digi_initialized ) return;

	EndAllSound();
}

int digi_xlat_sound(int soundno)
{
	if (soundno<0 || soundno>=MAX_SOUNDS)
		return -1;

	if (digi_lomem) { // why would anyone on the wii use this command line arg...
		soundno = AltSounds[soundno];
		if ( soundno == 255 )
			return -1;
	}

	soundno = Sounds[soundno];
	if (soundno == 255)
		return -1;

	return soundno;
}

void digi_play_sample_3d( int sndnum, int angle, int volume, int no_dups )
{
	int vol;
	int i, j, demo_angle;

	if (Newdemo_state == ND_STATE_RECORDING) {
		demo_angle = fixmuldiv(angle, F1_0, 255);
		if (no_dups)
			newdemo_record_sound_3d_once(sndnum, demo_angle, volume);
		else
			newdemo_record_sound_3d(sndnum, demo_angle, volume);
	}
	if (!digi_initialized) return;
	if (digi_paused) {
		digi_resume_all();
		if (digi_paused)
			return;
	}

	i = digi_xlat_sound(sndnum);
	if (i == -1) return;

	vol = fixmuldiv(volume, digi_volume, F1_0);
	j = GetNewVoice();
	ChangeSoundVolume(j, vol );
	ChangeSoundStereoPosition(j, angle);
	BeginSound(j, i, 0);
}

void digi_play_sample( int sndnum, fix max_volume )
{
	int i, j, vol;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_sound(sndnum);

	if (!digi_initialized) return;
	if (digi_paused) {
		digi_resume_all();
		if (digi_paused)
			return;
	}

	i = digi_xlat_sound(sndnum);
	if (i == -1)
		return;

	vol = fixmuldiv(max_volume, digi_volume, F1_0);
	j = GetNewVoice();
	ChangeSoundVolume(j, vol);
	ChangeSoundStereoPosition(j, 0);
	BeginSound(j, i, 0);
}

void digi_play_sample_once( int sndnum, fix max_volume )
{
	digi_play_sample( sndnum, F1_0 );
}

void digi_set_digi_volume(int dvolume)
{
	if ( !digi_initialized ) return;

//	dvolume = fixmuldiv( dvolume, _DIGI_MAX_VOLUME, 0x7fff);
	if ( dvolume > _DIGI_MAX_VOLUME )
		digi_volume = _DIGI_MAX_VOLUME;
	else if ( dvolume < 0 )
		digi_volume = 0;
	else
		digi_volume = dvolume;

	digi_sync_sounds();
}

static void digi_stop_current_song()
{
#ifdef MUSIC
	//stop_redbook();
	EndMidi();
	digi_midi_song_playing = 0;
#endif
}

static void ResumeMusic()
{
#ifdef MUSIC
	digi_midi_song_playing = 1;

	if (mp3_file) {
		MP3Player_PlayFile(mp3_file, mp3_cb, NULL);
	}
	else if (Timidity_Active()) {
		if (midi_state==2) {
			midi_state=1;
			LWP_SemPost(midi_sema);
			return;
		}
		which = 1;
		if (Timidity_PlaySome(audio_out[0], AUDIO_BUFFER_SAMPLES)==0) {
			int mix = (midi_volume*(Config_master_volume<<5))>>8;
			mix = MAX(0, MIN(255, mix));
			midi_state = 1;
			digi_midi_song_playing = 1;
			// argh libogc threads
			LWP_SemInit(&midi_sema, 0, 1);
			LWP_CreateThread(&midi_thread, playmidi_func, NULL, NULL, 0, LWP_PRIO_HIGHEST);
			ASND_SetVoice(0, VOICE_STEREO_16BIT, 44100, 0, audio_out[0], AUDIO_BUFFER_SAMPLES*2*sizeof(short), mix, mix, voice_cb);
		}
	} else
		digi_midi_song_playing = 0;

#endif
}

static void RestartSong()
{
#ifdef MUSIC
	digi_stop_current_song();

	if (mp3_file==NULL && Song==NULL)
		return;
	else if (Song)
		Timidity_Start(Song);
	else if (mp3_file)
		rewind(mp3_file);

	ResumeMusic();
#endif
}

void digi_play_midi_song(const char * filename, const char * melodic_bank, const char * drum_bank, int loop )
{
#ifdef MUSIC
	size_t len;
	digi_stop_current_song();

	if ( filename == NULL )	return;

	if (mp3_file) {
		fclose(mp3_file);
		mp3_file = NULL;
	}

	if (Song) {
		if (Song != EndLevelSong)
			Timidity_FreeSong(Song);
		Song = NULL;
	}

	len = strlen(filename);
	if (len>4 && strcasecmp(filename+len-4, ".mp3")==0) {
		mp3_file = fopen(filename, "rb");
	}
	else if (!strcasecmp(filename, ENDLEVEL_FILE) && EndLevelSong)
	{
		Song = EndLevelSong;
	}
	else
	{
		CFILE *cfin = cfopen(filename, "rb");
		if (cfin==NULL) return;

		Song = Timidity_LoadSong(cfin);
		cfclose(cfin);
	}

	digi_midi_looping = loop;

	RestartSong();
#endif
}

void digi_set_midi_volume(int n)
{
	int old_volume = midi_volume;

	if (!digi_initialized) return;

	if (n < 0)
		midi_volume = 0;
	else if (n > 255)
		midi_volume = 255;
	else
		midi_volume = n;

	//redbook_set_volume(midi_volume);
	ChangeMidiVolume();

	if (!old_volume && midi_volume)
		RestartSong();
	else if (old_volume && !midi_volume)
		digi_stop_current_song();
}

void digi_set_volume(int dvolume, int mvolume)
{
	if (!digi_initialized) return;
	digi_set_digi_volume(dvolume);
	digi_set_midi_volume(mvolume);
}

void digi_set_master_volume( int volume )
{
	if (!digi_initialized) return;

	Config_master_volume = volume;
	if ( Config_master_volume > 8 )
		Config_master_volume = 8;
	else if ( Config_master_volume < 0 )
		Config_master_volume = 0;

	SetMasterVolume();
}

void digi_reset()
{
}

void digi_resume_all()
{
	if (!digi_initialized) return;

	if (digi_paused) {
#ifdef MUSIC
		ResumeMusic();
		//redbook_pause(0);
#endif

		digi_paused = 0;
	}
}

void digi_pause_all()
{
	int i;

	if (!digi_initialized) return;

	if (!digi_paused) {
		for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
			if ((SoundObjects[i].flags & SOF_USED )	&& (SoundObjects[i].flags & SOF_PLAYING) && (SoundObjects[i].flags && SOF_PLAY_FOREVER) ) {
				EndSound(SoundObjects[i].voicenum);
				SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
			}
		}

#ifdef MUSIC
		PauseMusic();
		//redbook_pause(1);
#endif

		digi_paused = 1;
	}
}

void digi_stop_all()
{
	int i;

	if (!digi_initialized)	return;

	EndAllSound();

// reset the sound objects to be unused.  We stopped the sounds above.

	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		SoundObjects[i].flags = 0;
}

void digi_set_max_channels(int n)
{
	digi_max_channels	= n;

	if ( digi_max_channels < 1 )
		digi_max_channels = 1;
	if ( digi_max_channels > MAX_SOUND_CHANNELS )
		digi_max_channels = MAX_SOUND_CHANNELS;

	if ( !digi_initialized ) return;

	//if (digi_midi_song_playing)
	//	digi_stop_current_song();

	if (midi_volume <= 0)
		;//ChangeSystemVoices(0, digi_max_channels/2, digi_max_channels);
	else
		;//ChangeSystemVoices(MAX_MUSIC_CHANNELS, 4 + (digi_max_channels/2), digi_max_channels);

	//if (digi_last_midi_song != 0)
	//	digi_play_midi_song(digi_last_midi_song, digi_last_midi_song_loop);

	//digi_reset_digi_sounds();
}

void digi_start_sound_object(int i)
{
	int vol;

	if (!digi_initialized) return;

	vol = fixmuldiv(SoundObjects[i].volume, digi_volume, F1_0);
	SoundObjects[i].voicenum = GetNewVoice();
	ChangeSoundVolume(SoundObjects[i].voicenum, vol);
	ChangeSoundStereoPosition(SoundObjects[i].voicenum, SoundObjects[i].pan);
	BeginSound(SoundObjects[i].voicenum, SoundObjects[i].soundnum, 1);
	SoundObjects[i].flags |= SOF_PLAYING;
}

void digi_get_sound_loc( vms_matrix * listener, vms_vector * listener_pos, int listener_seg, vms_vector * sound_pos, int sound_seg, fix max_volume, int *volume, int *pan, fix max_distance )
{
	vms_vector	vector_to_sound;
	fix angle_from_ear, cosang,sinang;
	fix distance;
	fix path_distance;

	*volume = 0;
	*pan = 0;

	if (!digi_initialized) return;

	max_distance = (max_distance*5)/4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	distance = vm_vec_normalized_dir_quick( &vector_to_sound, sound_pos, listener_pos );

	if (distance < max_distance )	{
		int num_search_segs = f2i(max_distance/20);
		if ( num_search_segs < 1 ) num_search_segs = 1;

		path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG );
		//path_distance = distance;
		if ( path_distance > -1 )	{
			*volume = max_volume - (path_distance/f2i(max_distance));
			//mprintf( (0, "Sound path distance %.2f, volume is %d / %d\n", f2fl(distance), *volume, max_volume ));
			if (*volume > 0 )	{
				angle_from_ear = vm_vec_delta_ang_norm(&listener->rvec,&vector_to_sound,&listener->uvec);
				fix_sincos(angle_from_ear,&sinang,&cosang);
				//mprintf( (0, "volume is %.2f\n", f2fl(*volume) ));
				if (Config_channels_reversed) cosang *= -1;
				*pan = fixmuldiv(cosang, 255, F1_0);
			} else {
				*volume = 0;
			}
		}
	}
}

int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume )
{																									// 10 segs away
	return digi_link_sound_to_object2( soundnum, objnum, forever, max_volume, 256*F1_0  );
}

int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume )
{
	return digi_link_sound_to_pos2( soundnum, segnum, sidenum, pos, forever, max_volume, F1_0 * 256 );
}

int digi_link_sound_to_object2( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance )
{
	int i,volume,pan;
	object * objp;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if (!digi_initialized) return -1;
	if (max_volume < 0) return -1;
	if (soundnum < 0) return -1;
	if ((objnum<0)||(objnum>Highest_object_index))
		return -1;

	if ( !forever )	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc(&Viewer->orient, &Viewer->pos, Viewer->segnum, &Objects[objnum].pos, Objects[objnum].segnum, max_volume,&volume, &pan, max_distance);
		digi_play_sample_3d(org_soundnum, pan, volume, 0 );
		return -1;
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;

	if (i==MAX_SOUND_OBJECTS) {
		mprintf((1, "Too many sound objects!\n" ));
		return -1;
	}

	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_OBJ;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].link_type.obj.objnum = objnum;
	SoundObjects[i].link_type.obj.objsignature = Objects[objnum].signature;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	SoundObjects[i].soundnum = soundnum;

	objp = &Objects[SoundObjects[i].link_type.obj.objnum];
	digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
                       &objp->pos, objp->segnum, SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

	digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_pos2( int org_soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance )
{
	int i, volume, pan;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if (!digi_initialized) return -1;
	if ( max_volume < 0 ) return -1;

	if (soundnum < 0 ) return -1;
	if ((segnum<0)||(segnum>Highest_segment_index))
		return -1;

	if ( !forever )	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;

	if (i==MAX_SOUND_OBJECTS) {
		mprintf((1, "Too many sound objects!\n" ));
		return -1;
	}

	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_POS;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].link_type.pos.segnum = segnum;
	SoundObjects[i].link_type.pos.sidenum = sidenum;
	SoundObjects[i].link_type.pos.position = *pos;
	SoundObjects[i].soundnum = soundnum;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
                       &SoundObjects[i].link_type.pos.position, SoundObjects[i].link_type.pos.segnum, SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

	digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

void digi_sync_sounds()
{
	int i;
	int oldvolume, oldpan;

	if (!digi_initialized) return;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED )	{
			oldvolume = SoundObjects[i].volume;
			oldpan = SoundObjects[i].pan;

			if ( !(SoundObjects[i].flags & SOF_PLAY_FOREVER) )	{
			 	// Check if its done.
				if (SoundObjects[i].flags & SOF_PLAYING) {
					if ( IsThisSoundFXFinished(SoundObjects[i].voicenum) ) {
						SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this object
						continue;		// Go on to next sound...
					}
				}
			}

			if ( SoundObjects[i].flags & SOF_LINK_TO_POS )	{
				digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
                                &SoundObjects[i].link_type.pos.position, SoundObjects[i].link_type.pos.segnum, SoundObjects[i].max_volume,
                                &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

			} else if ( SoundObjects[i].flags & SOF_LINK_TO_OBJ )	{
				object * objp;

				objp = &Objects[SoundObjects[i].link_type.obj.objnum];

				if ((objp->type==OBJ_NONE) || (objp->signature!=SoundObjects[i].link_type.obj.objsignature))	{
					// The object that this is linked to is dead, so just end this sound if it is looping.
					if ( (SoundObjects[i].flags & SOF_PLAYING)  && (SoundObjects[i].flags & SOF_PLAY_FOREVER))	{
						EndSound(SoundObjects[i].voicenum);
					}
					SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
					continue;		// Go on to next sound...
				} else {
					digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
	                                &objp->pos, objp->segnum, SoundObjects[i].max_volume,
                                   &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );
				}
			}

			if (oldvolume != SoundObjects[i].volume) 	{
				if ( SoundObjects[i].volume < 1 )	{
					// Sound is too far away, so stop it from playing.
					if ((SoundObjects[i].flags & SOF_PLAYING)&&(SoundObjects[i].flags & SOF_PLAY_FOREVER))	{
						EndSound(SoundObjects[i].voicenum);
						SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
					}
				} else {
					if (!(SoundObjects[i].flags & SOF_PLAYING))	{
						digi_start_sound_object(i);
					} else {
						int vol = fixmuldiv(SoundObjects[i].volume, digi_volume,F1_0);
						ChangeSoundVolume(SoundObjects[i].voicenum, vol);
					}
				}
			}

			if (oldpan != SoundObjects[i].pan) 	{
				if (SoundObjects[i].flags & SOF_PLAYING) {
					ChangeSoundStereoPosition( SoundObjects[i].voicenum, SoundObjects[i].pan );
				}
			}
		}
	}
}

void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum )
{
	int i,killed;

	soundnum = digi_xlat_sound(soundnum);

	if (!digi_initialized) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_POS) )	{
			if ((SoundObjects[i].link_type.pos.segnum == segnum) && (SoundObjects[i].soundnum==soundnum ) && (SoundObjects[i].link_type.pos.sidenum==sidenum) )	{
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					EndSound(SoundObjects[i].voicenum);
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if ( killed > 1 )	{
		mprintf( (1, "ERROR: More than 1 sounds were deleted from seg %d\n", segnum ));
	}
}

void digi_kill_sound_linked_to_object( int objnum )
{
	int i,killed;

	if (!digi_initialized) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_OBJ ) )	{
			if (SoundObjects[i].link_type.obj.objnum == objnum)	{
				if ( SoundObjects[i].flags & SOF_PLAYING )
					EndSound(SoundObjects[i].voicenum);

				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}

	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if ( killed > 1 )	{
		mprintf( (1, "ERROR: More than 1 sounds were deleted from object %d\n", objnum ));
	}
}


void digi_close()
{
	if (!digi_initialized)
		return;
	digi_stop_current_song();

	EndAllSound();
#ifdef MUSIC
	if (mp3_file) {
		fclose(mp3_file);
		mp3_file=NULL;
	}
	if (Song) {
		Timidity_FreeSong(Song);
		if (Song == EndLevelSong)
			EndLevelSong = NULL;
		Song = NULL;
	}

	if (EndLevelSong) {
		Timidity_FreeSong(EndLevelSong);
		EndLevelSong = NULL;
	}

	Timidity_Close();
#endif

	ASND_End();

	digi_initialized = 0;
}

void digi_init_sounds()
{
	if (!digi_initialized) return;

	EndAllSound();			// kill off all current sound effect and music
}

void digi_load_sounds()
{
	// TODO: Copy all sounds to aligned memory here?
}

int digi_init()
{
	if (!use_sounds)
		return -1;

	ASND_Init();
	ASND_Pause(0);
	memset(Voices, 0, sizeof(Voices));
#ifdef MUSIC
	EndLevelSong = NULL;
	if (Timidity_Init(44100, AUDIO_S16MSB, 2, AUDIO_BUFFER_SAMPLES) >= 0) {
		CFILE *cfin = cfopen(ENDLEVEL_FILE, "rb");
		if (cfin) {
			EndLevelSong = Timidity_LoadSong(cfin);
			if (EndLevelSong) {
				// load instruments
				Timidity_Start(EndLevelSong);
				Timidity_Stop();
			}
			cfclose(cfin);
		}
	}
	Song = NULL;
	MP3Player_Init();
#endif
	//init_redbook();

 	digi_initialized = 1;

// set volumes and then out of here..

	digi_set_master_volume(Config_master_volume);
	digi_set_volume(digi_volume, midi_volume);
	//digi_reset_digi_sounds();
	atexit(digi_close);

	return 0;
}

