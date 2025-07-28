/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * cube.c
 * This is a tech demo for beacon, intended to show off math.c
 */

#include "string.h"
#include "command.h"
#include "console.h"
#include "screen.h"
#include "keyboard.h"
#include "math.h"
#include "os.h"
#include <stdint.h>

#define WIDTH 80
#define HEIGHT 25
#define CUBE_SIZE 20
#define FACE_RES 8

char framebuffer[HEIGHT][WIDTH + 1];

typedef struct { float x,y,z; } Vec3;

typedef struct {
    Vec3 vertices[8];
    int edges[12][2];
} Cube;

typedef struct {
    char pixels[FACE_RES][FACE_RES];
} FacePattern;

// cube geometry
static Cube cube = {
    .vertices = {
        {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1},
        {-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1},
    },
    .edges = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7},
    }
};

// which verts make each face
const int cube_faces[6][4] = {
    {0,1,2,3}, // front
    {5,4,7,6}, // back
    {4,0,3,7}, // left
    {1,5,6,2}, // right
    {3,2,6,7}, // top
    {4,5,1,0}  // bottom
};

// sample patterns
FacePattern face_patterns[6] = {
    { .pixels = { "@@@@@@@@","@      @","@ @@@@ @","@ @  @ @","@ @  @ @","@ @@@@ @","@      @","@@@@@@@@" } },
    { .pixels = { "# # # # "," # # # #","# # # # "," # # # #","# # # # "," # # # #","# # # # "," # # # #" } },
    { .pixels = { "/       "," /      ","  /     ","   /    ","    /   ","     /  ","      / ","       /" } },
    { .pixels = { "X      X"," X    X ","  X  X  ","   XX   ","   XX   ","  X  X  "," X    X ","X      X" } },
    { .pixels = { "  ++++  ","  ++++  ","++++++++","++++++++","++++++++","++++++++","  ++++  ","  ++++  " } },
    { .pixels = { "OOOOOOOO","O      O","O      O","O      O","O      O","O      O","O      O","OOOOOOOO" } },
};

// vec math
Vec3 subtract(Vec3 a, Vec3 b){ return (Vec3){a.x-b.x,a.y-b.y,a.z-b.z}; }
Vec3 cross(Vec3 a, Vec3 b){ return (Vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
float dot(Vec3 a, Vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }

// fb funcs
void clear_framebuffer(){
    for(int y=0;y<HEIGHT;++y){
        for(int x=0;x<WIDTH;++x) framebuffer[y][x]=' ';
        framebuffer[y][WIDTH]='\0';
    }
}
void draw_point(int x,int y,char c){
    if(x>=0&&x<WIDTH&&y>=0&&y<HEIGHT) framebuffer[y][x]=c;
}
int iabs(int x){return x<0?-x:x;}
void draw_line(int x0,int y0,int x1,int y1,char c){
    int dx=iabs(x1-x0), dy=-iabs(y1-y0),
        sx=x0<x1?1:-1, sy=y0<y1?1:-1, err=dx+dy;
    while(1){
        draw_point(x0,y0,c);
        if(x0==x1&&y0==y1) break;
        int e2=2*err;
        if(e2>=dy){ err+=dy; x0+=sx; }
        if(e2<=dx){ err+=dx; y0+=sy; }
    }
}

void render_frame(float yaw,float pitch){
    Vec3 world[8], proj[8];
    // rotate + translate to world coords
    for(int i=0;i<8;++i){
        Vec3 v=cube.vertices[i];
        float cY=cosf(yaw), sY=sinf(yaw);
        float cX=cosf(pitch), sX=sinf(pitch);
        // yaw
        float x=v.x*cY - v.z*sY, z=v.x*sY + v.z*cY;
        v.x=x; v.z=z;
        // pitch
        float y=v.y*cX - v.z*sX;
        z=v.y*sX + v.z*cX;
        v.y=y; v.z=z + 3.5f;
        world[i]=v;
    }

    clear_framebuffer();

    // cull+draw textured faces
    Vec3 cam_to = {0,0,0};
    for(int f=0;f<6;++f){
        const int *F=cube_faces[f];
        Vec3 A=world[F[0]], B=world[F[1]], C=world[F[2]], D=world[F[3]];
        Vec3 norm = cross(subtract(B,A), subtract(C,A));
        // vector from face to cam = cam(0,0,0) - A
        Vec3 toCam = subtract(cam_to, A);
        if(dot(norm,toCam)<=0) continue; // backface

        FacePattern *P = &face_patterns[f];
        // project verts only for visible faces
        for(int i=0;i<4;++i){
            Vec3 v = world[F[i]];
            float s = CUBE_SIZE / v.z;
            proj[F[i]].x = v.x*s + WIDTH/2;
            proj[F[i]].y = v.y*s + HEIGHT/2;
        }
        // render 8x8
        for(int py=0;py<FACE_RES;++py){
            float ty = py/(float)(FACE_RES-1);
            float xs = proj[F[0]].x + (proj[F[3]].x - proj[F[0]].x)*ty;
            float ys = proj[F[0]].y + (proj[F[3]].y - proj[F[0]].y)*ty;
            float xe = proj[F[1]].x + (proj[F[2]].x - proj[F[1]].x)*ty;
            float ye = proj[F[1]].y + (proj[F[2]].y - proj[F[1]].y)*ty;
            for(int px=0;px<FACE_RES;++px){
                float tx = px/(float)(FACE_RES-1);
                int sx = (int)(xs + (xe-xs)*tx);
                int sy = (int)(ys + (ye-ys)*tx);
                draw_point(sx, sy, P->pixels[py][px]);
            }
        }
    }

    // project remaining verts for wireframe
    for(int i=0;i<8;++i){
        Vec3 v=world[i];
        float s=CUBE_SIZE/v.z;
        proj[i].x=v.x*s+WIDTH/2;
        proj[i].y=v.y*s+HEIGHT/2;
    }
    // draw wireframe
    for(int e=0;e<12;++e){
        int a=cube.edges[e][0], b=cube.edges[e][1];
        draw_line((int)proj[a].x,(int)proj[a].y,(int)proj[b].x,(int)proj[b].y,'#');
    }

    // blit
    for(int y=0;y<HEIGHT;++y){
        gotoxy(0,y);
        print(framebuffer[y]);
    }
}

void cube_main(void) {
    clear_screen();
    disable_cursor();
    gotoxy(0, 0);
    print("Cube Renderer\n");
    print("Use arrow keys to rotate, +/- to change speed, Q to return to OS.\n");
    set_color(0x7,0x0);
    repaint_screen(0x7,0x0);

    float yaw=0,pitch=0;
    float speed=0.1f;
    while(1){
        int key = getch();
        switch(key) {
            case LEFT_ARROW:  yaw -= speed; break;
            case RIGHT_ARROW: yaw += speed; break;
            case UP_ARROW:    pitch -= speed; break;
            case DOWN_ARROW:  pitch += speed; break;
            case '-':         speed -= 0.01f; if(speed < 0.01f) speed = 0.01f; break;
            case '=': case '+': speed += 0.01f; break;
            case 'q': case 'Q': reset(); break;
            default: break;
        }        
        render_frame(yaw,pitch);
    }
}
