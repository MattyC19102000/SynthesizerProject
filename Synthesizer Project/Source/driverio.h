#include <windows.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/*
 This header file manages the interactions between the program and the sound card
using the windows multimedia framework/waveOut API

a seperate thread is opened for the generation of samples and the processing of
those samples.
*/

// Audio playback data
unsigned int sampleRate;
unsigned int channels;
unsigned int blocks;
unsigned int samples;
unsigned int current;

// Waveform block buffers
int *blockMemory;
WAVEHDR *waveHeaders;

// Device info
char** devices;
int devId;
unsigned int deviceNum;
WAVEOUTCAPS woc; // Wave Out Device Capabilities

// Wave Out Device Windows Handle
HWAVEOUT hwo;

// User defined function pointer for multithreading
double (*noiseFunc)();

// Atomic Variables for audio thread
_Atomic bool ready;
_Atomic double globalTime;
sem_t blockFree; // Posix Semaphore, counts up atomically

typedef enum DRIVER_ERROR{
    ERR_NO_DEVS, // No valid output devices
    ERR_INV_DEV, // Device selected is invalid
    ERR_NO_NOISE_FUNC, // No user function defined
    ERR_WAVEOUT_FAIL,
    ERR_THREAD_FAIL,
} DRIVER_ERROR;

// Error handling for waveOut Functions
void wave_error(MMRESULT *wavErr){
    switch(*wavErr){
        case MMSYSERR_ALLOCATED : printf("Wave Error: Specified resource is already allocated\n"); return;
        case MMSYSERR_BADDEVICEID : printf("Wave Error: Specified device identifier is out of range\n"); return;
        case MMSYSERR_NODRIVER : printf("Wave Error: No device driver is present\n"); return;
        case MMSYSERR_NOMEM : printf("Wave Error: Unable to allocate or lock memory\n"); return;
        case MMSYSERR_INVALHANDLE : printf("Wave Error: Specified device handle is invalid\n"); return;
        case MMSYSERR_NOTSUPPORTED : printf("Wave Error: Specified device is synchronous and does not support pausing\n"); return;
        
        case WAVERR_BADFORMAT : printf("Wave Error: Attempted to oopen with an unsupported waveform-audio format\n"); return;
        case WAVERR_SYNC : printf("Wave Error: The device is synchronous but waveOutOpen was called without using the WAVE_ALLOWSYNC flag\n"); return;
        case WAVERR_STILLPLAYING : printf("Wave Error: There are still buffers in the queue\n"); return;
    }
}

// error handling for pthread Functions
void thread_error(int *pthreadErr){
    
    switch(*pthreadErr){
        case EAGAIN : printf("Thread Error: The system lacked the necessary resources to create another thread, or the system-imposed limit on the total number of threads in a process PTHREAD_THREADS_MAX would be exceeded\n"); return;
        case EINVAL : printf("Thread Error: The value specified by attr is invalid\n"); return;
        case EPERM : printf("Thread Error: The caller does not have appropriate permission to set the required scheduling parameters or scheduling policy\n"); return;
        case EBUSY : printf("Thread Error: The mutex could not be acquired because it was already locked.\n"); return;
        case EDEADLK : printf("Thread Error: The current thread already owns the mutex\n"); return;
        case ENOMEM : printf("Thread Error: Insufficient memory exists to initialise the mutex\n"); return;
    }
}

// called when an error is thrown
void throw_error(DRIVER_ERROR err, void *param){
    
    switch(err){
        case ERR_NO_DEVS : printf("No Valid Audio Output Devices\n");
        break;
        case ERR_INV_DEV : printf("Selected Device does not exist\n");
        break;
        case ERR_NO_NOISE_FUNC : printf("No noise function has been defined, Use set_noise_func\n");
        break;
        case ERR_THREAD_FAIL : thread_error((int*)param);
        break;
        case ERR_WAVEOUT_FAIL : wave_error((MMRESULT*)param);
        break;
    }
    exit(1);
}

// Gets a list of all valid output devices and displays them to the screen, throws error if no devices
void audio_init_devs(){
    
    deviceNum = waveOutGetNumDevs();
    
    if(deviceNum == 0){
        throw_error(ERR_NO_DEVS, NULL);
    }
    
    //char** functions as a rudimentary list of strings
    devices = malloc(deviceNum * sizeof(char*));
    
    // iterate and add strings to list
    for(int i = 0; i < deviceNum; i++){
        // get device capabilities from device id
        if(waveOutGetDevCaps(i, &woc, sizeof(WAVEOUTCAPS)) == S_OK){
            printf("%d. %s\n", i, woc.szPname);
            devices[i] = woc.szPname;
        }
    }
}

// Select an output device to output sound data to
void set_output_device(int id){
    // throw error if selected device id doesn't exist
    if(id > deviceNum || id < 0){
        throw_error(ERR_INV_DEV, NULL);
    }
    else{
        devId = id; // set output device
        waveOutGetDevCaps(devId, &woc, sizeof(WAVEOUTCAPS)); // get capabilities for the device and store in woc
        printf("Output Device Selected\n");
    }
}

// set the parameters for sending sound data, 44.1khz is standard, channels describes mono or stereo sound
void set_wav_params(int _sampleRate, int _blocks, int _samples){
    sampleRate = _sampleRate;
    channels = woc.wChannels;
    blocks = _blocks;
    samples = _samples;
    printf("Parameters Set Successfully\n");
}

// set user defined function to generate samples for the audio thread
void set_noise_func(double(*func)()){
    noiseFunc = func;
    printf("Set Noise Function Sucessfully\n");
}

// clip samples to ensure they don't go past 1 or -1
double clip(double sample, double max){
    
    if(sample >= 0.0)
        return fmin(sample, max);
    else
        return fmax(sample, -max);
    
}

// This is a windows callback function which is called whenever the sound card is ready to recieve more data
void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1,DWORD_PTR dwParam2){
    
    /*
 uMsg can contain an enum which describes the state of the callback function,
if the callback function isn't in a state to recieve data we return from it
    */
    if(uMsg != WOM_DONE)
        return;
    
    // increment the semaphore and unlock it from this thread
    int semResult = sem_post(&blockFree);
    
    // if the semaphore post function doesn't return a 0, which means it has failed, throw an error
    if(semResult != 0)
        throw_error(ERR_THREAD_FAIL, &semResult);
    
}

// This is a seperate thread which handles the creation and handling of samples, runs asynchronously
void *audio_thread(void *args){
    
    // Confirm the thread has opened
    printf("Thread Opened\n");
    
    // instantiate global time counter
    globalTime = 0;
    // set how much the timer increments by every iteration of the thread loop
    double timeStep = 1.0 / (double)sampleRate;
    
    // instantiate the sample value
    int sample = 0;
    
    // Loop until closed
    while(ready){
        
        // wait for the callback function to unlock this semaphore and decrement it
        int semResult = sem_wait(&blockFree);
        
        // if the semaphore wait fails, throw and error
        if(semResult != 0)
            throw_error(ERR_THREAD_FAIL, &semResult);
        
        // if any headers are prepared, unprepare them
        if(waveHeaders[current].dwFlags & WHDR_PREPARED){
            // unprepare the wave headers
            MMRESULT result = waveOutUnprepareHeader(hwo, &waveHeaders[current], sizeof(WAVEHDR)); 
            // if waveOutUnprepareHeader fails throw an error
            if(result != MMSYSERR_NOERROR){
                throw_error(ERR_WAVEOUT_FAIL, &result);
            }
        }
        
        // instantiate the new sample
        int newSample = 0;
        // get the current block the sample is in
        int currentBlock = current * samples;
        // for every sample within the block
        for(unsigned int i = 0; i < samples; i++){
            // if the user defined function has been set
            if(noiseFunc != NULL){
                /*
 generate a new sample by calling the user defined function
clip it to ensure it stays within the bounds of -1 to 1 and
then normalize it to the integer domain because the sound
drivers handle data within the integer domain
*/
                newSample = (int)(clip(noiseFunc(), 1.0) * INT_MAX); 
            } 
            // if the user defined function has not been set throw an error
            else{
                throw_error(ERR_NO_NOISE_FUNC, NULL);
            }
            
            // in the block memory set the current sample in the block to the newsample
            blockMemory[currentBlock + i] = newSample;
            // set the previous sample to the newsample
            sample = newSample;
            // increment globalTime
            globalTime = globalTime + timeStep;
        }
        
        // prepare the waveheader
        MMRESULT prepResult = waveOutPrepareHeader(hwo, &waveHeaders[current], sizeof(WAVEHDR));
        
        // if waveOutPrepareHeader fails throw an error
        if(prepResult != MMSYSERR_NOERROR){
            throw_error(ERR_WAVEOUT_FAIL, &prepResult);
        }
        // write the waveheader to the sound card
        MMRESULT writeResult = waveOutWrite(hwo, &waveHeaders[current], sizeof(WAVEHDR));
        
        // if waveOutWrite fails throw an error
        if(writeResult != MMSYSERR_NOERROR){
            throw_error(ERR_WAVEOUT_FAIL, &writeResult);
        }
        
        // increment the current block
        current++;
        // ensure that blocks loop when it goes past the max value of blocks
        current %= blocks;
        
    }
}

// this function initializes all the necessarry values to ensure the audio thread can generate sound samples
void audio_init(){
    
    // initialize values to 0/NULL
    ready = false; 
    current = 0;
    blockMemory = NULL;
    waveHeaders = NULL;
    noiseFunc = NULL;
    
    // initialize the semaphore to the block amount
    int semInitResult = sem_init(&blockFree, 0, blocks);
    // if the initialization fails throw an error
    if(semInitResult != 0){
        throw_error(ERR_THREAD_FAIL, &semInitResult);
    }
    
    // configure the settings for the format that our Wave samples are going to be generated in
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nSamplesPerSec = sampleRate;
    waveFormat.wBitsPerSample = sizeof(int) * 8;
    waveFormat.nChannels = channels;
    waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;
    
    // attempt to open a waveOut channel
    MMRESULT result = waveOutOpen(&hwo, devId, &waveFormat, (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION);
    // if waveOutOpen fails throw an error
    if(result != MMSYSERR_NOERROR)
        throw_error(ERR_WAVEOUT_FAIL, &result);
    
    // allocate memory for two buffers which handle the samples and the waveheaders linked to them
    blockMemory = calloc(blocks * samples, sizeof(int));
    waveHeaders = calloc(blocks, sizeof(WAVEHDR));
    
    // for every block link a waveheader to it
    for(int i = 0; i < blocks; i++){
        waveHeaders[i].dwBufferLength = samples * sizeof(int);
        waveHeaders[i].lpData = (LPSTR)(blockMemory + (i * samples));
    }
    // set the thread to loop
    ready = true;
    
    // create a new thread
    pthread_t thread;
    int iret;
    iret = pthread_create(&thread, NULL, &audio_thread, NULL);
    // if creating the new thread fails throw an error
    if(iret != 0)
        throw_error(ERR_THREAD_FAIL, &iret);
    
}

#ifndef DRIVERIO_H
#define DRIVERIO_H



#endif //DRIVERIO_H
