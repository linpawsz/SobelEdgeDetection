// Swapnil Paratey
// ECEN 5653 - Summer 2015 [Real Time Digital Media]
// Code used to convert individual pixel into a color boundary
// using Sobel egde detection
// Use "gcc -o sobel sobel.c"
// Can also use "gcc -O3 -mssse3 -malign-double -S -o sobel sobel.c" for hardware speedup

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

typedef unsigned int UINT32;
typedef unsigned long long int UINT64;
typedef unsigned char UINT8;
typedef double FLOAT;

#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 720
#define FILTER_RADIUS 1
#define THRESHOLD 20

int main()
{
    int fdin, fdout, bytesLeft, bytesRead;
    int i, j, temp, dx, dy;
    FLOAT sumRX, sumRY, sumGX, sumGY, sumBX, sumBY;
    FLOAT pixelR, pixelG, pixelB;
    const float SobelMatrix[9] = {-1,0,1,-2,0,2,-1,0,1};
    struct timeval StartTime, StopTime;
    unsigned int microsecs;
    
    UINT8 header[22];
    UINT8 R[IMAGE_WIDTH*IMAGE_HEIGHT];
    UINT8 G[IMAGE_WIDTH*IMAGE_HEIGHT];
    UINT8 B[IMAGE_WIDTH*IMAGE_HEIGHT];
    UINT8 convR[IMAGE_WIDTH*IMAGE_HEIGHT];
    UINT8 convG[IMAGE_WIDTH*IMAGE_HEIGHT];
    UINT8 convB[IMAGE_WIDTH*IMAGE_HEIGHT];
    
    if((fdin = open("image3599.ppm", O_RDONLY, 0644)) < 0)
    {
        printf("Error opening image9.ppm\n");
    }

    printf("IMAGE OPENED %d\n", fdin);
    
    if((fdout = open("sobel_image.ppm", (O_RDWR | O_CREAT), 0666)) < 0)
    {
        printf("Error opening sharpen.ppm\n");
    }
    printf("OUTPUT FILE OPENED %d\n", fdout);
    
    bytesLeft=21;
    do
    {
        bytesRead=read(fdin, (void *)header, bytesLeft);
        bytesLeft -= bytesRead;
    } while(bytesLeft > 0);
    header[21]='\0'; 
    
    // Read and assign RGB components of input image
    gettimeofday(&StartTime, 0);
    for(i=0; i<(IMAGE_WIDTH*IMAGE_HEIGHT); i++)
    {
        read(fdin, (void *)&R[i], 1); convR[i]=R[i];
        read(fdin, (void *)&G[i], 1); convG[i]=G[i];
        read(fdin, (void *)&B[i], 1); convB[i]=B[i];
    }
    gettimeofday(&StopTime, 0);
    
    microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000) ;
    if(StopTime.tv_usec > StartTime.tv_usec)
	microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
    else
	microsecs-=(StartTime.tv_usec - StopTime.tv_usec);
    printf("Time required to open the file: %d ms\n", microsecs/1000);
    // Testing the image pixels
    // printf("%d %d %d\n", R[40], G[40], B[40]);
    
    gettimeofday(&StartTime, 0);
    for(i = 1; i < (IMAGE_HEIGHT - 1); i++)
    {
        for(j = 1; j < (IMAGE_WIDTH - 1); j++)
        {
            sumRX = 0; sumRY = 0; sumGX = 0; sumGY = 0; sumBX = 0; sumBY = 0;
            for(dy = -FILTER_RADIUS; dy <= FILTER_RADIUS; dy++) 
            {
                for(dx = -FILTER_RADIUS; dx <= FILTER_RADIUS; dx++) 
                {
                    pixelR = R[i*IMAGE_WIDTH + j +  (dy * IMAGE_WIDTH + dx)];
                    sumRX += pixelR * SobelMatrix[(dy + FILTER_RADIUS) * FILTER_RADIUS * 2 + (dx+FILTER_RADIUS)];
                    sumRY += pixelR * SobelMatrix[(dx + FILTER_RADIUS) * FILTER_RADIUS * 2 + (dy+FILTER_RADIUS)];
                    
                    pixelG = G[i*IMAGE_WIDTH + j +  (dy * IMAGE_WIDTH + dx)];
                    sumGX += pixelG * SobelMatrix[(dy + FILTER_RADIUS) * FILTER_RADIUS * 2 + (dx+FILTER_RADIUS)];
                    sumGY += pixelG * SobelMatrix[(dx + FILTER_RADIUS) * FILTER_RADIUS * 2 + (dy+FILTER_RADIUS)];
                    
                    pixelB = B[i*IMAGE_WIDTH + j +  (dy * IMAGE_WIDTH + dx)];
                    sumBX += pixelB * SobelMatrix[(dy + FILTER_RADIUS) * FILTER_RADIUS * 2 + (dx+FILTER_RADIUS)];
                    sumBY += pixelB * SobelMatrix[(dx + FILTER_RADIUS) * FILTER_RADIUS * 2 + (dy+FILTER_RADIUS)];
                }
                // printf("%f %f\n", sumX, sumY);
            }
            convR[i * IMAGE_WIDTH + j] = (abs(sumRX) + abs(sumRY)) > THRESHOLD ? 255 : 0;
            convG[i * IMAGE_WIDTH + j] = (abs(sumGX) + abs(sumGY)) > THRESHOLD ? 255 : 0;
            convB[i * IMAGE_WIDTH + j] = (abs(sumBX) + abs(sumBY)) > THRESHOLD ? 255 : 0;
        }
    }
    gettimeofday(&StopTime, 0);
    
    microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000) ;
    if(StopTime.tv_usec > StartTime.tv_usec)
	microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
    else
	microsecs-=(StartTime.tv_usec - StopTime.tv_usec);
    printf("Sobel edge detection time: %d ms\n", microsecs/1000);    
    // Write out the final image
    
    gettimeofday(&StartTime, 0);
    write(fdout, (void *)header, 21);
    for(i=0; i<IMAGE_WIDTH*IMAGE_HEIGHT; i++)
    {
        write(fdout, (void *)&convR[i], 1);
        write(fdout, (void *)&convG[i], 1);
        write(fdout, (void *)&convB[i], 1);
    }
    gettimeofday(&StopTime, 0);
    
    microsecs=((StopTime.tv_sec - StartTime.tv_sec)*1000000) ;
    if(StopTime.tv_usec > StartTime.tv_usec)
	microsecs+=(StopTime.tv_usec - StartTime.tv_usec);
    else
	microsecs-=(StartTime.tv_usec - StopTime.tv_usec);
    printf("Time required to write file: %d ms\n", microsecs/1000);
    
    close(fdin);
    close(fdout);
}
