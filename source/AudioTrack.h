#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H

#include <string>

using namespace std;

class AudioTrack
{
private:
public:
    AudioTrack(string filepath = NULL);
    string filepath;
    string artFilePath;
    string title;
    string artist;
    unsigned int duration;
};

#endif /* AUDIOTRACK_H */

