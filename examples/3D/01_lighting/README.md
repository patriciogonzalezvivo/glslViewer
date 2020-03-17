## 0. Gooch Model (Lamberd)

```bash
glslViewer 00_gooch.frag head.ply 
```

## 1. Shadows

```bash
glslViewer 01_shadows.frag head.ply 
```

## 2. CubeMap

```bash
glslViewer 02_cubemap.frag head.ply -C uffizi_cross.hdr
```

## 3. Spherical Harmonics

```bash
glslViewer 03_spherical_harmonics.frag head.ply -C uffizi_cross.hdr
```

## 4. Fresnel

```bash
glslViewer 04_fresnel.vert 04_fresnel.frag head.ply -C uffizi_cross.hdr
```