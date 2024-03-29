/****************************************************** 
* Copyright (c):   2014, All Rights Reserved. 
* Project:         CS 116B Homework #2
* File:            MySdlAppliation.cpp 
* Purpose:         Experiment with quadrics, fractals and SDL.
* Start date:      2/23/14 
* Programmer:      Zane Melcho, Jason Hungerford, Cesar I�arrea
* 
****************************************************** 
*/
#define SDL_MAIN_HANDLED
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <stdlib.h>
#include <time.h>
#if __GNUG__
#	include <tr1/memory>
#endif

#include "cvec.h"
#include "matrix4.h"
#include "geometrymaker.h"
#include "ppm.h"
#include "glsupport.h"
#include "rigtform.h"
#include "MySdlApplication.h"

using namespace std;
using namespace tr1;

// Forward Declarations
struct RigidBody;
struct ShaderState;
static const ShaderState& setupShader(int material);
static void fractal1(RigidBody *object, int cycle, int iteration, RigTForm reference);
static void fractal2(RigidBody *object, int cycle, int iteration, RigTForm reference);

// Enum
enum {DIFFUSE, SOLID, TEXTURE, NORMAL, ANISOTROPY, CUBE, FLOWER};

// Constants
static const float G_CAM_ROTATION = 1;
static const bool G_GL2_COMPATIBLE = false;
static const float G_FRUST_MIN_FOV = 60.0;  //A minimal of 60 degree field of view
static const unsigned char* KB_STATE = NULL;
static const int G_NUM_OF_OBJECTS = 2; //Number of objects to be drawn
static const int G_NUM_OF_FRACTAL_ITERATIONS = 10;

static const int G_NUM_SHADERS = 7;
static const char * const G_SHADER_FILES[G_NUM_SHADERS][2] = 
{
	{"./Shaders/basic-gl3.vshader", "./Shaders/diffuse-gl3.fshader"},
	{"./Shaders/basic-gl3.vshader", "./Shaders/solid-gl3.fshader"},
   {"./Shaders/basic-gl3.vshader", "./Shaders/texture-gl3.fshader"},
   {"./Shaders/basic-gl3.vshader", "./Shaders/normal-gl3.fshader"},
	{"./Shaders/basic-gl3.vshader", "./Shaders/anisotropy-gl3.fshader"},
	{"./Shaders/basic-gl3.vshader", "./Shaders/cube-gl3.fshader"},
	{"./Shaders/basic-gl3.vshader", "./Shaders/flower-gl3.fshader"}

};
static const char * const G_SHADER_FILES_GL2[G_NUM_SHADERS][2] = 
{
	{"./Shaders/basic-gl2.vshader", "./Shaders/diffuse-gl2.fshader"},
	{"./Shaders/basic-gl2.vshader", "./Shaders/solid-gl2.fshader"},
   {"./Shaders/basic-gl2.vshader", "./Shaders/texture-gl2.fshader"},
   {"./Shaders/basic-gl2.vshader", "./Shaders/normal-gl2.fshader"},
	{"./Shaders/basic-gl2.vshader", "./Shaders/anisotropy-gl2.fshader"},
	{"./Shaders/basic-gl2.vshader", "./Shaders/cube-gl2.fshader"},
	{"./Shaders/basic-gl2.vshader", "./Shaders/flower-gl2.fshader"}
};

static const float G_FRUST_NEAR = -0.1f;    // near plane
static const float G_FRUST_FAR = -50.0f;    // far plane
static const float G_GROUND_Y = -2.0f;      // y coordinate of the ground
static const float G_GROUND_SIZE = 10.0f;   // half the ground length

// Global variables
int g_windowWidth = 640;
int g_windowHeight = 480;
unsigned char kbPrevState[SDL_NUM_SCANCODES] = {0};

static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;
static bool g_isPopped = false;
int g_fractalPattern[3][G_NUM_OF_FRACTAL_ITERATIONS] = {0};
int g_flowerPattern[3] = {0};

static float g_frustFovY = G_FRUST_MIN_FOV; // FOV in y direction

static shared_ptr<GlTexture> g_tex0, g_tex1, g_tex2;

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

/*-----------------------------------------------*/

// --------- Scene

static Cvec3 g_light1(2.0, 3.0, 14.0);
static Cvec3 g_light2(-2000.0, -3000.0, -5000.0);
// define light positions in world space
static RigTForm g_skyRbt = RigTForm(Cvec3(0.0, 0.25, 4.0));
static RigTForm g_eyeRbt = g_skyRbt;
static Cvec3f g_objectColors[1] = {Cvec3f(1, 0, 0)};

///////////////// END OF G L O B A L S ///////////////////////

struct ShaderState 
{
	GlProgram program;

	// Handles to uniform variables
	GLint h_uLight, h_uLight2;
	GLint h_uProjMatrix;
	GLint h_uModelViewMatrix;
	GLint h_uNormalMatrix;
	GLint h_uColor;
	GLint h_uTexUnit0;
	GLint h_uTexUnit1;
	GLint h_uTexUnit2;

	// Handles to vertex attributes
	GLint h_aPosition;
	GLint h_aNormal;
	GLint h_aTangent;
	GLint h_aTexCoord0;
	GLint h_aTexCoord1;
	GLint h_aTexCoord2;

	/*-----------------------------------------------*/
	ShaderState(const char* vsfn, const char* fsfn) 
	{
		/*	PURPOSE:		Constructor for ShaderState Object 
			RECEIVES:	vsfn - Vertex Shader Filename
							fsfn - Fragement Shader Filename
			RETURNS:		ShaderState object
			REMARKS:		
		*/

		readAndCompileShader(program, vsfn, fsfn);

		const GLuint h = program; // short hand

		// Retrieve handles to uniform variables
		h_uLight = safe_glGetUniformLocation(h, "uLight");
		h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
		h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
		h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
		h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
		h_uColor = safe_glGetUniformLocation(h, "uColor");
		h_uTexUnit0 = safe_glGetUniformLocation(h, "uTexUnit0");
		h_uTexUnit1 = safe_glGetUniformLocation(h, "uTexUnit1");
		h_uTexUnit2 = safe_glGetUniformLocation(h, "uTexUnit2");

		// Retrieve handles to vertex attributes
		h_aPosition = safe_glGetAttribLocation(h, "aPosition");
		h_aNormal = safe_glGetAttribLocation(h, "aNormal");
		h_aTangent = safe_glGetAttribLocation(h, "aTangent");
		h_aTexCoord0 = safe_glGetAttribLocation(h, "aTexCoord0");
		h_aTexCoord1 = safe_glGetAttribLocation(h, "aTexCoord1");
		h_aTexCoord2 = safe_glGetAttribLocation(h, "aTexCoord2");

		if (!G_GL2_COMPATIBLE)
			glBindFragDataLocation(h, 0, "fragColor");
		checkGlErrors();
    }
	/*-----------------------------------------------*/
};
/*-----------------------------------------------*/
struct Geometry
{
	GlBufferObject vbo, texVbo, ibo;
   int vboLen, iboLen;

	/*-----------------------------------------------*/
	Geometry(GenericVertex *vtx, unsigned short *idx, int vboLen, int iboLen)
	{
		/*	PURPOSE:		Constructor for Geometry Object 
			RECEIVES:	vtx - vertex buffer array of Generic vertex
							idx - Index buffer array
							vboLen - Length of Vertex buffer array
							iboLen - Length of Index Buffer array
			RETURNS:		Geometry object
			REMARKS:		 
		*/

		this->vboLen = vboLen;
		this->iboLen = iboLen;

		// Now create the VBO and IBO
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GenericVertex) * vboLen, vtx,
						GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen,
						idx, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, texVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GenericVertex) * vboLen, vtx,
						GL_STATIC_DRAW);
	}
	/*-----------------------------------------------*/
	void draw(const ShaderState& curSS)
    {
		/*	PURPOSE:		Draws an OpenGL object 
			RECEIVES:	curSS - ShaderState to be used when drawing 
			RETURNS:		 
			REMARKS:		 
		*/

      // Enable the attributes used by our shader
		safe_glEnableVertexAttribArray(curSS.h_aPosition);
		safe_glEnableVertexAttribArray(curSS.h_aNormal);
		safe_glEnableVertexAttribArray(curSS.h_aTexCoord0);
		safe_glEnableVertexAttribArray(curSS.h_aTexCoord1);
		safe_glEnableVertexAttribArray(curSS.h_aTexCoord2);

		// bind vbo
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE,
			sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, pos));
		safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE,
				sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, normal));
		glBindBuffer(GL_ARRAY_BUFFER, texVbo);
		safe_glVertexAttribPointer(curSS.h_aTexCoord0, 2, GL_FLOAT, GL_FALSE,
				sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, tex));
		safe_glVertexAttribPointer(curSS.h_aTexCoord1, 2, GL_FLOAT, GL_FALSE,
				sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, tex));
		safe_glVertexAttribPointer(curSS.h_aTexCoord2, 2, GL_FLOAT, GL_FALSE,
				sizeof(GenericVertex), FIELD_OFFSET(GenericVertex, tex));
		// bind ibo
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

		// draw!
		glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

		// Disable the attributes used by our shader
		safe_glDisableVertexAttribArray(curSS.h_aPosition);
		safe_glDisableVertexAttribArray(curSS.h_aNormal);
		safe_glDisableVertexAttribArray(curSS.h_aTexCoord0);
		safe_glDisableVertexAttribArray(curSS.h_aTexCoord1);
		safe_glDisableVertexAttribArray(curSS.h_aTexCoord2);
    }
	/*-----------------------------------------------*/
	void draw(const ShaderState& curSS, Matrix4 MVM)
	{
		/*	PURPOSE:		Draws an OpenGL object with a specific Model View Matrix 
			RECEIVES:	curSS - ShaderState to be used when drawing 
							MVM - Model View Matrix to be drawn against 
			RETURNS:		 
			REMARKS:		 
		*/

		Matrix4 NMVM = normalMatrix(MVM);

		GLfloat glmatrix[16];
		MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
		safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

		NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
		safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);

		draw(curSS);
	}
};
/*-----------------------------------------------*/
struct RigidBody
{
	RigTForm rtf;
	Matrix4 scale;
	RigidBody **children;
	int numOfChildren;
	Cvec3 color;
	Geometry *geom;
	bool isVisible;
	bool isChildVisible;
	string name;
	int material;

	RigidBody()
	{
		/*	PURPOSE:		Constructor for RigidBody object 
			RECEIVES:	 
			RETURNS:		RigidBody object 
			REMARKS:		 
		*/

		rtf = RigTForm();
		scale = Matrix4();
		children = NULL;
		numOfChildren = 0;
		color = Cvec3(.5,.5,.5);
		geom = NULL;
		isVisible = true;
		isChildVisible = true;
		material = SOLID;
	}

	~RigidBody()
	{
		/*	PURPOSE:		Destructor for RigidBody object 
			RECEIVES:	 
			RETURNS:		 
			REMARKS:		 
		*/

		for (int i = 0; i < numOfChildren; i++)
		{
			delete children[i];	
		}
		delete children;
		delete geom;
	}

	RigidBody(RigTForm rtf_, Matrix4 scale_, RigidBody **children_, Geometry *geom_, Cvec3 color_, int material_)
	{
		/*	PURPOSE:		Constructor for RigidBody object 
			RECEIVES:	rtf_ - RigTForm for the RigidBody
							scale_ - Matrix representing RigidBody scale
							children_ - Pointer to an array of Child RigidBody objects
							geom_ - Geometry type of object
							color_ - Color to draw object
							material_ - Type of shader material to draw object with
			RETURNS:		RigidBody object 
			REMARKS:		 
		*/

		rtf = rtf_;
		scale = scale_;
		children = children_;
		numOfChildren = 0;
		geom = geom_;
		color = color_;
		isVisible = true;
		material = material_;
	}

	void drawRigidBody(RigTForm invEyeRbt)
	{
		/*	PURPOSE:		Draw the RigidBody object 
			RECEIVES:	invEyeRbt -  Inverse Eye Frame to use
			RETURNS:		 
			REMARKS:		Recursive starter function
		*/

		RigTForm respectFrame = invEyeRbt;
		draw(respectFrame, Matrix4());
	}

	void draw(RigTForm respectFrame_, Matrix4 respectScale_)
	{
		/*	PURPOSE:		Draws the RigidBody with respect to parent object 
			RECEIVES:	respectFrame_ - Parent Object frame
							respectScale_ - Parent Object scale
			RETURNS:		 
			REMARKS:		 Recursive function
		*/

		const ShaderState& curSS = setupShader(material);
		safe_glUniform3f(curSS.h_uColor, (GLfloat) color[0], (GLfloat) color[1], (GLfloat) color[2]);
	
		// Draw Parent
		RigTForm respectFrame = respectFrame_ * rtf;
		Matrix4 respectScale = respectScale_ * scale;
		Matrix4 MVM = RigTForm::makeTRmatrix(respectFrame, respectScale);

		if (isVisible)
		{
			if (geom != NULL)
				geom->draw(curSS, MVM);
		}

		//Draw Children
		if (isChildVisible)
		{
			for (int i = 0; i < numOfChildren; i++)
			{
				children[i]->draw(respectFrame, respectScale);
			}
		}
		
	}

	void draw(Matrix4 respectFrame_)
	{
		/*	PURPOSE:		Draws the RigidBody with respect to parent Frame 
			RECEIVES:	respectFrame_ - Parent Object frame 
			RETURNS:		 
			REMARKS:		Recursive Function
		*/

		const ShaderState& curSS = setupShader(material);
		safe_glUniform3f(curSS.h_uColor, (GLfloat) color[0], (GLfloat) color[1], (GLfloat) color[2]);
			
		//Draw parent
		Matrix4 respectFrame = respectFrame_ * RigTForm::makeTRmatrix(rtf, scale);
		Matrix4 MVM = respectFrame;

		if (isVisible)
		{
			if (geom != NULL)
				geom->draw(curSS, MVM);
		}

		//Draw Children
		for (int i = 0; i < numOfChildren; i++)
		{
			children[i]->draw(respectFrame);
		}
	}
};
/*-----------------------------------------------*/
static vector<shared_ptr<ShaderState> > g_shaderStates;
// our global shader states
// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_sphere, g_triangle;
static RigidBody g_rigidBodies[G_NUM_OF_OBJECTS]; // Array that holds each Rigid Body Object
static RigidBody g_flower;
/*-----------------------------------------------*/
static void initGround() 
{
	/*	PURPOSE:		Buils the Generic Vertices for the ground 
		RECEIVES:	
		RETURNS:		 
		REMARKS:		 
	*/

	// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
	GenericVertex vtx[4] = 
	{
		GenericVertex(-G_GROUND_SIZE, G_GROUND_Y, -G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
		GenericVertex(-G_GROUND_SIZE, G_GROUND_Y,  G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
		GenericVertex( G_GROUND_SIZE, G_GROUND_Y,  G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
		GenericVertex( G_GROUND_SIZE, G_GROUND_Y, -G_GROUND_SIZE, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0),
	};
	unsigned short idx[] = {0, 1, 2, 0, 2, 3};
	g_ground.reset(new Geometry(&vtx[0], &idx[0], 4, 6));
}
/*-----------------------------------------------*/
static Geometry* initCube()
{
	/*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a cube 
		RECEIVES:	 
		RETURNS:		Geometry - returns Geometry object 
		REMARKS:		 
	*/

	int ibLen, vbLen;
	getCubeVbIbLen(vbLen, ibLen);

	// Temporary storage for cube geometry
	vector<GenericVertex> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makeCube(1, vtx.begin(), idx.begin());
	return new Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static Geometry* initSpheres() 
{
	/*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a sphere
		RECEIVES:	 
		RETURNS:		Geometry - returns Geometry object 
		REMARKS:		 
	*/

	int slices = 20;
	int stacks = 20;
	float radius = 1;
	int ibLen, vbLen;
	getSphereVbIbLen(slices, stacks, vbLen, ibLen);

	// Temporary storage for Sphere geometry
	vector<GenericVertex> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makeSphere(radius, slices, stacks, vtx.begin(), idx.begin());
	return new Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static Geometry* initCylinders()
{
	/*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a cylinder
		RECEIVES:	 
		RETURNS:		Geometry - returns Geometry object 
		REMARKS:		 
	*/

	float radius = 1;
	float height = 1;
	int slices = 20;
	int ibLen, vbLen;
	getCylinderVbIbLen(slices, vbLen, ibLen);

	// Temporary storage for Cylinder geometry
	vector<GenericVertex> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makeCylinder(slices, radius, height, vtx.begin(), idx.begin());
	return new Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static Geometry* initPlane()
{
	/*	PURPOSE:		Sets up index and vertex buffers and calls geometrymaker for a plane 
		RECEIVES:	 
		RETURNS:		Geometry - returns Geometry object 
		REMARKS:		 
	*/

	int ibLen, vbLen;
	getPlaneVbIbLen(vbLen, ibLen);

	// Temporary storage for plane geometry
	vector<GenericVertex> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makePlane(1, vtx.begin(), idx.begin());
	return new Geometry(&vtx[0], &idx[0], vbLen, ibLen);
}
/*-----------------------------------------------*/
static RigidBody* buildCube()
{
	/*	PURPOSE:		Builds a cube object
		RECEIVES:	 
		RETURNS:		RigidBody - Returns RigidBody object containing a cube
		REMARKS:		Cube is inside an invisible container object
	*/

	float width = 1;
	float height = 1;
	float thick = 1;

	RigTForm rigTemp = RigTForm(Cvec3(0, 0, 0));
	Matrix4 scaleTemp = Matrix4();

	// Make container
	RigidBody *container = new RigidBody(RigTForm(), Matrix4(), NULL, initCube(), Cvec3(0.5, 0.5, 0.5), DIFFUSE);
	container->isVisible = false;
	container->name = "container";

	// Make Cube
	rigTemp = RigTForm(Cvec3(0, 0, 0));
	scaleTemp = Matrix4::makeScale(Cvec3(width, height, thick));	

	RigidBody *cube = new RigidBody(rigTemp, scaleTemp, NULL, initCube(), Cvec3(1,0,0), TEXTURE);
	cube->name = "cube";

	//Setup Children
	container->numOfChildren = 1;
	container->children = new RigidBody*[container->numOfChildren];
	container->children[0] = cube;

	return container;

}
/*-----------------------------------------------*/
static void initTextureCube()
{
	/*	PURPOSE:		Creates and addes a textured cube to the array of RigidBody objects
		RECEIVES:	
		RETURNS:		
		REMARKS:		
	*/

	RigidBody *cube;
	cube = buildCube();
	g_rigidBodies[0] = *cube;
}
/*-----------------------------------------------*/
static RigidBody* buildEgg()
{
	/*	PURPOSE:		Builds a Egg object
		RECEIVES:	 
		RETURNS:		RigidBody - Returns RigidBody object containing a cube 
		REMARKS:		Egg is inside an invisible container object
	*/

	float width = 1;
	float height = 1;
	float thick = 1;

	RigTForm rigTemp = RigTForm(Cvec3(0, 0, 0));
	Matrix4 scaleTemp = Matrix4();

	// Make container
	RigidBody *container = new RigidBody(RigTForm(), Matrix4(), NULL, initCube(), Cvec3(0.5, 0.5, 0.5), DIFFUSE);
	container->isVisible = false;
	container->name = "container";

	// Make Egg
	rigTemp = RigTForm(Cvec3(0, 0, 0));
	scaleTemp = Matrix4::makeScale(Cvec3(width, height, thick));	

	RigidBody *egg = new RigidBody(rigTemp, scaleTemp, NULL, initSpheres(), Cvec3(1,0,0), TEXTURE);
	egg->name = "egg";

	//Setup Children
	container->numOfChildren = 1;
	container->children = new RigidBody*[container->numOfChildren];
	container->children[0] = egg;

	return container;

}
/*-----------------------------------------------*/
static void initEgg()
{
	/*	PURPOSE:		Creates and addes a textured egg to the array of RigidBody objects
		RECEIVES:	
		RETURNS:		
		REMARKS:		
	*/

	RigidBody *egg;
	egg = buildEgg();
	g_rigidBodies[0] = *egg;
}
/*-----------------------------------------------*/
static RigidBody* buildPlant()
{
	/*	PURPOSE:		Builds a plant starter for Fractal function
		RECEIVES:	 
		RETURNS:		RigidBody - Returns RigidBody object containing a plant 
		REMARKS:		plant is inside an invisible container object
	*/

	float width = 0.1f;
	float height = 1.5f;
	float thick = 0.1f;

	RigTForm rigTemp = RigTForm(Cvec3(0, 0, 0));
	Matrix4 scaleTemp = Matrix4();

	// Make container
	RigidBody *container = new RigidBody(RigTForm(), Matrix4(), NULL, initCube(), Cvec3(0.5, 0.5, 0.5), DIFFUSE);
	container->isVisible = false;
	container->name = "container";

	// Make Plant
	rigTemp = RigTForm(Cvec3(0, 0, 0));
	scaleTemp = Matrix4::makeScale(Cvec3(width, height, thick));	

	RigidBody *plant = new RigidBody(rigTemp, scaleTemp, NULL, initCylinders(), Cvec3(0,0.7,0), DIFFUSE);
	plant->name = "plant";

	//Setup Children
	container->numOfChildren = 1;
	container->children = new RigidBody*[container->numOfChildren];
	container->children[0] = plant;

	return container;
}
/*-----------------------------------------------*/
static void initPlant()
{
	/*	PURPOSE:		Creates and addes a plant to the array of RigidBody objects
		RECEIVES:	
		RETURNS:		
		REMARKS:		
	*/

	RigidBody *plant;
	plant = buildPlant();
	plant->isChildVisible = false;
	g_rigidBodies[1] = *plant;
}
/*-----------------------------------------------*/
static void flower(RigidBody* object, int cycle, RigTForm reference)
{
	/*	PURPOSE:		Draws a flower texture at position of object
		RECEIVES:	object - A RigidBody to be used for coordinates
		RETURNS:		
		REMARKS:		Recursive end function
	*/

	if (g_flowerPattern[cycle])
	{
		// Draw a plane and put a flower texture on it.
		float width = .4;
		float height = .4;
		float thick = .4;

		RigTForm rigTemp = RigTForm(Cvec3(0, 0, .02));
		Matrix4 scaleTemp = Matrix4();

		// Make flower
		//rigTemp = RigTForm(Cvec3(0, 0, 0));
		scaleTemp = Matrix4::makeScale(Cvec3(width, height, thick));	

		RigidBody *flower = new RigidBody(rigTemp, scaleTemp, NULL, initPlane(), Cvec3(1,1,1), FLOWER);
		flower->name = "flower";

		flower->rtf.setRotation(Quat().makeXRotation(45));

		glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
		glEnable(GL_BLEND);
		flower->drawRigidBody(reference * flower->rtf);
		glDisable(GL_BLEND);
	}
}
/*-----------------------------------------------*/
static void fractalize(RigidBody *object)
{
	/*	PURPOSE:		Draws a RigidBody object in a fractal pattern
		RECEIVES:	object - A RigidBody to be used in the fractal
		RETURNS:		
		REMARKS:		Recursive starter function
	*/

	// Make object visible & Save current size and shape
	object->isChildVisible = true;
	RigTForm tempRTF = object->rtf;
	Matrix4 tempScale = object->scale;

	// Draw current object
	object->drawRigidBody(inv(g_eyeRbt));

	// Start Fractal Recursion
	for (int i = 0; i < 3; i++)
	{
		int iteration = 0;

		if (g_fractalPattern[i][iteration])
			fractal1(object, i, iteration, inv(g_eyeRbt));
		else
			fractal2(object, i, iteration, inv(g_eyeRbt));

		// Reset RigidBody Object
		object->rtf = tempRTF;
		object->scale = tempScale;
	}
	object->isChildVisible = false;
}
/*-----------------------------------------------*/
static void fractal1(RigidBody *object, int cycle, int iteration, RigTForm reference)
{
	/*	PURPOSE:		Draws a RigidBody object in a fractal pattern
		RECEIVES:	object - A RigidBody to be used in the fractal
						iteration - Number of steps taken for fractal
		RETURNS:		
		REMARKS:		Recursive function
	*/

	if (iteration >= G_NUM_OF_FRACTAL_ITERATIONS)
		flower(object, cycle, reference);
	else
	{
		reference = reference * object->rtf;

		// Fractal function
		//Cvec3 temp = object->rtf.getTranslation();		
		Cvec3 temp = Cvec3(0,0,0);
		float xScale = object->scale[0];
		float yScale = object->scale[5];
		//temp[0] = -abs(xScale * 0.3); // Move a scaled amount -x
		temp[1] = abs(yScale * 0.8); // Move a scaled amount +y
		object->rtf.setTranslation(temp);
		
		object->scale *= Matrix4().makeScale(Cvec3(0.75,0.75,0.75));
		float scaledRotation = 30 * xScale;
		object->rtf.setRotation(object->rtf.getRotation() * Quat().makeZRotation(scaledRotation));
		
		object->drawRigidBody(reference);

		// Recursive Call
		iteration++;
		if (g_fractalPattern[cycle][iteration])
			fractal1(object, cycle, iteration, reference);
		else
			fractal2(object, cycle, iteration, reference);
	}
}
/*-----------------------------------------------*/
static void fractal2(RigidBody *object, int cycle, int iteration, RigTForm reference)
{
	/*	PURPOSE:		Draws a RigidBody object in a fractal pattern
		RECEIVES:	object - A RigidBody to be used in the fractal
						iteration - Number of steps taken for fractal
		RETURNS:		
		REMARKS:		Recursive function
	*/

	if (iteration >= G_NUM_OF_FRACTAL_ITERATIONS)
		flower(object, cycle, reference);
	else
	{
		reference = reference * object->rtf;

		// Fractal function
		//Cvec3 temp = object->rtf.getTranslation();		
		Cvec3 temp = Cvec3(0,0,0);		
		float xScale = object->scale[0];
		float yScale = object->scale[5];
		//temp[0] = abs(xScale * 0.3); // Move a scaled amount -x
		temp[1] = abs(yScale * 0.8); // Move a scaled amount +y
		object->rtf.setTranslation(temp);
		
		object->scale *= Matrix4().makeScale(Cvec3(0.75,0.75,0.75));
		float scaledRotation = 360 - (30 * xScale);
		object->rtf.setRotation(object->rtf.getRotation() * Quat().makeZRotation(scaledRotation));
		
		object->drawRigidBody(reference);

		// Recursive Call
		iteration++;
		if (g_fractalPattern[cycle][iteration])
			fractal1(object, cycle, iteration, reference);
		else
			fractal2(object, cycle, iteration, reference);
	}
}
/*-----------------------------------------------*/
static void sendProjectionMatrix(const ShaderState& curSS,
                                 const Matrix4& projMatrix)
{
	/*	PURPOSE:		Sends projection matrix to OpenGL for shader use 
		RECEIVES:	curSS - The current ShaderState to be used for drawing
						projMatrix - The projection matrix to be used
		RETURNS:		 
		REMARKS:		 
	*/

	// takes a projection matrix and send to the the shaders
	GLfloat glmatrix[16];
	projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
	safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}
/*-----------------------------------------------*/
static void sendModelViewNormalMatrix(const ShaderState& curSS,
                                      const Matrix4& MVM, const Matrix4& NMVM)
{
	/*	PURPOSE:		Sends the regular and normal Model View Matrix to OpenGL for shader use
		RECEIVES:	curSS -	The current shader state to be used to draw 
						MVM -		The model view matrix to be used
						NMVM -	The normal model view matrix to be used
		RETURNS:		 
		REMARKS:		 
	*/

	// takes MVM and its normal matrix to the shaders
	GLfloat glmatrix[16];
	MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
	safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

	NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
	safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
}
/*-----------------------------------------------*/
static void updateFrustFovY()
{
	/*	PURPOSE:		Updates the Frustum field of view 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	if (g_windowWidth >= g_windowHeight)
		g_frustFovY = G_FRUST_MIN_FOV;
	else 
	{
		const float RAD_PER_DEG = 0.5f * (float) CS175_PI/180.0f;
		g_frustFovY = (float) atan2( (float) sin(G_FRUST_MIN_FOV * RAD_PER_DEG) * (float) g_windowHeight
									/ (float) g_windowWidth,
									(float) cos(G_FRUST_MIN_FOV * RAD_PER_DEG)) / RAD_PER_DEG;
	}
}
/*-----------------------------------------------*/
static void initCamera()
{
	/*	PURPOSE:		Initializes the camera position
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	Cvec3 eye = Cvec3(0.0, 2.0, 5.0);
	g_skyRbt.setTranslation(eye);
	g_eyeRbt = g_skyRbt;
}
/*-----------------------------------------------*/
static Matrix4 makeProjectionMatrix()
{
	/*	PURPOSE:		Builds a projection matrix for the scene 
		RECEIVES:	 
		RETURNS:		Matrix4 - Matrix corresponding to the Projection 
		REMARKS:		 
	*/

	return Matrix4::makeProjection(g_frustFovY,
        g_windowWidth / static_cast <double> (g_windowHeight),
        G_FRUST_NEAR, G_FRUST_FAR);
}
/*-----------------------------------------------*/
static void initGLState()
{
	/*	PURPOSE:		Initializes OpenGL 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	glClearColor((GLclampf)(128./255.), (GLclampf) (200./255.), (GLclampf) (255./255.), (GLclampf) 0.);
	glClearDepth(0.);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glReadBuffer(GL_BACK);
	if (!G_GL2_COMPATIBLE)
		glEnable(GL_FRAMEBUFFER_SRGB);
}
/*-----------------------------------------------*/
static void initShaders()
{
	/* PURPOSE:		Initializes Shaders to be used 
		RECEIVES:	 
		RETURNS:     
		REMARKS:     
	*/

	g_shaderStates.resize(G_NUM_SHADERS);
   for (int i = 0; i < G_NUM_SHADERS; ++i) 
	{
		if (G_GL2_COMPATIBLE) 
		{
			g_shaderStates[i].reset(new ShaderState(G_SHADER_FILES_GL2[i][0],
																	G_SHADER_FILES_GL2[i][1]));
		} 
		else 
		{
			g_shaderStates[i].reset(new ShaderState(G_SHADER_FILES[i][0],
																	G_SHADER_FILES[i][1]));
		}
	}
}
/*-----------------------------------------------*/
static void initGeometry()
{
	/*	PURPOSE:		Initializes all Geometry objects to be drawn
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	initGround();
	//initTextureCube();
	initEgg();
	initPlant();
}
/*-----------------------------------------------*/
static void loadTexture(GLuint type, GLuint texHandle, const char *filename)
{
	/*	PURPOSE:		Load a texture to use in SDL 
		RECEIVES:	type - Type of texture to load
						texHandle - OpenGL handle to texture
						filename - texture file to load
		RETURNS:		 
		REMARKS:		 
	*/

	int texWidth, texHeight;
	vector<PackedPixel> pixData;

	//    ppmRead(filename, texWidth, texHeight, pixData);
	SDL_Surface* newSurface = IMG_Load(filename); // read in image
	SDL_Surface* returnSurface;
	if(newSurface == NULL) 
	{
		cout << ":_( Surface null" <<endl;
		return;
	}
	returnSurface = SDL_ConvertSurfaceFormat(newSurface,
		SDL_PIXELFORMAT_RGB24, 0); // need to convert to RGB from RGBA
	texWidth = returnSurface->w;
	texHeight = returnSurface->h;
	glActiveTexture(type);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth,
	//                 texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixData[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth,
					texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, returnSurface->pixels);
	SDL_FreeSurface(newSurface);
	checkGlErrors();
}
/*-----------------------------------------------*/
static void loadCubeTexture(GLuint type, GLuint texHandle,
                            const char *ppmFilename1, const char *ppmFilename2,
                            const char *ppmFilename3, const char *ppmFilename4,
                            const char *ppmFilename5, const char *ppmFilename6)
{
	/*	PURPOSE:		Loads textures necessary for a cube map 
		RECEIVES:	type - Type of texture to load
						texHandle - OpenGL handle to texture
						ppmFilename1 - First face texture to load
						ppmFilename2 - Second face texture to load
						ppmFilename3 - Third face texture to load
						ppmFilename4 - Fourth face texture to load
						ppmFilename5 - Fifth face texture to load
						ppmFilename6 - Sixth face texture to load
		RETURNS:		 
		REMARKS:		 
	*/

	int texWidth, texHeight;
	vector<PackedPixel> pixData1, pixData2, pixData3,
	pixData4, pixData5, pixData6;

	ppmRead(ppmFilename1, texWidth, texHeight, pixData1);
	ppmRead(ppmFilename2, texWidth, texHeight, pixData2);
	ppmRead(ppmFilename3, texWidth, texHeight, pixData3);
	ppmRead(ppmFilename4, texWidth, texHeight, pixData4);
	ppmRead(ppmFilename5, texWidth, texHeight, pixData5);
	ppmRead(ppmFilename6, texWidth, texHeight, pixData6);
	glActiveTexture(type);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texHandle);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,
					GL_RGB, texWidth, texHeight, 0,
					GL_RGB, GL_UNSIGNED_BYTE, &pixData1[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0,
					GL_RGB, texWidth, texHeight, 0,
					GL_RGB, GL_UNSIGNED_BYTE, &pixData2[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0,
					GL_RGB, texWidth, texHeight, 0,
					GL_RGB, GL_UNSIGNED_BYTE, &pixData3[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0,
					GL_RGB, texWidth, texHeight, 0,
					GL_RGB, GL_UNSIGNED_BYTE, &pixData4[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0,
					GL_RGB, texWidth, texHeight, 0,
					GL_RGB, GL_UNSIGNED_BYTE, &pixData5[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0,
					GL_RGB, texWidth, texHeight, 0,
					GL_RGB, GL_UNSIGNED_BYTE, &pixData6[0]);
	checkGlErrors();
}
/*-----------------------------------------------*/
static void loadSphereNormalTexture(GLuint type, GLuint texHandle)
{
	/*	PURPOSE:		Creates textures to be used for a normal map 
		RECEIVES:	type - The type of texture
						texHandle - OpenGL handl to texture
		RETURNS:		 
		REMARKS:		 
	*/

	int width = 512, height = 512;
	vector<PackedPixel> pixels;
	float x = 0;
	float y = 0;
	float z = 0;
	float squareRootThree = sqrt((float) 3);
	float invRootThree = 1/squareRootThree;

	pixels.resize(width * height);
	for (int row = height - 1; row >= 0; row--) 
	{
		for (int l = 0; l < width; l++) 
		{
			PackedPixel &p = pixels[row * width + l];
			x = invRootThree * ((float)(row - width/2)/(width/2));
			y = invRootThree * ((float)(l - height/2)/(height/2));
			z = sqrt(1 - x*x - y*y);
			p.r = (unsigned char)(255 * (x + 1)/2);
			p.g = (unsigned char)(255 * (y + 1)/2);
			p.b = (unsigned char)(255 * (z + 1)/2);
		}
	}

	glActiveTexture(type);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, G_GL2_COMPATIBLE ? GL_RGB : GL_SRGB, width,
					height, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);
	checkGlErrors();
}
/*-----------------------------------------------*/
static void initTextures() 
{
	/*	PURPOSE:		Initializes all textures to use with OpenGL 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	g_tex0.reset(new GlTexture());

	loadTexture(GL_TEXTURE0, *g_tex0, "./Images/cool_texture.png");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *g_tex0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	g_tex1.reset(new GlTexture());

	loadSphereNormalTexture(GL_TEXTURE1, *g_tex1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *g_tex1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   
	/*
	g_tex2.reset(new GlTexture());
    
	loadCubeTexture(GL_TEXTURE2, *g_tex2, "./Images/one.ppm", "./Images/two.ppm",
						"./Images/three.ppm", "./Images/four.ppm", "./Images/five.ppm", "./Images/six.ppm");
    
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, *g_tex2);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	*/

	g_tex2.reset(new GlTexture());

	loadTexture(GL_TEXTURE2, *g_tex2, "./Images/flower.png");

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, *g_tex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
/*-----------------------------------------------*/
static void drawStuff()
{
	/*	PURPOSE:		Draws all objects 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	// short hand for current shader state
	const ShaderState& curSS = *g_shaderStates[g_activeShader];

	// build & send proj. matrix to vshader
	const Matrix4 projmat = makeProjectionMatrix();
	sendProjectionMatrix(curSS, projmat);

	// use the skyRbt as the eyeRbt
	//const Matrix4 eyeRbt = g_skyRbt;
	const RigTForm invEyeRbt = inv(g_eyeRbt);

	const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1));
	const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
	// G_LIGHT1 position in eye coordinates

	safe_glUniform3f(curSS.h_uLight, (GLfloat) eyeLight1[0], (GLfloat) eyeLight1[1], (GLfloat) eyeLight1[2]);
	safe_glUniform3f(curSS.h_uLight2, (GLfloat) eyeLight2[0], (GLfloat) eyeLight2[1], (GLfloat) eyeLight2[2]);

	safe_glUniform1i(curSS.h_uTexUnit0, 0);
	safe_glUniform1i(curSS.h_uTexUnit1, 1);
	safe_glUniform1i(curSS.h_uTexUnit2, 2);

	// Draw Ground
	const RigTForm groundRbt = RigTForm();  // identity
	Matrix4 MVM = RigTForm::makeTRmatrix(invEyeRbt * groundRbt, Matrix4());
	Matrix4 NMVM = normalMatrix(MVM);
	sendModelViewNormalMatrix(curSS, MVM, NMVM);
	safe_glUniform3f(curSS.h_uColor, (GLfloat) 0.1, (GLfloat) 0.95, (GLfloat) 0.1); // set color
	if (g_ground != NULL)
		g_ground->draw(curSS);

	// Draw all Rigid body objects
	for (int i = 0; i < G_NUM_OF_OBJECTS; i++)
		g_rigidBodies[i].drawRigidBody(invEyeRbt);

	// Draw Fractal plant
	if (g_isPopped)
		fractalize(&g_rigidBodies[1]);
}
/*-----------------------------------------------*/
static const ShaderState& setupShader(int material)
{
	/*	PURPOSE:		Sets up Shader based on material 
		RECEIVES:	material - enum value of shader to be used 
		RETURNS:		curSS - ShaderState to be used to draw object
		REMARKS:		 
	*/

	// Current Shader State
	glUseProgram(g_shaderStates[material]->program);
	const ShaderState& curSS = *g_shaderStates[material];

	safe_glUniform1i(curSS.h_uTexUnit0, 0);
	safe_glUniform1i(curSS.h_uTexUnit1, 1);
   safe_glUniform1i(curSS.h_uTexUnit2, 2);

	// Build & send proj. matrix to vshader
	const Matrix4 projmat = makeProjectionMatrix();
	sendProjectionMatrix(curSS, projmat);

	// Use the g_eyeRbt as the eyeRbt;
	const RigTForm eyeRbt = g_eyeRbt;
	const RigTForm invEyeRbt = inv(eyeRbt);

	const Cvec3 eyeLight1 = Cvec3(invEyeRbt * Cvec4(g_light1, 1)); // g_light1 position in eye coordinates
	const Cvec3 eyeLight2 = Cvec3(invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
	safe_glUniform3f(curSS.h_uLight, (GLfloat) eyeLight1[0], (GLfloat) eyeLight1[1], (GLfloat) eyeLight1[2]);
	safe_glUniform3f(curSS.h_uLight2, (GLfloat) eyeLight2[0], (GLfloat) eyeLight2[1], (GLfloat) eyeLight2[2]);

	const RigTForm identityRbt = RigTForm();
	Matrix4 MVM = RigTForm::makeTRmatrix(invEyeRbt * identityRbt, Matrix4());
	Matrix4 NMVM = normalMatrix(MVM);

	sendModelViewNormalMatrix(curSS, MVM, NMVM);

	return curSS;
}
/*-----------------------------------------------*/
static void reshape(const int w, const int h) 
{
	/*	PURPOSE:		Resizes everything based on new window dimensions 
		RECEIVES:	w - Width of window
						h - Height of window
		RETURNS:		 
		REMARKS:		 
	*/

	g_windowWidth = w;
	g_windowHeight = h;
	glViewport(0, 0, w, h);
	cerr << "Size of window is now " << w << "x" << h << endl;
	updateFrustFovY();
}
/*-----------------------------------------------*/
void MySdlApplication::keyboard()
{
	/*	PURPOSE:		Handles all keyboard inputs 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		Uses KB_STATE and kbPrevState to evaluate which keys are pressed or held down 
						 
	*/

	if (KB_STATE[SDL_SCANCODE_L])
	{
		Cvec4 toOrigin = (Cvec4) -g_eyeRbt.getTranslation();
		Cvec4 temp = Quat().makeYRotation(-G_CAM_ROTATION) * toOrigin;
		g_eyeRbt.setRotation(g_eyeRbt.getRotation() * Quat().makeYRotation(-G_CAM_ROTATION));
		g_eyeRbt.setTranslation((Cvec3) -temp);
	}
	else if (KB_STATE[SDL_SCANCODE_R])
	{
		Cvec4 toOrigin = (Cvec4) -g_eyeRbt.getTranslation();
		Cvec4 temp = Quat().makeYRotation(G_CAM_ROTATION) * toOrigin;
		g_eyeRbt.setRotation(g_eyeRbt.getRotation() * Quat().makeYRotation(G_CAM_ROTATION));
		g_eyeRbt.setTranslation((Cvec3) -temp);
	}
	else if (KB_STATE[SDL_SCANCODE_O] && !kbPrevState[SDL_SCANCODE_O])
	{
		g_isPopped = false;
		g_rigidBodies[0].isChildVisible = true;

		Matrix4 scaleTemp = Matrix4().makeScale(Cvec3(1.0,1.045,1.0));
		if (g_rigidBodies[0].scale[5] < 1.0)
			g_rigidBodies[0].scale *= scaleTemp;

		// Reset if too High
		if (g_rigidBodies[0].scale[5] >= 1.0)
			g_rigidBodies[0].scale[5] = 1.0;
	}
	else if (KB_STATE[SDL_SCANCODE_P] && !kbPrevState[SDL_SCANCODE_P])
	{
		Matrix4 scaleTemp = Matrix4().makeScale(Cvec3(1.0,0.955,1.0));
		if (g_rigidBodies[0].scale[5] >= 0.5)  // Change back to > instead of >=
			g_rigidBodies[0].scale *= scaleTemp;

		// Reset if too Low
		if (g_rigidBodies[0].scale[5] <= 0.5)
		{
			g_rigidBodies[0].scale[5] = 0.5;

			if (!g_isPopped)
			{
				g_rigidBodies[0].isChildVisible = false;
				g_isPopped = true;
			
				// Initialize fractal Pattern
				for (int j = 0; j < 3; j++)
				{
					g_flowerPattern[j] = rand() % 2;

					for (int i = 0; i < G_NUM_OF_FRACTAL_ITERATIONS; i++)
						g_fractalPattern[j][i] = rand() % 2;
				}
			}
		}
	}
	else if (KB_STATE[SDL_SCANCODE_T] && !kbPrevState[SDL_SCANCODE_T])
	{
		g_rigidBodies[0].children[0]->material++;
		if(g_rigidBodies[0].children[0]->material >= G_NUM_SHADERS) 
			g_rigidBodies[0].children[0]->material = 0;
	}
	else if (KB_STATE[SDL_SCANCODE_ESCAPE])
	{
		running = false;
	}
	else if (KB_STATE[SDL_SCANCODE_1] && !kbPrevState[SDL_SCANCODE_1])
	{
		if (g_rigidBodies[0].isChildVisible)
			g_rigidBodies[0].isChildVisible = false;
		else
			g_rigidBodies[0].isChildVisible = true;
	}
	else if (KB_STATE[SDL_SCANCODE_0] && !kbPrevState[SDL_SCANCODE_0])
	{
		// Cheat button // Remove when done.
		g_rigidBodies[0].scale[5] = 0.5;
	}
}
/*-----------------------------------------------*/
void MySdlApplication::mouse(SDL_MouseButtonEvent button)
{
	/*	PURPOSE:		Handles MouseButton events to set mouse related variables 
		RECEIVES:	button - MouseButtonEvent object that has current state of Mouse 
		RETURNS:		 
		REMARKS:		 
	*/

	g_mouseClickX = button.x;
	g_mouseClickY = g_windowHeight - button.y - 1;

	g_mouseLClickButton |= (button.button == SDL_BUTTON_LEFT &&
									button.state == SDL_PRESSED);
	g_mouseRClickButton |= (button.button == SDL_BUTTON_RIGHT &&
									button.state == SDL_PRESSED);
	g_mouseMClickButton |= (button.button == SDL_BUTTON_MIDDLE &&
									button.state == SDL_PRESSED);

	g_mouseLClickButton &= !(button.button == SDL_BUTTON_LEFT &&
									button.state == SDL_RELEASED);
	g_mouseRClickButton &= !(button.button == SDL_BUTTON_RIGHT &&
									button.state == SDL_RELEASED);
	g_mouseMClickButton &= !(button.button == SDL_BUTTON_MIDDLE &&
									button.state == SDL_RELEASED);

	g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton ||
		g_mouseMClickButton;
}
/*-----------------------------------------------*/
void MySdlApplication::motion(const int x, const int y)
{
	/*	PURPOSE:		Handles all Mouse related input 
		RECEIVES:	x - x position of the mouse in screen Coordinates
						y - y position of the mouse in screen Coordinates
		RETURNS:		 
		REMARKS:		 
	*/

	const double dx = x - g_mouseClickX;
	const double dy = g_windowHeight - y - 1 - g_mouseClickY;

	RigTForm m;
	if (g_mouseLClickButton && !g_mouseRClickButton) 
	{
		// left button down?
		//m = g_eyeRbt * RigTForm(Quat().makeXRotation(-dy)) * RigTForm(Quat().makeYRotation(dx)) * inv(g_eyeRbt);
		m = g_rigidBodies[0].rtf * RigTForm(Quat().makeXRotation(-dy)) * RigTForm(Quat().makeYRotation(dx)) * inv(g_rigidBodies[0].rtf);
	}
	else if (g_mouseRClickButton && !g_mouseLClickButton) 
	{
		// right button down?
		m = g_eyeRbt * RigTForm(Cvec3(dx, dy, 0) * 0.01) * inv(g_eyeRbt);
	}
	else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton)) 
	{
		// middle or (left and right) button down?
		//m = RigTForm(Cvec3(0, 0, -dy) * 0.01);
		m = g_eyeRbt * RigTForm(Cvec3(0, 0,dy) * 0.01) * inv(g_eyeRbt);
	}

	if (g_mouseClickDown) 
		//g_rigidBodies[0].rtf = m * g_rigidBodies[0].rtf;
		g_eyeRbt = m * g_eyeRbt;

	g_mouseClickX = x;
	g_mouseClickY = g_windowHeight - y - 1;

}
/*-----------------------------------------------*/
void MySdlApplication::onLoop()
{
	/*	PURPOSE:		Handles function calls that need to run once per SDL loop 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	// Logic goes here
	keyboard();
}
/*-----------------------------------------------*/
void MySdlApplication::onRender()
{
	/*	PURPOSE:		Handles all graphics related calls once per SDL loop 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	// All draw calls go here
	glUseProgram(g_shaderStates[g_activeShader]->program);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// clear framebuffer color&depth
	drawStuff();

	SDL_GL_SwapWindow(display);
	checkGlErrors();
}
/*-----------------------------------------------*/
int MySdlApplication::onExecute()
{
	/*	PURPOSE:		Main function loop of MySdlApplication 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	srand((unsigned int) time(NULL));

	if(onInit() == false) 
		return -1;
	
	SDL_Event Event;
	while(running) 
	{
		memcpy (kbPrevState, KB_STATE, sizeof( kbPrevState ));
        
		while(SDL_PollEvent(&Event)) 
		{
			onEvent(&Event);
		}

		onLoop();
		onRender();
	}

	onCleanup();

	return 0;
}
/*-----------------------------------------------*/
bool MySdlApplication::onInit()
{
	/*	PURPOSE:		Initializes SDL 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/
	
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) 
	{
		return false;
   }
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	/* Turn on double buffering with a 24bit Z buffer.
	* You may need to change this to 16 or 32 for your system */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	if((display = SDL_CreateWindow("My SDL Application",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g_windowWidth, g_windowHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)) == NULL) 
	{
		return false;
	}

	/* Create our opengl context and attach it to our window */
	SDL_GLContext maincontext = SDL_GL_CreateContext(display);
	/* This makes our buffer swap syncronized with the 
	monitor's vertical refresh */
	SDL_GL_SetSwapInterval(1);

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
	
	cout << (G_GL2_COMPATIBLE ?
		"Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3")
		<< endl;
	if ((!G_GL2_COMPATIBLE) && !GLEW_VERSION_3_0)
		throw runtime_error(
			"Error: does not support OpenGL Shading Language v1.3");
	else if (G_GL2_COMPATIBLE && !GLEW_VERSION_2_0)
		throw runtime_error(
			"Error: does not support OpenGL Shading Language v1.0");

	initGLState();
	initShaders();
	initGeometry();
	initTextures();
	initCamera();

	KB_STATE = SDL_GetKeyboardState(NULL);

	return true;
}
/*-----------------------------------------------*/
void MySdlApplication::onEvent(SDL_Event* event) 
{
	/*	PURPOSE:		Handles SDL events 
		RECEIVES:	event - SDL Event to be handled 
		RETURNS:		 
		REMARKS:		 
	*/

	Uint32 type = event->type;

	if (type == SDL_QUIT)
		running = false;
	else if (type == SDL_MOUSEBUTTONDOWN)
		mouse(event->button);
	else if (type == SDL_MOUSEBUTTONUP)
		mouse(event->button);
	else if (type == SDL_MOUSEMOTION)
		motion(event->motion.x, event->motion.y);
	else if (type == SDL_WINDOWEVENT)
		if (event->window.event == SDL_WINDOWEVENT_RESIZED)
			reshape(event->window.data1,event->window.data2);
}
/*-----------------------------------------------*/
void MySdlApplication::onCleanup()
{
	/*	PURPOSE:		Everything to be done before program closes 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	SDL_Quit();
}
/*-----------------------------------------------*/
MySdlApplication::MySdlApplication()
{
	/*	PURPOSE:		Constructor for MySdlApplication 
		RECEIVES:	 
		RETURNS:		 
		REMARKS:		 
	*/

	running = true;
}
/*-----------------------------------------------*/
int main(int argc, const char* argv[])
{	
	/*	PURPOSE:		Main function of MySdlApplication 
		RECEIVES:	argc - number of arguments passed
						argv - array of arguments
		RETURNS:		int - Whether or not program ran sucessfully
		REMARKS:		
	*/

	MySdlApplication application;
	return application.onExecute();
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



	/*	PURPOSE:		What does this function do? (must be present) 
		RECEIVES:	List every argument name and explain each argument. 
						(omit if the function has no arguments) 
		RETURNS:		Explain the value returned by the function. 
						(omit if the function returns no value) 
		REMARKS:		Explain any special preconditions or postconditions. 
						See example below. (omit if function is unremarkable) 
	*/