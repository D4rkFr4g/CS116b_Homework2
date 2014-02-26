/****************************************************** 
* Copyright (c):   2014, All Rights Reserved. 
* Project:         CS 116B Homework #2
* File:            main.cpp 
* Purpose:         Experiment with quadrics, fractals and SDL.
* Start date:      2/23/14 
* Programmer:      Zane Melcho, Jason Hungerford, Cesar Iñarrea
* 
****************************************************** 
*/

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>

// Constants
const int G_WINDOW_WIDTH = 640;
const int G_WINDOW_HEIGHT = 480;

// Global variables
bool shouldExit = false;
unsigned char kbPrevState[SDL_NUM_SCANCODES] = {0};
const unsigned char* kbState = NULL;
SDL_Window* window;

using namespace std;

// Forward declarations

/*-----------------------------------------------*/
static void clearBackground()
{
	/* PURPOSE:      What does this function do? (must be present) 
   RECEIVES:   List every argument name and explain each argument. 
               (omit if the function has no arguments) 
   RETURNS:      Explain the value returned by the function. 
               (omit if the function returns no value) 
   REMARKS:      Explain any special preconditions or postconditions. 
               See example below. (omit if function is unremarkable) 
	*/

	float r,g,b;
	r = 0;
	g = 0;
	b = 0;
	glClearColor(r,g,b,1);
	glClear(GL_COLOR_BUFFER_BIT);
}
/*-----------------------------------------------*/
static void keyboard()
{
	/* PURPOSE:      What does this function do? (must be present) 
   RECEIVES:   List every argument name and explain each argument. 
               (omit if the function has no arguments) 
   RETURNS:      Explain the value returned by the function. 
               (omit if the function returns no value) 
   REMARKS:      Explain any special preconditions or postconditions. 
               See example below. (omit if function is unremarkable) 
	*/

	if (kbState[ SDL_SCANCODE_L ])
	{
	}
	else if (kbState[ SDL_SCANCODE_R ])
	{
	}
	else if (kbState[ SDL_SCANCODE_O ])
	{
	}
	else if (kbState[ SDL_SCANCODE_P ])
	{
	}
}
/*-----------------------------------------------*/
static void onRender()
{
	/* PURPOSE:      What does this function do? (must be present) 
   RECEIVES:   List every argument name and explain each argument. 
               (omit if the function has no arguments) 
   RETURNS:      Explain the value returned by the function. 
               (omit if the function returns no value) 
   REMARKS:      Explain any special preconditions or postconditions. 
               See example below. (omit if function is unremarkable) 
	*/

	clearBackground();
	// All draw calls go here


	SDL_GL_SwapWindow(window);
}
/*-----------------------------------------------*/
int main( void )
{	
	// Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) 
	{
		return 1;
	}
 
	// Create the window, OpenGL context
	SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 32 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	window = SDL_CreateWindow(
	"SDL Test",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	G_WINDOW_WIDTH, G_WINDOW_HEIGHT,
	SDL_WINDOW_OPENGL);
	glCullFace( GL_BACK );


	if( !window ) 
	{
		SDL_Quit();
		return 1;
	}
	SDL_GL_CreateContext( window );

	// Make sure we have a recent version of OpenGL
	GLenum glewError = glewInit();
	if( glewError != GLEW_OK ) 
	{
		SDL_Quit();
		return 1;
	}
	if( !GLEW_VERSION_1_5 ) 
	{
		SDL_Quit();
		return 1;
	}

	// Setup calls
	kbState = SDL_GetKeyboardState( NULL );

	// The game loop
	while( !shouldExit ) 
	{
		memcpy (kbPrevState, kbState, sizeof( kbPrevState ));

		// Handle OS message pump
		SDL_Event event;
		while( SDL_PollEvent( &event )) 
		{
			switch( event.type ) 
			{
				case SDL_QUIT:
				return 0;
			}
		}

		// Logic goes here
		keyboard();

		onRender();
	}

	SDL_Quit();
	return 0;
}

//Coding Guidelines template (REMOVE before Submission)

/****************************************************** 
* Copyright (c):   1994, All Rights Reserved. 
* Project:         CS 46A Homework #4 
* File:            sortcomp.cpp 
* Purpose:         compare timings for sort routines 
* Start date:      4/2/97 
* Programmer:      John Chen 
* 
****************************************************** 
*/


/*-----------------------------------------------*/



/* PURPOSE:      What does this function do? (must be present) 
   RECEIVES:   List every argument name and explain each argument. 
               (omit if the function has no arguments) 
   RETURNS:      Explain the value returned by the function. 
               (omit if the function returns no value) 
   REMARKS:      Explain any special preconditions or postconditions. 
               See example below. (omit if function is unremarkable) 
*/