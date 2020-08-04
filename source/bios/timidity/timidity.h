/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

	 This program is free software; you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
	 (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#define FORMAT_8         0x0008
#define FORMAT_16        0x0016
#define FORMAT_SIGNED    0x8000
#define FORMAT_BIGENDIAN 0x0100

#define AUDIO_S8        FORMAT_8|FORMAT_SIGNED
#define AUDIO_U8        FORMAT_8
#define AUDIO_S16LSB    FORMAT_16|FORMAT_SIGNED
#define AUDIO_S16MSB    FORMAT_16|FORMAT_SIGNED|FORMAT_BIGENDIAN
#define AUDIO_U16LSB    FORMAT_16
#define AUDIO_U16MSB    FORMAT_16|FORMAT_BIGENDIAN

typedef struct _MidiSong MidiSong;

extern int Timidity_Init(int rate, int format, int channels, int samples);
extern void Timidity_SetVolume(int volume);
extern int Timidity_PlaySome(void *stream, int samples);
extern MidiSong *Timidity_LoadSong(void *midifile);
extern void Timidity_Start(MidiSong *song);
extern int Timidity_Active(void);
extern void Timidity_Stop(void);
extern void Timidity_FreeSong(MidiSong *song);
extern void Timidity_Close(void);
