all: sobel sobel_frames sobel_threaded

sobel: sobel.c
	gcc -o sobel sobel.c
    
sobel_frames: sobel_frames.c
	gcc -O3 -mssse3 -malign-double -o sobel_frames sobel_frames.c

sobel_threaded: sobel_threaded.c
	gcc -O3 -mssse3 -malign-double -o sobel_threaded sobel_threaded.c -lpthread

clean:
	rm -r sobel sobel_threaded sobel_frames
