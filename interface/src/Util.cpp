//
//  util.cpp
//  interface
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "InterfaceConfig.h"
#include <iostream>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SharedUtil.h>

#include "Log.h"
#include "ui/TextRenderer.h"
#include "world.h"
#include "Util.h"

using namespace std;

// no clue which versions are affected...
#define WORKAROUND_BROKEN_GLUT_STROKES
// see http://www.opengl.org/resources/libraries/glut/spec3/node78.html

void eulerToOrthonormals(glm::vec3 * angles, glm::vec3 * front, glm::vec3 * right, glm::vec3 * up) {
    //
    //  Converts from three euler angles to the associated orthonormal vectors
    //
    //  Angles contains (pitch, yaw, roll) in radians
    //
    
    //  First, create the quaternion associated with these euler angles
    glm::quat q(glm::vec3(angles->x, -(angles->y), angles->z));

    //  Next, create a rotation matrix from that quaternion
    glm::mat4 rotation;
    rotation = glm::mat4_cast(q);
    
    //  Transform the original vectors by the rotation matrix to get the new vectors
    glm::vec4 qup(0,1,0,0);
    glm::vec4 qright(-1,0,0,0);
    glm::vec4 qfront(0,0,1,0);
    glm::vec4 upNew    = qup*rotation;
    glm::vec4 rightNew = qright*rotation;
    glm::vec4 frontNew = qfront*rotation;
    
    //  Copy the answers to output vectors
    up->x = upNew.x;  up->y = upNew.y;  up->z = upNew.z;
    right->x = rightNew.x;  right->y = rightNew.y;  right->z = rightNew.z;
    front->x = frontNew.x;  front->y = frontNew.y;  front->z = frontNew.z;
}



//  Return the azimuth angle in degrees between two points.
float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) * 180.0f / PIf;
}

//  Return the angle in degrees between the head and an object in the scene.  The value is zero if you are looking right at it.  The angle is negative if the object is to your right.   
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) * 180.0f / PIf + render_yaw + head_yaw;
}

//  Helper function returns the positive angle in degrees between two 3D vectors 
float angleBetween(glm::vec3 * v1, glm::vec3 * v2) {
    return acos((glm::dot(*v1, *v2)) / (glm::length(*v1) * glm::length(*v2))) * 180.f / PI;
}

//  Draw a 3D vector floating in space
void drawVector(glm::vec3 * vector) {
    glDisable(GL_LIGHTING);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(3.0);
    glLineWidth(2.0);

    //  Draw axes
    glBegin(GL_LINES);
    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(1,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0, 1, 0);
    glColor3f(0,0,1);
    glVertex3f(0,0,0);
    glVertex3f(0, 0, 1);
    glEnd();
        
    // Draw the vector itself
    glBegin(GL_LINES);
    glColor3f(1,1,1);
    glVertex3f(0,0,0);
    glVertex3f(vector->x, vector->y, vector->z);
    glEnd();
    
    // Draw spheres for magnitude
    glPushMatrix();
    glColor3f(1,0,0);
    glTranslatef(vector->x, 0, 0);
    glutSolidSphere(0.02, 10, 10);
    glColor3f(0,1,0);
    glTranslatef(-vector->x, vector->y, 0);
    glutSolidSphere(0.02, 10, 10);
    glColor3f(0,0,1);
    glTranslatef(0, -vector->y, vector->z);
    glutSolidSphere(0.02, 10, 10);
    glPopMatrix();

}

void render_world_box() {
    //  Show edge of world 
    glDisable(GL_LIGHTING);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(WORLD_SIZE, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, WORLD_SIZE, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, WORLD_SIZE);
    glEnd();
    //  Draw little marker dots along the axis
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(WORLD_SIZE, 0, 0);
    glColor3f(1, 0, 0);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, WORLD_SIZE, 0);
    glColor3f(0, 1, 0);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, WORLD_SIZE);
    glColor3f(0, 0, 1);
    glutSolidSphere(0.125, 10, 10);
    glPopMatrix();
}

double diffclock(timeval *clock1,timeval *clock2)
{
	double diffms = (clock2->tv_sec - clock1->tv_sec) * 1000.0;
    diffms += (clock2->tv_usec - clock1->tv_usec) / 1000.0;   // us to ms
    
	return diffms;
}

static TextRenderer* textRenderer(int mono) {
    static TextRenderer* monoRenderer = new TextRenderer(MONO_FONT_FAMILY);
    static TextRenderer* proportionalRenderer = new TextRenderer(SANS_FONT_FAMILY);
    return mono ? monoRenderer : proportionalRenderer;
}

int widthText(float scale, int mono, char const* string) {
    return textRenderer(mono)->computeWidth(string) * (scale / 0.10);
}

float widthChar(float scale, int mono, char ch) {
    return textRenderer(mono)->computeWidth(ch) * (scale / 0.10);
}

void drawtext(int x, int y, float scale, float rotate, float thick, int mono,
              char const* string, float r, float g, float b) {
    //
    //  Draws text on screen as stroked so it can be resized
    //
    glPushMatrix();
    glTranslatef( static_cast<float>(x), static_cast<float>(y), 0.0f);
    glColor3f(r,g,b);
    glRotated(rotate,0,0,1);
    // glLineWidth(thick);
    glScalef(scale / 0.10, scale / 0.10, 1.0);
    
    textRenderer(mono)->draw(0, 0, string);
    
    glPopMatrix();

}

void drawvec3(int x, int y, float scale, float rotate, float thick, int mono, glm::vec3 vec, float r, float g, float b) {
    //
    //  Draws text on screen as stroked so it can be resized
    //
    char vectext[20];
    sprintf(vectext,"%3.1f,%3.1f,%3.1f", vec.x, vec.y, vec.z);
    int len, i;
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0);
    glColor3f(r,g,b);
    glRotated(180+rotate,0,0,1);
    glRotated(180,0,1,0);
    glLineWidth(thick);
    glScalef(scale, scale, 1.0);
    len = (int) strlen(vectext);
	for (i = 0; i < len; i++) {
        if (!mono) glutStrokeCharacter(GLUT_STROKE_ROMAN, int(vectext[i]));
        else glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, int(vectext[i]));
	}
    glPopMatrix();
} 


void drawGroundPlaneGrid(float size) {
	glColor3f( 0.4f, 0.5f, 0.3f ); 
	glLineWidth(2.0);
		
    for (float x = 0; x <= size; x++) {
		glBegin(GL_LINES);
		glVertex3f(x, 0.0f, 0);
		glVertex3f(x, 0.0f, size);
        glVertex3f(0, 0.0f, x);
		glVertex3f(size, 0.0f, x);
        glEnd();
    }
        
    // Draw a translucent quad just underneath the grid.
    glColor4f(0.5, 0.5, 0.5, 0.4);
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(size, 0, 0);
    glVertex3f(size, 0, size);
    glVertex3f(0, 0, size);
    glEnd();
}



void renderDiskShadow(glm::vec3 position, glm::vec3 upDirection, float radius, float darkness) {

    glColor4f( 0.0f, 0.0f, 0.0f, darkness );
    
    int   num = 20;
    float y  = 0.001f;
    float x2 = 0.0f;
    float z2 = radius;
    float x1;
    float z1;

    glBegin(GL_TRIANGLES);             

    for (int i=1; i<num+1; i++) {
        x1 = x2;
        z1 = z2;
        float r = ((float)i / (float)num) * PI * 2.0;
        x2 = radius * sin(r);
        z2 = radius * cos(r);
    
            glVertex3f(position.x,      y, position.z     ); 
            glVertex3f(position.x + x1, y, position.z + z1); 
            glVertex3f(position.x + x2, y, position.z + z2); 
    }
    
    glEnd();
}



void renderSphereOutline(glm::vec3 position, float radius, int numSides, glm::vec3 cameraPosition) {
    glm::vec3 vectorToPosition(glm::normalize(position - cameraPosition));
    glm::vec3 right = glm::cross(vectorToPosition, glm::vec3( 0.0f, 1.0f, 0.0f));
    glm::vec3 up    = glm::cross(right, vectorToPosition);
    
    glBegin(GL_LINE_STRIP);             
    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * PI * 2.0;
        float s = radius * sin(r);
        float c = radius * cos(r);
    
        glVertex3f
        (
            position.x + right.x * s + up.x * c, 
            position.y + right.y * s + up.y * c, 
            position.z + right.z * s + up.z * c 
        ); 
    }
    
    glEnd();
}


void renderCircle(glm::vec3 position, float radius, glm::vec3 surfaceNormal, int numSides ) {
    glm::vec3 perp1 = glm::vec3(surfaceNormal.y, surfaceNormal.z, surfaceNormal.x);
    glm::vec3 perp2 = glm::vec3(surfaceNormal.z, surfaceNormal.x, surfaceNormal.y);
    
    glBegin(GL_LINE_STRIP);             

    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * PI * 2.0;
        float s = radius * sin(r);
        float c = radius * cos(r);
        glVertex3f
        (
            position.x + perp1.x * s + perp2.x * c, 
            position.y + perp1.y * s + perp2.y * c, 
            position.z + perp1.z * s + perp2.z * c 
        ); 
    }
    glEnd();
}


void renderOrientationDirections( glm::vec3 position, Orientation orientation, float size ) {
	glm::vec3 pRight	= position + orientation.getRight() * size;
	glm::vec3 pUp		= position + orientation.getUp() * size;
	glm::vec3 pFront	= position + orientation.getFront() * size;
		
	glColor3f( 1.0f, 0.0f, 0.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( pRight.x, pRight.y, pRight.z );
	glEnd();

	glColor3f( 0.0f, 1.0f, 0.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( pUp.x, pUp.y, pUp.z );
	glEnd();

	glColor3f( 0.0f, 0.0f, 1.0f );
	glBegin( GL_LINE_STRIP );
	glVertex3f( position.x, position.y, position.z );
	glVertex3f( pFront.x, pFront.y, pFront.z );
	glEnd();
}

bool closeEnoughForGovernmentWork(float a, float b) {
    float distance = std::abs(a-b);
    //printLog("closeEnoughForGovernmentWork() a=%1.10f b=%1.10f distance=%1.10f\n",a,b,distance);
    return (distance < 0.00001f);
}


void testOrientationClass() {
    printLog("\n----------\ntestOrientationClass()\n----------\n\n");
    
    oTestCase tests[] = { 
        //       - inputs ------------, outputs --------------------  -------------------  ----------------------------
        //                              -- front -------------------, -- up -------------, -- right -------------------
        //       ( yaw  , pitch, roll , front.x , front.y , front.z , up.x , up.y , up.z , right.x , right.y , right.z  )

        // simple yaw tests
        oTestCase( 0.f  , 0.f  , 0.f  ,  0.f       , 0.f , 1.0f      ,  0.f , 1.0f , 0.f  , -1.0f     , 0.f     , 0.f      ),
        oTestCase(45.0f , 0.f  , 0.f  ,  0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , 0.707107f),
        oTestCase( 90.0f, 0.f  , 0.f  , 1.0f    , 0.f     , 0.f     ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , 1.0f     ),
        oTestCase(135.0f, 0.f  , 0.f  ,  0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , 0.707107f),
        oTestCase(180.0f, 0.f  , 0.f  ,  0.f    , 0.f     , -1.0f   ,   0.f , 1.0f , 0.f  , 1.0f    , 0.f     , 0.f      ),
        oTestCase(225.0f, 0.f  , 0.f  , -0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , -0.707107f),
        oTestCase(270.0f, 0.f  , 0.f  , -1.0f   , 0.f    , 0.f       ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , -1.0f    ),
        oTestCase(315.0f, 0.f  , 0.f  , -0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , -0.707107f),
        oTestCase(-45.0f, 0.f  , 0.f  , -0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , -0.707107f),
        oTestCase(-90.0f, 0.f  , 0.f  , -1.0f   , 0.f    , 0.f       ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , -1.0f    ),
        oTestCase(-135.0f,0.f  , 0.f  , -0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , -0.707107f),
        oTestCase(-180.0f,0.f  , 0.f  ,  0.f    , 0.f     , -1.0f   ,   0.f , 1.0f , 0.f  , 1.0f    , 0.f     , 0.f      ),
        oTestCase(-225.0f,0.f  , 0.f  ,  0.707107f , 0.f ,-0.707107f ,  0.f , 1.0f , 0.f  , 0.707107f, 0.f     , 0.707107f),
        oTestCase(-270.0f,0.f  , 0.f  , 1.0f    , 0.f     , 0.f     ,   0.f , 1.0f , 0.f  , 0.0f    , 0.f     , 1.0f     ),
        oTestCase(-315.0f,0.f  , 0.f  ,  0.707107f , 0.f , 0.707107f ,  0.f , 1.0f , 0.f  , -0.707107f, 0.f     , 0.707107f),

        // simple pitch tests
        oTestCase( 0.f  , 0.f  , 0.f  ,  0.f, 0.f       , 1.0f      ,  0.f , 1.0f    , 0.f       ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,45.0f , 0.f  ,  0.f, 0.707107f , 0.707107f,   0.f ,0.707107f, -0.707107f,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,90.f  , 0.f  ,  0.f, 1.0f      , 0.0f     ,   0.f ,0.0f     , -1.0f     ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,135.0f, 0.f  ,  0.f, 0.707107f , -0.707107f,  0.f ,-0.707107f, -0.707107f,   -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,180.f , 0.f  ,  0.f, 0.0f      ,-1.0f     ,   0.f ,-1.0f    , 0.f       ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,225.0f, 0.f  ,  0.f,-0.707107f , -0.707107f,  0.f ,-0.707107f, 0.707107f,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,270.f , 0.f  ,  0.f,-1.0f      , 0.0f     ,   0.f ,0.0f     , 1.0f      ,    -1.0f  , 0.f  , 0.f  ),
        oTestCase( 0.f  ,315.0f, 0.f  ,  0.f,-0.707107f , 0.707107f,  0.f , 0.707107f,  0.707107f,    -1.0f  , 0.f  , 0.f  ),

        // simple roll tests
        oTestCase( 0.f  , 0.f  , 0.f    , 0.f  , 0.f , 1.0f  ,  0.f       , 1.0f      ,0.0f   , -1.0f     , 0.f      , 0.0f ),
        oTestCase( 0.f  , 0.f  ,45.0f   , 0.f  , 0.f , 1.0f  ,  0.707107f , 0.707107f ,0.0f   , -0.707107f, 0.707107f, 0.0f ),
        oTestCase( 0.f  , 0.f  ,90.f    , 0.f  , 0.f , 1.0f  ,  1.0f      , 0.0f      ,0.0f   , 0.0f      , 1.0f     , 0.0f ),
        oTestCase( 0.f  , 0.f  ,135.0f  , 0.f  , 0.f , 1.0f  ,  0.707107f , -0.707107f,0.0f   , 0.707107f , 0.707107f, 0.0f ),
        oTestCase( 0.f  , 0.f  ,180.f   , 0.f  , 0.f , 1.0f  ,  0.0f      , -1.0f     ,0.0f   , 1.0f      , 0.0f     , 0.0f ),
        oTestCase( 0.f  , 0.f  ,225.0f  , 0.f  , 0.f , 1.0f  ,  -0.707107f, -0.707107f,0.0f   , 0.707107f ,-0.707107f, 0.0f ),
        oTestCase( 0.f  , 0.f  ,270.f   , 0.f  , 0.f , 1.0f  , -1.0f      , 0.0f      ,0.0f   , 0.0f      , -1.0f    , 0.0f ),
        oTestCase( 0.f  , 0.f  ,315.0f  , 0.f  , 0.f , 1.0f  ,  -0.707107f, 0.707107f ,0.0f   , -0.707107f,-0.707107f, 0.0f ),

        // yaw combo tests
        oTestCase( 90.f , 90.f , 0.f    ,  0.f  , 1.0f , 0.0f    ,  -1.0f , 0.0f , 0.f     , 0.0f , 0.f   , 1.0f       ),
        oTestCase( 90.f , 0.f , 90.f    ,  1.0f , 0.0f,  0.f     ,  0.0f , 0.0f , -1.f     , 0.0f , 1.0f  , 0.0f       ),
    };
    
    int failedCount = 0;
    int totalTests = sizeof(tests)/sizeof(oTestCase);
    
    for (int i=0; i < totalTests; i++) {
    
        bool passed = true; // I'm an optimist!

        float yaw   = tests[i].yaw;
        float pitch = tests[i].pitch;
        float roll  = tests[i].roll;

        Orientation o1;
        o1.setToIdentity();
        o1.yaw(yaw);
        o1.pitch(pitch);
        o1.roll(roll);
    
        glm::vec3 front = o1.getFront();
        glm::vec3 up    = o1.getUp();
        glm::vec3 right = o1.getRight();

        printLog("\n-----\nTest: %d - yaw=%f , pitch=%f , roll=%f \n",i+1,yaw,pitch,roll);

        printLog("\nFRONT\n");
        printLog(" + received: front.x=%f, front.y=%f, front.z=%f\n",front.x,front.y,front.z);
        
        if (closeEnoughForGovernmentWork(front.x, tests[i].frontX) 
            && closeEnoughForGovernmentWork(front.y, tests[i].frontY)
            && closeEnoughForGovernmentWork(front.z, tests[i].frontZ)) {
            printLog("  front vector PASSES!\n");
        } else {
            printLog("   expected: front.x=%f, front.y=%f, front.z=%f\n",tests[i].frontX,tests[i].frontY,tests[i].frontZ);
            printLog("  front vector FAILED! \n");
            passed = false;
        }
            
        printLog("\nUP\n");
        printLog(" + received: up.x=%f,    up.y=%f,    up.z=%f\n",up.x,up.y,up.z);
        if (closeEnoughForGovernmentWork(up.x, tests[i].upX) 
            && closeEnoughForGovernmentWork(up.y, tests[i].upY)
            && closeEnoughForGovernmentWork(up.z, tests[i].upZ)) {
            printLog("  up vector PASSES!\n");
        } else {
            printLog("  expected: up.x=%f, up.y=%f, up.z=%f\n",tests[i].upX,tests[i].upY,tests[i].upZ);
            printLog("  up vector FAILED!\n");
            passed = false;
        }


        printLog("\nRIGHT\n");
        printLog(" + received: right.x=%f, right.y=%f, right.z=%f\n",right.x,right.y,right.z);
        if (closeEnoughForGovernmentWork(right.x, tests[i].rightX) 
            && closeEnoughForGovernmentWork(right.y, tests[i].rightY)
            && closeEnoughForGovernmentWork(right.z, tests[i].rightZ)) {
            printLog("  right vector PASSES!\n");
        } else {
            printLog("   expected: right.x=%f, right.y=%f, right.z=%f\n",tests[i].rightX,tests[i].rightY,tests[i].rightZ);
            printLog("  right vector FAILED!\n");
            passed = false;
        }
        
        if (!passed) {
            printLog("\n-----\nTest: %d - FAILED! \n----------\n\n",i+1);
            failedCount++;
        }
    }
    printLog("\n-----\nTotal Failed: %d out of %d \n----------\n\n",failedCount,totalTests);
    printLog("\n----------DONE----------\n\n");
}




