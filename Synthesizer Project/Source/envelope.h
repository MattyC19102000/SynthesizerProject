#include "note.h"

#ifndef ENVELOPE_H
#define ENVELOPE_H


/*
This headers functionality is to apply an ADSR Envelope to the amplitude/volume of a wave
ADSR stands for Attack, Decay Sustain, Release which models the stages real instruments
go through when they are plucked/played.
the attack is how long it takes to reach it's loudest volume once it's played
the decay  is how long it takes to go from the peak volume to the sustain volume
the sustain volume is how loud the instrument is when the note is held
 the release is how long it takes for the sound to slowly dissipate to nothing once the note is released
*/
double attackTime;
double decayTime;
double sustainAmp;
double releaseTime;
double peak;

// this function applies an ADSR Envelope to the volume of a wave
double envelope_apply(Note *n){
    
    double returnAmp = 0.0; // the return value once the volume is calculated
    double releaseAmp = 0.0; // the volume caluclated once the note is released
    
    // if the note is being played
    if(n->on > n->off){
        // get how long the note has been held for
        double lifetime = globalTime - n->on;
        // if the lifetime of the note is in the attack phase
        if(lifetime <= attackTime){
            returnAmp = (lifetime / attackTime) * peak; // slowly increase to peak
        }
        
        // if the lifetime of the note is in the decay phase
        if(lifetime > attackTime && lifetime <= (decayTime + attackTime)){
            // Slowly decrease from peak towards sustain amplitude
            returnAmp = ((lifetime - attackTime) / decayTime) * (sustainAmp - peak) + peak;
        }
        
        // if the lifetime of the note is in the sustain phase
        if(lifetime > (attackTime + decayTime)){
            returnAmp = sustainAmp; // keep the volume at a consistent level
        }
    }
    // if the note has been released
    if(n->off > n->on){
        // get how long the note was played for
        double lifetime = n->off - n->on;
        // if the note was released in the attack phase
        if(lifetime <= attackTime){
            releaseAmp = (lifetime / attackTime) * peak; // get amp during attack phase
        }
        
        // if the note was released in the decay phase
        if(lifetime > attackTime && lifetime <= (decayTime + attackTime)){
            releaseAmp = ((lifetime - attackTime) / decayTime) * (sustainAmp - peak) + peak; // get amp during decay
        }
        
        // if the note was released in the sustain phase
        if(lifetime > (attackTime + decayTime)){
            releaseAmp = sustainAmp; // get amp from sustainAmp
        }
        // calculate the slow decay of the volume
        returnAmp = ((globalTime - n->off) / releaseTime) * (-releaseAmp) + releaseAmp;
    }
    
    // if the volume of the note is almost 0
    if(returnAmp <= 0.001){
        // set the value to 0
        returnAmp = 0.0;
        // if the note is released
        if(n->off > n->on)
            n->active = false; // flag the note to be removed
    }
    return returnAmp; // return volume
}

#endif //ENVELOPE_H
