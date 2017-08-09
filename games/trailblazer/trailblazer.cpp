/* Copyright (c) 2011-2014, Cyrille Favreau
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 * This file is part of Sol-R <https://github.com/cyrillefavreau/Sol-R>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// OpenGL Graphics Includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Includes
#ifdef WIN32
#include <windows.h>
#else
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#endif

#include <solr/engines/cuda/CudaKernel.h>

// General Settings
const long TARGET_FPS = 60;
const long REFRESH_DELAY = 1000 / TARGET_FPS; // ms
const bool gUseKinect = false;

// Rendering window vars
const unsigned int draft = 1;
unsigned int window_width = 512;
unsigned int window_height = window_width * 9 / 16;
const unsigned int window_depth = 4;

// Scene
const float gRoomSize = 500.f;
int currentMaterial = 0;

// Levels
const long NB_LEVELS = 1;
const long NB_LEVEL_COLUMNS = 5;
const long NB_LEVEL_LINES = 300;
const long LEVELS_SIZE = NB_LEVEL_COLUMNS * NB_LEVEL_LINES * NB_LEVELS;
int* gLevels = nullptr;

// Scene
int nbPrimitives = 0;
int nbLamps = 0;
int nbMaterials = 0;
int nbTextures = 0;
float transparentColor = 0.1f;
int currentPrimitive = 0;
float step = 10.f;
int powerpoint = 0;
int powerpointSlide = 0;

int gCarpetSize = 5000;
int gCarpetZ = -300; //-2000;

// sphere
vec4f activeSphereCenter[3] = {{0.f, 0.f, 0.f, 50.f}, {0.f, 0.f, 0.f, 80.f}, {0.f, 0.f, 0.f, 70.f}};
vec4f activeSphereDirection[3] = {{4.f, 0.f, 6.f, 10.f}, {-1.f, 0.f, 1.f, 0.f}, {2.f, 0.f, -1.f, 0.f}};
int activeSphereId = -1;
int activeSphereMaterial[3] = {28, 12, 7};

float gSphereSpeed = 20.f;
float gSphereTranslateX = 0.f;
float gSphereTranslateY = 0.f;
float gSphereTranslateZ = -3.0;

int gTickCount = 0.f;

float gEffectDuration = 0.f;
vec4f lampPosition;

// materials
float gReflection = 0.f;
float gRefraction = 1.f;
float gTransparency = 0.f;
float gSpecValue = 1.f;
float gSpecPower = 100.f;
float gSpecCoef = 1.f;
float gInnerIllumination = 0.f;

const size_t MATERIAL_0 = 0;
const size_t MATERIAL_1 = 1;
const size_t MATERIAL_2 = 2;
const size_t MATERIAL_3 = 3;
const size_t MATERIAL_4 = 4;
const size_t MATERIAL_5 = 5;
const size_t MATERIAL_6 = 6;
const size_t MATERIAL_7 = 7;
const size_t MATERIAL_LIGHT = 8;

#ifdef USE_KINECT
const float gSkeletonSize = 200.0;
const float gSkeletonThickness = 20.0;
#endif // USE_KINECT

// OpenGL
GLubyte* ubImage;
int gPreviousFps = 0;

/**
--------------------------------------------------------------------------------
OpenGL
--------------------------------------------------------------------------------
*/

// GL functionality
void initgl(int argc, char** argv);
void display();
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void timerEvent(int value);
void createScene();

// Helpers
void Cleanup(int iExitCode);
void (*pCleanup)(int) = &Cleanup;

// Sim and Auto-Verification parameters
float anim = 0.f;
float dof = 0.f;
bool bNoPrompt = false;

// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;

// Raytracer Module
solr::CudaKernel* cudaKernel = 0;

float getRandomValue(int range, int safeZone, bool allowNegativeValues = true)
{
    float value(static_cast<float>(rand() % range) + safeZone);
    if (allowNegativeValues)
    {
        value *= (rand() % 2 == 0) ? -1 : 1;
    }
    return value;
}

void idle()
{
}

void cleanup()
{
}

/*
--------------------------------------------------------------------------------
setup the window and assign callbacks
--------------------------------------------------------------------------------
*/
void initgl(int argc, char** argv)
{
    size_t len(window_width * window_height * window_depth);
    ubImage = new GLubyte[len];
    memset(ubImage, 0, len);

    glutInit(&argc, (char**)argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 2 - window_width / 2,
                           glutGet(GLUT_SCREEN_HEIGHT) / 2 - window_height / 2);

    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Trail Blazer Revival");

    glutDisplayFunc(display); // register GLUT callback functions
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(REFRESH_DELAY, timerEvent, REFRESH_DELAY);
    return;
}

void TexFunc(void)
{
    glEnable(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ubImage);

    glBegin(GL_QUADS);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(-1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(-1.0, -1.0, 0.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// Display callback
//*****************************************************************************
void display()
{
    // clear graphics
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    TexFunc();
    glFlush();

    glutSwapBuffers();
}

void timerEvent(int value)
{
#if USE_KINECT
    cudaKernel->updateSkeletons(0.f, gSkeletonSize - 200.f, -800.f, // Position
                                gSkeletonSize,                      // Skeleton size
                                gSkeletonThickness, 0,              // Default material
                                gSkeletonThickness * 2.0f, 1,       // Head size and material
                                gSkeletonThickness * 1.5f, 10,      // Hands size and material
                                gSkeletonThickness * 1.8f, 10       // Feet size and material
                                );
#endif // USE_KINECT
    cudaKernel->render_begin(0);
    cudaKernel->render_end();

    glutPostRedisplay();
    glutTimerFunc(REFRESH_DELAY, timerEvent, 0);

    gSphereTranslateX += (gSphereTranslateX > 0.f) ? -0.1f : 0.1f;
    gSphereTranslateX = (abs(gSphereTranslateX) < 1.f) ? 0.f : gSphereTranslateX;

    // Bouncing
    solr::CPUPrimitive* p = cudaKernel->getPrimitive(currentPrimitive);
    p->p0.y -= 1.f;

    if (gSphereSpeed != 0)
    {
        if (gSphereTranslateY != 0.f)
        {
            if (p->p0.y > p->size.x)
                gSphereTranslateY -= 0.4f;
            else
            {
                gSphereTranslateY = -gSphereTranslateY / 4.f;
                gSphereTranslateY = (gSphereTranslateY < 2.f) ? 0.f : gSphereTranslateY;
            }
        }
        else
            p->p0.y = p->size.x;
        p->p0.y = (p->p0.y < p->size.x) ? p->size.x : p->p0.y;
    }

    int index_y = int((anim - gCarpetZ - 240) / 200.f); // WDF? 200?
    int index_x = (p->p0.x + 400) / 160;                //(800/5=160)
    int index = index_x + index_y * 5;

    if (abs(p->p0.y - p->size.x) < 10.f)
    {
        switch (gLevels[index])
        {
        case 0:
            break;
        case 1:
            gSphereTranslateX = -4.f;
            break;
        case 2:
            gSphereTranslateX = 4.f;
            break;
        case 3:
            gSphereSpeed = 10.f;
            break;
        case 4:
            gSphereTranslateY = 8.f;
            break;
        case 5:
            currentMaterial = 0;
            // cudaKernel->setPrimitiveMaterial(currentPrimitive, currentMaterial, 0, 0);
            break;
        case 6:
            currentMaterial = 1;
            // cudaKernel->setPrimitiveMaterial(currentPrimitive, currentMaterial, 0, 0);
            break;
        case 7:
            currentMaterial = 2;
            // cudaKernel->setPrimitiveMaterial(currentPrimitive, currentMaterial, 0, 0);
            break;
        case 8:
            gSphereSpeed += 5.f;
            break;
        case 9:
            gSphereSpeed -= 2.f;
            break;
        case 10:
            gEffectDuration = 20.f;
            break;
        case 11:
            gSphereSpeed = 0;
            gSphereTranslateY = -4.f;
            break;
        }
    }

    if (index_y > NB_LEVEL_LINES - 50)
    {
        gSphereSpeed -= 0.2f;
        cudaKernel->getViewAngles().y = 0.01f;
        gSphereSpeed = (gSphereSpeed < 0.f) ? 0.f : gSphereSpeed;
    }
    else
    {
        if (gSphereSpeed != 0)
        {
            gSphereSpeed = (gSphereSpeed < 10.f) ? 10.f : gSphereSpeed;
            gSphereSpeed = (gSphereSpeed > 30.f) ? 30.f : gSphereSpeed;
        }
        else
        {
            if (p->p0.y < -200.f)
            {
                // Reset ball
                p->p0.x = 0.f;
                p->p0.y = p->size.x * 3;
                gSphereSpeed = 20.f;
                gSphereTranslateY = 4.f;
                gEffectDuration = 0.f;
            }
        }
    }

    p->p0.x += gSphereTranslateX;
    p->p0.y += gSphereTranslateY + 1.f;

    /*
    if (gEffectDuration > 0.f)
    {
        lampPosition.x = x + 100.f * cos(gEffectDuration);
        lampPosition.y = y + w + w * sin(gEffectDuration * 0.75f);
        lampPosition.z = z + 100.f * cos(gEffectDuration * 1.25f);
        gEffectDuration -= 0.1f;
        cudaKernel->setLamp(nbLamps, lampPosition.x, lampPosition.y, lampPosition.z, 10.f, 10.f, 1.f, 1.f, 1.f,
                            lampPosition.w);
    }
    else
    {
        lampPosition.x = 100;
        lampPosition.y = 800;
        lampPosition.z = -400;
        cudaKernel->setLamp(nbLamps, lampPosition.x, lampPosition.y, lampPosition.z, 10.f, 10.f, 1.f, 1.f, 1.f,
                            lampPosition.w);
    }
    */

    cudaKernel->getViewDir().y = 100;
    cudaKernel->getViewDir().z = 0;

    cudaKernel->getViewPos().y = p->p0.y + 600 - gSphereSpeed * 5.f;
    cudaKernel->getViewPos().z = p->p0.z - 800 - gSphereSpeed * 5.f;
#ifdef WIN32
    if (gSphereSpeed != 0)
        anim += ((GetTickCount() - gTickCount) + gSphereSpeed);
    gTickCount = GetTickCount();
#endif
}

// Keyboard events handler
//*****************************************************************************
void keyboard(unsigned char key, int x, int y)
{
    srand(static_cast<unsigned int>(time(NULL)));

    switch (key)
    {
    case 'R':
    case 'r':
    {
        // Reset scene
        delete cudaKernel;
        cudaKernel = 0;
        createScene();
        break;
    }
    case 'F':
    case 'f':
    {
        // Toggle to full screen mode
        glutFullScreen();
        break;
    }

    /*
    case 'C':
    case 'c':
    {
        float4 pos;
        pos.x = getRandomValue(static_cast<int>(gRoomSize / 2), 0);
        pos.y = getRandomValue(static_cast<int>(gRoomSize / 2), 0, false) - 100;
        pos.z = getRandomValue(static_cast<int>(gRoomSize / 2), 0);
        pos.w = getRandomValue(100, 10, false);
        int m = rand() % nbMaterials;
        nbPrimitives = cudaKernel->addCube(pos.x, pos.y, pos.z, pos.w, m, 1, 1);
        std::cout << "Cube added: " << nbPrimitives + 1 << " primitives" << std::endl;
        break;
    }
    */

    case 'S':
    case 's':
    {
        // Add sphere
        float r = getRandomValue(200, 50, false);
        nbPrimitives = cudaKernel->addPrimitive(ptSphere);
        cudaKernel->setPrimitive(nbPrimitives, getRandomValue(400, 0), getRandomValue(400, 0), getRandomValue(400, 0),
                                 r, 0, 1, 1);
        currentPrimitive = nbPrimitives;
        std::cout << "Sphere added: " << nbPrimitives + 1 << " primitives" << std::endl;
        cudaKernel->compactBoxes(true);
        break;
    }

    case 'Y':
    case 'y':
    {
        // Add Cylinder
        nbPrimitives = cudaKernel->addPrimitive(ptCylinder);
        float r = getRandomValue(50, 50, false);
        cudaKernel->setPrimitive(nbPrimitives, getRandomValue(400, 0), -200 + r, getRandomValue(400, 0), r, 0, 1, 1);
        currentPrimitive = nbPrimitives;
        std::cout << "Cylinder added: " << nbPrimitives + 1 << " primitives" << std::endl;
        cudaKernel->compactBoxes(true);
        break;
    }

    case 'P':
    case 'p':
    {
        // Add plan
        float r = getRandomValue(400, 50, false);
        float x = getRandomValue(400, 0);
        float y = getRandomValue(400, 0);
        float z = getRandomValue(400, 0);
        int m = rand() % nbMaterials;
        nbPrimitives = cudaKernel->addPrimitive(ptXYPlane);
        cudaKernel->setPrimitive(nbPrimitives, x, y, z, r, m, 1, 1);
        std::cout << "Plan added: " << nbPrimitives + 1 << " primitives" << std::endl;
        cudaKernel->compactBoxes(true);
        break;
    }
    case 'T':
    case 't':
    {
        // Add triangle
        int m = 0;
        nbPrimitives = cudaKernel->addPrimitive(ptTriangle);
        cudaKernel->setPrimitive(nbPrimitives, 10.f, 20.f, 100.f, 50.f, 0.f, 100.f, 0.f, 50.f, 100.f, m);
        std::cout << "Plan added: " << nbPrimitives + 1 << " primitives" << std::endl;
        cudaKernel->compactBoxes(true);
        break;
    }

    case ' ':
    {
        gSphereSpeed = 20.f;
        break;
    }

    case 'e':
    {
        transparentColor += 0.01f;
        if (transparentColor > 0.99f)
            transparentColor = 0.99f;
        break;
    }
    case 'd':
    {
        transparentColor -= 0.01f;
        if (transparentColor < 0.f)
            transparentColor = 0.f;
        break;
    }
    case 'v':
    {
        lampPosition.w += 0.1f;
        lampPosition.w = (lampPosition.w > 1.f) ? 1.f : lampPosition.w;
        cudaKernel->setPrimitive(nbLamps, lampPosition.x, lampPosition.y, lampPosition.z, 10.f, 0.f, 0.f, MATERIAL_0);
        break;
    }
    case 'V':
    {
        lampPosition.w -= 0.1f;
        lampPosition.w = (lampPosition.w < 0.f) ? 0.f : lampPosition.w;
        cudaKernel->setPrimitive(nbLamps, lampPosition.x, lampPosition.y, lampPosition.z, 10.f, 0.f, 0.f,
                                 DEFAULT_LIGHT_MATERIAL);
        break;
    }
    case 'a':
    {
        // Add lamp
        nbLamps = cudaKernel->addPrimitive(ptSphere);
        cudaKernel->setPrimitive(nbLamps, getRandomValue(1000, 0), 200 + getRandomValue(100, 0),
                                 getRandomValue(1000, 0), 10.f, 10.f, rand() % 100 / 100.f, rand() % 100 / 100.f,
                                 rand() % 100 / 100.f, 0.1f, DEFAULT_LIGHT_MATERIAL);
        std::cout << nbLamps + 1 << " lamps" << std::endl;
        break;
    }
    case 'B':
    {
        SceneInfo& sceneInfo = cudaKernel->getSceneInfo();
        sceneInfo.backgroundColor.x = getRandomValue(1000, 0);
        sceneInfo.backgroundColor.y = getRandomValue(1000, 0);
        sceneInfo.backgroundColor.z = getRandomValue(1000, 0);
    }
    /*
    case 'n':
    {
        for (int i(0); i < nbPrimitives - 5; ++i)
            cudaKernel->setPrimitiveMaterial(i + 5, currentMaterial % nbMaterials, 0, 0);
        currentMaterial++;
        break;
    }
    case 'i':
        cudaKernel->translate(currentPrimitive, 0.f, step, 0.f);
        break;
    case 'I':
        cudaKernel->rotatePrimitive(currentPrimitive, 0.f, 0.1f, 0.f);
        break;
    case 'J':
        cudaKernel->rotatePrimitive(currentPrimitive, -0.1f, 0.f, 0.f);
        break;
    case 'L':
        cudaKernel->rotatePrimitive(currentPrimitive, 0.1f, 0.f, 0.f);
        break;
    case 'm':
        cudaKernel->translatePrimitive(currentPrimitive, 0.f, -step, 0.f);
        break;
    case 'M':
        cudaKernel->rotatePrimitive(currentPrimitive, 0.f, -0.1f, 0.f);
        break;
    case 'u':
        cudaKernel->translatePrimitive(currentPrimitive, 0.f, 0.f, step);
        break;
    case 'o':
        cudaKernel->translatePrimitive(currentPrimitive, 0.f, 0.f, -step);
        break;
    */
    case '+':
        dof += 50.f;
        break;
    case '-':
        dof -= 50.f;
        break;
    case 'k':
        currentPrimitive++;
        if (currentPrimitive > nbPrimitives)
            currentPrimitive = 0;
        std::cout << "Selected primitive: " << currentPrimitive << std::endl;
        break;
    /*
case 'h':
    currentMaterial++;
    if (currentMaterial > nbMaterials)
        currentMaterial = 0;
    cudaKernel->setPrimitiveMaterial(currentPrimitive, currentMaterial, 0, 0);
    break;
    */
    case '8':
        gSphereSpeed += 1.f;
        cudaKernel->setCamera(cudaKernel->getViewPos(), cudaKernel->getViewDir(), cudaKernel->getViewAngles());
        break;
    case '2':
        gSphereSpeed -= 1.f;
        cudaKernel->setCamera(cudaKernel->getViewPos(), cudaKernel->getViewDir(), cudaKernel->getViewAngles());
        break;
    case '4':
        gSphereTranslateX = -4.f;
        break;
    case '6':
        gSphereTranslateX = 4.f;
        break;
    case '5':
        gSphereTranslateY = 4.f;
        break;

    case 'b':
        gSphereSpeed = 0.f;
        break;

    case '\033':
    case '\015':
    case 'X':
    case 'x':
    {
        // Cleanup up and quit
        bNoPrompt = true;
        Cleanup(EXIT_SUCCESS);
        break;
    }
    }
}

// Mouse event handlers
//*****************************************************************************
void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN)
        mouse_buttons |= 1 << button;
    else
    {
        if (state == GLUT_UP)
            mouse_buttons = 0;
    }
    mouse_old_x = x;
    mouse_old_y = y;
    cudaKernel->getViewAngles().x = 0.f;
    cudaKernel->getViewAngles().y = 0.f;
}

void motion(int x, int y)
{
    switch (mouse_buttons)
    {
    case 1:
        // Move cudaKernel->getViewPos() position along the Z axis
        cudaKernel->getViewPos().z += 2 * (mouse_old_y - y);
        if (glutGetModifiers() != GLUT_ACTIVE_SHIFT)
            cudaKernel->getViewDir().z += 2 * (mouse_old_y - y);
        break;
    case 2:
        // Rotates the scene around X and Y axis
        cudaKernel->getViewAngles().y = -asin((mouse_old_x - x) / 100.f);
        cudaKernel->getViewAngles().x = asin((mouse_old_y - y) / 100.f);
        break;
    case 4:
        // Move cudaKernel->getViewPos() postion along X and Y axis
        cudaKernel->getViewPos().x += (mouse_old_x - x);
        cudaKernel->getViewPos().y += (mouse_old_y - y);
        cudaKernel->getViewDir().x += (mouse_old_x - x);
        cudaKernel->getViewDir().y += (mouse_old_y - y);
        break;
    }
    mouse_old_x = x;
    mouse_old_y = y;
    cudaKernel->setCamera(cudaKernel->getViewPos(), cudaKernel->getViewDir(), cudaKernel->getViewAngles());
}

// Function to clean up and exit
//*****************************************************************************
void Cleanup(int iExitCode)
{
    // Cleanup allocated objects
    std::cout << "\nStarting Cleanup...\n\n" << std::endl;

    if (gLevels)
        delete gLevels;
    if (ubImage)
        delete[] ubImage;
    delete cudaKernel;

    exit(iExitCode);
}

void createMaterials()
{
    // Materials
    for (int i(0); i < 20 + nbTextures; ++i)
    {
        float reflection = 0.f;
        float refraction = 0.f;
        int texture = TEXTURE_NONE;
        float transparency = 0.f;
        bool procedural = false;
        float innerIllumination = 0.f;
        float specValue = 1.f;
        float specPower = 100.f;
        float specCoef = 1.f;

        float r = rand() % 100 / 100.f;
        float g = rand() % 100 / 100.f;
        float b = rand() % 100 / 100.f;

        switch (i)
        {
        case MATERIAL_0:
            r = 1.f;
            g = 1.f;
            b = 1.f;
            reflection = 0.9f;
            break;
        case MATERIAL_1:
            r = 1.f;
            g = 1.f;
            b = 1.f;
            reflection = 0.95f;
            transparency = 0.99f;
            refraction = 1.01f;
            break;
        case MATERIAL_2:
            r = 1.f;
            g = 1.f;
            b = 1.f;
            reflection = 0.95f;
            refraction = 1.01f;
            transparency = 0.99f;
            procedural = 1;
            break;
        case MATERIAL_3:
            r = 1.f;
            g = 1.f;
            b = 1.f;
            break;
        case MATERIAL_LIGHT:
            r = 1.f;
            g = 1.f;
            b = 1.f;
            innerIllumination = 1.f;
            break;
        default:
            if (i >= 20)
                texture = i - 20;
            else
                reflection = rand() % 100 / 100.f;
            specValue = 0.f;
            specPower = 0.f;
            specCoef = 0.f;
            break;
        }

        nbMaterials = cudaKernel->addMaterial();
        cudaKernel->setMaterial(nbMaterials, r, g, b, 0.f, reflection, refraction, procedural, false, 0, transparency,
                                1.f, texture, TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE,
                                TEXTURE_NONE, specValue, specPower, specCoef, innerIllumination, 1000.f, 1000.f, false);
    }
}

void createTextures()
{
    // Textures
    for (int i(0); i <= 12; i++)
    {
        char tmp[5];
        sprintf(tmp, "%03d", i);
        std::string filename = std::string(DEFAULT_MEDIA_FOLDER) + "/textures/trailblazer/icons/";
        filename += tmp;
        filename += ".bmp";
        cudaKernel->loadTextureFromFile(i, filename.c_str());
    }
}

void createScene()
{
    nbPrimitives = 0;
    nbLamps = 0;
    nbMaterials = 0;
    nbTextures = 13;
    anim = 0.f;
    srand(static_cast<unsigned int>(time(NULL)));
#ifdef WIN32
    gTickCount = GetTickCount();
#endif

    // Initialize Levels
    int i(0);
    gLevels = new int[LEVELS_SIZE];
#pragma omp parallel for
    for (i = 0; i < LEVELS_SIZE; ++i)
    {
        if (i / NB_LEVEL_COLUMNS > (NB_LEVEL_LINES - 50))
        {
            // Finish line
            if (i / NB_LEVEL_COLUMNS > (NB_LEVEL_LINES - 47))
                gLevels[i] = 0;
            else
                gLevels[i] = 12;
        }
        else
        {
            // Starting line
            if (i / NB_LEVEL_COLUMNS < 20)
                gLevels[i] = 12;
            else
                // Random
                gLevels[i] = (rand() % 10 > 5) ? rand() % 12 : 0;
        }
    }

    if (!cudaKernel)
        cudaKernel = new solr::CudaKernel();
    cudaKernel->initBuffers();
    cudaKernel->resetAll();
    cudaKernel->setFrame(0);

    SceneInfo& sceneInfo = cudaKernel->getSceneInfo();
    // Scene
    sceneInfo.size.x = window_width;
    sceneInfo.size.y = window_height;
    sceneInfo.graphicsLevel = (sceneInfo.cameraType == ctVolumeRendering) ? glNoShading : glFull;
    sceneInfo.nbRayIterations = 3;
    sceneInfo.transparentColor = 0.f;
    sceneInfo.viewDistance = 50000.f;
    sceneInfo.shadowIntensity = 1.0f;
    sceneInfo.eyeSeparation = 380.f;
    sceneInfo.backgroundColor.x = 0.1f;
    sceneInfo.backgroundColor.y = 0.1f;
    sceneInfo.backgroundColor.z = 0.1f;
    sceneInfo.backgroundColor.w = 0.5f;
    sceneInfo.renderBoxes = 0;
    sceneInfo.pathTracingIteration = 0;
    sceneInfo.maxPathTracingIterations = 5;
    sceneInfo.frameBufferType = ftRGB;
    sceneInfo.timestamp = 0;
    sceneInfo.atmosphericEffect = aeNone;
    sceneInfo.cameraType = ctPerspective;
    sceneInfo.doubleSidedTriangles = false;
    sceneInfo.extendedGeometry = true;
    sceneInfo.advancedIllumination = aiNone;
    sceneInfo.draftMode = false;
    sceneInfo.skyboxRadius = static_cast<int>(sceneInfo.viewDistance * 0.9f);
    sceneInfo.skyboxMaterialId = MATERIAL_NONE;
    sceneInfo.gradientBackground = 0;
    sceneInfo.geometryEpsilon = 0.001f;
    sceneInfo.rayEpsilon = 0.05f;

    cudaKernel->getViewPos().x = 0.f;
    cudaKernel->getViewPos().y = 200.f;
    cudaKernel->getViewPos().z = -800.f;

    cudaKernel->getViewDir().x = 0.f;
    cudaKernel->getViewDir().y = 0.f;
    cudaKernel->getViewDir().z = 0.f;

    cudaKernel->getViewAngles().x = 0.f;
    cudaKernel->getViewAngles().y = 0.f;
    cudaKernel->getViewAngles().z = 0.f;

    createTextures();
    createMaterials();

    // Floor
    nbPrimitives = cudaKernel->addPrimitive(ptMagicCarpet);
    cudaKernel->setPrimitive(nbPrimitives, 0.f, 0.f, gCarpetSize + gCarpetZ, 400, gCarpetSize, 20, MATERIAL_0);

    // Walls
    nbPrimitives = cudaKernel->addPrimitive(ptYZPlane);
    cudaKernel->setPrimitive(nbPrimitives, -400.f, 0.f, 0.f, gCarpetSize, 100, 0, MATERIAL_1);
    nbPrimitives = cudaKernel->addPrimitive(ptYZPlane);
    cudaKernel->setPrimitive(nbPrimitives, 400.f, 0.f, 0.f, gCarpetSize, 100, 0, MATERIAL_1);

    // Sides
    nbPrimitives = cudaKernel->addPrimitive(ptXZPlane);
    cudaKernel->setPrimitive(nbPrimitives, -800.f, 100.f, 0.f, 400, gCarpetSize, 20, 1, gCarpetSize / 200, 0.f,
                             MATERIAL_2);
    nbPrimitives = cudaKernel->addPrimitive(ptXZPlane);
    cudaKernel->setPrimitive(nbPrimitives, 800.f, 100.f, 0.f, 400, gCarpetSize, 20, 1, gCarpetSize / 200, 0.f,
                             MATERIAL_2);

    nbPrimitives = cudaKernel->addPrimitive(ptSphere);
    cudaKernel->setPrimitive(nbPrimitives, 0.f, 50.f, -200.f, 50.f, 0.f, 0.f, MATERIAL_3);
    currentPrimitive = nbPrimitives;

    // Cylinder
    nbPrimitives = cudaKernel->addPrimitive(ptCylinder);
    cudaKernel->setPrimitive(nbPrimitives, 0, -400, -300, 400, gCarpetSize, 20, MATERIAL_4);

    // Side spheres
    nbPrimitives = cudaKernel->addPrimitive(ptSphere);
    cudaKernel->setPrimitive(nbPrimitives, -700, 500, 100, 400, 400, 0, MATERIAL_5);
    nbPrimitives = cudaKernel->addPrimitive(ptSphere);
    cudaKernel->setPrimitive(nbPrimitives, 700, 500, 100, 400, 400, 0, MATERIAL_5);

    // Lamps
    lampPosition.x = 100;
    lampPosition.y = 800;
    lampPosition.z = -400;
    lampPosition.w = 1.f;
    nbLamps = cudaKernel->addPrimitive(ptSphere);
    cudaKernel->setPrimitive(nbLamps, lampPosition.x, lampPosition.y, lampPosition.z, lampPosition.w, 0.f, 0.f,
                             MATERIAL_LIGHT);

    cudaKernel->compactBoxes(true);
}

int main(int argc, char* argv[])
{
    // First initialize OpenGL context, so we can properly set the GL for CUDA.
    // This is necessary in order to achieve optimal performance with OpenGL/CUDA interop.
    initgl(argc, argv);

    // Create Scene
    createScene();

    atexit(cleanup);
    glutMainLoop();

    // Normally unused return path
    Cleanup(EXIT_SUCCESS);

    return 0;
}
