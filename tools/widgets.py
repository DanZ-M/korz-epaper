"""Pictures for the volume arc and the two half keyboards (1-bit, PIL images)."""
import math
from PIL import Image, ImageDraw

SS = 4
# ---- volume arc: 7 segments over the layer sphere, speaker on the right ----
ARC_W, ARC_H = 128, 78
ARC_CX, ARC_CY = 58, 56          # where the sphere centre is inside this picture
ARC_R = 49.0                     # radius of the arc's centre line
SEGMENTS = 7
A0, A1 = 200.0, 20.0             # from the lower left, over the top, to the upper right

def _poly(d, pts, fill, outline=None, width=0):
    d.polygon([(x * SS, y * SS) for x, y in pts], fill=fill, outline=outline, width=width)

def arc(level, muted):
    """level 0..7 segments filled (black); muted: all outlined, speaker crossed."""
    im = Image.new('L', (ARC_W * SS, ARC_H * SS), 255)
    d = ImageDraw.Draw(im)
    n = SEGMENTS
    for i in range(n):
        span = (A1 - A0) / n
        sgn = 1 if span > 0 else -1
        ta = A0 + span * i + 2.0 * sgn          # 2 degrees off each end: the gap
        tb = A0 + span * (i + 1) - 2.0 * sgn
        w = 3.5 + i * 1.1
        ro, ri = ARC_R + w / 2, ARC_R - w / 2
        pts = []
        steps = 16
        for k in range(steps + 1):
            a = math.radians(ta + (tb - ta) * k / steps)
            pts.append((ARC_CX + ro * math.cos(a), ARC_CY - ro * math.sin(a)))
        for k in range(steps, -1, -1):
            a = math.radians(ta + (tb - ta) * k / steps)
            pts.append((ARC_CX + ri * math.cos(a), ARC_CY - ri * math.sin(a)))
        if i < level and not muted:
            _poly(d, pts, 0)
        else:
            _poly(d, pts, 255, outline=0, width=SS)      # 1 px outline
    # pointer: an outlined triangle between the sphere and the arc, at the last filled segment
    if not muted:
        seg = max(level, 1) - 1
        a = math.radians(A0 + (A1 - A0) * (seg + 0.5) / n)
        r = 40.5
        px, py = ARC_CX + r * math.cos(a), ARC_CY - r * math.sin(a)
        _poly(d, [(px - 3, py - 3.5), (px - 3, py + 3.5), (px + 4, py)], 255, outline=0, width=SS)
    # speaker
    sx, sy = 106.0, float(ARC_CY)
    _poly(d, [(sx, sy - 4), (sx + 4, sy - 4), (sx + 10, sy - 10), (sx + 10, sy + 10), (sx + 4, sy + 4), (sx, sy + 4)], 0)
    if muted:
        d.line([((sx + 13) * SS, (sy - 5) * SS), ((sx + 21) * SS, (sy + 5) * SS)], fill=0, width=int(1.8 * SS))
        d.line([((sx + 13) * SS, (sy + 5) * SS), ((sx + 21) * SS, (sy - 5) * SS)], fill=0, width=int(1.8 * SS))
    else:
        for rr in (6, 11):
            cx = sx + 9
            d.arc([(cx - rr) * SS, (sy - rr) * SS, (cx + rr) * SS, (sy + rr) * SS], -50, 50, fill=0, width=int(1.6 * SS))
    small = im.resize((ARC_W, ARC_H), Image.BOX)
    return small.point(lambda v: 0 if v < 140 else 255).convert('1', dither=Image.NONE)

# ---- half keyboards: 6 columns x 3 rows plus 3 thumb keys, 48x37 ----
HALF_W, HALF_H = 48, 37
KEY, PITCH = 6, 8
OFFS = [6, 5, 2, 0, 2, 4]        # column stagger, outer -> inner

def half(side, connected):
    im = Image.new('1', (HALF_W, HALF_H), 1)
    px = im.load()
    cols = list(range(6))
    keys = []
    for c in cols:
        for r in range(3):
            keys.append((c * PITCH, OFFS[c] + r * PITCH))
    for c in (3, 4, 5):
        keys.append((c * PITCH, 30))
    if side == 'R':
        keys = [(HALF_W - 2 - x - KEY, y) for x, y in keys]
    for x, y in keys:
        for t in range(KEY):
            for (xx, yy) in ((x + t, y), (x + t, y + KEY - 1), (x, y + t), (x + KEY - 1, y + t)):
                if connected or (xx + yy) % 2 == 0:       # dotted when not connected
                    if 0 <= xx < HALF_W and 0 <= yy < HALF_H:
                        px[xx, yy] = 0
    return im


def _outline(side):
    """Contour of one half (stair-stepped over the columns and the thumb keys), clockwise."""
    xl = [max(0, c * PITCH - 1) for c in range(6)]
    xr = [c * PITCH + KEY for c in range(6)]
    top = [max(0, OFFS[c] - 1) for c in range(6)]
    bot = [OFFS[c] + 2 * PITCH + KEY + 1 for c in range(6)]
    p = []
    for c in range(6):
        p += [(xl[c], top[c]), (xr[c], top[c])]
    p += [(xr[5], 36), (xl[3], 36)]
    for c in (2, 1, 0):
        p += [(xr[c], bot[c]), (xl[c], bot[c])]
    if side == 'R':
        p = [(HALF_W - 1 - x, y) for x, y in p]
    return p

def half_lost(side):
    """Half not connected: short dashes along the contour and a bold X over it."""
    k = SS
    im = Image.new('L', (HALF_W * k, HALF_H * k), 255)
    d = ImageDraw.Draw(im)
    pts = _outline(side)
    pts.append(pts[0])
    s = 0.0
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        n = max(1, int(math.hypot(x1 - x0, y1 - y0) * k))
        for i in range(n):
            t = i / n
            if int((s + t * math.hypot(x1 - x0, y1 - y0)) // 3) % 2 == 0:
                x, y = (x0 + (x1 - x0) * t) * k, (y0 + (y1 - y0) * t) * k
                d.rectangle([x - k / 2, y - k / 2, x + k / 2, y + k / 2], fill=0)
        s += math.hypot(x1 - x0, y1 - y0)
    a, b = 8.0, 6.0
    for (p0, p1) in (((a, b), (HALF_W - a, HALF_H - b)), ((HALF_W - a, b), (a, HALF_H - b))):
        q = [(p0[0] * k, p0[1] * k), (p1[0] * k, p1[1] * k)]
        d.line(q, fill=255, width=int(5.0 * k))     # white halo: the dashes do not touch the X
        d.line(q, fill=0, width=int(2.4 * k))
    small = im.resize((HALF_W, HALF_H), Image.BOX)
    return small.point(lambda v: 0 if v < 140 else 255).convert('1', dither=Image.NONE)
