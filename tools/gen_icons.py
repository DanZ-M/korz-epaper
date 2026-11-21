#!/usr/bin/env python3
"""Regenerates src/epd_icons.c and src/epd_icons.h.

  python3 tools/gen_icons.py

Needs Pillow and numpy (pip install pillow numpy). The battery, Bluetooth, USB, profile and dog
pictures are read from the old OLED icon sources in tools/legacy-icons (LVGL 8 format), turned
upright, enlarged 2x and written as LVGL 9 images. The layer sphere is drawn by tools/sphere.py.
Edit sphere.py (font, sizes, row order) or this file to change a widget picture, then run it.
"""
import os, re, glob, json, sys
HERE=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0,HERE)

def load_legacy():
    icons={}
    for f in glob.glob(os.path.join(HERE,'legacy-icons','*.c')):
        t=open(f).read()
        arrs={}
        for m in re.finditer(r'uint8_t\s+(\w+)\[\]\s*=\s*\{(.*?)\};',t,re.S):
            b=re.sub(r'/\*.*?\*/','',m.group(2),flags=re.S)
            arrs[m.group(1)]=[int(x,16) for x in re.findall(r'0x[0-9a-fA-F]+',b)]
        for m in re.finditer(r'const lv_img_dsc_t\s+(\w+)\s*=\s*\{(.*?)\};',t,re.S):
            b=m.group(2)
            icons[m.group(1)]=dict(w=int(re.search(r'header\.w\s*=\s*(\d+)',b).group(1)),
                                   h=int(re.search(r'header\.h\s*=\s*(\d+)',b).group(1)),
                                   data=arrs[re.search(r'\.data\s*=\s*(\w+)',b).group(1)])
    return icons
from PIL import Image, ImageOps, ImageDraw
ic=load_legacy()
SCALE=2
def bitmap(v):
    d=v['data'];w,h=v['w'],v['h'];s=(w+7)//8
    im=Image.new('1',(w,h),1)
    for y in range(h):
        for x in range(w):
            b=(d[8+y*s+x//8]>>(7-x%8))&1
            im.putpixel((x,y),0 if b else 1)
    return im
def pack(im):
    w,h=im.size;s=(w+7)//8
    out=[0xff,0xff,0xff,0xff,0x00,0x00,0x00,0xff]
    for y in range(h):
        for bx in range(s):
            byte=0
            for k in range(8):
                x=bx*8+k
                byte=(byte<<1)|(1 if (x<w and im.getpixel((x,y))==0) else 0)
            out.append(byte)
    return w,h,s,out
def up2(im): return im.resize((im.width*SCALE,im.height*SCALE),Image.NEAREST)
def icon(name): return up2(bitmap(ic[name]).rotate(90,expand=True))
# ---- dog: the old OLED was inverted (black dog, white markings on a black field) ----
def dogframes():
    fr=[bitmap(ic[n]).rotate(90,expand=True) for n in('pet_sit_1','pet_walk_1','pet_run_1')]
    x0=y0=999;x1=y1=-1
    for im in fr:
        px=im.load()
        for y in range(im.height):
            for x in range(im.width):
                if px[x,y]==0: x0=min(x0,x);x1=max(x1,x);y0=min(y0,y);y1=max(y1,y)
    m=3;box=(max(0,x0-m),max(0,y0-m),min(64,x1+1+m),min(64,y1+1+m))
    return [up2(im.crop(box)) for im in fr]
items=[('epd_bat_charging',icon('battery_charging'))]
bat=[99,93,87,80,73,67,60,53,47,40,33,27,20,13,7,2]
items+=[('epd_bat_%d'%p,icon('battery_%d'%p)) for p in bat]
items+=[('epd_out_usb',icon('usb_connected')),('epd_out_ble_adv',icon('bluetooth_advertising')),('epd_out_ble_conn',icon('bluetooth_connected')),('epd_out_ble_disc',icon('bluetooth_disconnected'))]
items+=[('epd_prof_usb',icon('usb_profile')),('epd_prof_unknown',icon('bluetooth_profile_unknown'))]+[('epd_prof_%d'%i,icon('bluetooth_profile_%d'%i)) for i in range(1,6)]
df=dogframes()
items+=[('epd_pet_sit',df[0]),('epd_pet_walk',df[1]),('epd_pet_run',df[2])]
import sphere
items+=[('epd_sphere_%d'%i,sphere.render(i)) for i in range(4)]+[('epd_sphere_unknown',sphere.render(None))]
c=['''/*
 * korz-epaper: icons for the e-paper status screen (generated, 1-bit indexed LVGL 9 images:
 * white = 0, black = 1).
 *  - battery, output, profile and pet: from the old left OLED icons (HellTM, MIT), turned
 *    upright (the OLED was mounted sideways) and enlarged 2x.
 *  - sphere: the layers engraved on a sphere (NUMB, BASE, MOUSE, symbols from bottom to top);
 *    the active layer faces the viewer in a dark band between two pointers.
 */
#include "epd_icons.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif
''']
h=['''/* korz-epaper: icons for the e-paper status screen (generated, see epd_icons.c) */
#pragma once
#include <lvgl.h>
''']
for sym,im in items:
    w,hh,s,data=pack(im)
    lines=['\t'+', '.join('0x%02x'%b for b in data[i:i+16])+',' for i in range(0,len(data),16)]
    c.append('static const LV_ATTRIBUTE_MEM_ALIGN uint8_t %s_map[] = {\n%s\n};\n'%(sym,'\n'.join(lines)))
    c.append('const lv_image_dsc_t %s = {\n\t.header.magic = LV_IMAGE_HEADER_MAGIC,\n\t.header.cf = LV_COLOR_FORMAT_I1,\n\t.header.flags = 0,\n\t.header.w = %d,\n\t.header.h = %d,\n\t.header.stride = %d,\n\t.data_size = sizeof(%s_map),\n\t.data = %s_map,\n};\n'%(sym,w,hh,s,sym,sym))
    h.append('extern const lv_image_dsc_t %s; /* %dx%d */'%(sym,w,hh))
c.append('''
const lv_image_dsc_t *const epd_bat_level_icon[16] = {
%s
};
const uint8_t epd_bat_level_min[16] = {%s};

const lv_image_dsc_t *const epd_prof_ble_icon[5] = {&epd_prof_1, &epd_prof_2, &epd_prof_3, &epd_prof_4, &epd_prof_5};

const lv_image_dsc_t *const epd_sphere_icon[4] = {&epd_sphere_0, &epd_sphere_1, &epd_sphere_2, &epd_sphere_3};
'''%(',\n'.join('\t&epd_bat_%d'%p for p in bat),', '.join(str(p) for p in bat)))
h.append('''
/* battery: the icon of the first entry whose minimum level is <= the charge */
extern const lv_image_dsc_t *const epd_bat_level_icon[16];
extern const uint8_t epd_bat_level_min[16];
extern const lv_image_dsc_t *const epd_prof_ble_icon[5]; /* BLE profile 1..5 */
/* layer sphere: the picture for layer 0..3 (BASE, NUMB, MOUSE, FUNC), epd_sphere_unknown otherwise */
extern const lv_image_dsc_t *const epd_sphere_icon[4];
''')
d=os.path.join(HERE,'..','src')+os.sep
open(d+'epd_icons.c','w').write('\n'.join(c)); open(d+'epd_icons.h','w').write('\n'.join(h))
print('wrote',d+'epd_icons.c')
