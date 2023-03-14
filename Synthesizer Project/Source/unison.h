#include "modulate.h"

#ifndef UNISON_H
#define UNISON_H

/*
This header is used to apply Unison to a wave
Unison is when multiple of the same waves (called voices) are played at the same time
with a slight detuning between them, this causes the sound to appear fuller and with
a slight deviation in volime over time
*/
double unison(double detune, int voices, double f, double blend, double volume){
    
    // instantiate sample return value
    double r = 0.0;
    
    // for every voice
    for(int i = 1; i <= voices; i++){
        // if there is only one voice
        if(voices == 1){
            return modulate(f, f, modDepth, volume); // return only one wave with no detuning
        }
        // if there is more than one voice
        else{
            double newf = (f - detune) + i*((2 * detune) / (voices - 1)); // deviate a frequency +/- detune value
            
            // for the first one or two voices there is no volume dampening
            double detunedMain = modulate(newf, newf, modDepth, volume);
            // for the other voices dampen the volume
            double detunedSide = modulate(newf, newf, modDepth, volume * blend);
            
            // if Odd
            if(voices % 2 == 1){
                // if the voice is the centre/first voice
                if(i == 1)
                    r += detunedMain; // no blending
                // if a side voice
                else
                    r += detunedSide; // blending
            }
            // if Even
            if(voices % 2 == 0){
                // if the voice is one of the centre voices
                if(i == 1 || i == 2)
                    r += detunedMain; // no blending
                // if a side voice
                else
                    r += detunedSide; // blending
            }
        }
    }
    return r / voices; // return a sample normalized by the amount of voices;
}

#endif //UNISON_H
