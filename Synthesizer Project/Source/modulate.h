#include "osc.h"

#ifndef MODULATE_H
#define MODULATE_H

/*
this header is used to apply a mathmatical algorithm using the oscillator functions
to achieve frequency modulation
*/

double modDepth; // how much the carrier wave is modulated by the modulation wave

double modulate(double cf, double mf, double depth, double v){
    
    // returns the result of the FM algorithm as a sample
    return osc(carrier, (toAng(cf) * globalTime) + depth * (osc(mod, toAng(mf) * globalTime))) * v;
    
}


#endif //MODULATE_H
