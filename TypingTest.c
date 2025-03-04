#include <pthread.h>
#include <stdio.h>
#include <stdlib.h> // example code used this, but I don't see a reason for it being here

#include "raylib.h"

// COMMS VARS BETWEEN THREADS
#define TEXTCAP 26
char TEXT[TEXTCAP+1] = "";    // ONLY EDIT WITHIN textInput()
int TEXTSIZE = 0;           // ONLY EDIT WITHIN textInput()

void *textInput(void *vargp){
    char key;
    const char *pText;
    int fKey;
    double time = -1.0;
    bool holdBack = false;
    bool canCopy = true;
    bool canPaste = true;
	while(!WindowShouldClose()){
        // copy
        if(canCopy && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_C)){
            SetClipboardText(&TEXT);
            canCopy = false;
            continue;
        }
        // paste
        if(canPaste && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_V)){
            pText = GetClipboardText();
            while(*pText!='\0'){
                if(*pText=='\r' || *pText=='\n'){
                    pText++;
                    continue;
                }
                TEXT[TEXTSIZE] = (*pText=='\t'?' ':*pText);
                TEXTSIZE++;
                pText++;
                if(TEXTSIZE==TEXTCAP){
                    TEXTSIZE = TEXTCAP-1;
                    for(int i = 0; i<TEXTCAP; i++){
                        TEXT[i] = TEXT[i+1];
                    }
                }
            }
            canPaste = false;
            continue;
        }
        // reset copy/paste // if(not copying and not pasting)
        if(!( (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_C) ) && !( (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_V) )){
            canCopy = true;
            canPaste = true;
        }
        // backspace
        if(IsKeyDown(KEY_BACKSPACE) && TEXTSIZE>0 && GetTime()>(time+(holdBack?1.0/30.0:1.0/2.0) ) ){
            TEXT[TEXTSIZE-1] = '\0';
            TEXTSIZE--;
            if(time>0) // time is less than zero before first press
                holdBack = true;
            time = GetTime();
            continue;
        }
        // reset backspace
        if(!IsKeyDown(KEY_BACKSPACE)){
            holdBack = false;
            time = -1.0;
        }
        // delete
        fKey = GetKeyPressed();
        if(fKey==KEY_DELETE){
            for(int i = 0; i<TEXTCAP; i++){
                    TEXT[i] = '\0';
                }
            TEXTSIZE = 0;
            continue;
        }
        // type
        key = (char)GetCharPressed();
        if(key!='\0'){
            TEXT[TEXTSIZE] = key;
            TEXTSIZE++;
            if(TEXTSIZE==TEXTCAP){
                TEXTSIZE = TEXTCAP-1;
                for(int i = 0; i<TEXTCAP; i++){
                    TEXT[i] = TEXT[i+1];
                }
            }
        }
    }
	return NULL;
}

int main(void){
    // initialize window
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Vigenere Cipher Processor");
    SetTargetFPS(60);
    
    // vars
    int txtWidth = 0;
    int txtSize = 40;
    SetTextLineSpacing(txtSize); // set vertical spacing // setting it to the font size seems to work generally well
    
    // threads
    pthread_t inputThread;
    pthread_create(&inputThread,NULL,textInput,NULL);
    
    while(!WindowShouldClose()){
        // detect inputs
        {
            if(IsMouseButtonPressed(0)){ // check where mouse button pressed and react accordingly
                
            }
        }
        // logic
        {
            // do shit
        }
        // draw
        {
            BeginDrawing();
                ClearBackground(LIGHTGRAY);
                txtWidth = MeasureText(TEXT,txtSize);
                DrawText(TEXT,640-txtWidth/2, 320, txtSize, DARKGRAY);
            EndDrawing();
        }
    }
    // join threads
    pthread_join(inputThread,NULL);
    
    // close window
    CloseWindow();
}