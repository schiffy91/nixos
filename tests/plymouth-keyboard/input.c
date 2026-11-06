/* A kernel uinput fixture: Plymouth still consumes real udev/libinput events. */
#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void emit(int fd, unsigned type, unsigned code, int value)
{
        struct input_event e={.type=type,.code=code,.value=value};
        assert(write(fd,&e,sizeof(e))==sizeof(e));
}
static void abs_axis(int fd, unsigned code, int max)
{
        assert(ioctl(fd,UI_SET_ABSBIT,code)==0);
        struct uinput_abs_setup a={.code=code,.absinfo={.minimum=0,.maximum=max}};
        assert(ioctl(fd,UI_ABS_SETUP,&a)==0);
}
static int create(int touch)
{
        int fd=open("/dev/uinput",O_WRONLY|O_NONBLOCK); assert(fd>=0);
        assert(ioctl(fd,UI_SET_EVBIT,EV_KEY)==0);
        if (touch) {
                assert(ioctl(fd,UI_SET_KEYBIT,BTN_TOUCH)==0);
                assert(ioctl(fd,UI_SET_EVBIT,EV_ABS)==0);
                assert(ioctl(fd,UI_SET_PROPBIT,INPUT_PROP_DIRECT)==0);
                abs_axis(fd,ABS_X,32767); abs_axis(fd,ABS_Y,32767);
                abs_axis(fd,ABS_MT_SLOT,9); abs_axis(fd,ABS_MT_TRACKING_ID,65535);
                abs_axis(fd,ABS_MT_POSITION_X,32767); abs_axis(fd,ABS_MT_POSITION_Y,32767);
        } else {
                for (int key=KEY_ESC;key<=KEY_SPACE;key++) assert(ioctl(fd,UI_SET_KEYBIT,key)==0);
        }
        struct uinput_setup setup={.id={.bustype=BUS_USB,.vendor=0xfeed,.product=touch?1:2}};
        snprintf(setup.name,sizeof(setup.name),"Plymouth test %s",touch?"touchscreen":"keyboard");
        assert(ioctl(fd,UI_DEV_SETUP,&setup)==0);
        assert(ioctl(fd,UI_DEV_CREATE)==0);
        return fd;
}
int main(int argc,char **argv)
{
        int touch=-1,keyboard=-1,tracking=0;
        if (argc>1 && !strcmp(argv[1],"keyboard")) keyboard=create(0);
        if (argc>1 && !strcmp(argv[1],"touchscreen")) touch=create(1);
        assert(mkfifo("/run/test-input",0600)==0);
        FILE *input=fdopen(open("/run/test-input",O_RDWR),"r"); assert(input);
        char line[128],command[32];
        while (fgets(line,sizeof(line),input)) {
                int x=0,y=0,slot=0; sscanf(line,"%31s %d %d %d",command,&x,&y,&slot);
                if (!strcmp(command,"touch-on") && touch<0) touch=create(1);
                else if (!strcmp(command,"touch-off") && touch>=0) { close(touch); touch=-1; }
                else if (!strcmp(command,"keyboard-on") && keyboard<0) keyboard=create(0);
                else if (!strcmp(command,"keyboard-off") && keyboard>=0) { close(keyboard); keyboard=-1; }
                else if ((!strcmp(command,"down") || !strcmp(command,"move")) && touch>=0) {
                        emit(touch,EV_ABS,ABS_MT_SLOT,slot);
                        if (!strcmp(command,"down")) emit(touch,EV_ABS,ABS_MT_TRACKING_ID,++tracking);
                        emit(touch,EV_ABS,ABS_MT_POSITION_X,x); emit(touch,EV_ABS,ABS_MT_POSITION_Y,y);
                        emit(touch,EV_ABS,ABS_X,x); emit(touch,EV_ABS,ABS_Y,y);
                        emit(touch,EV_KEY,BTN_TOUCH,1); emit(touch,EV_SYN,SYN_REPORT,0);
                } else if (!strcmp(command,"up") && touch>=0) {
                        emit(touch,EV_ABS,ABS_MT_SLOT,slot); emit(touch,EV_ABS,ABS_MT_TRACKING_ID,-1);
                        emit(touch,EV_KEY,BTN_TOUCH,0); emit(touch,EV_SYN,SYN_REPORT,0);
                } else if (!strcmp(command,"key") && keyboard>=0) {
                        emit(keyboard,EV_KEY,x,1); emit(keyboard,EV_SYN,SYN_REPORT,0);
                        emit(keyboard,EV_KEY,x,0); emit(keyboard,EV_SYN,SYN_REPORT,0);
                }
        }
        return 0;
}
