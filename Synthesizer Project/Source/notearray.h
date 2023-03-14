#include "note.h"

#ifndef NOTEARRAY_H
#define NOTEARRAY_H

/*
this header is for functions which modify an array of notes
which will be iterated over to generate samples from the notes
data
*/

Note* notes; // list of notes
int notesMax; // maximum amount of notes which can be played at once
int notesCurrent; // the current amount of notes in the array

// instantiate the note list
void notes_init(int max){
    
    // set the max and current
    notesMax = max;
    notesCurrent = 0;
    
    // allocate memory based on the max size of the list
    notes = malloc(notesMax * sizeof(Note));
    
    // generate a blank note and fill the list with it to ensure there are no NULL spaces
    Note blankNote;
    blankNote.id = -1;
    blankNote.f = 0;
    
    for(int i = 0; i < notesMax; i++){
        notes[i] = blankNote; 
    }
}

// this function is used for adding a note to the note array
void note_add(Note n){
    
    // if the array is not full
    if(notesCurrent < notesMax - 1){
        notes[notesCurrent] = n; // add the note
        notesCurrent++; // increment current notes int
    }
}

// this function is used for removing a note from the list
void note_remove(Note n){
    // iterate through every note in the list to find the one to be removed
    for(int i = 0; i < notesCurrent; i++){
        // once the note is found
        if(notes[i].id == n.id){
            // iterate over every note ahead of this note and decrement its position by one
            for(int j = i; j < notesCurrent; j++){
                notes[j] = notes[j + 1];
            }
            notesCurrent--; // decrement the current amount of notes
            return;
        }
    }
}

// this function returns a pointer to a note from it's id
Note* note_get(char id){
    // iterate over each note
    for(int i = 0; i < notesCurrent; i++){
        // if the ID and the note ID match
        if(notes[i].id == id){
            return &notes[i]; // return the note pointer
        }
    }
    
    // if the note required is not found return NULL
    return NULL; 
    
}

#endif //NOTEARRAY_H
