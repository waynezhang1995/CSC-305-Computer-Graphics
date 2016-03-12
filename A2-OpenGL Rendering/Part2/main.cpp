/*
 * Author: YuWei(Wayne) Zhang
 * V00805647
 */

#include "Canvas.h"
#include <math.h>
#include "Eigen/Dense"
#include <iostream>
#include "data.h"
#include "loadtexture.h"

unsigned int width = 1024;
unsigned int height = 1024;

using namespace Eigen;
using namespace std;

Canvas canvas;

float vppos_x = 0;//cursor position; x coordinate
float vppos_y = 0;//cursor position; y coordinate
bool leftButtonPressed = false;//whether left button is pressed
bool rightButtonPressed = false;//whether right button is pressed
float lastx = vppos_x;//last cursor position; x coordinate
float lasty = vppos_y;//last cursor position; y coordiante

//take inverse transpose
const char * vshader_square = "\
    #version 330 core \n \
    in vec3 vpoint;\
    in vec3 npoint;\
    in vec3 vskypoint;\
    out vec4 fnormal;\
    out vec4 interPoint;\
    in vec2 vtexcoord;\
    out vec4 skyinterPoint;\
    out vec2 uv;\
    uniform mat4 UseMvp;\
    \
    void RotationMatrix(mat4 UseMvp){\
        vec4 temp = UseMvp * vec4(vpoint,1);\
        vec4 tempsky = UseMvp * vec4(vskypoint,1);\
        gl_Position = temp ;\
        interPoint = temp;\
        skyinterPoint = tempsky;\
        uv = vtexcoord;\
        fnormal = inverse((transpose(UseMvp))) * vec4(npoint,0);\
    }\
    void main(){\
    RotationMatrix(UseMvp);}";

const char * fshader_square = "\
    #version 330 core \n \
    out vec3 color;\
    in vec4 fnormal;\
    in vec4 interPoint;\
    in vec4 skyinterPoint;\
    in vec2 uv;\
    uniform mat4 UseMvp;\
    uniform sampler2D tex;\
    uniform sampler2D sky;\
    void main(){\
    vec2 uv_center = vec2(0.5,0.5);\
    vec3 LightPos = vec3(0,0,1.0f);\
    vec4 Lp = UseMvp *vec4(LightPos,1);\
    vec4 LightDir = normalize(Lp - interPoint );\
    vec4 n = vec4(normalize(cross(dFdy(interPoint.xyz),dFdx(interPoint.xyz))),0);\
    vec3 tmp = (2*dot(n.xyz,LightDir.xyz)) * n.xyz;\
    vec3 R = normalize(tmp - LightDir.xyz);\
    float rv = max(0.0f,dot(R,normalize(-(interPoint.xyz))));\
    float specular = pow(rv,100);\
    float diffuseterm = max(dot(LightDir,n),0.0);\
        color = texture(tex,uv).rgb + texture(tex,uv).rgb * diffuseterm + vec3(1.0f,1.0f,1.0f) * specular;\
    }";



float rotateAngle = 0 ; //The angle the camera currently rotated
                        //The angle between cube center and camera in a spherical coordinate
float rotateAngle1 = M_PI * 0.5; //The angle the camera currently rotated
float RotatingSpeed = 0.02;

GLuint VertexArrayID = 0;
GLuint ProgramID = 0;//the program we wrote
GLuint MvpID = 0;

float dis = 2.0;

void InitializeGL()
{
    //vertex Array Object
    glGenVertexArrays(1,&VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //Vertex Buffer Object
    ProgramID = compile_shaders(vshader_square,fshader_square);
    glUseProgram(ProgramID);
    MvpID = glGetUniformLocation(ProgramID,"UseMvp");
    GLuint vertexBufferID;
    GLuint normalBufferID;
    GLuint skyBufferID;
    glGenBuffers(1,&vertexBufferID);    //actually contains the vertices of square
    glGenBuffers(1,&normalBufferID);
    glGenBuffers(1,&skyBufferID);
    glBindBuffer(GL_ARRAY_BUFFER,vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vpoint),vpoint,GL_STATIC_DRAW);
    GLuint vpoint_id = glGetAttribLocation(ProgramID,"vpoint");
    glEnableVertexAttribArray(vpoint_id);
    glVertexAttribPointer(vpoint_id,3,GL_FLOAT,false,0,0);

    glBindBuffer(GL_ARRAY_BUFFER,normalBufferID);
    glBufferData(GL_ARRAY_BUFFER,sizeof(npoint),npoint,GL_DYNAMIC_DRAW);
    GLuint npoint_id = glGetAttribLocation(ProgramID,"npoint");
    glEnableVertexAttribArray(npoint_id);
    glVertexAttribPointer(npoint_id,3,GL_FLOAT,false,0,0);

    glBindBuffer(GL_ARRAY_BUFFER,skyBufferID);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vskypoint),vskypoint,GL_DYNAMIC_DRAW);
    GLuint skypoint_id = glGetAttribLocation(ProgramID,"vskypoint");
    glEnableVertexAttribArray(skypoint_id);
    glVertexAttribPointer(skypoint_id,3,GL_FLOAT,false,0,0);

    loadpng(ProgramID); //load png file



    //glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);







}

void MouseMove(double x, double y)
{
    vppos_x = (float)(x) / 256 - 1;
    vppos_y = 1 - (float)(y) / 256;
    float dx = vppos_x- lastx;//record the last cursor postion x
    float dy = vppos_y- lasty;//record the last cursor postion y

    if(leftButtonPressed == true){//left button
         rotateAngle += RotatingSpeed * -dx;//rotate camera left or right (dx > 0 --> right; dx < 0 --> left )
         rotateAngle1 += RotatingSpeed * -dy;//rotate camera up or down (dy > 0 --> up; dy < 0 --> down)
    }
    if(rightButtonPressed == true){//right button
         dis += dy*0.01;//move camera along the gaze direction (dis > 0 --> away; dis < 0 --> closer)
    }
}

void MouseButton(MouseButtons mouseButton, bool press)
{
    if (mouseButton == LeftButton)
    {
        if (press == true) leftButtonPressed = true;
        else leftButtonPressed = false;
    }

    if (mouseButton == RightButton)
    {
        if (press == true) rightButtonPressed = true;
        else rightButtonPressed = false;
    }

}

void KeyPress(char keychar)
{

}

void OnPaint()
{
    /******************Setting up Mvp***********************/
    Matrix4f Mvp;
    Matrix4f viewtmp;
    Matrix4f viewrot;
    Matrix4f view;  //Mv
    Matrix4f Orth;  //Mo
    Matrix4f perspective;//Mp
    Vector3f EysPos(dis*sin(rotateAngle1)*sin(rotateAngle),dis*cos(rotateAngle1),dis*sin(rotateAngle1)*cos(rotateAngle));//0,0,5 initially
    Vector3f ViewUp(0,1,0);//up vector in Mv
    Vector3f GazeDir(0,0,0);//gaze vector in Mv

    /*Derive a coordinate system with origin e and uvw basis*/
    Vector3f W = -(GazeDir-EysPos).normalized();
    Vector3f U =(ViewUp.cross(W)).normalized();
    Vector3f V = W.cross(U);

    viewtmp<<1,0,0,-EysPos.x(),
             0,1,0,-EysPos.y(),
             0,0,1,-EysPos.z(),
             0,0,0,1;

    viewrot<<U.x(),U.y(),U.z(),0,
             V.x(),V.y(),V.z(),0,
             W.x(),W.y(),W.z(),0,
             0,0,0,1;

    view = viewrot*viewtmp;

    perspective<<1,0,0,0,
                 0,1,0,0,
                 0,0,-1-20/-1,20,
                 0,0,1/-1,0;

    Orth<<1,0,0,0,
          0,1,0,0,
          0,0,2/(-1-(-20)),-(-1-20)/(-1+20),
          0,0,0,1;

    Mvp = Orth * perspective * view;
    /*******************End**********************/
    //cout<<Mvp<<endl;


    glUseProgram(ProgramID);
    glBindVertexArray(VertexArrayID);
    glUniformMatrix4fv(MvpID,1,GL_FALSE,Mvp.data());
    /*
     * if leftbutton pressed --> RotationAngle
     * if rightbutton pressed --> RotationAngle2
     */
    //glClearDepth(1.0f);                   // Set background depth to farthest
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    //glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glDrawArrays(GL_TRIANGLES,0,36);//12 triangles each one has 3 vertices

    glUseProgram(0);
    glBindVertexArray(0);

}

void OnTimer()
{
    rotateAngle += RotatingSpeed;
}


int main(int, char **){
    //Link the call backs

    canvas.SetMouseMove(MouseMove);
    canvas.SetMouseButton(MouseButton);
    canvas.SetKeyPress(KeyPress);
    canvas.SetOnPaint(OnPaint);
    //canvas.SetTimer(0.05, OnTimer);
    //Show Window
    canvas.Initialize(width, height, "OpenGL Intro Demo");
    //Do our initialization
    InitializeGL();
    canvas.Show();

    return 0;
}

