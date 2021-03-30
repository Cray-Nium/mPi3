/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <iostream>
#include <queue>
#include <mpg123.h>
#include <out123.h>
#include "globals.h"
#include "Album.h"
#include "AudioLibrary.h"
#include "Playlist.h"
#include "audioPlayback.h"

//For file searching
//#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <map>
#include <cstring>
#include <string>
#include <algorithm> //To get sort()
#include <random>
#include <ctime>
#include <chrono> //To get system_clock


using namespace std;

queue<uint8_t> audioPlaybackMessages;
bool audioThreadDone = false;

AudioLibrary coreLibrary;
vector<Album> albumLibrary;

mpg123_handle *currentTrack = NULL;
int mp3DecoderStatus = MPG123_OK;

size_t audioBufferSize = 0;  // 267886704 Is enough for 25 minutes of 16-bit 2-channel music at 44100Hz
unsigned char* currentTrackBuffer = NULL;
long currentTrackSampleRate = 0; //Note: long is same size as int, but the library wants long.
int currentTrackChannelCount = 0;
int currentTrackEncodingType = 0;
int audioFramesize = 1;
size_t bytesRead;
size_t bytesPlayed;
double baseVolume = 1;
double realVolume = 1;
double rva_db = 0;

out123_handle *audioOutput = NULL;
char* audioOutputDriver = NULL; //NULL: default
char* audioOutputDevice = NULL; //NULL: default

off_t framesPlayed;
static bool paused = false;

//Temporary data structures


bool audioPlaybackInit();
void processAudioPlaybackMessage();
bool playChunk();
bool stabilizeStream();
bool loadMp3File(int index);
bool loadPreviousTrack();
bool loadNextTrack();
void setTrackTime(double seconds);
void increaseVolume();
void decreaseVolume();

bool populateAlbumLibrary(string rootDirPath, vector<Album> &library);

//Temporary fns
void compilePrimaryPlaylist();


void* audioThreadRun(void* param)
{
    if (!loadMp3File(songNumber)){
        audioThreadDone = true;
        return NULL;
    }
    
    while (!mPi3ShuttingDown){
        
        while (!audioPlaybackMessages.empty()){
            processAudioPlaybackMessage();
        }
        
        if (!paused){
            playChunk();
        }
        
        if (mp3DecoderStatus != MPG123_OK){
            if (mp3DecoderStatus != MPG123_DONE && mp3DecoderStatus != MPG123_NEW_FORMAT ){
                fprintf( stderr, "Warning: Error with decoding because: %s\n",
                         mp3DecoderStatus == MPG123_ERR ? mpg123_strerror(currentTrack) : mpg123_plain_strerror(mp3DecoderStatus) );
            }
            if (!stabilizeStream()) break;
        }
        
    }
    
    audioThreadDone = true;
    return NULL;
}

bool audioPlaybackInit(){
    clock_t t1, elapsed;
    if ( (mpg123_init() != MPG123_OK) || ( (currentTrack = mpg123_new(NULL, &mp3DecoderStatus)) == NULL) )
    {
        printf("Decoder init failed.\n");
        return false;
    }
    audioOutput = out123_new();
    if (!audioOutput)
    {
        printf("Output init failed.\n");
        return false;
    }
    if(out123_open(audioOutput, audioOutputDriver, audioOutputDevice) != OUT123_OK)
    {
        printf("Trouble with out123: %s\n", out123_strerror(audioOutput));
        return false;
    }
    
    t1 = clock();
    populateAlbumLibrary(libraryDirectory, albumLibrary);
    compilePrimaryPlaylist();
    elapsed = clock() - t1;
    cout << "Time to compile library and primary playlist: " << elapsed / (CLOCKS_PER_SEC / 1000) << " ms" << endl;
    
    cout << "Size of coreLibrary track list: " << coreLibrary.tracks.size() << endl;
    cout << "First entry of coreLibrary: " << coreLibrary.tracks.at(0).filepath << endl;
    cout << "Size of primaryPlaylist entries: " << primaryPlaylist.entries.size() << endl;
    cout << "First entry of primaryPlaylist: " << coreLibrary.tracks.at(primaryPlaylist.entries.at(0).audioTrackKey).filepath << endl;
    
    return true;
}

void audioPlaybackCleanup(){
    out123_del(audioOutput);
    mpg123_close(currentTrack);
    mpg123_delete(currentTrack);
    mpg123_exit();
}

double getTrackLength(){
    return ( mpg123_framelength(currentTrack) * mpg123_tpf(currentTrack) );
}

double getTrackTime(){
    return ( mpg123_tellframe(currentTrack) * mpg123_tpf(currentTrack) );
}


void processAudioPlaybackMessage(){
    uint8_t messageCode = audioPlaybackMessages.front();
    audioPlaybackMessages.pop();
    
    switch (messageCode) {
        case COMMAND_NEXT_TRACK:
            loadNextTrack();
            break;
        case COMMAND_PREVIOUS_TRACK:
            loadPreviousTrack();
            break;
        case COMMAND_RESTART_TRACK:
            setTrackTime(0);
            break;
        case COMMAND_VOLUME_UP:
            increaseVolume();
            break;
        case COMMAND_VOLUME_DOWN:
            decreaseVolume();
            break;
        case COMMAND_TOGGLE_PAUSE:
            paused = !paused;
            break;
        default:
            break;
    }
}

bool playChunk(){
    
    mp3DecoderStatus = mpg123_read(currentTrack, currentTrackBuffer, audioBufferSize, &bytesRead);
    bytesPlayed = out123_play(audioOutput, currentTrackBuffer, bytesRead);

    if(bytesPlayed != bytesRead)
    {
        fprintf(stderr, "Warning: written less than gotten from libmpg123: %li != %li\n", (long)bytesPlayed, (long)bytesRead);
    }
    framesPlayed += bytesPlayed/audioFramesize;
    
    return true;
}

bool stabilizeStream(){
    if (mp3DecoderStatus == MPG123_DONE){
        printf("%li frames written (%d bytes).\n", (long)framesPlayed, (framesPlayed * audioFramesize));
        return loadNextTrack();
    }
    else return false;
}

bool loadMp3File(int index){

    string audioFilePath = coreLibrary.tracks.at(primaryPlaylist.entries.at(index).audioTrackKey).filepath;

    cout << endl << "Track index: " << songNumber << endl;
    cout << "File path: " << audioFilePath << endl;

    mp3DecoderStatus = mpg123_open(currentTrack, audioFilePath.c_str());
    mpg123_getformat(currentTrack, &currentTrackSampleRate, &currentTrackChannelCount, &currentTrackEncodingType);

    if (mp3DecoderStatus == MPG123_OK)
    {
        if (mpg123_scan(currentTrack) != MPG123_OK){
            cout << "Scan failed." << endl;
        }
        printf("Opened file %s. Sample rate: %i, Channels: %d, Encoding: %x, Expected Length: %f\n",
               audioFilePath.c_str(), currentTrackSampleRate, currentTrackChannelCount, currentTrackEncodingType, getTrackLength());
    }
    else
    {
        printf(mpg123_plain_strerror(mp3DecoderStatus));
        printf(mpg123_strerror(currentTrack));
        return false;
    }

    if( out123_start(audioOutput, currentTrackSampleRate, currentTrackChannelCount, currentTrackEncodingType) != OUT123_OK
       || out123_getformat(audioOutput, NULL, NULL, NULL, &audioFramesize) != OUT123_OK )
    {
        printf("Cannot start output / get framesize: %s\n", out123_strerror(audioOutput));
        return false;
    }
    printf("Framesize after opening output: %d.\n", audioFramesize);

    audioBufferSize = mpg123_outblock(currentTrack);
    currentTrackBuffer = (unsigned char*)malloc(audioBufferSize);
    cout << "Buffer size: " << (audioBufferSize / audioFramesize) << " frames." << endl;

    framesPlayed = 0;
    
    return true;
}

bool loadPreviousTrack(){
    if ( (songNumber - 1) >= 0 && (songNumber - 1) < primaryPlaylist.entries.size() ){
        songNumber--;
        mpg123_close(currentTrack);
        free(currentTrackBuffer);
        return loadMp3File(songNumber);
    }
    else return false;
}

bool loadNextTrack(){
    if ( (songNumber + 1) >= 0 && (songNumber + 1) < primaryPlaylist.entries.size() ){
        songNumber++;
        mpg123_close(currentTrack);
        free(currentTrackBuffer);
        return loadMp3File(songNumber);
    }
    else return false;
}

void setTrackTime(double seconds){
    mpg123_seek_frame(currentTrack, mpg123_timeframe(currentTrack, seconds), SEEK_SET);
}

void increaseVolume(){
    mpg123_volume_change(currentTrack, VOLUME_INCREMENT);
    mpg123_getvolume(currentTrack, &baseVolume, &realVolume, &rva_db);
}

void decreaseVolume(){
    mpg123_volume_change(currentTrack, -1 * VOLUME_INCREMENT);
    mpg123_getvolume(currentTrack, &baseVolume, &realVolume, &rva_db);
}

bool populateAlbumLibrary(string rootDirPath, vector<Album> &library)
{
    DIR* rootDir;
    dirent* currentFile;
    vector<string> albumFileNames;
    
    if( (rootDir = opendir(rootDirPath.c_str())) == NULL )
    {
        cout << "Error(" << errno << ") opening " << rootDirPath << endl;
        return false;
    }
    
    while ( (currentFile = readdir(rootDir)) != NULL )
    {
        if (   (currentFile->d_type == DT_DIR)
            && ((string)currentFile->d_name != ".")
            && ((string)currentFile->d_name != "..") )
        {
            albumFileNames.push_back((string)currentFile->d_name);
            cout << "Found album folder: " << string(currentFile->d_name) << "." << endl;
        }   
    }
    if (!albumFileNames.empty())
    {
        sort(albumFileNames.begin(), albumFileNames.end());
        for (int i=0; i<albumFileNames.size(); i++)
        {
            cout << "Adding album '" << albumFileNames[i] << "' to library" << endl;
            library.push_back(Album(rootDirPath + albumFileNames[i] + "/" ));
        }
        cout << endl;
    }
    
    return true;
}

//Temporary fns
void compilePrimaryPlaylist()
{
    for (int i=0; i<albumLibrary.size(); i++)
    {
        cout << "Adding album '" << albumLibrary[i].getTitle() << "' to playlist" << endl;
        for (int j=0; j<albumLibrary[i].discs.size(); j++)
        {
            for (int k=0; k<albumLibrary[i].discs[j].tracks.size(); k++)
            {
                primaryPlaylist.entries.push_back(PlaylistEntry(albumLibrary[i].discs[j].trackKeys[k]));
            }
        }
    }
    primaryPlaylist.shuffle(PLAYLIST_SHUFFLE_ALL);
}
