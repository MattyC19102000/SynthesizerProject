#include "driverio.h"
#include "midi.h"
#include "notearray.h"
#include "osc.h"
#include "unison.h"
#include "envelope.h"

// Controls the amount each voice is detuned by
double detune;

// Method called by the audio thread which generates a sample of a waveform to be sent to the sound drivers
double generate_wave(){
    
    double r = 0.0;
    // For each note currently pressed
    for(int i = 0; i < notesCurrent; i++){
        // add the frequencies and waveforms of each note together to produce polyphony
        r += unison(detune, 5, notes[i].f, 0.4, envelope_apply(&notes[i]));
        // if the note is no longer producing sound remove it from the note list
        if(notes[i].active == false)
            note_remove(notes[i]);
    }
    // send this sample to the audio thread
    return r;
}

int main(){
    
    // Initialize Detune value to 0
    detune = 0;
    
    // Initialize Audio Data & Thread
    audio_init_devs();
    set_output_device(0);
    /*
44100hz/44.1khz is the standard sample rate for most audio tools
8 blocks with 1024 samples per block generates the best quality sound without sacraficing latency
*/
    set_wav_params(44100, 8, 1024);
    audio_init();
    set_noise_func(generate_wave);
    
    // Initialize Midi Data & Thread
    midi_init_devs();
    set_midi_device(0);
    midi_init();
    
    // Initialze Note List
    notes_init(9);
    
    // Initialize Modulation Options
    carrier = OSC_SINE;
    mod = OSC_SINE;
    modDepth = 0;
    
    // Initialize Envelope Options
    attackTime = 2.0;
    peak = 0.3;
    decayTime = 2.0;
    sustainAmp = 0.3;
    releaseTime = 2.0;
    
    while(1){
        // Capture keyboard inputs from the user on a seperate thread to not disturb the audio thread
        if(GetAsyncKeyState(VK_ESCAPE) & 0x01)
            exit(0); // close the program
        
        if(GetAsyncKeyState(VK_LEFT) & 0x01)
            detune += 0.2; 
        
        if(GetAsyncKeyState(VK_RIGHT) & 0x01)
            detune -= 0.2;
        
        if(GetAsyncKeyState(VK_UP) & 0x01)
            modDepth += 0.1;
        
        if(GetAsyncKeyState(VK_DOWN) & 0x01)
            modDepth -= 0.1;
        
        if(GetAsyncKeyState(VK_NUMPAD1) & 0x01)
            carrier = OSC_SINE;
        
        if(GetAsyncKeyState(VK_NUMPAD2) & 0x01)
            carrier = OSC_TRIANGLE;
        
        if(GetAsyncKeyState(VK_NUMPAD3) & 0x01)
            carrier = OSC_SQUARE;
        
        if(GetAsyncKeyState(VK_NUMPAD4) & 0x01)
            mod = OSC_SINE;
        
        if(GetAsyncKeyState(VK_NUMPAD5) & 0x01)
            mod = OSC_TRIANGLE;
        
        if(GetAsyncKeyState(VK_NUMPAD6) & 0x01)
            mod = OSC_SQUARE;
        // Reset all modifiers back to default
        if(GetAsyncKeyState(VK_BACK) & 0x01){
            carrier = OSC_SINE;
            mod = OSC_SINE;
            detune = 0.0;
            modDepth = 0.0;
        }
    }
    
    return 0;
}