#include <dirent.h>
#include <errno.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include "AudioLibrary.h"
#include "globals.h"
#include "Album.h"

//For debug purposes; remove when done
#include <iostream>
#include "AudioTrack.h"

using namespace std;

Album::Album(string directoryPath)
{
    this->directoryPath = directoryPath;
    Album::populate();
}

bool Album::populate()
{
    bool retVal = false;
    
    retVal = (   Album::populateTitleAndArtist()
              && Album::populateCoverArtFilePath()
              && Album::populateDiscs()
              && Album::populateTracks() );
    
    return retVal;
}

bool Album::populateTitleAndArtist()
{
    bool retVal = false;
    size_t titleStartPos;
    size_t separatorPos;
    
    if (   ( (separatorPos = this->directoryPath.rfind(" - ")) != string::npos )
        && ( (titleStartPos = this->directoryPath.rfind("/", separatorPos)) != string::npos ) )
    {
        this->title = this->directoryPath.substr( (titleStartPos + 1), (separatorPos - (titleStartPos + 1)) );
        this->artist = this->directoryPath.substr( (separatorPos + 3), (this->directoryPath.length() - (separatorPos + 4)) );
        retVal = true;
    }
    
    return retVal;
}

bool Album::populateCoverArtFilePath()
{
    bool retVal = false;
    string artworkFolderPath = this->directoryPath + "Artwork/";
    DIR* artworkDir = opendir(artworkFolderPath.c_str());
    struct dirent* currentFile;
    string currentFileName;
    
    while ( (currentFile = readdir(artworkDir)) != NULL )
    {
        currentFileName = (string)currentFile->d_name;
        if (   (currentFileName.length() >= 12)
            && (currentFileName.compare(0, 9, "CoverArt.") == 0) )
        {
            this->coverArtFilePath = artworkFolderPath += currentFileName;
            retVal = true;
            break;
        }
    }
    
    return retVal;
}

bool Album::populateDiscs()
{
    string mp3FolderPath = this->directoryPath + "MP3/";
    DIR* mp3Dir = opendir(mp3FolderPath.c_str());
    vector<string> discNames;
    struct dirent* currentFile;
    albumDisc currentDisc;
    
    while ( (currentFile = readdir(mp3Dir)) != NULL )
    {
        if (   (currentFile->d_type == DT_DIR)
            && ((string)currentFile->d_name != ".")
            && ((string)currentFile->d_name != "..") )
        {
            discNames.push_back((string)currentFile->d_name);
            cout << "Added disc with name: " << (string)currentFile->d_name << endl;
        }   
    }
    if (!discNames.empty())
    {
        sort(discNames.begin(), discNames.end());
        cout << "Final disc order: ";
        for (int i=0; i<discNames.size(); i++)
        {
            currentDisc.discName = discNames.at(i);
            this->discs.push_back(currentDisc);
            cout << currentDisc.discName << ", ";
        }
        cout << endl;
    }
    return true;
}

bool Album::populateTracks()
{
    bool retVal = false;
    string currentFolderPath;
    DIR* currentFolder;
    struct dirent* currentFile;
    vector<string> fileNames;
    albumDisc newDisc;
    albumDisc* currentDisc;
    audioLibraryReturnPair audioLibraryInfo;
    
    if (this->discs.empty())
    {
        currentFolderPath = this->directoryPath + "MP3/";
        currentFolder = opendir(currentFolderPath.c_str());
        while ( (currentFile = readdir(currentFolder)) != NULL )
        {
            if (currentFile->d_type == DT_DIR) continue;
            fileNames.push_back((string)currentFile->d_name);
        }
        if (!fileNames.empty())
        {
            sort(fileNames.begin(), fileNames.end());
            newDisc.discName = "Disc 1";
            cout << "Populating Disc 1" << endl;
            for (int i=0; i<fileNames.size(); i++)
            {
                newDisc.tracks.push_back(AudioTrack(currentFolderPath + fileNames[i]));
                cout << "Added file '" << fileNames[i] << "'" << endl;
                cout << "  with path " << newDisc.tracks[i].filepath << endl;
                audioLibraryInfo = coreLibrary.addTrack(currentFolderPath + fileNames[i]);
                if (audioLibraryInfo.second)
                {
                    newDisc.trackKeys.push_back(audioLibraryInfo.first);
                    coreLibrary.tracks[audioLibraryInfo.first].artFilePath = this->coverArtFilePath;
                    coreLibrary.tracks[audioLibraryInfo.first].artist = this->artist;
                    coreLibrary.tracks[audioLibraryInfo.first].title = fileNames[i];
                }
                else
                {
                    cout << "coreLibrary.addTrack returned false" << endl;
                }
            }
            this->discs.push_back(newDisc);
            retVal = true;
        }
    }
    else
    {
        for (int i=0; i<this->discs.size(); i++)
        {
            currentDisc = &(this->discs[i]);
            currentFolderPath = this->directoryPath + "MP3/" + currentDisc->discName + "/";
            currentFolder = opendir(currentFolderPath.c_str());
            fileNames.clear();
            while ( (currentFile = readdir(currentFolder)) != NULL )
            {
                if (currentFile->d_type == DT_DIR) continue;
                fileNames.push_back((string)currentFile->d_name);
            }
            sort(fileNames.begin(), fileNames.end());
            cout << "Populating " << currentDisc->discName << endl;
            for (int i=0; i<fileNames.size(); i++)
            {
                currentDisc->tracks.push_back(AudioTrack(currentFolderPath + fileNames[i]));
                cout << "Added file '" << fileNames[i] << "'" << endl;
                cout << "  with path " << currentDisc->tracks[i].filepath << endl;
                audioLibraryInfo = coreLibrary.addTrack(currentFolderPath + fileNames[i]);
                if (audioLibraryInfo.second)
                {
                    newDisc.trackKeys.push_back(audioLibraryInfo.first);
                }
            }
        }
    }
    
    return retVal;
}