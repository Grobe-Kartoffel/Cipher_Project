#include <pthread.h> // threading
#include <stdio.h> // printf()
#include <stdlib.h> // malloc() // file IO
#include <math.h> // pow()

#include "raylib.h"

/* TO DO:
 * only open and work with files when necessary
    * when inputting a file, open, read, close
    * when processing text, pass the file paths needed
       * open files when needed, close when done
 * VIGENERE CIPHER
    * cracking
 * display errors for:
    ^ dragging a file over the wrong location
    * typing too many characters into a text window?
    ^ importing a file who's path is too long
    ^ importing a file with the wrong extension
    ^ importing a file that is too long
    ^ file could not be imported for any other reason
    ^ output path could not be generated
    - output path not valid
    ^ could not open output file
    * could not output to file for any reason
    * cannot run processor because another one is running
    - trying to crack a cipher with no expected word
 * display messages for:
    * successful file output
    ^ processing started
    * processing complete
 */

int GLOBALFONTSIZE = 15;

enum Cipher{Vigenere,Ceasar,ZigZag,Spiral};
enum Operation{Encrypt,Decrypt,Crack,Info};
enum SelWindow{None,Input,Output,InputFile,OutputFile,V_Key,ExpectedWord};

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
struct TextWindow{
    TextWindow(){
        this->text = (char *)malloc(sizeof(char[128]));
        this->size = 0;
        this->capacity = 128;
        this->charLimCapacity = 8;
        this->x = 0;
        this->y = 0;
        this->width = 250;
        this->height = 250;
        this->textChanged = false; // keep false, creates race condition where display text is not properly initialized otherwise
    }
    TextWindow(int cap, int charLimCap, int x, int y, int w, int h){
        this->text = (char *)malloc(sizeof(char[cap]));
        this->size = 0;
        this->capacity = cap;
        this->charLimCapacity = charLimCap;
        this->x = x;
        this->y = y;
        this->width = w;
        this->height = h;
        this->textChanged = false; // keep false, creates race condition where display text is not properly initialized otherwise
    }
    char *text;
    int size;
    int capacity;
    int charLimCapacity;
    int x;
    int y;
    int width;
    int height;
    bool textChanged;
    void Clear(){
        for(int i = 0; i<capacity; i++)
            text[i] = '\0';
        size = 0;
    }
    bool InputKey(const char c){
        if(size!=capacity){
            text[size] = c;
            size++;
            return true;
        }
        return false;
    }
    int InputString(const char *input){
        for(int i = 0; i<capacity; i++){ // put a limit on the loop so that inputted text cannot exceed the length of the window
            if(input[i]=='\0')
                return (i==0?-1:0); // return fail if no text entered, success if exit by the end of the text
            if(!InputKey(input[i])){
                return 1; // input key fails if text must be truncated
            }
        }
        return 1; // if last char was '\0' then loop would have returned 0, therefore the text was truncated if we are here
    }
    bool InputNum(const int n){
        int num = n;
        char buffer;
        if(num<0){
            if(!InputKey('-'))
                return false;
            num *= -1;
        }
        //return true;
        int start = size;
        while(num>9){ // input all but last digit in reverse
            if(!InputKey('0'+(num%10))){ // if we fail to input a key
                for(int i = start; i<size; i++) // erase it all and abort
                    text[i] = '\0';
                size = start;
                return false;
            }
            num /= 10;
        } // input last digit in reverse
        if(!InputKey('0'+num)){ // if we fail to input a key
            for(int i = start; i<size; i++) // erase it all and abort
                text[i] = '\0';
            size = start;
            return false;
        }
        // reverse digits
        int end = size-1;
        while(start<end){
            buffer = text[start];
            text[start] = text[end];
            text[end] = buffer;
            start++;
            end--;
        }
        return true;
    }
    void DisplayText(char **displayText){
        int dSize = 0;
        int dCap = capacity*2+1; // large enough for every character to be on its own line
        char *display = *displayText;
        for(int i = 0; i<dCap; i++) // clear text
            display[i] = '\0';
        
        int lSize = 0;
        int lCap = capacity;
        char *line = (char *)malloc(sizeof(char[capacity]));
        for(int i = 0; i<lCap; i++) // clear line
            line[i] = '\0';
        
        int wSize = 0;
        int wCap = capacity;
        char *word = (char *)malloc(sizeof(char[capacity]));
        for(int i = 0; i<wCap; i++) // clear word
            word[i] = '\0';
        
        int numLines = 0;
        char bufferChar;
        int bufferLength = MeasureText("...",GLOBALFONTSIZE);
        
        // abort if window is too small for text
        if(GLOBALFONTSIZE>height || GLOBALFONTSIZE>width){
            display[0] = '.';
            display[1] = '.';
            display[2] = '.';
            display[3] = '\0';
            free(line);
            free(word);
            return;
        }
        
        for(int i = 0; i<capacity; i++){ // loop through the text
            // add character to word
            word[wSize] = text[i];
            wSize++;
            // code if we reach the end of the word
            if(text[i]==' ' || text[i]=='\n'){
                // code if it is the last line and word is too long
                if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(line,GLOBALFONTSIZE)+MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                    // add as much of the word to the line as possible before truncating
                    for(int i = 0; i<wSize; i++){
                        if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                            line[lSize] = word[i];
                            lSize++;
                            if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                                line[lSize] = '.';
                                line[lSize+1] = '.';
                                line[lSize+2] = '.';
                                line[lSize+3] = '\0';
                                lSize += 4;
                                break;
                            }
                        }
                    }
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    free(line);
                    free(word);
                    display[dSize] = '\0';
                    return;
                }
                // code if line would be too long with word
                if(MeasureText(word,GLOBALFONTSIZE)+MeasureText(line,GLOBALFONTSIZE)>=width){
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    display[dSize] = '\n'; // signify end of line
                    dSize++;
                    lSize = 0;
                    // increase lines
                    numLines++;
                    // copy word to line
                    // last line and word is too long
                    if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                        // add as much of the word to the line as possible before truncating
                        for(int i = 0; i<wSize; i++){
                            if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                                line[lSize] = word[i];
                                lSize++;
                                if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                                    line[lSize] = '.';
                                    line[lSize+1] = '.';
                                    line[lSize+2] = '.';
                                    line[lSize+3] = '\0';
                                    lSize += 4;
                                    break;
                                }
                            }
                        }
                        // copy line to display
                        for(int j = 0; j<lSize; j++){
                            display[dSize] = line[j];
                            dSize++;
                            line[j] = '\0'; // erase the line as we go
                        }
                        lSize = 0;
                        free(line);
                        free(word);
                        display[dSize] = '\0';
                        return;
                    }
                    // otherwise, we add the word normally
                    for(int j = 0; j<wSize; j++){
                        line[lSize] = word[j];
                        lSize++;
                        word[j] = '\0'; // erase the line as we go
                    }
                    wSize = 0;
                }
                // otherwise word fits on line
                else{
                    // copy word to line
                    for(int j = 0; j<wSize; j++){
                        line[lSize] = word[j];
                        lSize++;
                        word[j] = '\0'; // erase the line as we go
                    }
                    wSize = 0;
                }
                // '\n' immediately finishes line (if we get here in the code, the word was added to the line)
                if(text[i]=='\n'){
                    // if this is the last line
                    if((numLines+1)*GLOBALFONTSIZE>=height){
                        // add '...' to line
                        line[lSize-1] = '.';
                        line[lSize] = '.';
                        line[lSize+1] = '.';
                        lSize += 2;
                    }
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    // increase lines
                    numLines++;
                    if(numLines*GLOBALFONTSIZE>=height){
                        lSize = 0;
                        display[dSize] = '\0'; // just in case, somehow, the last character is not '\0'
                        free(line);
                        free(word);
                        return;
                    }
                }
            }
            // code if the word is too long
            else if(MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                // if we are on the last 2 lines, add '...' and return // need more than that to display word
                if( ((numLines+2)*GLOBALFONTSIZE)>=height){
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    // add as much of the word to the line as possible before truncating
                    for(int j = 0; j<wSize; j++){
                        if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                            line[lSize] = word[j];
                            lSize++;
                            if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                                line[lSize] = '.';
                                line[lSize+1] = '.';
                                line[lSize+2] = '.';
                                line[lSize+3] = '\0';
                                lSize += 4;
                                break;
                            }
                        }
                    }
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    free(line);
                    free(word);
                    display[dSize] = '\0';
                    return;
                }
                // save last valid character to buffer
                bufferChar = word[wSize-1];
                // replace last character with dash
                word[wSize-1] = '-';
                // copy line to display
                if(lSize!=0){
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    display[dSize] = '\n'; // signify end of line
                    dSize++;
                    lSize = 0;
                    // increase lines
                    numLines++;
                }
                // copy word to line
                for(int j = 0; j<wSize; j++){
                    line[lSize] = word[j];
                    lSize++;
                    word[j] = '\0'; // erase the line as we go
                }
                wSize = 0;
                // copy line to display
                for(int j = 0; j<lSize; j++){
                    display[dSize] = line[j];
                    dSize++;
                    line[j] = '\0'; // erase the line as we go
                }
                display[dSize] = '\n'; // signify end of line
                dSize++;
                lSize = 0;
                // increase lines
                numLines++;
                // add buffer character to word
                word[0] = bufferChar;
                wSize++;
            }
        }
        // if we make it here, we found the end of the text and have one last (should be '\0' terminated) word to add
        // code if it is the last line and word is too long
        if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(line,GLOBALFONTSIZE)+MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
            // add as much of the word to the line as possible before truncating
            for(int i = 0; i<wSize; i++){
                if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                    line[lSize] = word[i];
                    lSize++;
                    if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                        line[lSize] = '.';
                        line[lSize+1] = '.';
                        line[lSize+2] = '.';
                        line[lSize+3] = '\0';
                        lSize += 4;
                        break;
                    }
                }
            }
            // copy line to display
            for(int j = 0; j<lSize; j++){
                display[dSize] = line[j];
                dSize++;
                line[j] = '\0'; // erase the line as we go
            }
            lSize = 0;
            free(line);
            free(word);
            display[dSize] = '\0';
            return;
        }
        // code if line would be too long with word
        if(MeasureText(word,GLOBALFONTSIZE)+MeasureText(line,GLOBALFONTSIZE)>=width){
            // copy line to display
            for(int j = 0; j<lSize; j++){
                display[dSize] = line[j];
                dSize++;
                line[j] = '\0'; // erase the line as we go
            }
            display[dSize] = '\n'; // signify end of line
            dSize++;
            lSize = 0;
            // increase lines
            numLines++;
            // copy word to line
            // last line and word is too long
            if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                // add as much of the word to the line as possible before truncating
                for(int i = 0; i<wSize; i++){
                    if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                        line[lSize] = word[i];
                        lSize++;
                        if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                            line[lSize] = '.';
                            line[lSize+1] = '.';
                            line[lSize+2] = '.';
                            line[lSize+3] = '\0';
                            lSize += 4;
                            break;
                        }
                    }
                }
                // copy line to display
                for(int j = 0; j<lSize; j++){
                    display[dSize] = line[j];
                    dSize++;
                    line[j] = '\0'; // erase the line as we go
                }
                lSize = 0;
                free(line);
                free(word);
                display[dSize] = '\0';
                return;
            }
            // otherwise, we add the word normally
            for(int j = 0; j<wSize; j++){
                line[lSize] = word[j];
                lSize++;
                word[j] = '\0'; // erase the line as we go
            }
            wSize = 0;
        }
        // otherwise word fits on line
        else{
            // copy word to line
            for(int j = 0; j<wSize; j++){
                line[lSize] = word[j];
                lSize++;
                word[j] = '\0'; // erase the line as we go
            }
            wSize = 0;
        }
        // '\n' immediately finishes line (if we get here in the code, the word was added to the line)
        if(line[lSize-1]=='\n'){
            // if this is the last line
            if((numLines+1)*GLOBALFONTSIZE>=height){
                // add '...' to line
                line[lSize-1] = '.';
                line[lSize] = '.';
                line[lSize+1] = '.';
                lSize += 2;
            }
            // copy line to display
            for(int j = 0; j<lSize; j++){
                display[dSize] = line[j];
                dSize++;
                line[j] = '\0'; // erase the line as we go
            }
            lSize = 0;
            // increase lines
            numLines++;
            if(numLines*GLOBALFONTSIZE>=height){
                lSize = 0;
                display[dSize] = '\0'; // just in case, somehow, the last character is not '\0'
                free(line);
                free(word);
                return;
            }
        }
        // copy line to display
        for(int j = 0; j<lSize; j++){
            display[dSize] = line[j];
            dSize++;
            line[j] = '\0'; // erase the line as we go
        }
        lSize = 0;
        display[dSize] = '\0'; // just in case, somehow, the last character is not '\0'
        free(line);
        free(word);
        return;
    }
    void DisplayCharLimit(char **charLimText){
        char *charLim = *charLimText;
        for(int i = 0; i<charLimCapacity; i++)
            charLim[i] = '\0';
        
        // display the text backward
        int c = capacity;
        int s = size;
        int index = 0;
        while(c>9){
            charLim[index] = '0'+(c%10);
            c = c/10;
            index++;
        }
        charLim[index] = '0'+c;
        charLim[index+1] = '/';
        index += 2;
        while(s>9){
            charLim[index] = '0'+(s%10);
            s = s/10;
            index++;
        }
        charLim[index] = '0'+s;
        charLim[index+1] = '\0';
        
        // reverse the backward text
        s = 0;
        while(index>s){
            charLim[s] = charLim[s]+charLim[index];
            charLim[index] = charLim[s]-charLim[index];
            charLim[s] = charLim[s]-charLim[index];
            s++;
            index--;
        }
        return;
    }
};
struct ErrorMessages{
    ErrorMessages(Texture2D t){
        messages[0][0] = '\0';
        messages[1][0] = '\0';
        messages[2][0] = '\0';
        curErr = 0;
        frame = 0;
        x = 1280-288;
        y = 0;
        width = 240;
        height = 92;
        tex = t;
    }
    char messages[3][256];
    int curErr;
    int frame;
    int x;
    int y;
    int width;
    int height;
    Texture2D tex;
    void AddMessage(const char *c){
        // code to format message
        int dSize = 0;
        int dCap = 256; // large enough for every character to be on its own line
        char *display;
        // set display to next message
        if(frame<=0) // currently, animation only uses 1 frame
            display = messages[(curErr+1)%3];
        else // currently, animation only uses 2 frames
            display = messages[(curErr+2)%3];
        
        for(int i = 0; i<dCap; i++) // clear text
            display[i] = '\0';
        
        int lSize = 0;
        int lCap = 128;
        char *line = (char *)malloc(sizeof(char[128]));
        for(int i = 0; i<lCap; i++) // clear line
            line[i] = '\0';
        
        int wSize = 0;
        int wCap = 128;
        char *word = (char *)malloc(sizeof(char[128]));
        for(int i = 0; i<wCap; i++) // clear word
            word[i] = '\0';
        
        int numLines = 0;
        char bufferChar;
        int bufferLength = MeasureText("...",GLOBALFONTSIZE);
        
        // abort if window is too small for text
        if(GLOBALFONTSIZE>height || GLOBALFONTSIZE>width){
            display[0] = '.';
            display[1] = '.';
            display[2] = '.';
            display[3] = '\0';
            free(line);
            free(word);
            return;
        }
        
        for(int i = 0; i<128; i++){ // loop through the text
            // add character to word
            word[wSize] = c[i];
            wSize++;
            // code if we reach the end of the word
            if(c[i]==' ' || c[i]=='\n'){
                // code if it is the last line and word is too long
                if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(line,GLOBALFONTSIZE)+MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                    // add as much of the word to the line as possible before truncating
                    for(int i = 0; i<wSize; i++){
                        if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                            line[lSize] = word[i];
                            lSize++;
                            if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                                line[lSize] = '.';
                                line[lSize+1] = '.';
                                line[lSize+2] = '.';
                                line[lSize+3] = '\0';
                                lSize += 4;
                                break;
                            }
                        }
                    }
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    free(line);
                    free(word);
                    display[dSize] = '\0';
                    return;
                }
                // code if line would be too long with word
                if(MeasureText(word,GLOBALFONTSIZE)+MeasureText(line,GLOBALFONTSIZE)>=width){
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    display[dSize] = '\n'; // signify end of line
                    dSize++;
                    lSize = 0;
                    // increase lines
                    numLines++;
                    // copy word to line
                    // last line and word is too long
                    if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                        // add as much of the word to the line as possible before truncating
                        for(int i = 0; i<wSize; i++){
                            if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                                line[lSize] = word[i];
                                lSize++;
                                if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                                    line[lSize] = '.';
                                    line[lSize+1] = '.';
                                    line[lSize+2] = '.';
                                    line[lSize+3] = '\0';
                                    lSize += 4;
                                    break;
                                }
                            }
                        }
                        // copy line to display
                        for(int j = 0; j<lSize; j++){
                            display[dSize] = line[j];
                            dSize++;
                            line[j] = '\0'; // erase the line as we go
                        }
                        lSize = 0;
                        free(line);
                        free(word);
                        display[dSize] = '\0';
                        return;
                    }
                    // otherwise, we add the word normally
                    for(int j = 0; j<wSize; j++){
                        line[lSize] = word[j];
                        lSize++;
                        word[j] = '\0'; // erase the line as we go
                    }
                    wSize = 0;
                }
                // otherwise word fits on line
                else{
                    // copy word to line
                    for(int j = 0; j<wSize; j++){
                        line[lSize] = word[j];
                        lSize++;
                        word[j] = '\0'; // erase the line as we go
                    }
                    wSize = 0;
                }
                // '\n' immediately finishes line (if we get here in the code, the word was added to the line)
                if(c[i]=='\n'){
                    // if this is the last line
                    if((numLines+1)*GLOBALFONTSIZE>=height){
                        // add '...' to line
                        line[lSize-1] = '.';
                        line[lSize] = '.';
                        line[lSize+1] = '.';
                        lSize += 2;
                    }
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    // increase lines
                    numLines++;
                    if(numLines*GLOBALFONTSIZE>=height){
                        lSize = 0;
                        display[dSize] = '\0'; // just in case, somehow, the last character is not '\0'
                        free(line);
                        free(word);
                        return;
                    }
                }
            }
            // code if the word is too long
            else if(MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                // if we are on the last 2 lines, add '...' and return // need more than that to display word
                if( ((numLines+2)*GLOBALFONTSIZE)>=height){
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    // add as much of the word to the line as possible before truncating
                    for(int j = 0; j<wSize; j++){
                        if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                            line[lSize] = word[j];
                            lSize++;
                            if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                                line[lSize] = '.';
                                line[lSize+1] = '.';
                                line[lSize+2] = '.';
                                line[lSize+3] = '\0';
                                lSize += 4;
                                break;
                            }
                        }
                    }
                    // copy line to display
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    lSize = 0;
                    free(line);
                    free(word);
                    display[dSize] = '\0';
                    return;
                }
                // save last valid character to buffer
                bufferChar = word[wSize-1];
                // replace last character with dash
                word[wSize-1] = '-';
                // copy line to display
                if(lSize!=0){
                    for(int j = 0; j<lSize; j++){
                        display[dSize] = line[j];
                        dSize++;
                        line[j] = '\0'; // erase the line as we go
                    }
                    display[dSize] = '\n'; // signify end of line
                    dSize++;
                    lSize = 0;
                    // increase lines
                    numLines++;
                }
                // copy word to line
                for(int j = 0; j<wSize; j++){
                    line[lSize] = word[j];
                    lSize++;
                    word[j] = '\0'; // erase the line as we go
                }
                wSize = 0;
                // copy line to display
                for(int j = 0; j<lSize; j++){
                    display[dSize] = line[j];
                    dSize++;
                    line[j] = '\0'; // erase the line as we go
                }
                display[dSize] = '\n'; // signify end of line
                dSize++;
                lSize = 0;
                // increase lines
                numLines++;
                // add buffer character to word
                word[0] = bufferChar;
                wSize++;
            }
        }
        // if we make it here, we found the end of the text and have one last (should be '\0' terminated) word to add
        // code if it is the last line and word is too long
        if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(line,GLOBALFONTSIZE)+MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
            // add as much of the word to the line as possible before truncating
            for(int i = 0; i<wSize; i++){
                if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                    line[lSize] = word[i];
                    lSize++;
                    if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                        line[lSize] = '.';
                        line[lSize+1] = '.';
                        line[lSize+2] = '.';
                        line[lSize+3] = '\0';
                        lSize += 4;
                        break;
                    }
                }
            }
            // copy line to display
            for(int j = 0; j<lSize; j++){
                display[dSize] = line[j];
                dSize++;
                line[j] = '\0'; // erase the line as we go
            }
            lSize = 0;
            free(line);
            free(word);
            display[dSize] = '\0';
            return;
        }
        // code if line would be too long with word
        if(MeasureText(word,GLOBALFONTSIZE)+MeasureText(line,GLOBALFONTSIZE)>=width){
            // copy line to display
            for(int j = 0; j<lSize; j++){
                display[dSize] = line[j];
                dSize++;
                line[j] = '\0'; // erase the line as we go
            }
            display[dSize] = '\n'; // signify end of line
            dSize++;
            lSize = 0;
            // increase lines
            numLines++;
            // copy word to line
            // last line and word is too long
            if( ((numLines+1)*GLOBALFONTSIZE)>=height && MeasureText(word,GLOBALFONTSIZE)+bufferLength>=width){
                // add as much of the word to the line as possible before truncating
                for(int i = 0; i<wSize; i++){
                    if(MeasureText(line,GLOBALFONTSIZE)+bufferLength<width){
                        line[lSize] = word[i];
                        lSize++;
                        if(MeasureText(line,GLOBALFONTSIZE)+bufferLength>=width){
                            line[lSize] = '.';
                            line[lSize+1] = '.';
                            line[lSize+2] = '.';
                            line[lSize+3] = '\0';
                            lSize += 4;
                            break;
                        }
                    }
                }
                // copy line to display
                for(int j = 0; j<lSize; j++){
                    display[dSize] = line[j];
                    dSize++;
                    line[j] = '\0'; // erase the line as we go
                }
                lSize = 0;
                free(line);
                free(word);
                display[dSize] = '\0';
                return;
            }
            // otherwise, we add the word normally
            for(int j = 0; j<wSize; j++){
                line[lSize] = word[j];
                lSize++;
                word[j] = '\0'; // erase the line as we go
            }
            wSize = 0;
        }
        // otherwise word fits on line
        else{
            // copy word to line
            for(int j = 0; j<wSize; j++){
                line[lSize] = word[j];
                lSize++;
                word[j] = '\0'; // erase the line as we go
            }
            wSize = 0;
        }
        // '\n' immediately finishes line (if we get here in the code, the word was added to the line)
        if(line[lSize-1]=='\n'){
            // if this is the last line
            if((numLines+1)*GLOBALFONTSIZE>=height){
                // add '...' to line
                line[lSize-1] = '.';
                line[lSize] = '.';
                line[lSize+1] = '.';
                lSize += 2;
            }
            // copy line to display
            for(int j = 0; j<lSize; j++){
                display[dSize] = line[j];
                dSize++;
                line[j] = '\0'; // erase the line as we go
            }
            lSize = 0;
            // increase lines
            numLines++;
            if(numLines*GLOBALFONTSIZE>=height){
                lSize = 0;
                display[dSize] = '\0'; // just in case, somehow, the last character is not '\0'
                free(line);
                free(word);
                return;
            }
        }
        // copy line to display
        for(int j = 0; j<lSize; j++){
            display[dSize] = line[j];
            dSize++;
            line[j] = '\0'; // erase the line as we go
        }
        lSize = 0;
        display[dSize] = '\0'; // just in case, somehow, the last character is not '\0'
        free(line);
        free(word);
        
        // set animation into motion
        if(frame==0) // no animation, start new transition
            frame = 1;
        // if an animation is happening, it will handle the transition
        return;
    }
    void DrawMessage(){
        if(frame<0){ // message fade away
            if(frame>-180){ // stay on screen for 3 seconds
                DrawTexture(tex,x,y,(Color){255,255,255,255});
                DrawText(messages[curErr],x+26,y+44,GLOBALFONTSIZE,(Color){0,0,0,255});
                if(messages[(curErr+1)%3][0]!='\0'){ // if there is another message in queue
                    frame = 1;
                    return; // frame will be decremented to 0, stopping animation if this is not here
                }
            }
            else{ // fade after 2 seconds
                DrawTexture(tex,x,y,(Color){255,255,255,(int)(255-((((float)frame * -1.0)-180)/60.0)*255.0)});
                DrawText(messages[curErr],x+26,y+44,GLOBALFONTSIZE,(Color){0,0,0,(int)(255-((((float)frame*-1.0)-180)/60.0)*255.0)});
            }
            frame--;
            if(frame<-240){
                messages[curErr][0] = '\0';
                if(messages[(curErr+1)%3][0]!='\0') // if there is another message, start transition animation
                    frame = 1;
                else // otherwise, stop animating
                    frame = 0;
                return;
            }
        }
        if(frame>0){ // load new message
            // draw curErr transitioning to !curErr
            // new curErr
            DrawTexture(tex,x,-160+((float)(frame-1)/60.0)*160.0,(Color){255,255,255,(int)(0+((float)(frame-1)/60.0)*255.0)});
            DrawText(messages[(curErr+1)%3],x+26,44-160+((float)(frame-1)/60.0)*160.0,GLOBALFONTSIZE,(Color){0,0,0,(int)(0+((float)(frame-1)/(float)60)*255.0)});
            // curErr falling away
            if(messages[curErr][0]!='\0'){
                DrawTexture(tex,x,y+((float)(frame-1)/60.0)*160.0,(Color){255,255,255,(int)(255-((float)(frame-1)/60.0)*255.0)});
                DrawText(messages[curErr],x+26,y+44+((float)(frame-1)/60.0)*160.0,GLOBALFONTSIZE,(Color){0,0,0,(int)(255-((float)(frame-1)/(float)60)*255.0)});
            }
            frame++;
            if(frame>60){
                frame = -1;
                messages[curErr][0] = '\0';
                curErr = (curErr+1)%3;
            }
        }
    }
};

int findChar(char c, char *alphabet){
    int i = 0;
    while(alphabet[i]!='\0'){
        if(c==alphabet[i])
            return i;
        i++;
    }
    return -1;
}

struct t_ThreadArgs{
    t_ThreadArgs(){
        selWindow = NULL;
        windows[0] = NULL;
        windows[1] = NULL;
        windows[2] = NULL;
        windows[3] = NULL;
        windows[4] = NULL;
        windows[5] = NULL;
        windows[6] = NULL;
    }
    t_ThreadArgs(SelWindow *selWindow, TextWindow *input, TextWindow *output, TextWindow *inputFile, TextWindow *outputFile, TextWindow *v_Key, TextWindow *expectedWord){
        this->selWindow = selWindow;
        windows[0] = NULL;
        windows[1] = input;
        windows[2] = output;
        windows[3] = inputFile;
        windows[4] = outputFile;
        windows[5] = v_Key;
        windows[6] = expectedWord;
    }
    SelWindow *selWindow;
    TextWindow *windows[7];
};
struct V_CipherArgs{
    V_CipherArgs(){
        fileInput = false;
        running = false;
        operation = NULL;
        alphabetChoice = NULL;
        inputFile = NULL;
        outputFile = NULL;
        inputText = NULL;
        outputText = NULL;
        key = NULL;
        expectedWord = NULL;
    }
    V_CipherArgs(Operation *operation, int *alphabetChoice, FILE **inputFile, FILE **outputFile, TextWindow *inputText, TextWindow *outputText, TextWindow *key, TextWindow *expectedWord){
        this->fileInput = false;
        this->running = false;
        this->operation = operation;
        this->alphabetChoice = alphabetChoice;
        this->inputFile = inputFile;
        this->outputFile = outputFile;
        this->inputText = inputText;
        this->outputText = outputText;
        this->key = key;
        this->expectedWord = expectedWord;
    }
    bool fileInput;
    bool running;
    Operation *operation;
    int *alphabetChoice;
    FILE **inputFile;
    FILE **outputFile;
    TextWindow *inputText;
    TextWindow *outputText;
    TextWindow *key;
    TextWindow *expectedWord;
};

void *textInput(void *input){ // text input thread
    t_ThreadArgs *t_input = (t_ThreadArgs *)input;
    char key;
    const char *pText;
    int fKey;
    double time = -1.0;
    bool holdBack = false;
    //bool holdTab = false;
    //bool holdEnter = false;
    bool canCopy = true;
    bool canPaste = true;
    
    TextWindow *window = NULL;
    // CLEAR ALL TEXT WINDOWS
    for(int i = 1; i<7; i++){
        window = t_input->windows[i];
        for(int j = 0; j<window->capacity; j++){
            window->text[j] = '\0';
        }
        window->size = 0;
        window->textChanged = true;
    }
    
	while(!WindowShouldClose()){
        if( *(t_input->selWindow)==None )
            continue;
        window = t_input->windows[*(t_input->selWindow)];
        // copy
        if(canCopy && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_C)){
            SetClipboardText(window->text);
            canCopy = false;
            continue;
        }
        // paste
        if(canPaste && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_V)){
            pText = GetClipboardText();
            while(*pText!='\0' && window->size<window->capacity){
                if(*pText=='\r' || *pText=='\n'){
                    pText++;
                    continue;
                }
                window->text[window->size] = (*pText=='\t'?' ':*pText);
                window->size++;
                pText++;
            }
            canPaste = false;
            window->textChanged = true;
            continue;
        }
        // reset copy/paste // if(not copying and not pasting)
        if(!( (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_C) ) && !( (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyDown(KEY_V) )){
            canCopy = true;
            canPaste = true;
        }
        // backspace
        if(IsKeyDown(KEY_BACKSPACE) && window->size>0 && GetTime()>(time+(holdBack?1.0/30.0:1.0/2.0) ) ){
            window->text[window->size-1] = '\0';
            window->size--;
            if(time>0) // time is less than zero before first press
                holdBack = true;
            time = GetTime();
            window->textChanged = true;
            continue;
        }
        
        // unsure if these keys are necessary
        //// enter
        //if(IsKeyDown(KEY_ENTER) && GetTime()>(time+(holdEnter?1.0/30.0:1.0/2.0) ) ){
        //    window->InputKey('\n');
        //    if(time>0) // time is less than zero before first press
        //        holdEnter = true;
        //    time = GetTime();
        //    window->textChanged = true;
        //    continue;
        //}
        //// tab
        //if(IsKeyDown(KEY_TAB) && window->size>0 && GetTime()>(time+(holdTab?1.0/30.0:1.0/2.0) ) ){
        //    window->InputKey('\t');
        //    if(time>0) // time is less than zero before first press
        //        holdTab = true;
        //    time = GetTime();
        //    window->textChanged = true;
        //    continue;
        //}
        
        // reset backspace, enter, and tab
        if(!IsKeyDown(KEY_BACKSPACE)){// && !IsKeyDown(KEY_ENTER) && !IsKeyDown(KEY_TAB)){
            holdBack = false;
            //holdEnter = false;
            //holdTab = false;
            time = -1.0;
        }
        
        // delete
        fKey = GetKeyPressed();
        if(fKey==KEY_DELETE){
            for(int i = 0; i<window->capacity; i++){
                window->text[i] = '\0';
            }
            window->size = 0;
            window->textChanged = true;
            continue;
        }
        // type
        key = (char)GetCharPressed();
        if(key!='\0' && window->size<window->capacity){
            window->text[window->size] = key;
            window->size++;
            window->textChanged = true;
        }
    }
	return NULL;
}
void *VigenereCipher(void *input){
    V_CipherArgs *V_input = (V_CipherArgs *)input;
    if( V_input->running ) // ensure thread only runs one at a time
        return NULL;
    V_input->running = true;
    
    char alphabets[][53] =
    {"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",
    "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    char *alphabet = alphabets[*(V_input->alphabetChoice)];
    
    char inText[2048];
    char outText[2048];
    char *inChar = inText;
    char *outChar = outText;
    char *keyChar = V_input->key->text;
    //char *expectChar = V_input->expectedWord->text;
    //int expectLen = V_input->expectedWord->size;
    int textIndex, keyIndex;
    int charCounter = 1;
    
    if(V_input->fileInput){ // fill inText with the file text // set inChar to point to that text
        rewind(*(V_input->inputFile));
        do{
            *inChar = fgetc(*(V_input->inputFile));
        } while( *(inChar++)!=EOF );
        *(--inChar) = '\0';
        inChar = inText;
    }
    else{ // set inChar to point to text
        inChar = V_input->inputText->text;
    }
    
    // right now, we have a char pointer to our plain text and our key, and our alphabet at the ready
    switch( *(V_input->operation) ){
        case Encrypt:{
            V_input->outputText->Clear();
            V_input->outputText->textChanged = true;
            while(*inChar!='\0'){
                textIndex = findChar(*inChar,alphabet);
                keyIndex = findChar(*keyChar,alphabet);
                if(textIndex<0 || keyIndex<0)
                    *outChar = *inChar;
                else
                    *outChar = alphabet[(textIndex+keyIndex)%52];
                
                // output
                if(V_input->fileInput){
                    fputc(*outChar,*(V_input->outputFile));
                    V_input->outputText->Clear();
                    V_input->outputText->InputString("Vigenere Cipher - ");
                    V_input->outputText->InputNum(charCounter);
                    V_input->outputText->InputString(" Characters Encrytped");
                }
                else
                    V_input->outputText->InputKey(*outChar);
                V_input->outputText->textChanged = true;
                
                // increment pointers
                outChar++;
                inChar++;
                keyChar++;
                charCounter++;
                if(*keyChar=='\0') // wrap key if reach end
                    keyChar = V_input->key->text;
            }
            if(V_input->fileInput)
                fflush(*(V_input->outputFile));
            break;
        }
        case Decrypt:{
            V_input->outputText->Clear();
            V_input->outputText->textChanged = true;
            while(*inChar!='\0'){
                textIndex = findChar(*inChar,alphabet);
                keyIndex = findChar(*keyChar,alphabet);
                if(textIndex<0 || keyIndex<0)
                    *outChar = *inChar;
                else
                    *outChar = alphabet[(textIndex-keyIndex+52)%52];
                
                // output
                if(V_input->fileInput){
                    fputc(*outChar,*(V_input->outputFile));
                    V_input->outputText->Clear();
                    V_input->outputText->InputString("Vigenere Cipher - ");
                    V_input->outputText->InputNum(charCounter);
                    V_input->outputText->InputString(" Characters Encrytped");
                }
                else
                    V_input->outputText->InputKey(*outChar);
                V_input->outputText->textChanged = true;
                
                // increment pointers
                outChar++;
                inChar++;
                keyChar++;
                charCounter++;
                if(*keyChar=='\0') // wrap key if reach end
                    keyChar = V_input->key->text;
            }
            if(V_input->fileInput)
                fflush(*(V_input->outputFile));
            break;
        }
        case Crack:{
            break;
        }
    }
    V_input->running = false;
    return NULL;
}

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
    
    // GUI variables
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
        gui[19].init(60,130,402,76,LoadTexture("assets/V_ParameterBackground.png"));        // 19 - V - parameter background
        gui[20].init(350,124,38,38,LoadTexture("assets/V_AlphabetButton_Enabled.png"));     // 20 - V - alphabet button
        gui[21].init(350,124,38,38,LoadTexture("assets/V_AlphabetButton_Disabled.png"));
        gui[22].init(350,124,38,38,LoadTexture("assets/V_AlphabetButton_Pressed.png"));
        gui[23].init(214,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_1.png"));     // 23 - V - alphabet menu - selection 1
        gui[24].init(214,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_2.png"));
        gui[25].init(214,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_3.png"));
        gui[26].init(214,124,134,38,LoadTexture("assets/V_AlphabetMenu_Closed_4.png"));
        gui[27].init(214,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_0.png"));       // 27 - V - alphabet menu - open selection 0
        gui[28].init(214,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_1.png"));
        gui[29].init(214,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_2.png"));
        gui[30].init(214,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_3.png"));
        gui[31].init(214,124,134,96,LoadTexture("assets/V_AlphabetMenu_Open_4.png"));
        gui[32].init(60,130,64,24,LoadTexture("assets/Info.png"));                          // 32 LAST ITEM - Info background
    }
    
    Cipher cipher = Vigenere;
    Operation operation = Encrypt;
    SelWindow selWindow = None;
    enum ButtonState{Enabled,Disabled,Pressed,Not_Rendered};
    ButtonState importButton = Enabled;
    ButtonState exportButton = Enabled;
    ButtonState executeButton = Enabled;
    
    // Vigenere Menu Variables
    ButtonState V_alphabetButton = Enabled;
    int V_alphabetMenuOption = 0;
    bool V_alphabetMenu_Open = false;
    int V_alphabetMenu_OpenOption = 2;
    ButtonState *lastPressed = NULL;
    
    // text windows
    TextWindow input(512,8,60,418,544,240);
    char *inputText = (char *)malloc(sizeof(char[input.capacity*2+1]));
    char *inputCharLim = (char *)malloc(sizeof(char[input.charLimCapacity]));
    TextWindow output(512,8,676,418,544,240);
    char *outputText = (char *)malloc(sizeof(char[output.capacity*2+1]));
    char *outputCharLim = (char *)malloc(sizeof(char[output.charLimCapacity]));
    TextWindow inputFile(256,8,264,378,200,15);
    char *inputFileText = (char *)malloc(sizeof(char[inputFile.capacity*2+1]));
    char *inputFileCharLim = (char *)malloc(sizeof(char[inputFile.charLimCapacity]));
    TextWindow outputFile(256,8,908,378,172,15);
    char *outputFileText = (char *)malloc(sizeof(char[outputFile.capacity*2+1]));
    char *outputFileCharLim = (char *)malloc(sizeof(char[outputFile.charLimCapacity]));
    TextWindow v_Key(128,8,225,180,170,15);
    char *v_KeyText = (char *)malloc(sizeof(char[v_Key.capacity*2+1]));
    char *v_KeyCharLim = (char *)malloc(sizeof(char[v_Key.charLimCapacity]));
    TextWindow expectedWord(128,8,338,136,170,15);
    char *expectedWordText = (char *)malloc(sizeof(char[expectedWord.capacity*2+1]));
    char *expectedWordCharLim = (char *)malloc(sizeof(char[expectedWord.charLimCapacity]));
    
    SetTextLineSpacing(GLOBALFONTSIZE); // set vertical spacing // setting it to the font size seems to work generally well
    
    // error messages
    ErrorMessages ems(LoadTexture("assets/ErrMsg.png"));
    
    // mouse coords
    float mx;
    float my;
    
    // file vars
    FilePathList droppedFiles; // list of dragged&dropped files
    FILE *inFile = NULL; // input file pointer
    FILE *outFile = NULL; // output file pointer
    bool inputFileFrame = false;
    char c; // buffer char to read to and from files
    
    // threads
    t_ThreadArgs tArgs(&selWindow,&input,&output,&inputFile,&outputFile,&v_Key,&expectedWord);
    pthread_t inputThread;
    pthread_create(&inputThread,NULL,textInput,&tArgs);
    
    V_CipherArgs vArgs(&operation,&V_alphabetMenuOption,&inFile,&outFile,&input,&output,&v_Key,&expectedWord);
    pthread_t vigenereThread;
    
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
            if(V_alphabetButton==Enabled && mx>=350 && mx<388 && my>=124 && my<162){ // toggle vigenere alphabet button
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
            selWindow = None;
            if(mx>=46 && mx<616 && my>=404 && my<684){ // select input window
                selWindow = Input;
            }
            if(mx>=662 && mx<1232 && my>=404 && my<684){ // select output window
                selWindow = Output;
            }
            if(mx>=250 && mx<534 && my>=364 && my<404){ // select inputFile window
                selWindow = InputFile;
            }
            if(mx>=894 && mx<1150 && my>=364 && my<404){ // select outputFile window
                selWindow = OutputFile;
            }
            if(mx>=212 && mx<462 && my>=166 && my<206 && cipher==Vigenere && operation<Crack){ // select V_Key window
                selWindow = V_Key;
            }
            if(mx>=224 && mx<574 && my>=122 && my<162 && operation==Crack){ // select ExpectedWord window
                selWindow = ExpectedWord;
            }
            
            // close alphabet menu on click away
            if(V_alphabetMenu_Open && !(mx>=350 && mx<388 && my>=124 && my<162) && !(mx>=200 && mx<324 && my>=134 && my<212))
                V_alphabetMenu_Open = false;
            // update execute button state on click
            if(cipher!=Vigenere || operation==Crack)
                executeButton = Disabled;
            else
                executeButton = Enabled;
            if(operation==Info)
                executeButton = Not_Rendered;
        }
        if(IsMouseButtonReleased(0)){ // release buttons
            if(importButton==Pressed && mx>=536 && mx<616 && my>=366 && my<404){ // activate import button
                importButton = Enabled;
                if(inputFile.size>=5 && (IsFileExtension(inputFile.text, ".txt") || IsFileExtension(inputFile.text, ".csv")) ){
                    if(inFile!=NULL){
                        fclose(inFile);
                        inFile = NULL;
                    }
                    inFile = fopen(inputFile.text,"r");
                    if(inFile!=NULL && GetFileLength(inputFile.text)<2048){
                        // input
                        c = fgetc(inFile);
                        input.Clear();
                        while(c!=EOF){
                            if( input.size==input.capacity-3 && GetFileLength(inputFile.text)>=input.capacity ){ // if there's no room left and text is too long to display
                                input.InputString("...");
                                break;
                            }
                            if(!input.InputKey(c))
                                break;
                            c = fgetc(inFile);
                        }
                        input.textChanged = true;
                        outputFile.Clear();
                        outputFile.InputNum(GetFileLength(inputFile.text));
                        outputFile.textChanged = true;
                        
                        // create output file name
                        outputFile.Clear();
                        outputFile.InputString(GetFileNameWithoutExt(inputFile.text));
                        switch(cipher){
                            case Vigenere:
                                outputFile.InputString("_Vigenere_");
                                vArgs.fileInput = true;
                                break;
                            case Ceasar:
                                outputFile.InputString("_Ceasar_");
                                //cArgs.fileInput = true;
                                break;
                            case ZigZag:
                                outputFile.InputString("_ZigZag_");
                                //zArgs.fileInput = true;
                                break;
                            case Spiral:
                                outputFile.InputString("_Spiral_");
                                //sArgs.fileInput = true;
                                break;
                        }
                        switch(operation){
                            case Encrypt:
                                outputFile.InputString("Encryption.txt");
                                break;
                            case Decrypt:
                                outputFile.InputString("Decryption.txt");
                                break;
                            case Crack:
                                outputFile.InputString("Cracked.txt");
                                break;
                        }
                        if(!IsFileExtension(outputFile.text,".txt")){ // verify the whole name could be printed
                            ems.AddMessage("Output Path Could Not Be Generated.");
                            outputFile.Clear();
                        }
                        outputFile.textChanged = true;
                        
                        // load output file
                        if(outFile!=NULL){
                            fflush(outFile);
                            fclose(outFile);
                            outFile = NULL;
                        }
                        outFile = fopen(outputFile.text,"w");
                        if(outFile==NULL){
                            ems.AddMessage("Output File Could Not Be Opened.");
                            outputFile.Clear();
                        }
                        // tell program not to clear input file because of text update this frame
                        inputFileFrame = true;
                    }
                    else{ // failed to open input file, clear file input
                        if(GetFileLength(inputFile.text)>=2048)
                            ems.AddMessage("Imported File Is Too Long.\nPlease Be Sure To Import A File Less Than 2048 Characters Long.");
                        else
                            ems.AddMessage("Imported File Could Not Be Opened.\nPlease Make Sure The File Is Closed Before Trying Again.");
                        // these two variables will cause later code to clear everything for us
                        input.textChanged = true;
                        inputFileFrame = false;
                    }
                }
                else{ // failed to open input file, clear file input
                    if( !(IsFileExtension(inputFile.text, ".txt") || IsFileExtension(inputFile.text, ".csv")) )
                        ems.AddMessage("Imported File Type Is Incorrect.\nPlease Be Sure To Import A Text File (.txt) Or CSV File (.csv)");
                    else
                        ems.AddMessage("Imported File Too Short To Be Valid.\nPlease Be Sure To Import A File That Looks Like This:\n\"filename.txt\"");
                    // these two variables will cause later code to clear everything for us
                    input.textChanged = true;
                    inputFileFrame = false;
                }
            }
            if(exportButton==Pressed && mx>=1152 && mx<1232 && my>=366 && my<404){ // activate export button
                exportButton = Enabled;
                if(IsFileExtension(outputFile.text,".txt")){
                    if(outFile!=NULL){
                        fflush(outFile);
                        fclose(outFile);
                        outFile = NULL;
                    }
                    outFile = fopen(outputFile.text,"w");
                    if(outFile!=NULL){
                        for(int i = 0; i<output.capacity; i++){
                            c = output.text[i];
                            if(c=='\0')
                                break;
                            fputc(c,outFile);
                        }
                        fflush(outFile);
                        fclose(outFile);
                        outFile = NULL;
                    }
                }
            }
            if(executeButton==Pressed && mx>=1068 && mx<1220 && my>=276 && my<328){ // activate execute button
                executeButton = Enabled;
                if(cipher==Vigenere && !vArgs.running){
                    pthread_create(&vigenereThread,NULL,VigenereCipher,&vArgs);
                    ems.AddMessage("Process Started");
                }
                else
                    ems.AddMessage("Another Process Is Already Running.\nPlease Wait Until It Is Complete Before Starting Another Process.");
            }
            if(V_alphabetButton==Pressed && mx>=350 && mx<388 && my>=124 && my<162){ // activate vigenere alphabet button
                V_alphabetButton = Enabled;
                V_alphabetMenu_Open = !V_alphabetMenu_Open;
            }
            if(V_alphabetMenu_Open && mx>=220 && mx<344 && my>=134 && my<212){ // select vigenere alphabet
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
            if(V_alphabetButton==Pressed && !(mx>=350 && mx<388 && my>=124 && my<162)) // release vigenere alphabet button
                V_alphabetButton = Enabled;
            // reclick buttons if rehovered
            if(importButton==Enabled && lastPressed== &importButton && mx>=536 && mx<616 && my>=366 && my<404) // release import button
                importButton = Pressed;
            if(exportButton==Enabled && lastPressed== &exportButton && mx>=1152 && mx<1232 && my>=366 && my<404) // release export button
                exportButton = Pressed;
            if(executeButton==Enabled && lastPressed== &executeButton && mx>=1068 && mx<1220 && my>=276 && my<328) // release execute button
                executeButton = Pressed;
            if(V_alphabetButton==Enabled && lastPressed== &V_alphabetButton && mx>=350 && mx<388 && my>=124 && my<162) // release vigenere alphabet button
                V_alphabetButton = Pressed;
        }
        if(IsMouseButtonUp(0)){ // hover over menu options
            // hover over vigenere alphabet menu options
            if(V_alphabetMenu_Open && mx>=220 && mx<344 && my>=134 && my<212){
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
        if(IsFileDropped()){ // file dropped on app
            droppedFiles = LoadDroppedFiles();
            if(droppedFiles.count==1 && mx>=46 && mx<616 && my>=404 && my<684){ // file was dropped into input window // read
                if(TextLength(droppedFiles.paths[0])<inputFile.capacity && (IsFileExtension(droppedFiles.paths[0], ".txt") || IsFileExtension(droppedFiles.paths[0], ".csv")) && GetFileLength(droppedFiles.paths[0])<2048){
                    if(inFile!=NULL){
                        fclose(inFile);
                        inFile = NULL;
                    }
                    inFile = fopen(droppedFiles.paths[0],"r");
                    if(inFile!=NULL){
                        // grab input file path
                        inputFile.Clear();
                        inputFile.InputString(droppedFiles.paths[0]);
                        inputFile.textChanged = true;
                        
                        // input file
                        c = fgetc(inFile);
                        input.Clear();
                        while(c!=EOF){
                            if( input.size==input.capacity-3 && GetFileLength(droppedFiles.paths[0])>=input.capacity ){ // if there's no room left and text is too long to display
                                input.InputString("...");
                                break;
                            }
                            if(!input.InputKey(c))
                                break;
                            c = fgetc(inFile);
                        }
                        input.textChanged = true;
                        
                        // create output file name
                        outputFile.Clear();
                        outputFile.InputString(GetFileNameWithoutExt(droppedFiles.paths[0]));
                        switch(cipher){
                            case Vigenere:
                                outputFile.InputString("_Vigenere_");
                                vArgs.fileInput = true;
                                break;
                            case Ceasar:
                                outputFile.InputString("_Ceasar_");
                                //cArgs.fileInput = true;
                                break;
                            case ZigZag:
                                outputFile.InputString("_ZigZag_");
                                //zArgs.fileInput = true;
                                break;
                            case Spiral:
                                outputFile.InputString("_Spiral_");
                                //sArgs.fileInput = true;
                                break;
                        }
                        switch(operation){
                            case Encrypt:
                                outputFile.InputString("Encryption.txt");
                                break;
                            case Decrypt:
                                outputFile.InputString("Decryption.txt");
                                break;
                            case Crack:
                                outputFile.InputString("Cracked.txt");
                                break;
                        }
                        if(!IsFileExtension(outputFile.text,".txt")){ // verify the whole name could be printed
                            ems.AddMessage("Output Path Could Not Be Generated.");
                            outputFile.Clear();
                        }
                        outputFile.textChanged = true;
                        
                        // load output file
                        if(outFile!=NULL){
                            fflush(outFile);
                            fclose(outFile);
                            outFile = NULL;
                        }
                        outFile = fopen(outputFile.text,"w");
                        if(outFile==NULL){
                            ems.AddMessage("Output File Could Not Be Opened.");
                            outputFile.Clear();
                        }
                        // tell program not to clear input file because of text update this frame
                        inputFileFrame = true;
                    }
                    else{ // new file failed to open, reopen file from old path
                        ems.AddMessage("Dropped File Could Not Be Loaded.\nAttempting To Reopen Previously Loaded File.");
                        if(inputFile.size>=5 && (IsFileExtension(inputFile.text, ".txt") || IsFileExtension(inputFile.text, ".csv")) ){
                            inFile = fopen(inputFile.text,"r");
                            if(inFile==NULL || GetFileLength(inputFile.text)>=2048){
                                // old file path couldn't be opened, clear file path
                                // these two variables will cause later code to clear everything for us
                                ems.AddMessage("Previously Loaded File Could Not Be Reopened.");
                                input.textChanged = true;
                                inputFileFrame = false;
                            }
                            else{
                                ems.AddMessage("Previously Loaded File Was Successfully Reopened.");
                            }
                        }
                    }
                }
                else{
                    if( !(IsFileExtension(droppedFiles.paths[0], ".txt") || IsFileExtension(droppedFiles.paths[0], ".csv")) )
                        ems.AddMessage("Dropped File Type Is Incorrect.\nPlease Be Sure To Drop A Text File (.txt) Or CSV File (.csv)");
                    else if(GetFileLength(droppedFiles.paths[0])>=2048)
                        ems.AddMessage("Dropped File Is Too Long.\nPlease Be Sure To Drop A File Less Than 2048 Characters Long.");
                    else if(TextLength(droppedFiles.paths[0])>=inputFile.capacity)
                        ems.AddMessage("Dropped File Path Is Too Long.\nPlease Be Sure To Drop A File With A File Path Less Than 256 Characters Long.");
                }
            }
            else{
                ems.AddMessage("File Dropped In Wrong Location.\nPlease Be Sure To Drop File Over Input Window.");
            }
            UnloadDroppedFiles(droppedFiles);
        }
        if(!vArgs.running){
            pthread_join(vigenereThread,NULL);
        }
        // draw
        BeginDrawing();{
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
                        // draw vigenere key word here to prevent drawing order error
                        if(cipher==Vigenere && operation<Crack){
                            if(v_Key.textChanged){
                                v_Key.DisplayText(&v_KeyText);
                                v_Key.DisplayCharLimit(&v_KeyCharLim);
                                v_Key.textChanged = false;
                            }
                            if(v_KeyText[0]=='\0' && selWindow!=V_Key)
                                DrawText("Type or Paste Text...",v_Key.x,v_Key.y,GLOBALFONTSIZE,(selWindow==V_Key?BLACK:LIGHTGRAY));
                            else
                                DrawText(v_KeyText,v_Key.x,v_Key.y,GLOBALFONTSIZE,(selWindow==V_Key?BLACK:LIGHTGRAY));
                            if(selWindow==V_Key)
                                DrawText(v_KeyCharLim,v_Key.x+v_Key.width+MeasureText("128/128",GLOBALFONTSIZE)-MeasureText(v_KeyCharLim,GLOBALFONTSIZE),v_Key.y,GLOBALFONTSIZE,(v_Key.size==v_Key.capacity?RED:SKYBLUE));
                        }
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
           
            // draw text windows
            // input
            // abandon file input if input text is changed
            // needs additional flag to know that this didn't happen because of loading a file
            if(input.textChanged){
                input.DisplayText(&inputText);
                input.DisplayCharLimit(&inputCharLim);
                input.textChanged = false;
                if(!inputFileFrame){ // stop taking file input
                    inputFile.Clear();
                    inputFile.textChanged = true;
                    fclose(inFile);
                    inFile = NULL;
                    outputFile.Clear();
                    outputFile.textChanged = true;
                    fclose(outFile);
                    outFile = NULL;
                    switch(cipher){ // tell the ciphers about this change
                        case Vigenere:{
                            vArgs.fileInput = false;
                            break;
                        }
                        case Ceasar:{
                            //cArgs.fileInput = false;
                            break;
                        }
                        case ZigZag:{
                            //zArgs.fileInput = false;
                            break;
                        }
                        case Spiral:{
                            //sArgs.fileInput = false;
                            break;
                        }
                    }
                }
            }
            inputFileFrame = false;
            if(inputText[0]=='\0' && selWindow!=Input)
                DrawText("Type, Drag & Drop, or Paste Text...",input.x,input.y,GLOBALFONTSIZE,LIGHTGRAY);
            else
                DrawText(inputText,input.x,input.y,GLOBALFONTSIZE,(selWindow==Input?BLACK:LIGHTGRAY));
            if(selWindow==Input)
                DrawText(inputCharLim,input.x+input.width-MeasureText(inputCharLim,GLOBALFONTSIZE),input.y+input.height,GLOBALFONTSIZE,(input.size==input.capacity?RED:SKYBLUE));
            // output
            if(output.textChanged){
                output.DisplayText(&outputText);
                output.DisplayCharLimit(&outputCharLim);
                output.textChanged = false;
            }
            if(outputText[0]=='\0' && selWindow!=Output)
                DrawText("Output...",output.x,output.y,GLOBALFONTSIZE,LIGHTGRAY);
            else
                DrawText(outputText,output.x,output.y,GLOBALFONTSIZE,(selWindow==Output?BLACK:LIGHTGRAY));
            if(selWindow==Output)
                DrawText(outputCharLim,output.x+output.width-MeasureText(outputCharLim,GLOBALFONTSIZE),output.y+output.height,GLOBALFONTSIZE,(output.size==output.capacity?RED:SKYBLUE));
            // input file
            if(inputFile.textChanged){
                inputFile.DisplayText(&inputFileText);
                inputFile.DisplayCharLimit(&inputFileCharLim);
                inputFile.textChanged = false;
            }
            if(inputFileText[0]=='\0' && selWindow!=InputFile)
                DrawText("Type or Paste File Path...",inputFile.x,inputFile.y,GLOBALFONTSIZE,LIGHTGRAY);
            else
                DrawText(inputFileText,inputFile.x,inputFile.y,GLOBALFONTSIZE,(selWindow==InputFile?BLACK:LIGHTGRAY));
            if(selWindow==InputFile)
                DrawText(inputFileCharLim,inputFile.x+inputFile.width+MeasureText("256/256",GLOBALFONTSIZE)-MeasureText(inputFileCharLim,GLOBALFONTSIZE),inputFile.y,GLOBALFONTSIZE,(inputFile.size==inputFile.capacity?RED:SKYBLUE));
            // output file
            if(outputFile.textChanged){
                outputFile.DisplayText(&outputFileText);
                outputFile.DisplayCharLimit(&outputFileCharLim);
                outputFile.textChanged = false;
            }
            if(outputFileText[0]=='\0' && selWindow!=OutputFile)
                DrawText("Output File Path...",outputFile.x,outputFile.y,GLOBALFONTSIZE,(selWindow==OutputFile?BLACK:LIGHTGRAY));
            else
                DrawText(outputFileText,outputFile.x,outputFile.y,GLOBALFONTSIZE,(selWindow==OutputFile?BLACK:LIGHTGRAY));
            if(selWindow==OutputFile)
                DrawText(outputFileCharLim,outputFile.x+outputFile.width+MeasureText("256/256",GLOBALFONTSIZE)-MeasureText(outputFileCharLim,GLOBALFONTSIZE),outputFile.y,GLOBALFONTSIZE,(outputFile.size==outputFile.capacity?RED:SKYBLUE));
            // vigenere key
                // drawn in GUI loop to fix order issue
            // expected word
            if(operation==Crack){
                if(expectedWord.textChanged){
                    expectedWord.DisplayText(&expectedWordText);
                    expectedWord.DisplayCharLimit(&expectedWordCharLim);
                    expectedWord.textChanged = false;
                }
                if(expectedWordText[0]=='\0' && selWindow!=ExpectedWord)
                    DrawText("Type or Paste Text...",expectedWord.x,expectedWord.y,GLOBALFONTSIZE,(selWindow==ExpectedWord?BLACK:LIGHTGRAY));
                else
                    DrawText(expectedWordText,expectedWord.x,expectedWord.y,GLOBALFONTSIZE,(selWindow==ExpectedWord?BLACK:LIGHTGRAY));
                if(selWindow==ExpectedWord)
                    DrawText(expectedWordCharLim,expectedWord.x+expectedWord.width+MeasureText("128/128",GLOBALFONTSIZE)-MeasureText(expectedWordCharLim,GLOBALFONTSIZE),expectedWord.y,GLOBALFONTSIZE,(expectedWord.size==expectedWord.capacity?RED:SKYBLUE));
            }
            // draw info text
            if(operation==Info){
                SetTextLineSpacing(GLOBALFONTSIZE*1.5);
                switch(cipher){
                    case Vigenere:
                        DrawText("\tThe Vigenere Cipher uses an alphabet, for example, 'A-Z', and a keyword, 'example' to encode a\nmessage. Each letter in the alphabet is assigned an index number: A-0, B-1, C-2...\n\tWhen encrypting a message, the index number of each letter is added to the index number of the next\nletter in the keyword, producing a new, encrypted letter.\n\tDecrypting is very similar, but the keyword index number is subtracted from the message index number\nto produce the original text.",gui[32].x+gui[32].w+20,gui[32].y,GLOBALFONTSIZE*1.35,BLACK);
                        break;
                    case Ceasar:
                        DrawText("\tThe Ceasar Box Cipher works by arranging the letters of a message into a grid and reading by\ncolumns instead of rows. You select the width of the grid, for example, 5. Then every 5 letters forms a\nnew row. The encrypted text then displays every 5th character in the message.",gui[32].x+gui[32].w+20,gui[32].y,GLOBALFONTSIZE*1.35,BLACK);
                        break;
                    case ZigZag:
                        DrawText("\tThe Rail Fence, or Zig-Zag, Cipher works by placing letters of a message onto any number of rows.\n\tThe first letter starts at either the top or bottom row. If it started at the top, the next letter is\nadded to the next row down, and so on until you reach the bottom row, where the next letter is added\nto the next row up. Once all letters have been placed on a row, each row, from top to bottom is read to\ncreate the encrypted message.\n\tTo decrypt a message, place the letters where they would appear in the zig-zag pattern and read\nalong the zigzag instead of along each row.",gui[32].x+gui[32].w+20,gui[32].y,GLOBALFONTSIZE*1.35,BLACK);
                        break;
                    case Spiral:
                        DrawText("\tThe Spiral Cipher works by arranging the letters of a message into a grid, following a spiral pattern,\nand reading by rows or columns.\n\tTo encrypt, chose to either start at one of the corners of the grid and spiral inward clockwise or\ncounterclockwise, or start at the center and move up/down/left/right to start spiraling outward\nclockwise or counterclockwise.\n\tTo decrypt a message, fill the grid by row or column, and then read using the same spiral used to\nencrypt it.",gui[32].x+gui[32].w+20,gui[32].y,GLOBALFONTSIZE*1.35,BLACK);
                        break;
                }
                SetTextLineSpacing(GLOBALFONTSIZE);
            }
        
            // draw error messages
            ems.DrawMessage();
        }
        EndDrawing();
    }
    
    // JOIN YOUR THREADS
    pthread_join(inputThread,NULL);
    
    // CLOSE YOUR FILES
    if(inFile!=NULL)
        fclose(inFile);
    if(outFile!=NULL)
        fclose(outFile);
    
    // FREE YOUR VARIABLES
    free(inputText);
    free(inputCharLim);
    free(outputText);
    free(outputCharLim);
    free(inputFileText);
    free(inputFileCharLim);
    free(outputFileText);
    free(outputFileCharLim);
    free(v_KeyText);
    free(v_KeyCharLim);
    free(expectedWordText);
    free(expectedWordCharLim);
    
    CloseWindow();
    return 0;
}