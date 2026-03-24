#!/usr/bin/env python3
"""Export solar sail Coons patch as OBJ + MTL."""
import numpy as np
from numpy.linalg import norm

N = 16  # grid resolution
W, curv = 5.0, 1.5
ang = np.radians(80)
cx, cy = np.cos(ang), np.sin(ang)
tx, ty = -cy, cx

aft = np.array([cx*W, cy*W, 0])
fwd = np.array([0, 0, 13])
mid = (aft + fwd) / 2
latA = mid + np.array([tx*3.5, ty*3.5, 0])
latB = mid - np.array([tx*3.5, ty*3.5, 0])
bow = np.array([cx, cy, 0]) * curv

def bezier_edge(p0, p3):
    p1 = p0 + (p3-p0)/3 + bow
    p2 = p0 + 2*(p3-p0)/3 + bow
    return p0, p1, p2, p3

edges = [bezier_edge(aft, latB), bezier_edge(latB, fwd),
         bezier_edge(fwd, latA), bezier_edge(latA, aft)]

def cbez(e, t):
    u = 1-t
    return u**3*e[0] + 3*u**2*t*e[1] + 3*u*t**2*e[2] + t**3*e[3]

def coons(s, t):
    p = ((1-t)*cbez(edges[0],s) + t*cbez(edges[2],1-s) +
         (1-s)*cbez(edges[3],1-t) + s*cbez(edges[1],t) -
         ((1-s)*(1-t)*aft + s*(1-t)*latB + (1-s)*t*latA + s*t*fwd))
    p[2] -= 0.3 * np.sin(np.pi*s) * np.sin(np.pi*t)  # billowing
    return p

# Build grid
g = np.linspace(0, 1, N+1)
pts = np.array([[coons(s, t) for s in g] for t in g])

# Finite-difference normals
eps = 1/N
norms = np.zeros_like(pts)
for j, t in enumerate(g):
    for i, s in enumerate(g):
        ds = coons(min(s+eps,1), t) - coons(max(s-eps,0), t)
        dt = coons(s, min(t+eps,1)) - coons(s, max(t-eps,0))
        n = np.cross(ds, dt)
        nl = norm(n)
        norms[j,i] = n/nl if nl > 1e-4 else [0,1,0]

# Write MTL
with open("solar_sail.mtl", "w") as f:
    f.write("newmtl SailMaterial\nNs 50\nKa 0.2 0.2 0.25\n")
    f.write("Kd 0.6 0.6 0.65\nKs 1 1 1\nillum 2\nmap_Kd iridescent.png\n")

# Write OBJ
with open("solar_sail.obj", "w") as f:
    f.write("# Solar sail - Coons patch, 16x16 grid\nmtllib solar_sail.mtl\ng SolarSail\nusemtl SailMaterial\n\n")
    for row in pts.reshape(-1,3):
        f.write(f"v {row[0]:.6f} {row[1]:.6f} {row[2]:.6f}\n")
    f.write("\n")
    for j in range(N+1):
        for i in range(N+1):
            f.write(f"vt {i/N:.6f} {j/N:.6f}\n")
    f.write("\n")
    for row in norms.reshape(-1,3):
        f.write(f"vn {row[0]:.6f} {row[1]:.6f} {row[2]:.6f}\n")
    f.write("\n")
    V = N + 1
    for j in range(N):
        for i in range(N):
            a, b, c, d = [j*V+i+1, (j+1)*V+i+1, j*V+i+2, (j+1)*V+i+2]
            f.write(f"f {a}/{a}/{a} {c}/{c}/{c} {b}/{b}/{b}\n")
            f.write(f"f {c}/{c}/{c} {d}/{d}/{d} {b}/{b}/{b}\n")

print(f"Wrote solar_sail.obj + .mtl ({(N+1)**2} verts, {2*N*N} faces)")
