/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   globals.h
 * Author: NEON
 *
 * Created on August 11, 2018, 3:54 PM
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <queue>
#include <map>
#include <ctime>
#include "AudioLibrary.h"
#include "Playlist.h"

using namespace std;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

extern string libraryDirectory;
extern unsigned int songNumber;
extern Playlist primaryPlaylist;

extern bool mPi3ShuttingDown;
extern bool audioThreadDone;

extern queue<uint8_t> audioPlaybackMessages;
#define COMMAND_NEXT_TRACK        0x3E
#define COMMAND_PREVIOUS_TRACK    0x3F
#define COMMAND_RESTART_TRACK     0x40
#define COMMAND_VOLUME_UP         0x41
#define COMMAND_VOLUME_DOWN       0x42
#define COMMAND_TOGGLE_PAUSE      0x43

extern AudioLibrary coreLibrary;

extern clock_t timeMarks[];

/*
 * SAMPLE USEAGE:
    t1 = clock();
    mp3DecoderError = mpg123_read(currentTrack, currentTrackBuffer, audioBufferSize, &bytesRead);
    elapsed = clock() - t1;
    cout << "Time to load file: " << elapsed / (CLOCKS_PER_SEC / 1000) << " ms" << endl;
*/

#endif /* GLOBALS_H */

