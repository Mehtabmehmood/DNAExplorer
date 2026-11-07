//==============================================================================
/*
    Software License Agreement (BSD License)
    Copyright (c) 2003-2024, CHAI3D
    (www.chai3d.org)

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.


    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    * Neither the name of CHAI3D nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    \author    <http://www.chai3d.org>
    \author    Francois Conti
    \version   3.3.0
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// STATES
//------------------------------------------------------------------------------
enum MouseState
{
    MOUSE_IDLE,
    MOUSE_SELECTION
};


//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a viewport to display the scene viewed by the camera
cViewport* viewport = nullptr;

// a light source to illuminate the objects in the world
cDirectionalLight* light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a virtual tool representing the haptic device in the scene
cToolCursor* tool;

// some objects
cMesh* mesh;
cShapeSphere* sphere;
cShapeBox* box;
cShapeCylinder* cylinder;
cShapeTorus* torus;
cShapeLine* line1;
cShapeLine* line2;

// a small sphere which displays the position of a click hit in the world
cShapeSphere* sphereSelect;

// a small line to display the surface normal at the selection point
cShapeLine* normalSelect;

// a pointer to the selected object
cGenericObject* selectedObject;


// DNA molecule reference for identification
cMultiMesh* adenineMolecule = nullptr;

// References for identifying the other DNA molecules
cMultiMesh* thymineMolecule = nullptr;
cMultiMesh* guanineMolecule = nullptr;
cMultiMesh* cytosineMolecule = nullptr;

// offset between the position of the mmouse click on the object and the object reference frame location.
cVector3d selectedObjectOffset;

// position of mouse click.
cVector3d selectedPoint;

// a colored background
cBackground* background;

// a font for rendering text
cFontPtr font;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelRates;

// a label to explain what is happening
cLabel* labelMessage;

// a widget panel
cPanel* panel;

// some labels
cLabel* labelRed;
cLabel* labelGreen;
cLabel* labelBlue;
cLabel* labelOrange;
cLabel* labelGray;

// a flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// a flag to indicate if the haptic simulation has terminated
bool simulationFinished = true;

// mouse position
double mouseX, mouseY;

// mouse state
MouseState mouseState = MOUSE_IDLE;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = nullptr;

// current size of GLFW window
int windowW = 0;
int windowH = 0;

// current size of GLFW framebuffer
int framebufferW = 0;
int framebufferH = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window is resized
void onWindowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when the window framebuffer is resized
void onFrameBufferSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void onErrorCallback(int a_error, const char* a_description);

// callback when a key is pressed
void onKeyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// callback when window content scaling is modified
void onWindowContentScaleCallback(GLFWwindow* a_window, float a_xscale, float a_yscale);

// callback to handle mouse click
void onMouseButtonCallback(GLFWwindow* a_window, int a_button, int a_action, int a_mods);

// callback to handle mouse motion
void onMouseMotionCallback(GLFWwindow* a_window, double a_posX, double a_posY);

// this function renders the scene
void renderGraphics(void);

// this function contains the main haptics simulation loop
void renderHaptics(void);

// this function closes the application
void close(void);


//==============================================================================
/*
    DEMO:   07-mouse-select.cpp

    This application illustrates how the computer mouse can be used to select
    widgets and objects in the virtual world.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "DNA EXPLORER - MY VERSION" << endl;
    cout << "Copyright 2003-2024" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;


    //--------------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set GLFW error callback
    glfwSetErrorCallback(onErrorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    windowW = 0.8 * mode->height;
    windowH = 0.5 * mode->height;
    int x = 0.5 * (mode->width - windowW);
    int y = 0.5 * (mode->height - windowH);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // enable double buffering
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    // set the desired number of samples to use for multisampling
    glfwWindowHint(GLFW_SAMPLES, 4);

    // specify that window should be resized based on monitor content scale
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(windowW, windowH, "DNAExplorer", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // set GLFW key callback
    glfwSetKeyCallback(window, onKeyCallback);

    // set GLFW window size callback
    glfwSetWindowSizeCallback(window, onWindowSizeCallback);

    // set GLFW framebuffer size callback
    glfwSetFramebufferSizeCallback(window, onFrameBufferSizeCallback);

    // set GLFW window content scaling callback
    glfwSetWindowContentScaleCallback(window, onWindowContentScaleCallback);

    // set GLFW mouse position callback
    glfwSetCursorPosCallback(window, onMouseMotionCallback);

    // set GLFW mouse button callback
    glfwSetMouseButtonCallback(window, onMouseButtonCallback);

    // get width and height of window
    glfwGetFramebufferSize(window, &framebufferW, &framebufferH);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set window size
    glfwSetWindowSize(window, windowW, windowH);

    // set GLFW current display context
    glfwMakeContextCurrent(window);

    // set GLFW swap interval for the current display context
    glfwSwapInterval(swapInterval);


#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setWhite();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set(cVector3d(0.8, 0.8, 0.6),    // camera position (eye)
        cVector3d(0.0, 0.0, 0.0),    // lookat position (target)
        cVector3d(0.0, 0.0, 1.0));   // direction of the (up) vector

    // set the near and far clipping planes of the camera
    // anything in front or behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.03);
    camera->setStereoFocalLength(1.8);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a light source
    light = new cDirectionalLight(world);

    // attach light to camera
    camera->addChild(light);

    // enable light source
    light->setEnabled(true);

    // position the light source
    light->setLocalPos(0.0, 0.5, 0.0);

    // define the direction of the light beam
    light->setDir(-2.5, -0.8, 0.0);


    //--------------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get access to the first available haptic device
    handler->getDevice(hapticDevice, 0);

    // retrieve information about the current haptic device
    cHapticDeviceInfo hapticDeviceInfo = hapticDevice->getSpecifications();

    // if the haptic devices carries a gripper, enable it to behave like a user switch
    hapticDevice->setEnableGripperUserSwitch(true);

    // create a tool (cursor) and insert into the world
    tool = new cToolCursor(world);
    world->addChild(tool);

    // connect the haptic device to the tool
    tool->setHapticDevice(hapticDevice);

    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(0.9);

    // define the radius of the tool (sphere)
    double toolRadius = 0.01;

    // define a radius for the tool
    tool->setRadius(toolRadius);

    // hide the device sphere. only show proxy.
    tool->setShowContactPoints(true, false);

    // enable if objects in the scene are going to rotate of translate
    // or possibly collide against the tool. If the environment
    // is entirely static, you can set this parameter to "false"
    tool->enableDynamicObjects(true);

    // start the haptic tool
    tool->start();


    //--------------------------------------------------------------------------
    // CREATE OBJECTS
    //--------------------------------------------------------------------------

    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

    // stiffness properties
    double maxStiffness = hapticDeviceInfo.m_maxLinearStiffness / workspaceScaleFactor;

    // create a small sphere to display a selection hit with the mouse
    sphereSelect = new cShapeSphere(0.02);
    world->addChild(sphereSelect);
    sphereSelect->m_material->setRedCrimson();
    sphereSelect->setShowEnabled(false);
    sphereSelect->setGhostEnabled(true);

    normalSelect = new cShapeLine();
    sphereSelect->addChild(normalSelect);
    normalSelect->m_colorPointA.setRedCrimson();
    normalSelect->m_colorPointB.setRedCrimson();
    normalSelect->setShowEnabled(false);
    normalSelect->setGhostEnabled(true);


    ////////////////////////////////////////////////////////////////////////////
   // SHAPE - SPHERE
   ////////////////////////////////////////////////////////////////////////////


    // --------------------------------------------------
   // ADENINE - Original MSc Project (2016)
   // --------------------------------------------------

   // Create the adenine molecule
    cMultiMesh* Adenine = new cMultiMesh();
    adenineMolecule = Adenine;
    world->addChild(Adenine);

    // Position the molecule in our current scene
    Adenine->setLocalPos(-0.18, 0.0, 0.20);

    // Carbon atoms - red
    cMesh* AC = Adenine->newMesh();
    AC->m_material->setRedFireBrick();

    // Hydrogen atoms - blue
    cMesh* AH = Adenine->newMesh();
    AH->m_material->setBlue();

    // Create nitrogen mesh
    cMesh* AN = Adenine->newMesh();
    AN->m_material->setGreen();

    // Original 2016 MSc nitrogen coordinates
    cCreateSphere(AN, 0.7, 32, 32,
        cVector3d(-1.1990, -1.3997, 0.0000));
    // Remaining nitrogen atoms
    cCreateSphere(AN, 0.7, 32, 32,
        cVector3d(0.0337, 1.8594, 0.0000));
    cCreateSphere(AN, 0.7, 32, 32,
        cVector3d(1.9998, 0.4150, 0.0000));
    cCreateSphere(AN, 0.7, 32, 32,
        cVector3d(1.8241, -1.9760, 0.0000));

    // Carbon atoms
    cCreateSphere(AC, 0.7, 32, 32,
        cVector3d(-0.1359, -0.5421, 0.0000));
    cCreateSphere(AC, 0.7, 32, 32,
        cVector3d(-0.7056, 0.7241, 0.0000));
    cCreateSphere(AC, 0.7, 32, 32,
        cVector3d(1.2359, -0.7004, 0.0000));
    cCreateSphere(AC, 0.7, 32, 32,
        cVector3d(-2.3386, -0.6394, 0.0000));
    cCreateSphere(AC, 0.7, 32, 32,
        cVector3d(1.3607, 1.6092, 0.0000));

    // Hydrogen atoms
    cCreateSphere(AH, 0.7, 32, 32,
        cVector3d(-1.1566, -2.4103, 0.0000));
    cCreateSphere(AH, 0.7, 32, 32,
        cVector3d(-3.3225, -1.0864, 0.0000));
    cCreateSphere(AH, 0.7, 32, 32,
        cVector3d(2.0034, 2.4831, 0.0000));
    cCreateSphere(AH, 0.7, 32, 32,
        cVector3d(2.8322, -2.0528, 0.0000));
    cCreateSphere(AH, 0.7, 32, 32,
        cVector3d(1.2503, -2.8082, 0.0000));

    cCreateSphere(AN, 0.7, 32, 32,
        cVector3d(-2.0752, 0.6499, 0.0000));

    // Scale the entire molecule to fit our GLFW scene
    Adenine->scale(0.05);

    // Enable mouse picking for all adenine atoms
    Adenine->createAABBCollisionDetector(0.01);

    // --------------------------------------------------
// THYMINE - Original MSc Project (2016)
// --------------------------------------------------

cMultiMesh* Thymine = new cMultiMesh();
thymineMolecule = Thymine;
world->addChild(Thymine);

// Position thymine separately from adenine
Thymine->setLocalPos(0.18, 0.0, 0.20);

// Atom meshes and original colors
cMesh* TC = Thymine->newMesh();
TC->m_material->setRedFireBrick();

cMesh* TN = Thymine->newMesh();
TN->m_material->setGreen();

cMesh* TH = Thymine->newMesh();
TH->m_material->setBlue();

cMesh* TO = Thymine->newMesh();
TO->m_material->setGrayGainsboro();

// Oxygen atoms
cCreateSphere(TO, 0.7, 32, 32, cVector3d(-1.5193, 1.8067, 0.0));
cCreateSphere(TO, 0.7, 32, 32, cVector3d(2.8394, 0.2913, 0.0));

// Nitrogen atoms
cCreateSphere(TN, 0.7, 32, 32, cVector3d(0.6643, 1.0623, 0.0));
cCreateSphere(TN, 0.7, 32, 32, cVector3d(1.1148, -1.2316, 0.0));

// Carbon atoms
cCreateSphere(TC, 0.7, 32, 32, cVector3d(-1.1612, -0.5432, 0.0));
cCreateSphere(TC, 0.7, 32, 32, cVector3d(-0.7129, 0.8802, 0.0));
cCreateSphere(TC, 0.7, 32, 32, cVector3d(-0.2260, -1.5013, 0.0));
cCreateSphere(TC, 0.7, 32, 32, cVector3d(-2.6300, -0.8189, 0.0));
cCreateSphere(TC, 0.7, 32, 32, cVector3d(1.6310, 0.0545, 0.0));

// Hydrogen atoms
cCreateSphere(TH, 0.7, 32, 32, cVector3d(-0.4787, -2.5553, 0.0));
cCreateSphere(TH, 0.7, 32, 32, cVector3d(1.0014, 2.0209, 0.0));
cCreateSphere(TH, 0.7, 32, 32, cVector3d(1.7564, -2.0191, 0.0));
cCreateSphere(TH, 0.7, 32, 32, cVector3d(-3.1004, -0.3860, 0.0));
cCreateSphere(TH, 0.7, 32, 32, cVector3d(-2.8433, -1.8931, 0.0));

// Scale thymine to match adenine
Thymine->scale(0.05);

// Enable mouse picking
Thymine->createAABBCollisionDetector(0.01);

// --------------------------------------------------
// GUANINE - Original MSc Project (2016)
// --------------------------------------------------

cMultiMesh* Guanine = new cMultiMesh();
world->addChild(Guanine);
guanineMolecule = Guanine;

// Starting position, separate from adenine and thymine
Guanine->setLocalPos(-0.18, -0.35, 0.20);

// Atom meshes
cMesh* GO = Guanine->newMesh();
GO->m_material->setGrayGainsboro();

cMesh* GC = Guanine->newMesh();
GC->m_material->setRedFireBrick();

cMesh* GN = Guanine->newMesh();
GN->m_material->setGreen();

cMesh* GH = Guanine->newMesh();
GH->m_material->setBlue();

// Oxygen atom
cCreateSphere(GO, 0.7, 32, 32, cVector3d(0.2823, 2.6356, 0.0000));

// Nitrogen atoms
cCreateSphere(GN, 0.7, 32, 32, cVector3d(0.6610, -1.4308, -0.0005));
cCreateSphere(GN, 0.7, 32, 32, cVector3d(-2.0357, 0.7135, 0.0002));
cCreateSphere(GN, 0.7, 32, 32, cVector3d(-1.7649, -1.5059, 0.0001));
cCreateSphere(GN, 0.7, 32, 32, cVector3d(1.6520, 0.7775, 0.0002));
cCreateSphere(GN, 0.7, 32, 32, cVector3d(2.9476, -1.1758, 0.0004));

// Carbon atoms
cCreateSphere(GC, 0.7, 32, 32, cVector3d(-0.6876, 0.4956, -0.0001));
cCreateSphere(GC, 0.7, 32, 32, cVector3d(-0.5602, -0.8699, -0.0003));
cCreateSphere(GC, 0.7, 32, 32, cVector3d(0.4151, 1.4175, -0.0002));
cCreateSphere(GC, 0.7, 32, 32, cVector3d(1.7307, -0.5374, -0.0001));
cCreateSphere(GC, 0.7, 32, 32, cVector3d(-2.6403, -0.5199, 0.0003));

// Hydrogen atoms
cCreateSphere(GH, 0.7, 32, 32, cVector3d(0.8216, -2.4326, -0.0003));
cCreateSphere(GH, 0.7, 32, 32, cVector3d(-2.5039, 1.6101, 0.0004));
cCreateSphere(GH, 0.7, 32, 32, cVector3d(-3.7159, -0.6258, 0.0006));
cCreateSphere(GH, 0.7, 32, 32, cVector3d(3.0148, -2.1877, 0.0001));
cCreateSphere(GH, 0.7, 32, 32, cVector3d(3.8151, -0.6504, 0.0008));

// Match the scale of adenine and thymine
Guanine->scale(0.05);

// Enable mouse selection and dragging
Guanine->createAABBCollisionDetector(0.01);

// --------------------------------------------------
// CYTOSINE - Original MSc Project (2016)
// --------------------------------------------------

cMultiMesh* Cytosine = new cMultiMesh();
world->addChild(Cytosine);
cytosineMolecule = Cytosine;

// Starting position, separate from the other bases
Cytosine->setLocalPos(0.18, -0.35, 0.20);

// Atom meshes
cMesh* CO = Cytosine->newMesh();
CO->m_material->setGrayGainsboro();

cMesh* CC = Cytosine->newMesh();
CC->m_material->setRedFireBrick();

cMesh* CN = Cytosine->newMesh();
CN->m_material->setGreen();

cMesh* CH = Cytosine->newMesh();
CH->m_material->setBlue();

// Oxygen atom
cCreateSphere(CO, 0.7, 32, 32,
    cVector3d(2.2134, 1.1188, 0.0001));

// Nitrogen atoms
cCreateSphere(CN, 0.7, 32, 32,
    cVector3d(-0.0595, 0.9816, 0.0001));

cCreateSphere(CN, 0.7, 32, 32,
    cVector3d(-2.4422, 0.9214, 0.0001));

cCreateSphere(CN, 0.7, 32, 32,
    cVector3d(1.2935, -0.9744, 0.0000));

// Carbon atoms
cCreateSphere(CC, 0.7, 32, 32,
    cVector3d(-1.2406, 0.2676, -0.0001));

cCreateSphere(CC, 0.7, 32, 32,
    cVector3d(-1.1643, -1.0726, -0.0002));

cCreateSphere(CC, 0.7, 32, 32,
    cVector3d(1.2088, 0.4056, -0.0003));

cCreateSphere(CC, 0.7, 32, 32,
    cVector3d(0.1908, -1.6478, 0.0003));

// Hydrogen atoms
cCreateSphere(CH, 0.7, 32, 32,
    cVector3d(-0.1331, 1.9946, 0.0003));

cCreateSphere(CH, 0.7, 32, 32,
    cVector3d(-2.0223, -1.7317, -0.0003));

cCreateSphere(CH, 0.7, 32, 32,
    cVector3d(0.2351, -2.7487, 0.0005));

cCreateSphere(CH, 0.7, 32, 32,
    cVector3d(-3.3089, 0.3984, 0.0003));

cCreateSphere(CH, 0.7, 32, 32,
    cVector3d(-2.4533, 1.9332, 0.0006));

// Match the scale of the other molecules
Cytosine->scale(0.05);

// Enable mouse selection and dragging
Cytosine->createAABBCollisionDetector(0.01);

    //// Bond between gray sphere and red carbon sphere
    //cShapeCylinder* bond = new cShapeCylinder(0.005, 0.005, 0.06);
    //world->addChild(bond);

    //// Position the bond halfway between the two atoms
    //bond->setLocalPos(0.03, 0.00, 0.20);

    //// Rotate the cylinder so its axis follows the X direction
    //bond->rotateAboutLocalAxisDeg(cVector3d(0, 1, 0), 90);

    //// Bond colour
    //bond->m_material->setGrayDark();



    // set default selected object
    selectedObject = torus;


    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    font = NEW_CFONT_CALIBRI_20();

    // create a label to display the haptic and graphic rate of the simulation
    labelRates = new cLabel(font);
    camera->m_frontLayer->addChild(labelRates);

    // set font color
    labelRates->m_fontColor.setBlack();

    // create a background
    background = new cBackground();
    camera->m_backLayer->addChild(background);

    // set background properties
    background->setCornerColors(cColorf(1.0, 1.0, 1.0),
        cColorf(1.0, 1.0, 1.0),
        cColorf(0.8, 0.8, 0.8),
        cColorf(0.8, 0.8, 0.8));

    // a widget panel
    panel = new cPanel();
    camera->m_frontLayer->addChild(panel);
    panel->setSize(100, 115);
    panel->m_material->setGrayDim();
    panel->setTransparencyLevel(0.8);
    // Hide the original CHAI3D color-selection panel
    panel->setShowEnabled(false);

    // create some labels
    labelRed = new cLabel(font);
    panel->addChild(labelRed);
    labelRed->setText("red");
    labelRed->setLocalPos(15, 10, 0.1);
    labelRed->m_fontColor.setWhite();

    labelGreen = new cLabel(font);
    panel->addChild(labelGreen);
    labelGreen->setText("green");
    labelGreen->setLocalPos(15, 30, 0.1);
    labelGreen->m_fontColor.setWhite();

    labelBlue = new cLabel(font);
    panel->addChild(labelBlue);
    labelBlue->setText("blue");
    labelBlue->setLocalPos(15, 50, 0.1);
    labelBlue->m_fontColor.setWhite();

    labelOrange = new cLabel(font);
    panel->addChild(labelOrange);
    labelOrange->setText("orange");
    labelOrange->setLocalPos(15, 70, 0.1);
    labelOrange->m_fontColor.setWhite();

    labelGray = new cLabel(font);
    panel->addChild(labelGray);
    labelGray->setText("gray");
    labelGray->setLocalPos(15, 90, 0.1);
    labelGray->m_fontColor.setWhite();

    // create a label with a small message
    labelMessage = new cLabel(font);
    camera->m_frontLayer->addChild(labelMessage);

    // set font color
    labelMessage->m_fontColor.setBlack();

    // set text message
    labelMessage->setText("DNA Explorer | Drag molecules with the mouse");

    // DNA molecule identification labels
    cLabel* labelAdenine = new cLabel(font);
    camera->m_frontLayer->addChild(labelAdenine);
    labelAdenine->setText("A = Adenine");
    labelAdenine->m_fontColor.setBlack();
    labelAdenine->setLocalPos(20, 75);

    cLabel* labelThymine = new cLabel(font);
    camera->m_frontLayer->addChild(labelThymine);
    labelThymine->setText("T = Thymine");
    labelThymine->m_fontColor.setBlack();
    labelThymine->setLocalPos(20, 50);

    // Guanine identification label
    cLabel* labelGuanine = new cLabel(font);
    camera->m_frontLayer->addChild(labelGuanine);
    labelGuanine->setText("G = Guanine");
    labelGuanine->m_fontColor.setBlack();
    labelGuanine->setLocalPos(20, 100);

    // Cytosine identification label
    cLabel* labelCytosine = new cLabel(font);
    camera->m_frontLayer->addChild(labelCytosine);
    labelCytosine->setText("C = Cytosine");
    labelCytosine->m_fontColor.setBlack();
    labelCytosine->setLocalPos(20, 125);

    //--------------------------------------------------------------------------
    // VIEWPORT DISPLAY
    //--------------------------------------------------------------------------

    // get content scale factor
    float contentScaleW, contentScaleH;
    glfwGetWindowContentScale(window, &contentScaleW, &contentScaleH);

    // create a viewport to display the scene.
    viewport = new cViewport(camera, contentScaleW, contentScaleH);


    //--------------------------------------------------------------------------
    // START HAPTIC SIMULATION THREAD
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(renderHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // render graphics
        renderGraphics();

        // process events
        glfwPollEvents();
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void onWindowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    windowW = a_width;
    windowH = a_height;

    // render scene
    renderGraphics();
}

//------------------------------------------------------------------------------

void onFrameBufferSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update frame buffer size
    framebufferW = a_width;
    framebufferH = a_height;
}

//------------------------------------------------------------------------------

void onWindowContentScaleCallback(GLFWwindow* a_window, float a_xscale, float a_yscale)
{
    // update window content scale factor
    viewport->setContentScale(a_xscale, a_yscale);
}

//------------------------------------------------------------------------------

void onErrorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void onKeyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
        }

        // set the desired swap interval and number of samples to use for multisampling
        glfwSwapInterval(swapInterval);
        glfwWindowHint(GLFW_SAMPLES, 4);
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
}

//------------------------------------------------------------------------------

void onMouseButtonCallback(GLFWwindow* a_window, int a_button, int a_action, int a_mods)
{
    if (a_button == GLFW_MOUSE_BUTTON_LEFT && a_action == GLFW_PRESS)
    {
        // store mouse position
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // variable for storing collision information
        cCollisionRecorder recorder;
        cCollisionSettings settings;

        // detect for any collision between mouse and front layer widgets
        bool hit = viewport->selectFrontLayer(mouseX, (windowH - mouseY), windowW, windowH, recorder, settings);
        if (hit)
        {
            // reset all label font colors to white
            labelRed->m_fontColor.setWhite();
            labelGreen->m_fontColor.setWhite();
            labelBlue->m_fontColor.setWhite();
            labelOrange->m_fontColor.setWhite();
            labelGray->m_fontColor.setWhite();

            // check mouse selection
            if (recorder.m_nearestCollision.m_object == labelRed)
            {
                labelRed->m_fontColor.setBlack();
                if (selectedObject != NULL)
                    selectedObject->m_material->setRedCrimson();
            }
            else if (recorder.m_nearestCollision.m_object == labelGreen)
            {
                labelGreen->m_fontColor.setBlack();
                if (selectedObject != NULL)
                    selectedObject->m_material->setGreenLightSea();
            }
            else if (recorder.m_nearestCollision.m_object == labelBlue)
            {
                labelBlue->m_fontColor.setBlack();
                if (selectedObject != NULL)
                    selectedObject->m_material->setBlueCornflower();
            }
            else if (recorder.m_nearestCollision.m_object == labelOrange)
            {
                labelOrange->m_fontColor.setBlack();
                if (selectedObject != NULL)
                    selectedObject->m_material->setOrangeRed();
            }
            else if (recorder.m_nearestCollision.m_object == labelGray)
            {
                labelGray->m_fontColor.setBlack();
                if (selectedObject != NULL)
                    selectedObject->m_material->setGrayLight();
            }
        }
        else
        {
            // detect for any collision between mouse and world
            hit = camera->selectWorld(mouseX, (windowH - mouseY), windowW, windowH, recorder, settings);
            if (hit)
            {
                sphereSelect->setShowEnabled(true);
                normalSelect->setShowEnabled(true);
                selectedPoint = recorder.m_nearestCollision.m_globalPos;
                sphereSelect->setLocalPos(selectedPoint);
                normalSelect->m_pointA.zero();
                normalSelect->m_pointB = 0.1 * recorder.m_nearestCollision.m_globalNormal;
                // Select the entire molecule when clicking one of its atoms
                cGenericObject* clickedObject = recorder.m_nearestCollision.m_object;

                if (clickedObject->getParent() != nullptr &&
                    clickedObject->getParent()->getParent() == world)
                {
                    selectedObject = clickedObject->getParent();
                }
                else
                {
                    selectedObject = clickedObject;
                }

                // Identify the selected DNA molecule
                if (selectedObject == adenineMolecule)
                {
                    labelMessage->setText("Adenine (A) | Pairs with Thymine (T)");
                }
                else if (selectedObject == thymineMolecule)
                {
                    labelMessage->setText("Thymine (T) | Pairs with Adenine (A)");
                }
                else if (selectedObject == guanineMolecule)
                {
                    labelMessage->setText("Guanine (G) | Pairs with Cytosine (C)");
                }
                else if (selectedObject == cytosineMolecule)
                {
                    labelMessage->setText("Cytosine (C) | Pairs with Guanine (G)");
                }
                selectedObjectOffset = recorder.m_nearestCollision.m_globalPos - selectedObject->getLocalPos();
                mouseState = MOUSE_SELECTION;
            }
        }
    }
    else
    {
        mouseState = MOUSE_IDLE;
    }
}

//------------------------------------------------------------------------------

void onMouseMotionCallback(GLFWwindow* a_window, double a_posX, double a_posY)
{
    if ((selectedObject != NULL) && (mouseState == MOUSE_SELECTION))
    {
        // get the vector that goes from the camera to the selected point (mouse click)
        cVector3d vCameraObject = selectedPoint - camera->getLocalPos();

        // get the vector that point in the direction of the camera. ("where the camera is looking at")
        cVector3d vCameraLookAt = camera->getLookVector();

        // compute the angle between both vectors
        double angle = cAngle(vCameraObject, vCameraLookAt);

        // compute the distance between the camera and the plane that intersects the object and 
        // which is parallel to the camera plane
        double distanceToObjectPlane = vCameraObject.length() * cos(angle);

        // convert the pixel in mouse space into a relative position in the world
        double factor = (distanceToObjectPlane * tan(0.5 * camera->getFieldViewAngleRad())) / (0.5 * windowH);
        double posRelX = factor * (a_posX - (0.5 * windowW));
        double posRelY = factor * ((windowH - a_posY) - (0.5 * windowH));

        // compute the new position in world coordinates
        cVector3d pos = camera->getLocalPos() +
            distanceToObjectPlane * camera->getLookVector() +
            posRelX * camera->getRightVector() +
            posRelY * camera->getUpVector();

        // compute position of object by taking in account offset
        cVector3d posObject = pos - selectedObjectOffset;

        // apply new position to object
        selectedObject->setLocalPos(posObject);

        // Magnetic snapping between Adenine and Thymine
        if (adenineMolecule != nullptr && thymineMolecule != nullptr)
        {
            if (selectedObject == adenineMolecule ||
                selectedObject == thymineMolecule)
            {
                cVector3d posA = adenineMolecule->getLocalPos();
                cVector3d posT = thymineMolecule->getLocalPos();

                double distance = cDistance(posA, posT);

                const double snapDistance = 0.30;

                // Only snap when the molecules are approaching
                if (distance < snapDistance)
                {
                    // Keep the molecules side by side instead of overlapping
                    if (selectedObject == adenineMolecule)
                    {
                        adenineMolecule->setLocalPos(
                            posT + cVector3d(-0.20, 0.0, 0.0));
                    }
                    else
                    {
                        thymineMolecule->setLocalPos(
                            posA + cVector3d(0.20, 0.0, 0.0));
                    }

                    labelMessage->setText(
                        "Adenine + Thymine | Magnetic pair formed!");
                }
            }
        }
        // Magnetic snapping between Guanine and Cytosine
        if (guanineMolecule != nullptr && cytosineMolecule != nullptr)
        {
            if (selectedObject == guanineMolecule ||
                selectedObject == cytosineMolecule)
            {
                cVector3d posG = guanineMolecule->getLocalPos();
                cVector3d posC = cytosineMolecule->getLocalPos();

                double distance = cDistance(posG, posC);

                const double snapDistance = 0.30;

                if (distance < snapDistance)
                {
                    if (selectedObject == guanineMolecule)
                    {
                        guanineMolecule->setLocalPos(
                            posC + cVector3d(-0.20, 0.0, 0.0));
                    }
                    else
                    {
                        cytosineMolecule->setLocalPos(
                            posG + cVector3d(0.20, 0.0, 0.0));
                    }

                    labelMessage->setText(
                        "Guanine + Cytosine | Magnetic pair formed!");
                }
            }
        }

        // place cursor at the position of the mouse click
        sphereSelect->setLocalPos(pos);
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    hapticDevice->close();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------

void renderGraphics(void)
{
    // sanity check
    if (viewport == nullptr) { return; }

    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // get width and height of CHAI3D internal rendering buffer
    int displayW = viewport->getDisplayWidth();
    int displayH = viewport->getDisplayHeight();

    // update haptic and graphic rate data
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates->setLocalPos((int)(0.5 * (displayW - labelRates->getWidth())), 15);

    // update panel position
    panel->setLocalPos(10, (displayH - panel->getHeight()) - 10);

    // update position of label
    labelMessage->setLocalPos((int)(0.5 * (displayW - labelMessage->getWidth())), 40);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    viewport->renderView(framebufferW, framebufferH);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(error);

    // swap buffers
    glfwSwapBuffers(window);

    // signal frequency counter
    freqCounterGraphics.signal(1);
}

//------------------------------------------------------------------------------

void renderHaptics(void)
{
    // simulation in now running
    simulationRunning = true;
    simulationFinished = false;

    // main haptic simulation loop
    while (simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////////
        // HAPTIC RENDERING
        /////////////////////////////////////////////////////////////////////////

        // signal frequency counter
        freqCounterHaptics.signal(1);

        // compute global reference frames for each object
        world->computeGlobalPositions(true);

        // update position and orientation of tool
        tool->updateFromDevice();

        // compute interaction forces
        tool->computeInteractionForces();

        // send forces to haptic device
        tool->applyToDevice();
    }

    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------
