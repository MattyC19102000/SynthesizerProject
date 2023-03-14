#include <stdbool.h>

#ifndef NOTE_H
#define NOTE_H

// structure which holds all the data needed to abstract a note
typedef struct Note{
    char id; // the unique identifier of a note
    double f; // the frequency (pitch) of the note
    double on; // the time the note was turned on
    double off; // the time the note was turned off
    bool active; // if the note is still producing sound
} Note;

#endif //NOTE_H
