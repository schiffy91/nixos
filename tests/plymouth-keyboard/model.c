/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "osk-layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void visibility(void)
{
        for (unsigned touches = 0; touches < 3; touches++)
        for (unsigned keyboards = 0; keyboards < 3; keyboards++)
        for (unsigned requested = 0; requested < 2; requested++) {
                osk_state s = { .touchscreens = touches, .keyboards = keyboards, .requested = requested };
                assert(!osk_visible(&s));
                osk_prompt(&s, true);
                assert(osk_visible(&s) == (touches > 0 && (requested || keyboards == 0)));
                osk_prompt(&s, false);
                assert(!osk_visible(&s));
        }
        osk_state s = { .touchscreens = 1, .prompt = true };
        assert(osk_visible(&s));
        s.keyboards = 1;
        assert(!osk_visible(&s));
        s.requested = true;
        assert(osk_visible(&s));
        osk_prompt(&s, false);
        osk_prompt(&s, true);
        assert(osk_visible(&s)); /* A retry keeps the user's touch preference. */
        s.touchscreens = 0;
        assert(!osk_visible(&s));
}

static void layouts(void)
{
        const double sizes[][2] = {
                {320,240}, {640,480}, {1280,800}, {800,1280},
                {1920,1080}, {3440,1440}, {3840,2160}, {2160,3840}
        };
        for (size_t n = 0; n < sizeof(sizes)/sizeof(sizes[0]); n++) {
                double w=sizes[n][0], h=sizes[n][1];
                assert(osk_hit(-1, -1, w, h) == -1);
                assert(osk_hit(w/2, h/4, w, h) == -1);
                for (size_t i=0; i<osk_key_count; i++) {
                        osk_rect r=osk_key_rect(i,w,h);
                        assert(r.x>=0 && r.y>=0 && r.x+r.width<=w && r.y+r.height<=h);
                        assert(r.width>0 && r.height>0);
                        assert(osk_hit(r.x+r.width/2,r.y+r.height/2,w,h)==(int)i);
                        assert(osk_hit(r.x+r.width+0.1,r.y+r.height/2,w,h)==-1);
                }
        }
}

static void characters(void)
{
        bool available[128]={false};
        for (size_t i=0; i<osk_key_count; i++) {
                if (osk_keys[i].action!=OSK_CHARACTER) continue;
                for (int shift=0; shift<2; shift++) {
                        osk_state s={.shift=shift};
                        const char *c=osk_key_text(&s,i);
                        assert(strlen(c)==1);
                        available[(unsigned char)*c]=true;
                }
                if (!strcmp(osk_keys[i].normal,"a")) {
                        osk_state s={.caps=true}; assert(!strcmp(osk_key_text(&s,i),"A"));
                        s.shift=true; assert(!strcmp(osk_key_text(&s,i),"a"));
                }
                if (!strcmp(osk_keys[i].normal,"1")) {
                        osk_state s={.caps=true}; assert(!strcmp(osk_key_text(&s,i),"1"));
                        s.shift=true; assert(!strcmp(osk_key_text(&s,i),"!"));
                }
        }
        for (int c=32;c<=126;c++) assert(available[c]);
}

static void rotations(void)
{
        double x=.2,y=.7;
        osk_transform(90,&x,&y); assert(fabs(x-.7)<1e-9 && fabs(y-.8)<1e-9);
        osk_transform(270,&x,&y); assert(fabs(x-.2)<1e-9 && fabs(y-.7)<1e-9);
        osk_transform(180,&x,&y); assert(fabs(x-.8)<1e-9 && fabs(y-.3)<1e-9);
        for (unsigned r=0;r<360;r+=90) {
                x=.123;y=.456; osk_transform(r,&x,&y); osk_transform((360-r)%360,&x,&y);
                assert(fabs(x-.123)<1e-9 && fabs(y-.456)<1e-9);
        }
}

int main(void)
{
        visibility(); layouts(); characters(); rotations();
        puts("PASS: visibility, hotplug policy, all printable US keys, hit regions, rotation");
        return 0;
}
