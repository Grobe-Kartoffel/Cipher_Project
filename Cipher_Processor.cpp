#include <pthread.h> // threading
#include <stdio.h> // printf()
#include <stdlib.h> // malloc()
#include <math.h> // pow()

#include "raylib.h"

enum Cipher{Vigenere,Ceasar,ZigZag,Spiral};
enum Operation{Encrypt,Decrypt,Crack,Info};
enum SelWindow{None,Input,InputFile,OutputFile,V_Key,ExpectedWord};
struct Graphic{
    Graphic(){
        this->x = 0;
        this->y = 0;
        this->w = 0;
        this->h = 0;
        this->tex = LoadTexture("assets/err.png");
    }
    void init(float x, float y, int w, int h, Texture2D tex){
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;
        this->tex = tex;
    }
    float x;
    float y;
    float w;
    float h;
    Texture2D tex;
};

int main(void){
    
    // Initialize
    const int WIDTH = 1280;
    const int HEIGHT = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT); // antialiasing
    InitWindow(WIDTH, HEIGHT, "Vigenere Cipher Processor");
    SetTargetFPS(60);
    
    // set window icon
    Image icon = LoadImage("assets/Icon.png");
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(icon);
    
    // variables
    int numGraphics = 33;
    Graphic gui[numGraphics];{ // initialize gui elements here
        gui[0].init(0,0,1280,720,LoadTexture("assets/Background.png"));                     // 0 - background
        gui[1].init(234,36,390,38,LoadTexture("assets/CipherMenu_Vigenere.png"));           // 1 - cipher menu
        gui[2].init(234,36,390,38,LoadTexture("assets/CipherMenu_Ceasar.png"));
        gui[3].init(234,36,390,38,LoadTexture("assets/CipherMenu_ZigZag.png"));
        gui[4].init(234,36,390,38,LoadTexture("assets/CipherMenu_Spiral.png"));
        gui[5].init(234,80,342,38,LoadTexture("assets/OperationMenu_Encrypt.png"));         // 5 - operation menu
        gui[6].init(234,80,342,38,LoadTexture("assets/OperationMenu_Decrypt.png"));
        gui[7].init(234,80,342,38,LoadTexture("assets/OperationMenu_Crack.png"));
        gui[8].init(234,80,342,38,LoadTexture("assets/OperationMenu_Info.png"));
        gui[9].init(536,366,80,38,LoadTexture("assets/Import_Enabled.png"));                // 9 - import button
        gui[10].init(536,366,80,38,LoadTexture("assets/Import_Disabled.png"));
        gui[11].init(536,366,80,38,LoadTexture("assets/Import_Pressed.png"));
        gui[12].init(1152,366,80,38,LoadTexture("assets/Export_Enabled.png"));              // 12 - export button
        gui[13].init(1152,366,80,38,LoadTexture("assets/Export_Disabled.png"));
        gui[14].init(1152,366,80,38,LoadTexture("assets/Export_Pressed.png"));
        gui[15].init(1068,276,152,52,LoadTexture("assets/Execute_Enabled.png"));            // 15 - execute button
        gui[16].init(1068,276,152,52,LoadTexture("assets/Execute_Disabled.png"));
        gui[17].init(1068,276,152,52,LoadTexture("assets/Execute_Pressed.png"));
        gui[18].init(60,124,514,38,LoadTexture("assets/ExpectedWord.png"));                 // 18 - expected word
        gui[19].init(60,130,382,76,LoadTexture("assets/V_ParameterBackground.png"));        // 19 - V - parameter background
        gui[20].init(330,124,38,38,LoadTexture("assets/V_AlphabetButton_Enabled.png"));     // 20 - V - alphabet button
        gui[21].init(330,124,38,38,LoadTexture("assets/V_AlphabetButton_Disabled.png"));
        gui[22].init(330,124,38,38,LoadTexture("assets/V_AlphabetButton_Pressed.png"));
        gui[23].init(194,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_1.png"));     // 23 - V - alphabet menu - selection 1
        gui[24].init(194,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_2.png"));
        gui[25].init(194,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_3.png"));
        gui[26].init(194,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_4.png"));
        gui[27].init(194,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_0.png"));       // 27 - V - alphabet menu - open selection 0
        gui[28].init(194,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_1.png"));
        gui[29].init(194,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_2.png"));
        gui[30].init(194,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_3.png"));
        gui[31].init(194,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_4.png"));
        gui[32].init(60,130,64,24,LoadTexture("assets/Info.png"));                          // LAST ITEM - Info background
    }
    Cipher cipher = Vigenere;
    Operation operation = Encrypt;
    SelWindow selWindow = None;
    enum ButtonState{Enabled,Disabled,Pressed,Not_Rendered};
    ButtonState importButton = Enabled;
    ButtonState exportButton = Enabled;
    ButtonState executeButton = Enabled;
    // put into vigenere struct when possible
    ButtonState V_alphabetButton = Enabled;
    int V_alphabetMenuOption = 0;
    bool V_alphabetMenu_Open = false;
    int V_alphabetMenu_OpenOption = 2;
    ButtonState *lastPressed = NULL;
    
    float mx; // mouse coords
    float my;
    
    // running loop
    while(!WindowShouldClose()){
        // detect inputs
        mx = GetMouseX();
        my = GetMouseY();
        
        // logic
        if(IsMouseButtonPressed(0)){ // click buttons and windows
            if(importButton==Enabled && mx>=536 && mx<616 && my>=366 && my<404){ // toggle import button
                importButton = Pressed;
                lastPressed = &importButton;
            }
            if(exportButton==Enabled && mx>=1152 && mx<1232 && my>=366 && my<404){ // toggle export button
                exportButton = Pressed;
                lastPressed = &exportButton;
            }
            if(executeButton==Enabled && mx>=1068 && mx<1220 && my>=276 && my<328){ // toggle execute button
                executeButton = Pressed;
                lastPressed = &executeButton;
            }
            if(V_alphabetButton==Enabled && mx>=330 && mx<368 && my>=124 && my<162){ // toggle vigenere alphabet button
                V_alphabetButton = Pressed;
                lastPressed = &V_alphabetButton;
            }
            if(mx>=234 && mx<624 && my>=36 && my<74){ // set cipher
                if(mx<330)
                    cipher = Vigenere;
                if(mx>=330 && mx<428)
                    cipher = Ceasar;
                if(mx>=428 && mx<526)
                    cipher = ZigZag;
                if(mx>=526)
                    cipher = Spiral;
            }
            if(mx>=234 && mx<576 && my>=80 && my<118){ // set operation
                if(mx<318)
                    operation = Encrypt;
                if(mx>=318 && mx<404)
                    operation = Decrypt;
                if(mx>=404 && mx<490)
                    operation = Crack;
                if(mx>=490)
                    operation = Info;
            }
            // close alphabet menu on click away
            if(V_alphabetMenu_Open && !(mx>=330 && mx<368 && my>=124 && my<162) && !(mx>=200 && mx<324 && my>=134 && my<212))
                V_alphabetMenu_Open = false;
            // update execute button state on click
            if(cipher!=Vigenere)
                executeButton = Disabled;
            else
                executeButton = Enabled;
            if(operation==Info)
                executeButton = Not_Rendered;
        }
        if(IsMouseButtonReleased(0)){ // release buttons
            if(importButton==Pressed && mx>=536 && mx<616 && my>=366 && my<404)// activate import button
                importButton = Enabled;
            if(exportButton==Pressed && mx>=1152 && mx<1232 && my>=366 && my<404)// activate export button
                exportButton = Enabled;
            if(executeButton==Pressed && mx>=1068 && mx<1220 && my>=276 && my<328)// activate execute button
                executeButton = Enabled;
            if(V_alphabetButton==Pressed && mx>=330 && mx<368 && my>=124 && my<162){// activate vigenere alphabet button
                V_alphabetButton = Enabled;
                V_alphabetMenu_Open = !V_alphabetMenu_Open;
            }
            // select vigenere alphabet
            if(V_alphabetMenu_Open && mx>=200 && mx<324 && my>=134 && my<212){
                if(my<152){
                    V_alphabetMenuOption = 0;
                    V_alphabetMenu_Open = false;
                }
                if(my>=152 && my<172){
                    V_alphabetMenuOption = 1;
                    V_alphabetMenu_Open = false;
                }
                if(my>=172 && my<192){
                    V_alphabetMenuOption = 2;
                    V_alphabetMenu_Open = false;
                }
                if(my>=192){
                    V_alphabetMenuOption = 3;
                    V_alphabetMenu_Open = false;
                }
            }
            lastPressed = NULL;
        }
        if(IsMouseButtonDown(0)){ // button interactions if move mouse while clicking
            // release clicked buttons if not hover
            if(importButton==Pressed && !(mx>=536 && mx<616 && my>=366 && my<404)) // release import button
                importButton = Enabled;
            if(exportButton==Pressed && !(mx>=1152 && mx<1232 && my>=366 && my<404)) // release export button
                exportButton = Enabled;
            if(executeButton==Pressed && !(mx>=1068 && mx<1220 && my>=276 && my<328)) // release execute button
                executeButton = Enabled;
            if(V_alphabetButton==Pressed && !(mx>=330 && mx<368 && my>=124 && my<162)) // release vigenere alphabet button
                V_alphabetButton = Enabled;
            // reclick buttons if rehovered
            if(importButton==Enabled && lastPressed== &importButton && mx>=536 && mx<616 && my>=366 && my<404) // release import button
                importButton = Pressed;
            if(exportButton==Enabled && lastPressed== &exportButton && mx>=1152 && mx<1232 && my>=366 && my<404) // release export button
                exportButton = Pressed;
            if(executeButton==Enabled && lastPressed== &executeButton && mx>=1068 && mx<1220 && my>=276 && my<328) // release execute button
                executeButton = Pressed;
            if(V_alphabetButton==Enabled && lastPressed== &V_alphabetButton && mx>=330 && mx<368 && my>=124 && my<162) // release vigenere alphabet button
                V_alphabetButton = Pressed;
        }
        if(IsMouseButtonUp(0)){ // hover over menu options
            // hover over vigenere alphabet menu options
            if(V_alphabetMenu_Open && mx>=200 && mx<324 && my>=134 && my<212){
                if(my<152)
                    V_alphabetMenu_OpenOption = 1;
                if(my>=152 && my<172)
                    V_alphabetMenu_OpenOption = 2;
                if(my>=172 && my<192)
                    V_alphabetMenu_OpenOption = 3;
                if(my>=192)
                    V_alphabetMenu_OpenOption = 4;
            }
            else
                V_alphabetMenu_OpenOption = 0;
        }
        
        // draw
        BeginDrawing();
            ClearBackground(WHITE);
            
            // draw GUI
            for(int i = 0; i<numGraphics; i++){
                switch(i){
                    default:{ // gui elements that don't need any extra code to manage drawing
                        DrawTextureV(gui[i].tex,(Vector2){gui[i].x,gui[i].y},WHITE);
                        break;
                    }
                    case 1:{ // cipher menu
                        DrawTextureV(gui[i+cipher].tex,(Vector2){gui[i+cipher].x,gui[i+cipher].y},WHITE);
                        i = 4; // end of the cipher menu
                        break;
                    }
                    case 5:{ // operation menu
                        DrawTextureV(gui[i+operation].tex,(Vector2){gui[i+operation].x,gui[i+operation].y},WHITE);
                        i = 8; // end of the operation menu
                        break;
                    }
                    case 9:{ // import button
                        DrawTextureV(gui[i+importButton].tex,(Vector2){gui[i+importButton].x,gui[i+importButton].y},WHITE);
                        i = 11; // end of the import button
                        break;
                    }
                    case 12:{ // export button
                        DrawTextureV(gui[i+exportButton].tex,(Vector2){gui[i+exportButton].x,gui[i+exportButton].y},WHITE);
                        i = 14; // end of the export button
                        break;
                    }
                    case 15:{ // execute button
                        if(executeButton!=Not_Rendered)
                            DrawTextureV(gui[i+executeButton].tex,(Vector2){gui[i+executeButton].x,gui[i+executeButton].y},WHITE);
                        i = 17; // end of the execute button
                        break;
                    }
                    case 18:{ // expected word
                        if(operation==Crack)
                            DrawTextureV(gui[i].tex,(Vector2){gui[i].x,gui[i].y},WHITE);
                        break;
                    }
                    case 19:{ // Vigenere Parameter Background
                        if(cipher==Vigenere && operation<Crack)
                            DrawTextureV(gui[i].tex,(Vector2){gui[i].x,gui[i].y},WHITE);
                        break;
                    }
                    case 20:{ // Vigenere Alphabet Button
                        if(cipher==Vigenere && operation<Crack)
                            DrawTextureV(gui[i+V_alphabetButton].tex,(Vector2){gui[i+V_alphabetButton].x,gui[i+V_alphabetButton].y},WHITE);
                        i = 22; // end of the alphabet button
                        break;
                    }
                    case 23:{ // Vigenere Alphabet Menu Closed
                        if(cipher==Vigenere && operation<Crack && !V_alphabetMenu_Open)
                            DrawTextureV(gui[i+V_alphabetMenuOption].tex,(Vector2){gui[i+V_alphabetMenuOption].x,gui[i+V_alphabetMenuOption].y},WHITE);
                        i = 26;
                        break;
                    }
                    case 27:{ // Vigenere Alphabet Menu Open
                        if(cipher==Vigenere && operation<Crack && V_alphabetMenu_Open)
                            DrawTextureV(gui[i+V_alphabetMenu_OpenOption].tex,(Vector2){gui[i+V_alphabetMenu_OpenOption].x,gui[i+V_alphabetMenu_OpenOption].y},WHITE);
                        i = 31;
                        break;
                    }
                    case 32:{ // info background
                        if(operation==Info)
                            DrawTextureV(gui[i].tex,(Vector2){gui[i].x,gui[i].y},WHITE);
                        break;
                    }
                }
            }
            
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}