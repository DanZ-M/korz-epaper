import numpy as np, math
from PIL import Image, ImageDraw, ImageFont
import os
FONT=next((f for f in (os.environ.get('SPHERE_FONT',''),
    '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf',
    'C:/Windows/Fonts/arialnb.ttf','C:/Windows/Fonts/arialbd.ttf',
    '/Library/Fonts/Arial Bold.ttf') if f and os.path.exists(f)),None)   # any bold sans-serif TTF
K=float(os.environ.get('SPHERE_SCALE','0.9'))   # 1.0 = the first size (104x80)
R=39.0*K; W_IMG,H_IMG=round(104*K),round(80*K); CX,CY=W_IMG/2.0,H_IMG/2.0
SS=4               # supersampling of the output
TS=4               # texture pixels per sphere pixel
PITCH=0.76*R       # arc length between rows (rows are 0.76 rad apart)
TEXT_W=64*K; CAP=21*K
BAND=15.0*K          # half height of the dark band (pixels)
ORDER=['SYM','MOUSE','BASE','NUMB']          # top -> bottom
LAYER_NAME={0:'BASE',1:'NUMB',2:'MOUSE',3:'SYM'}
def row_img(name):
    w,h=int(TEXT_W*TS),int(CAP*TS)
    im=Image.new('L',(w,h),0);d=ImageDraw.Draw(im)
    if name=='SYM':
        # X [] O /\ : thick geometric shapes in four equal cells
        cell=w/4; s=h*0.78; lw=int(2.6*TS)
        cy=h/2
        for i in range(4):
            cx=cell*(i+0.5)
            if i==0:
                d.line([(cx-s/2,cy-s/2),(cx+s/2,cy+s/2)],fill=255,width=lw)
                d.line([(cx-s/2,cy+s/2),(cx+s/2,cy-s/2)],fill=255,width=lw)
            elif i==1:
                d.rectangle([cx-s/2,cy-s/2,cx+s/2,cy+s/2],outline=255,width=lw)
            elif i==2:
                d.ellipse([cx-s/2,cy-s/2,cx+s/2,cy+s/2],outline=255,width=lw)
            else:
                d.polygon([(cx,cy-s*0.56),(cx+s*0.6,cy+s*0.48),(cx-s*0.6,cy+s*0.48)],outline=255,width=lw)
        return im
    f=ImageFont.truetype(FONT,int(h*1.45))
    bb=d.textbbox((0,0),name,font=f)
    tmp=Image.new('L',(bb[2]-bb[0],bb[3]-bb[1]),0)
    ImageDraw.Draw(tmp).text((-bb[0],-bb[1]),name,font=f,fill=255)
    return tmp.resize((w,h),Image.LANCZOS)         # every word the same width and height
def texture(active_layer):
    a=ORDER.index(LAYER_NAME[active_layer])
    TW=int(150*TS);TH=int(8*PITCH*TS)
    tex=Image.new('L',(TW,TH),0)
    for i,name in enumerate(ORDER):
        r=row_img(name)
        yc=TH/2+(i-a)*PITCH*TS
        tex.paste(r,(int(TW/2-r.width/2),int(yc-r.height/2)))
    return np.asarray(tex,dtype=np.float32)/255.0
BAYER=np.array([[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]])
def render(active_layer, chevrons=False):
    n=SS
    H,W=H_IMG*n,W_IMG*n
    ys,xs=np.mgrid[0:H,0:W]
    x=(xs+0.5)/n-CX; y=(ys+0.5)/n-CY
    r=np.hypot(x,y); inside=r<=R
    yy=np.clip(y/R,-0.999,0.999); phi=np.arcsin(yy)
    cphi=np.cos(phi)
    sl=np.clip(x/(R*cphi+1e-6),-0.999,0.999); lam=np.arcsin(sl)
    u=lam*R; v=phi*R                                        # arc lengths on the sphere
    if active_layer is None:
        text=np.zeros((H,W),dtype=np.float32)
    else:
        tex=texture(active_layer); th,tw=tex.shape
        tx=np.clip((tw/2+u*TS).astype(int),0,tw-1); ty=np.clip((th/2+v*TS).astype(int),0,th-1)
        text=tex[ty,tx]
        text[(np.abs(u)>R*1.2)]=0
    band=(np.abs(y)<=BAND)&inside if active_layer is not None else np.zeros((H,W),bool)
    white=~band                       # body is white, band is dark
    ink=np.where(band,text<0.5,text>0.5)   # ink=1 means black: text on the body, background of the band
    ink=ink&inside
    # light dither shading at the rim (light from the upper left), only on white pixels
    rho=r/R; shade=np.clip((rho-0.80)/0.20,0,1)**1.3
    dirf=0.55+0.45*np.clip((x*0.6+y*0.8)/R,-1,1)
    shade=np.clip(shade*dirf*1.2,0,1)
    bx=(np.floor(xs/n).astype(int))%4; by=(np.floor(ys/n).astype(int))%4
    dither=(shade*16)>(BAYER[by,bx]+0.5)
    ink=ink|(dither&inside&~ink&~band&(text<0.5))   # shade only empty body (keeps letters clean)
    # outline
    ink=ink|((r<=R)&(r>=R-1.8))
    out=~ink|~inside
    img=Image.fromarray((np.where(ink&inside,0,255)).astype(np.uint8),'L')
    # downsample SSxSS: a pixel is black if more than half of its samples are
    arr=np.asarray(img,dtype=np.float32).reshape(H_IMG,n,W_IMG,n).mean(axis=(1,3))
    small=Image.fromarray((arr<150).astype(np.uint8)*0+np.where(arr<150,0,255).astype(np.uint8),'L')
    out=small.convert('1',dither=Image.NONE)
    d=ImageDraw.Draw(out)
    if chevrons:
        for sx in (1,-1):
            bx0=CX+sx*(R+5)
            pts=[(bx0-sx*0 ,CY-9),(bx0+sx*8*(-1) , CY)]
        # > on the left (points right) and < on the right (points left)
        L=[(2,CY-8),(8,CY),(2,CY+8)]; Rr=[(W_IMG-3,CY-8),(W_IMG-9,CY),(W_IMG-3,CY+8)]
        d.line(L,fill=0,width=3,joint='curve'); d.line(Rr,fill=0,width=3,joint='curve')
    return out
if __name__=='__main__':
    sheet=Image.new('RGB',(5*(W_IMG*3+10)+10,H_IMG*3+20),(170,170,255))
    for i,a in enumerate([0,1,2,3,None]):
        im=render(a).resize((W_IMG*3,H_IMG*3),Image.NEAREST).convert('RGB')
        sheet.paste(im,(10+i*(W_IMG*3+10),10))
    sheet.save(os.path.join(os.path.dirname(os.path.abspath(__file__)),'sphere_preview.png'))
