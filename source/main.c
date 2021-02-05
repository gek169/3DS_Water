/*
	Water Rendering demo for the Nintendo 3DS by Gek
*/

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
//This include a header containing definitions of our image
#define CHAD_API_IMPL
#include "api.h"
#define WIDTH 320
#define HEIGHT 240
#define SCREENBYTES (320*240*3)
#define MAX(x,y) (x>y?x:y)
#define MIN(x,y) (x<y?x:y)

#define SAMPLERATE 22050
#define SAMPLESPERBUF (SAMPLERATE / 30)
#define BYTESPERSAMPLE 4

float du[WIDTH][HEIGHT], dv[WIDTH][HEIGHT], dunew[WIDTH][HEIGHT];



void initFluid(){
	for(int x = 0; x < WIDTH; x++)
	for(int y = 0; y < HEIGHT; y++)
	{
		du[x][y] = fabs(sinf(y/5.0f) + cosf(x/5.0f));
		dv[x][y] = 0.0;
		dunew[x][y] = fabs(sinf(y/5.0f) + cosf(x/5.0f));
	}
}
float get_u(int x, int y){
	if(x<0)x=0;
	if(x>=WIDTH)x=WIDTH-1;
	if(y<0)y=0;
	if(y>=HEIGHT)y=HEIGHT-1;
	return du[x][y];
}
void stepFluid(){
	for(int x = 0; x < WIDTH; x++)
		for(int y = 0; y < HEIGHT; y++)
		{
			dv[x][y] += (get_u(x-1,y) + get_u(x+1, y) + get_u(x,y+1) + get_u(x,y-1))*0.25 - get_u(x,y);
			dv[x][y] *= 0.99;
			dunew[x][y] += dv[x][y];
		}
	for(int x = 0; x < WIDTH; x++)
	for(int y = 0; y < HEIGHT; y++)
	du[x][y] = dunew[x][y];
}

//----------------------------------------------------------------------------
void fill_buffer(void *audioBuffer,size_t offset, size_t size, int frequency ) {
//----------------------------------------------------------------------------

	u32 *dest = (u32*)audioBuffer;

	for (int i=0; i<size; i++) {

		s16 sample = INT16_MAX * sin(frequency*(2*M_PI)*(offset+i)/SAMPLERATE);

		dest[i] = (sample<<16) | (sample & 0xffff);
	}

	DSP_FlushDataCache(audioBuffer,size);

}

int myscandir(){
	{ 
	    struct dirent *de;  // Pointer for directory entry 
	  
	    // opendir() returns a pointer of DIR type.  
	    DIR *dr = opendir("romfs:/"); 
	  
	    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
	    { 
	        printf("Could not open current directory" ); 
	        return 0; 
	    } 
	  
	    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
	    // for readdir() 
	    while ((de = readdir(dr)) != NULL) 
	            printf("%s\n", de->d_name); 
	  
	    closedir(dr);     
	    return 0 ; 
	} 
}

int main(int argc, char **argv)
{
	uint RR_=3;
	uint GG_=2;
	uint BB_=1;
	uint AA_=0;
	track* mytrack = NULL;
	//gfxInitDefault();
	gfxInit(GSP_RGBA8_OES,GSP_RGBA8_OES,false);
	fsInit();
	romfsInit();
	init();ainit();
	consoleInit(GFX_TOP, NULL);
	myscandir();
	//mytrack = lmus("romfs:/WWGW.wav");
	mytrack = lmus("romfs:/Strongest.mp3");
		if(!mytrack){
			printf("\nError loading sounds\n");
		}
	if(mytrack)
		mplay(mytrack, -1, 1000);
	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	
	initFluid();
	printf("\nBitmap CPU Rendering demo by Gek!\nThanks Evie!");

	printf("\n\n<Start>: Exit program.\n");

	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	
	

	//Get the bottom screen's frame buffer
	u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	u8* ifb = linearAlloc(320 * 240 * 4); //Enough for the bottom screen.
	for(int i = 0; i < 320 * 240; i++)
		{
			ifb[i*4+RR_] = rand() % 255;//R
		}
	if(!ifb) return 1;
	//Copy our image in the bottom screen's frame buffer
	//memcpy(fb, brew2_bgr, brew2_bgr_size);

	// Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START) break; // break in order to return to hbmenu
		fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
		
		touchPosition touch;
		//Read the touch screen coordinates
		
		if (kHeld & KEY_TOUCH)
		{
			hidTouchRead(&touch);	
			float amt = rand()%100>50?-50:50;
			touch.py = HEIGHT - touch.py;
			dunew[MIN(touch.px,WIDTH-1)][MIN(touch.py,HEIGHT-1)] += amt;
			du[MIN(touch.px,WIDTH-1)][MIN(touch.py,HEIGHT-1)] += amt;
			printf("Detected Touch!\n");
		}
		//Copy our image in the bottom screen's frame buffer
		//memcpy(fb, ifb, 320*240*3);
		//memcpy(fb, brew2_bgr, brew2_bgr_size);
			stepFluid();
			for(int x = 0; x < WIDTH; x++)
			for(int y = 0; y < HEIGHT; y++)
			{
				u8* datum = ifb;
				float valr = MIN(255,80+du[x][y]*50);
				float valg = MIN(255,80+du[x][y]*50);
				float valb = MIN(255,80+du[x][y]*100);
				datum[4 * (y + x * HEIGHT)+RR_] = (u8)valr;
				datum[4 * (y + x * HEIGHT)+GG_] = (u8)valg;
				datum[4 * (y + x * HEIGHT)+BB_] = (u8)valb;
			}
		
		memcpy(fb, ifb, 320*240*4);
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}
	if(ifb) linearFree(ifb);
	
	// Exit services
	
	Mix_Quit();
	SDL_Quit();
	gfxExit();
	return 0;
}
