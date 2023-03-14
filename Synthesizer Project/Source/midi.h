#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include "notearray.h"

#ifndef MIDI_H
#define MIDI_H


/*

this header handles inputting and parsing midi messages to be handled by the program
midi messages are composed of a status byte, which describes the type of midi message
and two data bytes which describe the data attached to the status byte,
for instance a midi message can be comprised of a status byte which describes a note
being turned on, and the following data bytes will describe the specific note which was 
pressed and how hard it was pressed.



*/

// this is an enum which abstracts the status byte of a midi message
enum midi_status{
    NOTE_OFF = 0x8,
    NOTE_ON = 0x9
};

// Midi Device Data
char** midiDevices;
int midiDevId;

// Midi in handler variable
HMIDIIN hmi;

// gather all midi devices and display their ID and name
void midi_init_devs(){
    
    // get number of devices
    unsigned int numInDevs = midiInGetNumDevs();
    
    // allocate memory to a buffer which holds all device names
    midiDevices = malloc(numInDevs * sizeof(char*));
    
    // variable which holds the capabilities of the midi device
    MIDIINCAPS minc;
    
    for(int i = 0; i < numInDevs; i++){
        // if devices are found
        if(midiInGetDevCaps(i, &minc, sizeof(MIDIINCAPS)) == S_OK){
            midiDevices[i] = minc.szPname; // store name in midiDevices
            printf("%d. %s\n", i, minc.szPname); // print the name and ID
        }
    }
}

// set the midi device for input
void set_midi_device(int id){
    midiDevId = id;
}

// bitwise operation to get just the status bit from a midi message
enum midi_status midi_get_status_bit(DWORD msg){
    return (((1 << 4) - 1) & (msg >> (5 - 1)));
}

// this function gathers the first data bit from the midi message and converts it into a 1 byte character
char midi_get_note_num(DWORD msg){
    return (((1 << 8) - 1) & (msg >> (9 - 1)));
}

// convert the character from the previous function into a notes frequency
double midi_note_num_to_f(char key){
    return 440 * pow(2, ((double)key - 69) / 12);
}

/*
MidiInProc is a placeholder function that the midiInOpen function uses to perform functions
 whenever a new Midi In event is triggered.
The function takes 5 parameters:
* hMidiIn is the device that we're generating input from
* wMsg defines the type of data that's being sent to the function (defines dwParams)
* dwInstance is user instance data that we can pass through, such as a user defined struct
* dwParam1 is the MIDI Message format when wMsg = MIM_DATA
* dwParam2 is the Timestamp when the midi message is sent
dwParams are changed based on what wMsg is
*/
void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2){
    switch(wMsg){
        // in the case that uMsg shows that a new midi input has been detected
        case MIM_DATA:{
            
            // create a new note based off of the midi message
            Note n;
            n.id = midi_get_note_num(dwParam1); // get the note id
            n.f = midi_note_num_to_f(n.id); // create a frequency from the id
            n.active = true; // bool ensures the note won't be removed and will use Envelope system
            n.on = globalTime; // get the time the note was turned on at
            n.off = 0.0; // the time the note was turned off at (hasn't been turned off)
            
            // check if the note already exists within the note list
            Note* found = note_get(n.id); 
            
            // check the status bit to see if the note is pressed or released
            switch(midi_get_status_bit(dwParam1)){
                // if the note is pressed
                case NOTE_ON: {
                    // if the note is not yet in the note list
                    if(found == NULL)
                        note_add(n); // add it to the list
                    // if the note is already in the note list but not finished making noise
                    else{
                        found->on = globalTime; // reset the envelope
                        found->active = true; // ensure it isn't removed from the notelist
                    }
                };
                break;
                // if the note is released
                case NOTE_OFF: {
                    found->off = globalTime; // set the note off time for the Envelope release phase
                }; break;
                default : break;
            }
        };break;
        default : return;
    }
}

// starts the midi thread
void midi_init(){
    // opens the midi channel for input
    midiInOpen(&hmi, devId, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION);
    // starts the midi handler
    midiInStart(hmi);
}

#endif //MIDI_H
