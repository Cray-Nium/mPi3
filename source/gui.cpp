/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "globals.h"
#include "gui.h"

using namespace sf;

//For displaying on TV
/*
Vector2f viewOrigin(40,25); //First visible pixel
Vector2f viewLimit(1881,1056); //First non-visible pixel beyond center
Vector2f viewSize = viewLimit - viewOrigin;
*/

Vector2u windowSize;
Vector2u windowOrigin = Vector2u(0,0);

//Getting album art working
Texture albumArtTexture;
Vector2u albumArtNativeSize;
Sprite albumArtSprite;
float albumArtFrameSideMarginPercent = .10;
float albumArtFrameTopMarginPercent = .30;
float albumArtFrameSideLength;
float albumArtTextureScaleFactor;
Vector2f albumArtFrameOrigin;
RectangleShape albumArtFrame;


void* guiThreadRun(void* param){
    RenderWindow window( VideoMode::getFullscreenModes()[0], "mPi3", Style::Fullscreen);
    windowSize = window.getSize();
    window.setFramerateLimit(60);

    albumArtTexture.loadFromFile("/home/pi/Music/mPi3_library/Kings & Queens - LEAH/Artwork/CoverArt.jpg");
    albumArtNativeSize = albumArtTexture.getSize();

    albumArtFrameSideLength = windowSize.x * (1 - 2 * albumArtFrameSideMarginPercent);
    albumArtFrameOrigin.x = windowSize.x * albumArtFrameSideMarginPercent;
    albumArtFrameOrigin.y = windowSize.y * albumArtFrameTopMarginPercent;
    albumArtFrame.setOrigin(albumArtFrameOrigin);
    albumArtFrame.setSize(Vector2f(albumArtFrameSideLength, albumArtFrameSideLength));
    
    albumArtTextureScaleFactor = ( albumArtNativeSize.x >= albumArtNativeSize.y 
                                  ? albumArtFrameSideLength / albumArtNativeSize.x
                                  : albumArtFrameSideLength / albumArtNativeSize.y);
    
    albumArtTexture.setSmooth(true);
    albumArtSprite.setTexture(albumArtTexture);
    albumArtSprite.setPosition(albumArtFrame.getOrigin());
    albumArtSprite.setScale(albumArtTextureScaleFactor, albumArtTextureScaleFactor);

    
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::TouchBegan:
                    cout << "Touch began with finger: " << event.touch.finger << " and coordinates: " << event.touch.x << ", " << event.touch.y << endl;
                    break;
                case sf::Event::TouchMoved:
                    cout << "Touch moved with finger: " << event.touch.finger << " and coordinates: " << event.touch.x << ", " << event.touch.y << endl;
                    break;
                case sf::Event::TouchEnded:
                    cout << "Touch ended with finger: " << event.touch.finger << " and coordinates: " << event.touch.x << ", " << event.touch.y << endl;
                    break;
            }
        }
        
        if (audioThreadDone){
            window.close();
        }

        window.clear(Color::Black);
        window.draw(albumArtSprite);
        window.display();
    }
}

bool guiInit(){
    
    return true;
}

void guiCleanup(){
}