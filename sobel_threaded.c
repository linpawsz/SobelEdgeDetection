// Threaded Sobel algorithm for converting large number of frames
// gcc -O3 -mssse3 -malign-double -o sobel sobel_threaded.c -lpthread
// This is a multi-thread program, use lpthread

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>

#define IMG_HEIGHT (720)
#define IMG_WIDTH (1280)
#define NUM_ROW_THREADS (6)
#define NUM_COL_THREADS (8)
#define IMG_H_SLICE (IMG_HEIGHT/NUM_ROW_THREADS)
#define IMG_W_SLICE (IMG_WIDTH/NUM_COL_THREADS)
#define FILTER_RADIUS 1
#define THRESHOLD 20

pthread_t threads[NUM_ROW_THREADS*NUM_COL_THREADS];

typedef unsigned int UINT32;
typedef unsigned long long int UINT64;
typedef unsigned char UINT8;
typedef double FLOAT;

typedef struct _threadArgs
{
    int thread_idx;
    int i;
    int j;
    int h;
    int w;
} threadArgsType;

threadArgsType threadarg[NUM_ROW_THREADS*NUM_COL_THREADS];
pthread_attr_t fifo_sched_attr;
pthread_attr_t orig_sched_attr;
struct sched_param fifo_param;

UINT8 header[22];
UINT8 R[IMG_HEIGHT][IMG_WIDTH];
UINT8 G[IMG_HEIGHT][IMG_WIDTH];
UINT8 B[IMG_HEIGHT][IMG_WIDTH];
UINT8 convR[IMG_HEIGHT][IMG_WIDTH];
UINT8 convG[IMG_HEIGHT][IMG_WIDTH];
UINT8 convB[IMG_HEIGHT][IMG_WIDTH];

void *threaded_sobel(void *threadptr)
{
    const float SobelMatrix[9] = {-1,0,1,-2,0,2,-1,0,1};
    threadArgsType thargs=*((threadArgsType *)threadptr);
    int i=thargs.i;
    int j=thargs.j;
    int dx, dy;
    double temp=0;
    FLOAT sumRX, sumRY, sumGX, sumGY, sumBX, sumBY;
    FLOAT pixelR, pixelG, pixelB;

    for(i=thargs.i; i<(thargs.i+thargs.h); i++)
    {
        for(j=thargs.j; j<(thargs.j+thargs.w); j++)
        {
            temp=0;
            temp += (SobelMatrix[0] * (FLOAT)R[(i-1)][j-1]);
            temp += (SobelMatrix[1] * (FLOAT)R[(i-1)][j]);
            temp += (SobelMatrix[2] * (FLOAT)R[(i-1)][j+1]);
            temp += (SobelMatrix[3] * (FLOAT)R[(i)][j-1]);
            temp += (SobelMatrix[4] * (FLOAT)R[(i)][j]);
            temp += (SobelMatrix[5] * (FLOAT)R[(i)][j+1]);
            temp += (SobelMatrix[6] * (FLOAT)R[(i+1)][j-1]);
            temp += (SobelMatrix[7] * (FLOAT)R[(i+1)][j]);
            temp += (SobelMatrix[8] * (FLOAT)R[(i+1)][j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convR[i][j]=(UINT8)temp;

            temp=0;
            temp += (SobelMatrix[0] * (FLOAT)G[(i-1)][j-1]);
            temp += (SobelMatrix[1] * (FLOAT)G[(i-1)][j]);
            temp += (SobelMatrix[2] * (FLOAT)G[(i-1)][j+1]);
            temp += (SobelMatrix[3] * (FLOAT)G[(i)][j-1]);
            temp += (SobelMatrix[4] * (FLOAT)G[(i)][j]);
            temp += (SobelMatrix[5] * (FLOAT)G[(i)][j+1]);
            temp += (SobelMatrix[6] * (FLOAT)G[(i+1)][j-1]);
            temp += (SobelMatrix[7] * (FLOAT)G[(i+1)][j]);
            temp += (SobelMatrix[8] * (FLOAT)G[(i+1)][j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convG[i][j]=(UINT8)temp;

            temp=0;
            temp += (SobelMatrix[0] * (FLOAT)B[(i-1)][j-1]);
            temp += (SobelMatrix[1] * (FLOAT)B[(i-1)][j]);
            temp += (SobelMatrix[2] * (FLOAT)B[(i-1)][j+1]);
            temp += (SobelMatrix[3] * (FLOAT)B[(i)][j-1]);
            temp += (SobelMatrix[4] * (FLOAT)B[(i)][j]);
            temp += (SobelMatrix[5] * (FLOAT)B[(i)][j+1]);
            temp += (SobelMatrix[6] * (FLOAT)B[(i+1)][j-1]);
            temp += (SobelMatrix[7] * (FLOAT)B[(i+1)][j]);
            temp += (SobelMatrix[8] * (FLOAT)B[(i+1)][j+1]);
	    if(temp<0.0) temp=0.0;
	    if(temp>255.0) temp=255.0;
	    convB[i][j]=(UINT8)temp;
        }
    }

    pthread_exit((void **)0);
}

int main()
{
    int fdin, fdout, bytesLeft, bytesRead;
    int imagelen, count;
    struct timeval StartTime, StopTime;
    char imagename[14];
    char outputimage[14];
    double readtime, writetime, cputime, microsecs;
    readtime = 0; writetime = 0; cputime = 0;
    int i, j, idx, jdx;
    unsigned int thread_idx;
    
    for (count=3500; count<=3505; count++)
    {
        sprintf(imagename, "image%d.ppm", count);
        // Start image opening
        if((fdin = open(imagename, O_RDONLY, 0644)) < 0)
        {
            printf("Error opening image9.ppm\n");
        }
        printf("IMAGE OPENED %d | ", fdin);

        sprintf(outputimage, "trans%d.ppm", count);
        if((fdout = open(outputimage, (O_RDWR | O_CREAT), 0666)) < 0)
        {
            printf("Error opening sharpen.ppm\n");
        }
        printf("OUTPUT FILE OPENED %d\n", fdout);
        printf("Handling file: %s\n", imagename);
        
        bytesLeft=21;
        do
        {
            bytesRead=read(fdin, (void *)header, bytesLeft);
            bytesLeft -= bytesRead;
        } while(bytesLeft > 0);
        header[21]='\0'; 
        
        // Calculating total read time for individual frames
        gettimeofday(&StartTime, 0);
        // Read and assign RGB components of input image
        for(i=0; i<IMG_HEIGHT; i++)
        {
            for(j=0; j<IMG_WIDTH; j++)
            {
                read(fdin, (void *)&R[i][j], 1); convR[i][j]=R[i][j];
                read(fdin, (void *)&G[i][j], 1); convG[i][j]=G[i][j];
                read(fdin, (void *)&B[i][j], 1); convB[i][j]=B[i][j];
            }
        }
        gettimeofday(&StopTime, 0);
        microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000) ;
        if(StopTime.tv_usec > StartTime.tv_usec)
	        microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
        else
	        microsecs-=(StartTime.tv_usec - StopTime.tv_usec);
	    readtime += microsecs;
	    
	    
	    // printf("CAME HERE");
	    // Initialize threads for starting frame transformations
	    gettimeofday(&StartTime, 0);
	    for(thread_idx=0; thread_idx<(NUM_ROW_THREADS*NUM_COL_THREADS); thread_idx++)
        {

            if(thread_idx == 0) {idx=1; jdx=1;}
            if(thread_idx == 1) {idx=1; jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 2) {idx=1; jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 3) {idx=1; jdx=(thread_idx*(IMG_W_SLICE-1));}
    
            if(thread_idx == 4) {idx=IMG_H_SLICE; jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 5) {idx=IMG_H_SLICE; jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 6) {idx=IMG_H_SLICE; jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 7) {idx=IMG_H_SLICE; jdx=(thread_idx*(IMG_W_SLICE-1));}

            if(thread_idx == 8) {idx=(2*(IMG_H_SLICE-1)); jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 9) {idx=(2*(IMG_H_SLICE-1)); jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 10) {idx=(2*(IMG_H_SLICE-1)); jdx=(thread_idx*(IMG_W_SLICE-1));}
            if(thread_idx == 11) {idx=(2*(IMG_H_SLICE-1)); jdx=(thread_idx*(IMG_W_SLICE-1));}

            //printf("idx=%d, jdx=%d\n", idx, jdx);

            threadarg[thread_idx].i=idx;      
            threadarg[thread_idx].h=IMG_H_SLICE-1;        
            threadarg[thread_idx].j=jdx;        
            threadarg[thread_idx].w=IMG_W_SLICE-1;

            //printf("create thread_idx=%d\n", thread_idx);    
            pthread_create(&threads[thread_idx], (void *)0, threaded_sobel, (void *)&threadarg[thread_idx]);

        }

        for(thread_idx=0; thread_idx<(NUM_ROW_THREADS*NUM_COL_THREADS); thread_idx++)
        {
            //printf("join thread_idx=%d\n", thread_idx);    
            if((pthread_join(threads[thread_idx], (void **)0)) < 0)
                perror("pthread_join");
        }
        gettimeofday(&StopTime, 0);
        microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000) ;
        if(StopTime.tv_usec > StartTime.tv_usec)
	        microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
        else
	        microsecs-=(StartTime.tv_usec - StopTime.tv_usec);
	    cputime += (double)microsecs;
        
        
        gettimeofday(&StartTime, 0);
        write(fdout, (void *)header, 21);
        // Write RGB data
        for(i=0; i<IMG_HEIGHT; i++)
        {
            for(j=0; j<IMG_WIDTH; j+=1)
            {
                write(fdout, (void *)&convR[i][j], 1);
                write(fdout, (void *)&convG[i][j], 1);
                write(fdout, (void *)&convB[i][j], 1);
            }
        }
        gettimeofday(&StopTime, 0);
        microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000) ;
        if(StopTime.tv_usec > StartTime.tv_usec)
	        microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
        else
	        microsecs-=(StartTime.tv_usec - StopTime.tv_usec);
	    writetime += (double)microsecs;  
    
        close(fdin);
        close(fdout); 
    
    }
    
    printf("\n-----------\n");
    printf("Total read time for files: %f ms\n", readtime/1000);
    printf("Total write time for files: %f ms\n", writetime/1000);
    printf("Total single thread Sobel computation time: %f ms\n", cputime/1000);
}
