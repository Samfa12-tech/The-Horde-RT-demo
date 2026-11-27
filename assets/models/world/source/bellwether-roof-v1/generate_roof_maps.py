"""Deterministic, original 1K plain clay roof material; no external inputs."""
from pathlib import Path
import hashlib
import json
import math
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[5]
OUT = Path(__file__).resolve().parent / "maps"
REPORT = ROOT / "reports/bellwether-roof-v1"
SIZE = 1024
PAD = SIZE * 3
PAD_ORIGIN = SIZE
REPEAT_M = 2.4
TILE_COLS, COURSES = 12, 16
PX_PER_M = SIZE / REPEAT_M
TILE_W = SIZE / TILE_COLS
COURSE_PITCH = SIZE / COURSES
TILE_H = 0.244 * PX_PER_M
SEED = 27112026


def smooth(a, b, x):
    t = np.clip((x-a)/(b-a), 0.0, 1.0)
    return t*t*(3.0-2.0*t)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.mkdir(parents=True, exist_ok=True)
    yy, xx = np.mgrid[0:PAD, 0:PAD].astype(np.float32)
    world_x, world_y = xx-PAD_ORIGIN, yy-PAD_ORIGIN
    # Broad, exactly periodic low-amplitude mineral and kiln variation.
    tau = 2.0 * math.pi / SIZE
    clouds = (np.sin(world_x*tau*3 + np.sin(world_y*tau*2)*1.7) * 2.6 +
              np.sin(world_y*tau*5 + np.sin(world_x*tau*2)*1.1) * 2.2 +
              np.sin((world_x+world_y)*tau*7) * 1.1)
    grit = (np.sin(world_x*tau*119+world_y*tau*23) * np.sin(world_y*tau*83-world_x*tau*31) +
            np.sin(world_x*tau*73-world_y*tau*141) * np.sin(world_y*tau*49+world_x*tau*89)) * 1.1
    base = np.empty((PAD, PAD, 3), np.float32)
    base[:] = [83, 54, 43]  # dark clay bed visible only in tight joints
    height = np.zeros((PAD, PAD), np.float32)
    ao = np.full((PAD, PAD), 0.63, np.float32)
    rough = np.full((PAD, PAD), 0.94, np.float32)

    # Muted iron-rich clay: aged brick, umber and dusty ochre, without lighting.
    palette = np.array([[139, 75, 53], [150, 82, 57], [128, 68, 50],
                        [157, 88, 61], [135, 78, 59], [146, 77, 52],
                        [121, 67, 52], [153, 91, 68]], np.float32)
    rng = np.random.default_rng(SEED)
    colors = {(r, c): palette[int(rng.integers(len(palette)))] *
              rng.uniform(.96, 1.045) for r in range(COURSES)
              for c in range(TILE_COLS)}

    # Draw lower courses first so upper courses physically lap over them.
    # The tile lattice and deterministic variations close exactly at 2.4 m.
    for row in range(COURSES*2, -COURSES-1, -1):
        for col in range(-TILE_COLS, TILE_COLS*2):
            seed = SEED + (row % COURSES)*977 + (col % TILE_COLS)*131
            local = np.random.default_rng(seed)
            stagger = (0.5*TILE_W) if (row % 2) else 0.0
            cx = PAD_ORIGIN + col*TILE_W + stagger + TILE_W*.5
            # Place the course's downhill lip on the course-pitch boundary.
            # This makes every one of the 16 lips land inside the 2.4m repeat,
            # including the lip that meets the V=0/eaves wrap edge.
            cy = PAD_ORIGIN + row*COURSE_PITCH - TILE_H*.5 + local.uniform(-1.2, 1.2)
            # Keep all calculations to a tile-local ROI; rounded edges are
            # gently hand-cut, with a few shallow chips rather than damage.
            left, right = cx-TILE_W*.49, cx+TILE_W*.49
            top, bottom = cy-TILE_H*.5, cy+TILE_H*.5
            x0=max(0, int(math.floor(left-3))); x1=min(PAD, int(math.ceil(right+3)))
            y0=max(0, int(math.floor(top-3))); y1=min(PAD, int(math.ceil(bottom+3)))
            if x0 >= x1 or y0 >= y1: continue
            X, Y = xx[y0:y1, x0:x1]-cx, yy[y0:y1, x0:x1]-cy
            hx, hy, rad = TILE_W*.5-1.7, TILE_H*.5-1.4, 5.0
            qx, qy = np.abs(X)-hx+rad, np.abs(Y)-hy+rad
            sdf = np.sqrt(np.maximum(qx,0)**2 + np.maximum(qy,0)**2) + np.minimum(np.maximum(qx,qy),0)-rad
            # Gentle wavy hand-formed perimeter; periodic tile-to-tile variety.
            edge_wobble = (0.75*np.sin(Y*.14+seed*.13) +
                           0.48*np.sin(X*.19-seed*.07))
            # Rare, tiny edge nicks, at most 1.5 px and not repeated uniformly.
            for _ in range(2):
                if local.random() < .48:
                    ax=local.uniform(-hx*.8,hx*.8); ay=local.choice([-1,1])*hy
                    nick=np.sqrt((X-ax)**2+(Y-ay)**2)
                    edge_wobble -= 1.3*np.exp(-(nick/1.8)**2)
            sd = sdf-edge_wobble
            inside = sd < 0
            if not inside.any(): continue
            depth = np.maximum(0, -sd)
            bevel = smooth(0, 5.2, depth)
            # Slight convex handmade faces, a lower downhill rolled lip, and
            # very shallow mineral/pitting relief. Metre-scale amplitude.
            dome = 0.0095*(1.0-np.clip((X/(hx+1))**2,0,1)) * (1.0-np.clip((Y/(hy+1))**2,0,1))
            lip = .0026*smooth(hy-10, hy-3, Y)*smooth(0,4,depth)
            face_h = (.0020 + .0075*bevel + dome + lip +
                      .00028*np.sin(X*.39+seed)*np.sin(Y*.27-seed*.2))
            tile_h = height[y0:y1, x0:x1]
            tile_h[inside] = face_h[inside]
            # Deep narrow joints, darker tile margins and a softly dusty lip.
            color = colors[(row % COURSES, col % TILE_COLS)]
            edge = 1.0-smooth(0, 8, depth)
            lip_dust = smooth(hy-12,hy-7,Y)*smooth(hy+1,hy-9,Y)*smooth(0,3,depth)
            streak = np.maximum(0, np.sin(X*.31+seed*.31))**14
            streak *= smooth(-hy+3,-hy+13,Y) * .045
            mottled = clouds[y0:y1,x0:x1] + grit[y0:y1,x0:x1]
            weather = mottled + edge*5.0 + lip_dust*4.2 + streak*255
            # Top courses overlay their laps; deterministic material-only color.
            for ch, ratio in enumerate((1.0, .94, .84)):
                channel = base[y0:y1,x0:x1,ch]
                val = np.clip(color[ch]*ratio + weather, 0, 255)
                channel[inside] = val[inside]
            tile_ao = ao[y0:y1,x0:x1]
            av = .91 - edge*.31 - lip_dust*.10
            tile_ao[inside] = av[inside]
            tile_rough = rough[y0:y1,x0:x1]
            rv = .89 + np.clip(mottled/300,-.025,.025) + edge*.015
            tile_rough[inside] = rv[inside]

    # Slight tile-to-tile hairline joint and lower-lip contact darkening is
    # derived from the relief boundary, not a shadow or directional bake.
    # File row 0 is glTF V=0 at the eaves. Flip source construction before
    # computing tangent gradients so +V points uphill in the final image.
    crop = np.s_[PAD_ORIGIN:PAD_ORIGIN+SIZE, PAD_ORIGIN:PAD_ORIGIN+SIZE]
    # Render three complete repeats with overlapping neighbors, then crop the
    # middle repeat, so no texture edge or missing neighboring course is clamped.
    base, height, ao, rough = base[crop], height[crop], ao[crop], rough[crop]
    yy = yy[crop]; xx = xx[crop]
    height_v = np.flipud(height)
    base_v, ao_v, rough_v = np.flipud(base), np.flipud(ao), np.flipud(rough)
    # Periodic central differences avoid a normal-map discontinuity at wraps.
    du = (np.roll(height_v,-1,axis=1)-np.roll(height_v,1,axis=1)) * (.5*SIZE/REPEAT_M)
    dv = (np.roll(height_v,-1,axis=0)-np.roll(height_v,1,axis=0)) * (.5*SIZE/REPEAT_M)
    nz = np.ones_like(du)
    nx = -du*1.8
    ny = -dv*1.8  # OpenGL +Y; increasing glTF V rises uphill.
    norm = np.sqrt(nx*nx+ny*ny+nz*nz)
    normal = np.stack(((nx/norm+1)*127.5, (ny/norm+1)*127.5,
                       (nz/norm+1)*127.5), axis=-1)
    # Subtle fine ceramic grain in normals; avoid noisy/sharp aliasing at mips.
    tau = 2.0*math.pi/SIZE
    micro = (np.sin(xx*tau*137+yy*tau*81)*np.sin(yy*tau*83-xx*tau*43))*.008
    normal[:,:,0] += micro*255
    normal[:,:,1] -= micro*255
    # Height source is linear, 0..20 mm. AO/Roughness/Metalness = R/G/B.
    maps = {
        "base-color.png": np.dstack((np.clip(base_v,0,255), np.full((SIZE,SIZE),255))).astype(np.uint8),
        "height.png": np.dstack((np.clip(height_v/.020*255,0,255),)*3 + (np.full((SIZE,SIZE),255),)).astype(np.uint8),
        "normal.png": np.dstack((np.clip(normal,0,255),np.full((SIZE,SIZE),255))).astype(np.uint8),
        "orm.png": np.dstack((np.clip(ao_v*255,0,255),np.clip(rough_v*255,0,255),np.zeros((SIZE,SIZE)),np.full((SIZE,SIZE),255))).astype(np.uint8),
    }
    records={}
    for name, pixels in maps.items():
        path=OUT/name
        Image.fromarray(pixels,"RGBA").save(path,optimize=True)
        records[name]={"sha256":digest(path),"bytes":path.stat().st_size,"size":[SIZE,SIZE]}
        if name == "base-color.png":
            Image.open(path).convert("RGB").save(REPORT/"roof-albedo-review.png", optimize=True)
    recipe={
      "asset":"Bellwether hand-laid flat clay plain roof tiles v1",
      "authoring":"Original deterministic Python/Numpy/Pillow procedural material; no external source images, assets, providers, or paid generation.",
      "seed":SEED,"resolution":SIZE,"repeatMetres":[2.4,2.4],
      "uv":"+U along ridge; +V uphill from eaves to ridge; lower exposed lip faces downhill. Final stored file row 0 is V=0 at eaves; rows increase toward ridge.",
      "layout":{"tilesAcross":TILE_COLS,"exposedCourses":COURSES,"tilePitchWidthMetres":round(REPEAT_M/TILE_COLS,6),"tileWidthMetres":0.194,"coursePitchMetres":round(REPEAT_M/COURSES,6),"tileLengthMetres":0.244,"overlapMetres":round(.244-REPEAT_M/COURSES,6)},
      "maps":{"base-color":"sRGB albedo, muted iron-rich red/brown clay; no illumination or specular baked.","height":"Linear 0..20 mm source relief, bevel and lower overlapping lip; source-only, converted into normal.","normal":"OpenGL +Y tangent-space normal derived from height and physical repeat.","orm":"Linear RGBA, R=AO, G=roughness, B=metallic=0, A=255; non-metallic rough ceramic."},
      "periodicSeams":"Lattice and deterministic variation wrap at 2.4 m; output edge comparisons are included in validation receipt.",
      "quantitativeSourceChecks": {
        "opaqueAlpha": all(np.asarray(Image.open(OUT/name))[:,:,3].min() == 255 for name in maps),
        "baseColorOppositeEdgeMeanAbsRgb": round(float(np.abs(maps["base-color.png"][:,0,:3].astype(np.float32)-maps["base-color.png"][:,-1,:3]).mean()),3),
        "heightOppositeEdgeMeanAbs": round(float(np.abs(maps["height.png"][:,0,0].astype(np.float32)-maps["height.png"][:,-1,0]).mean()),3),
        "normalVectorLengthMean": round(float(np.linalg.norm(normal/127.5-1.0,axis=-1).mean()),4),
        "ormMetalMaximum": int(maps["orm.png"][:,:,2].max()),
        "roughnessRange": [round(float(rough.min()),4),round(float(rough.max()),4)],
        "ambientOcclusionRange": [round(float(ao.min()),4),round(float(ao.max()),4)]
      },
      "products":records}
    (OUT/"recipe.json").write_bytes((json.dumps(recipe,indent=2)+"\n").encode("utf-8"))
    print(json.dumps(recipe,indent=2))

if __name__ == "__main__": main()
