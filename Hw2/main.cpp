#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>

int g_windowWidth = 640;
int g_windowHeight = 480;

bool shouldExit = false;

using namespace std;

// Forward declarations
static void clearBackground();

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
	SDL_Window* window = SDL_CreateWindow(
	"SDL Test",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	g_windowHeight, g_windowHeight,
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
	
	// The game loop
	while( !shouldExit ) 
	{
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
		
		clearBackground();
		// All calls to glDraw go here

		//===========================
		SDL_GL_SwapWindow( window );
	}

	SDL_Quit();
	return 0;
}

static void clearBackground()
{
	float r,g,b;
	r = 0;
	g = 0;
	b = 0;
	glClearColor(r,g,b,1);
	glClear(GL_COLOR_BUFFER_BIT);
}