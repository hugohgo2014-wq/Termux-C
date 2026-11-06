#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/input.h>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include "font8x8_basic.h"

#define LOG_TAG "TermuxC"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define COLS 64
#define ROWS 28
#define CELL 8

typedef struct { EGLDisplay display; EGLSurface surface; EGLContext context; EGLConfig config; int w,h; } Graphics;
static Graphics g={EGL_NO_DISPLAY,EGL_NO_SURFACE,EGL_NO_CONTEXT,0,0,0};
static int pty=-1;
static pid_t child=-1;
static unsigned char screen[ROWS][COLS];
static int cx=0,cy=0, scroll=0;
static bool running=false;

static void term_put(unsigned char c) {
 if(c=='\r'){cx=0;return;}
 if(c=='\n'){cx=0;if(++cy>=ROWS){memmove(screen[0],screen[1],(ROWS-1)*COLS);memset(screen[ROWS-1],' ',COLS);cy=ROWS-1;}return;}
 if(c=='\b'){if(cx>0)cx--;screen[cy][cx]=' ';return;}
 if(c=='\t'){cx=(cx+4)&~3;return;}
 if(c<32||c>126)return;
 screen[cy][cx++]=c;
 if(cx>=COLS){cx=0;if(++cy>=ROWS){memmove(screen[0],screen[1],(ROWS-1)*COLS);memset(screen[ROWS-1],' ',COLS);cy=ROWS-1;}}
}
static void term_write(const char *s){while(*s)term_put((unsigned char)*s++);}
static void pty_start(void) {
 struct winsize ws={.ws_row=ROWS,.ws_col=COLS,.ws_xpixel=0,.ws_ypixel=0};
 pty=posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK);
 if(pty<0||grantpt(pty)<0||unlockpt(pty)<0){term_write("PTY initialization failed\n");return;}
 char *slave=ptsname(pty); if(!slave){term_write("PTY slave unavailable\n");return;}
 child=fork();
 if(child==0){
  close(pty); setsid(); int fd=open(slave,O_RDWR);
  if(fd>=0){ioctl(fd,TIOCSWINSZ,&ws);dup2(fd,0);dup2(fd,1);dup2(fd,2);if(fd>2)close(fd);}
  setenv("TERM","xterm-256color",1);setenv("HOME","/data/data/com.terminal.nativeapp/files",1);
  chdir(getenv("HOME")); execl("/system/bin/sh","sh","-i",(char*)NULL); _exit(127);
 }
 if(child<0){term_write("Could not start Android shell\n");close(pty);pty=-1;return;}
 ioctl(pty,TIOCSWINSZ,&ws);running=true;
}
static void pty_read(void){
 if(pty<0)return; char b[2048]; ssize_t n;
 while((n=read(pty,b,sizeof(b)))>0)for(ssize_t i=0;i<n;i++)term_put((unsigned char)b[i]);
}
static void pty_send(const char *s,size_t n){if(pty>=0&&n) (void)write(pty,s,n);}
static bool gfx_init(ANativeWindow *win){
 const EGLint ca[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
 const EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE}; EGLint count=0;
 g.display=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(g.display==EGL_NO_DISPLAY||!eglInitialize(g.display,0,0))return false;
 if(!eglChooseConfig(g.display,ca,&g.config,1,&count)||!count)return false;
 g.surface=eglCreateWindowSurface(g.display,g.config,win,NULL);g.context=eglCreateContext(g.display,g.config,EGL_NO_CONTEXT,ctx);
 if(g.surface==EGL_NO_SURFACE||g.context==EGL_NO_CONTEXT||!eglMakeCurrent(g.display,g.surface,g.surface,g.context))return false;
 eglQuerySurface(g.display,g.surface,EGL_WIDTH,&g.w);eglQuerySurface(g.display,g.surface,EGL_HEIGHT,&g.h);return true;
}
static void rect(int x,int y,int w,int h,float r,float gg,float b){
 glScissor(x,y,w,h);glClearColor(r,gg,b,1);glClear(GL_COLOR_BUFFER_BIT);
}
static void gfx_draw(void){
 if(g.display==EGL_NO_DISPLAY)return;
 glViewport(0,0,g.w,g.h);glDisable(GL_DITHER);glEnable(GL_SCISSOR_TEST);
 glScissor(0,0,g.w,g.h);glClearColor(.018f,.025f,.04f,1);glClear(GL_COLOR_BUFFER_BIT);
 int cw=g.w/COLS,ch=g.h/ROWS;if(cw<1||ch<1){glDisable(GL_SCISSOR_TEST);return;}
 for(int y=0;y<ROWS;y++)for(int x=0;x<COLS;x++){
  unsigned char c=screen[y][x];if(c==' ')continue;
  for(int py=0;py<8;py++){unsigned char bits=font8x8_basic[c][py];
   for(int px=0;px<8;px++)if(bits&(1u<<px))rect(x*cw+px*cw/8,(ROWS-1-y)*ch+(7-py)*ch/8,(cw/8)+1,(ch/8)+1,.48f,.94f,.72f);
  }
 }
 glDisable(GL_SCISSOR_TEST);eglSwapBuffers(g.display,g.surface);
}
static void gfx_term(void){if(g.display!=EGL_NO_DISPLAY){eglMakeCurrent(g.display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(g.context!=EGL_NO_CONTEXT)eglDestroyContext(g.display,g.context);if(g.surface!=EGL_NO_SURFACE)eglDestroySurface(g.display,g.surface);eglTerminate(g.display);}g.display=EGL_NO_DISPLAY;g.surface=EGL_NO_SURFACE;g.context=EGL_NO_CONTEXT;}
static int32_t input(struct android_app *app,AInputEvent *e){
 (void)app;if(AInputEvent_getType(e)!=AINPUT_EVENT_TYPE_KEY||AKeyEvent_getAction(e)!=AKEY_EVENT_ACTION_DOWN)return 0;
 int k=AKeyEvent_getKeyCode(e),m=AKeyEvent_getMetaState(e);char out[2];size_t n=0;
 if(k==AKEYCODE_ENTER){out[0]='\n';n=1;}
 else if(k==AKEYCODE_DEL){out[0]=0x7f;n=1;}
 else if(k==AKEYCODE_SPACE){out[0]=' ';n=1;}
 else if(k>=AKEYCODE_A&&k<=AKEYCODE_Z){out[0]=(char)((m&AMETA_SHIFT_ON)?'A':'a'+(k-AKEYCODE_A));if(m&AMETA_SHIFT_ON)out[0]='A'+(k-AKEYCODE_A);n=1;}
 else if(k>=AKEYCODE_0&&k<=AKEYCODE_9){out[0]='0'+(k-AKEYCODE_0);n=1;}
 else {switch(k){case AKEYCODE_PERIOD:out[0]='.';break;case AKEYCODE_COMMA:out[0]=',';break;case AKEYCODE_MINUS:out[0]='-';break;case AKEYCODE_EQUALS:out[0]='=';break;case AKEYCODE_SLASH:out[0]='/';break;case AKEYCODE_APOSTROPHE:out[0]='\'';break;case AKEYCODE_GRAVE:out[0]='`';break;case AKEYCODE_LEFT_BRACKET:out[0]='[';break;case AKEYCODE_RIGHT_BRACKET:out[0]=']';break;case AKEYCODE_BACKSLASH:out[0]='\\';break;default:return 0;}n=1;}
 if(n){pty_send(out,n);return 1;}return 0;
}
static void cmd(struct android_app *app,int32_t c){switch(c){case APP_CMD_INIT_WINDOW:if(app->window&&gfx_init(app->window))gfx_draw();break;case APP_CMD_TERM_WINDOW:gfx_term();break;case APP_CMD_WINDOW_RESIZED:if(g.display!=EGL_NO_DISPLAY){eglQuerySurface(g.display,g.surface,EGL_WIDTH,&g.w);eglQuerySurface(g.display,g.surface,EGL_HEIGHT,&g.h);gfx_draw();}break;default:break;}}
void android_main(struct android_app *app){
 app_dummy();memset(screen,' ',sizeof(screen));term_write("NEBULA TERMINAL / native C + OpenGL ES 3.0\nAndroid system shell (app sandbox)\n");
 app->onAppCmd=cmd;app->onInputEvent=input;pty_start();
 while(!app->destroyRequested){int events;struct android_poll_source *source=NULL;int timeout=pty>=0?40:-1;int ident=ALooper_pollOnce(timeout,NULL,&events,(void**)&source);
  if(ident==ALOOPER_POLL_ERROR)break;if(source)source->process(app);
  if(pty>=0){pty_read();int status;if(child>0&&waitpid(child,&status,WNOHANG)==child){child=-1;term_write("\n[shell exited]\n");close(pty);pty=-1;}}
  gfx_draw();
 }
 if(child>0){kill(child,SIGHUP);waitpid(child,NULL,WNOHANG);}if(pty>=0)close(pty);gfx_term();
}
