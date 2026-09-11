# Asset sources

Collected 2026-09-11 for the opengl2 project. Every asset below is CC0, public domain, CC BY 3.0/4.0 or OFL. CC BY items need the attribution line from the "Credit" field in the readme/credits screen.

Layout: `assets/raw/<category>/<name>/`. Raw downloads (zips, GLB) are not kept; everything is already in the project formats: OBJ + MTL + JPG/PNG, PNG/JPG textures, Radiance .hdr and JPG panoramas, TTF.

## Conventions for the converted models

- All OBJ files were converted from glTF binary (.glb, the Sketchfab / poly.pizza download) with a small script: node transforms baked into vertices, triangles only, `v/vt/vn` on every face, texture V flipped to the OBJ convention (v = 0 at the bottom of the image, so load images with stbi_set_flip_vertically_on_load(1) or flip V yourself).
- Axes after conversion: Y up, front (nose, cab) along +Z. Aircraft, vehicles and buildings stand on Y = 0 with the X/Z centre of the bounding box at the origin; missiles are centred on their bounding box.
- Textures are at most 2048 px. MTL keys used: `map_Kd` diffuse, `map_d` alpha (same PNG as map_Kd), `map_Bump` and `norm` tangent-space normal map (OpenGL convention, +Y up), `map_Pr` glTF metallic/roughness texture (G = roughness, B = metallic, R = occlusion when present), `map_Ke` emissive, `Kd` base colour factor, `d` opacity, `Pm`/`Pr` metallic/roughness factors. Lines starting with `#` inside a material are comments (alpha mode, double sided).
- Materials without `map_Kd` still have UVs; they use the `Kd` colour.
- Numbers were rounded after conversion (positions to 3-6 decimals depending on model size, UV 4, normals 3) to save space.

## Model summary

| Model | Triangles | UV | Diffuse texture | Normal map | Groups | Up / front | Size (units) |
|---|---|---|---|---|---|---|---|
| aircraft/f16_rickslash | 15290 | yes | yes | yes | 76 | +Y / +Z | 1.244 x 0.607 x 2 |
| aircraft/f16_andertan | 35474 | yes | 6 of 30 materials | no | 31 | +Y / +Z | 18.673 x 7.806 x 26.834 |
| aircraft/f16_bohmerang_lowpoly | 30244 | yes | no, colours only | no | 7 | +Y / +Z | 116.51 x 64.255 x 187.188 |
| aircraft/jet_generic_google_poly | 1512 | yes | yes | no | 1 | +Y / +Z | 946.334 x 141.422 x 833.274 |
| weapons/aim9_rickslash | 1700 | yes | yes | yes | 10 | +Y / +Z (nose) | 0.319 x 0.319 x 2 |
| weapons/agm154_jsow_rickslash | 1278 | yes | yes | yes | 9 | +Y / +Z (nose) | 0.276 x 0.211 x 2 |
| weapons/agm65g_maverick_ekmek | 400 | yes | 1 of 2 materials | no | 2 | +Y / +Z (nose) | 1.389 x 1.389 x 4.9 |
| weapons/aim9m_ekmek | 960 | yes | 1 of 2 materials | no | 2 | +Y / +Z (nose) | 0.792 x 0.842 x 4.942 |
| targets/sam_2k12_kub | 9354 | yes | 2 of 10 materials | no | 10 | +Y / +Z (missiles and hull front) | 1495.51 x 2428.46 x 3133.71 |
| targets/truck_ural | 29866 | yes | yes | no | 11 | +Y / +Z (cab) | 289.442 x 265.141 x 733.04 |
| targets/truck_m939 | 9938 | yes | no, colours only | no | 24 | +Y / +Z (cab) | 2.668 x 3.052 x 7.258 |
| targets/fuel_depot_storage_tanks | 9811 | yes | yes | yes | 43 | +Y / n/a (static) | 28.422 x 14.38 x 33.451 |
| targets/fuel_tank_rusty | 2532 | yes | yes | yes | 1 | +Y / n/a (static), tanks lie along Z | 1.724 x 1.09 x 2 |
| targets/radome_geodesic_dome_quaternius | 1432 | yes | yes | no | 1 | +Y / n/a (round) | 8.53 x 5.281 x 8.53 |
| targets/radar_dish_quaternius | 972 | yes | yes | no | 1 | +Y / dish faces +Z | 1.627 x 2.713 x 1.649 |
| airfield/shelter_quonset_hut | 3473 | yes | yes | yes | 3 | +Y / opening at +Z/-Z ends | 1.275 x 0.765 x 2 |
| airfield/missile_cart_r27 | 8250 | yes | yes | no | 2 | +Y / tow bar at -X | 5.852 x 1.146 x 2.017 |
| airfield/maintenance_ladder | 5816 | yes | yes | yes | 1 | +Y / n/a | 0.321 x 2.263 x 0.366 |

Size is X x Y x Z of the bounding box (width x height x length).

## Aircraft

### aircraft/f16_rickslash

- Name: F-16 jet
- Author: rickslash (https://sketchfab.com/rickslash)
- URL: https://sketchfab.com/3d-models/f-16-jet-69c32a15b8e24d6ebf276b76dde3ab95
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "F-16 jet" by rickslash, https://sketchfab.com/3d-models/f-16-jet-69c32a15b8e24d6ebf276b76dde3ab95, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 15290 triangles, 76 groups, 1 material(s); up +Y, front +Z; conversion rotation: none; units: length 2.0 units; scale x7.5 for metres.
- Note: Recommended main jet. Game-ready ("made for small aviasimulator"), 2016.
- Note: Textures 2048: diffuse PNG with alpha (map_Kd + map_d, about 7% of texels are translucent, the canopy), normal map, metallic/roughness (map_Pr).
- Note: glTF material is alphaMode BLEND on the whole mesh: render opaque with alpha test or blend only the cab_* groups.
- Note: 76 named groups: cab_keep / cab_around (canopy), chassis_* (landing gear and doors), wing_back* (stabilators), middle_wing_move* (flaperons), rocket_holder_aim9 / _aim_120 / _jsow (pylons), nozzle, main_body, ...
- Note: Same author and style as weapons/aim9_rickslash and weapons/agm154_jsow_rickslash (the pylons are named for them).
- Files:
  - `aircraft/f16_rickslash/f16_rickslash.mtl` (401 bytes)
  - `aircraft/f16_rickslash/f16_rickslash.obj` (1.49 MB)
  - `aircraft/f16_rickslash/f16_rickslash_diffuse_0.png` (1.69 MB)
  - `aircraft/f16_rickslash/f16_rickslash_metalRough_1.jpg` (0.44 MB)
  - `aircraft/f16_rickslash/f16_rickslash_normal_2.jpg` (0.14 MB)

### aircraft/f16_andertan

- Name: Lockheed Martin F-16E/F "Fighting Falcon"
- Author: andertan (https://sketchfab.com/andertan)
- URL: https://sketchfab.com/3d-models/lockheed-martin-f-16ef-fighting-falcon-d54c72e055a647ad9173e01548e50db3
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "Lockheed Martin F-16E/F "Fighting Falcon"" by andertan, https://sketchfab.com/3d-models/lockheed-martin-f-16ef-fighting-falcon-d54c72e055a647ad9173e01548e50db3, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 35474 triangles, 31 groups, 30 material(s); up +Y, front +Z; conversion rotation: rotated -90 deg about Y (nose was +X); units: length 26.8 units incl. pitot and stores; about x0.56 for metres.
- Note: Block 60 (UAE) variant, 2024. Most detailed of the three, carries external stores (wingtip AAMs, underwing missiles, tanks) that are part of the mesh.
- Note: Mostly material colours: 30 materials, only 6 use a diffuse texture (panel decals and lettering, largest 4096x2048 downscaled to 2048x1024). No normal map.
- Note: Groups are per material (F16_E_2__<material>), not per part; the canopy glass is one of the glass-like materials ("Darker_blue_hue_glass", or the translucent "Not_so_Dark" with d 0.56), not verified which. No separate gear or control surfaces.
- Files:
  - `aircraft/f16_andertan/f16_andertan.mtl` (4 KB)
  - `aircraft/f16_andertan/f16_andertan.obj` (4.29 MB)
  - `aircraft/f16_andertan/f16_andertan_diffuse_0.jpg` (87 KB)
  - `aircraft/f16_andertan/f16_andertan_diffuse_1.jpg` (86 KB)
  - `aircraft/f16_andertan/f16_andertan_diffuse_2.jpg` (0.29 MB)
  - `aircraft/f16_andertan/f16_andertan_diffuse_3.jpg` (79 KB)

### aircraft/f16_bohmerang_lowpoly

- Name: F-16 Fighting Falcon, from "FREE - Fighter Jet Collection - Low Poly"
- Author: bohmerang (https://sketchfab.com/bohmerang)
- URL: https://sketchfab.com/3d-models/free-fighter-jet-collection-low-poly-cb5966c988d9403895be89b364c2252f
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "F-16 Fighting Falcon, from "FREE - Fighter Jet Collection - Low Poly"" by bohmerang, https://sketchfab.com/3d-models/free-fighter-jet-collection-low-poly-cb5966c988d9403895be89b364c2252f, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 30244 triangles, 7 groups, 1 material(s); up +Y, front +Z; conversion rotation: rotated -90 deg about Y (nose was +X); units: length 187 units; about x0.08 for metres.
- Note: Only the F-16 was extracted from the six-jet collection (F-35, F-22, F-18, F-16, F-15, F-14 are all in it, the F-35 conversion is kept aside if needed).
- Note: NO TEXTURE: one grey material, but full UVs, so a paint texture or the roundel decal can be added. Fails the "diffuse texture" requirement, kept for its structure.
- Note: Best part split of all candidates: F-16Airframe, F-16Canopy, F-16Cockpit, F-16InstrGlass, F-16LandingOn (gear down), F-16LandingOff (gear up), F-16WeaponRails. Both gear states overlap: draw one of them.
- Files:
  - `aircraft/f16_bohmerang_lowpoly/f16_bohmerang_lowpoly.mtl` (197 bytes)
  - `aircraft/f16_bohmerang_lowpoly/f16_bohmerang_lowpoly.obj` (7.36 MB)

### aircraft/jet_generic_google_poly

- Name: Jet
- Author: Poly by Google (via poly.pizza)
- URL: https://poly.pizza/m/dukcCKsLDrS
- License: CC BY 3.0 (https://creativecommons.org/licenses/by/3.0/)
- Credit: "Jet" by Poly by Google, https://poly.pizza/m/dukcCKsLDrS, licensed under CC BY 3.0. Converted to OBJ and resized textures.
- Model: 1512 triangles, 1 groups, 1 material(s); up +Y, front +Z; conversion rotation: none; units: length 833, span 946 (cm-like units).
- Note: Fallback only: 1512 triangles (below the 5k target), generic delta wing, one 2048 texture. No normals in the file (compute them).
- Files:
  - `aircraft/jet_generic_google_poly/jet_generic_google_poly.mtl` (220 bytes)
  - `aircraft/jet_generic_google_poly/jet_generic_google_poly.obj` (0.17 MB)
  - `aircraft/jet_generic_google_poly/jet_generic_google_poly_diffuse_0.jpg` (0.31 MB)

## Weapons

### weapons/aim9_rickslash

- Name: AIM-9 missile
- Author: rickslash (https://sketchfab.com/rickslash)
- URL: https://sketchfab.com/3d-models/aim-9-missile-caaf15b49bac4144b6c6be577c2b872a
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "AIM-9 missile" by rickslash, https://sketchfab.com/3d-models/aim-9-missile-caaf15b49bac4144b6c6be577c2b872a, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 1700 triangles, 10 groups, 1 material(s); up +Y, front +Z (nose); conversion rotation: none; units: length 2.0; real AIM-9 is 2.85 m.
- Note: AIM-9B, for the wingtip rails. Diffuse, normal and metallic/roughness 2048 (made for 1024 detail).
- Files:
  - `weapons/aim9_rickslash/aim9_rickslash.mtl` (368 bytes)
  - `weapons/aim9_rickslash/aim9_rickslash.obj` (0.17 MB)
  - `weapons/aim9_rickslash/aim9_rickslash_diffuse_0.jpg` (0.14 MB)
  - `weapons/aim9_rickslash/aim9_rickslash_metalRough_1.jpg` (0.11 MB)
  - `weapons/aim9_rickslash/aim9_rickslash_normal_2.jpg` (84 KB)

### weapons/agm154_jsow_rickslash

- Name: AGM-154 JSOW missile
- Author: rickslash (https://sketchfab.com/rickslash)
- URL: https://sketchfab.com/3d-models/agm-154-jsow-missile-03148c5054e54a1c99980aff68952c0f
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "AGM-154 JSOW missile" by rickslash, https://sketchfab.com/3d-models/agm-154-jsow-missile-03148c5054e54a1c99980aff68952c0f, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 1278 triangles, 9 groups, 1 material(s); up +Y, front +Z (nose); conversion rotation: none; units: length 2.0; real JSOW is 4.1 m.
- Note: Air-to-ground glide weapon (not a Maverick, but the same author as the F-16 and a matching pylon). Diffuse, normal, metallic/roughness 2048.
- Files:
  - `weapons/agm154_jsow_rickslash/agm154_jsow_rickslash.mtl` (398 bytes)
  - `weapons/agm154_jsow_rickslash/agm154_jsow_rickslash.obj` (0.11 MB)
  - `weapons/agm154_jsow_rickslash/agm154_jsow_rickslash_diffuse_0.jpg` (0.16 MB)
  - `weapons/agm154_jsow_rickslash/agm154_jsow_rickslash_metalRough_1.jpg` (0.22 MB)
  - `weapons/agm154_jsow_rickslash/agm154_jsow_rickslash_normal_2.jpg` (0.11 MB)

### weapons/agm65g_maverick_ekmek

- Name: AGM-65G Maverick, from "Simple GameReady Weapons for Fighter Jet Games"
- Author: ekmekarasikodlamaytb (https://sketchfab.com/ekmekarasikodlamaytb)
- URL: https://sketchfab.com/3d-models/simple-gameready-weapons-for-fighter-jet-games-4c231dda5e8a4db39c2f53c3e16f57e9
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "AGM-65G Maverick, from "Simple GameReady Weapons for Fighter Jet Games"" by ekmekarasikodlamaytb, https://sketchfab.com/3d-models/simple-gameready-weapons-for-fighter-jet-games-4c231dda5e8a4db39c2f53c3e16f57e9, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 400 triangles, 2 groups, 2 material(s); up +Y, front +Z (nose); conversion rotation: none; units: length 4.9; real AGM-65 is 2.5 m.
- Note: Extracted from the weapons pack and centred. 400 triangles, one diffuse texture plus a plain black material. Author notes the pack is "not very accurate".
- Files:
  - `weapons/agm65g_maverick_ekmek/agm65g_maverick_ekmek.mtl` (364 bytes)
  - `weapons/agm65g_maverick_ekmek/agm65g_maverick_ekmek.obj` (58 KB)
  - `weapons/agm65g_maverick_ekmek/agm65g_maverick_ekmek_diffuse_1.jpg` (0.16 MB)

### weapons/aim9m_ekmek

- Name: AIM-9M Sidewinder, from the same weapons pack
- Author: ekmekarasikodlamaytb (https://sketchfab.com/ekmekarasikodlamaytb)
- URL: https://sketchfab.com/3d-models/simple-gameready-weapons-for-fighter-jet-games-4c231dda5e8a4db39c2f53c3e16f57e9
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "AIM-9M Sidewinder, from the same weapons pack" by ekmekarasikodlamaytb, https://sketchfab.com/3d-models/simple-gameready-weapons-for-fighter-jet-games-4c231dda5e8a4db39c2f53c3e16f57e9, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 960 triangles, 2 groups, 2 material(s); up +Y, front +Z (nose); conversion rotation: none; units: length 4.94; real AIM-9 is 2.85 m.
- Note: Alternative AIM-9 that matches the Maverick above in style. 960 triangles, one diffuse texture.
- Files:
  - `weapons/aim9m_ekmek/aim9m_ekmek.mtl` (354 bytes)
  - `weapons/aim9m_ekmek/aim9m_ekmek.obj` (0.12 MB)
  - `weapons/aim9m_ekmek/aim9m_ekmek_diffuse_0.jpg` (0.19 MB)

## Ground targets

### targets/sam_2k12_kub

- Name: 2K12 Kub (SA-6 launcher)
- Author: UltraKill (https://sketchfab.com/ultrakill)
- URL: https://sketchfab.com/3d-models/2k12-kub-4e2e8506c9e546b3b7da1554db8185bc
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "2K12 Kub (SA-6 launcher)" by UltraKill, https://sketchfab.com/3d-models/2k12-kub-4e2e8506c9e546b3b7da1554db8185bc, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 9354 triangles, 10 groups, 10 material(s); up +Y, front +Z (missiles and hull front); conversion rotation: rotated -90 deg about X (was Z-up); units: length 3134 units.
- Note: SAM launcher with three missiles raised. Staff pick from 2013, no description on the page.
- Note: Original file was Z-up; rotated to Y-up here. Unusual units (about 3134 long, real vehicle is about 7.4 m, so scale by about 0.0024).
- Note: 10 groups (hull, turret parts, missiles by material), one diffuse texture shared by 2 materials, the rest are plain colours.
- Files:
  - `targets/sam_2k12_kub/sam_2k12_kub.mtl` (1 KB)
  - `targets/sam_2k12_kub/sam_2k12_kub.obj` (1.24 MB)
  - `targets/sam_2k12_kub/sam_2k12_kub_diffuse_0.jpg` (0.43 MB)

### targets/truck_ural

- Name: Vehicle - Ural Truck 44202
- Author: Thcyrax (https://sketchfab.com/thcyrax)
- URL: https://sketchfab.com/3d-models/vehicle-ural-truck-44202-dad03c13ecc045c7b21657d6d5bc5fe8
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "Vehicle - Ural Truck 44202" by Thcyrax, https://sketchfab.com/3d-models/vehicle-ural-truck-44202-dad03c13ecc045c7b21657d6d5bc5fe8, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 29866 triangles, 11 groups, 3 material(s); up +Y, front +Z (cab); conversion rotation: none; units: centimetres.
- Note: Military truck, units are centimetres (7.33 m long). Wheels are separate groups (FLW, FRW, RLW1/2, RRW1/2), plus Body, Body_Glass, Body_Emission (lights), Driver, Suspension_Engine.
- Note: Texture is a tiny colour atlas (256 px), so it looks flat-shaded; map_Ke is the lights.
- Files:
  - `targets/truck_ural/truck_ural.mtl` (627 bytes)
  - `targets/truck_ural/truck_ural.obj` (4.51 MB)
  - `targets/truck_ural/truck_ural_diffuse_0.jpg` (2 KB)
  - `targets/truck_ural/truck_ural_diffuse_1.png` (980 bytes)
  - `targets/truck_ural/truck_ural_emissive_0.jpg` (2 KB)

### targets/truck_m939

- Name: M939 Truck
- Author: J-Toastie (via poly.pizza)
- URL: https://poly.pizza/m/y8lBpvMlim
- License: CC BY 3.0 (https://creativecommons.org/licenses/by/3.0/)
- Credit: "M939 Truck" by J-Toastie, https://poly.pizza/m/y8lBpvMlim, licensed under CC BY 3.0. Converted to OBJ and resized textures.
- Model: 9938 triangles, 24 groups, 11 material(s); up +Y, front +Z (cab); conversion rotation: rotated +90 deg about Y (cab was -X); units: metres.
- Note: Alternative truck (US 6x6 with canvas top). NO TEXTURE: 11 colour materials, has UVs. Rotated so the cab faces +Z. Units are metres.
- Files:
  - `targets/truck_m939/truck_m939.mtl` (1 KB)
  - `targets/truck_m939/truck_m939.obj` (1.60 MB)

### targets/fuel_depot_storage_tanks

- Name: Large Industrial Storage Tanks
- Author: Duane's Mind (https://sketchfab.com/duanesmind)
- URL: https://sketchfab.com/3d-models/large-industrial-storage-tanks-bd3ac97071104b64bc388a155b64f79e
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "Large Industrial Storage Tanks" by Duane's Mind, https://sketchfab.com/3d-models/large-industrial-storage-tanks-bd3ac97071104b64bc388a155b64f79e, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 9811 triangles, 43 groups, 5 material(s); up +Y, front n/a (static); conversion rotation: none; units: metres (approx.).
- Note: Fuel depot: one big vertical tank, two spherical tanks, a long tank, pipes, stairs, railings (43 groups). Diffuse + normal (1024). Units look like metres (33 x 28 m).
- Files:
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks.mtl` (1 KB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks.obj` (1.07 MB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_diffuse_0.jpg` (0.34 MB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_diffuse_2.jpg` (0.41 MB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_diffuse_4.jpg` (0.34 MB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_diffuse_6.jpg` (0.34 MB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_normal_1.jpg` (47 KB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_normal_3.jpg` (0.23 MB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_normal_5.jpg` (87 KB)
  - `targets/fuel_depot_storage_tanks/fuel_depot_storage_tanks_normal_7.jpg` (0.13 MB)

### targets/fuel_tank_rusty

- Name: Rusty airbase fuel tank
- Author: LuddePudde (https://sketchfab.com/luddepubde)
- URL: https://sketchfab.com/3d-models/rusty-airbase-fuel-tank-5ac257aee23e47e0a0f91a4e07a40692
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "Rusty airbase fuel tank" by LuddePudde, https://sketchfab.com/3d-models/rusty-airbase-fuel-tank-5ac257aee23e47e0a0f91a4e07a40692, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 2532 triangles, 1 groups, 1 material(s); up +Y, front n/a (static), tanks lie along Z; conversion rotation: none; units: normalised, 2 units long.
- Note: Two horizontal tanks on a frame (Swedish airfield reference). 2532 triangles, diffuse + normal + metallic/roughness (1024). Normalised to 2 units long.
- Files:
  - `targets/fuel_tank_rusty/fuel_tank_rusty.mtl` (473 bytes)
  - `targets/fuel_tank_rusty/fuel_tank_rusty.obj` (0.25 MB)
  - `targets/fuel_tank_rusty/fuel_tank_rusty_diffuse_0.jpg` (0.13 MB)
  - `targets/fuel_tank_rusty/fuel_tank_rusty_metalRough_1.jpg` (0.20 MB)
  - `targets/fuel_tank_rusty/fuel_tank_rusty_normal_2.jpg` (0.15 MB)

### targets/radome_geodesic_dome_quaternius

- Name: Geodesic Dome (from Ultimate Space Kit)
- Author: Quaternius (https://quaternius.com, via poly.pizza)
- URL: https://poly.pizza/m/T7Ge6maWq4
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Model: 1432 triangles, 1 groups, 1 material(s); up +Y, front n/a (round); conversion rotation: none; units: metres (approx.).
- Note: Stand-in for the radar dome: geodesic dome on a platform, 8.5 m wide. Low-poly palette texture. The panels are dark glass in the original colours; tint them white for a radome.
- Files:
  - `targets/radome_geodesic_dome_quaternius/radome_geodesic_dome.mtl` (231 bytes)
  - `targets/radome_geodesic_dome_quaternius/radome_geodesic_dome.obj` (0.18 MB)
  - `targets/radome_geodesic_dome_quaternius/radome_geodesic_dome_diffuse_0.jpg` (5 KB)

### targets/radar_dish_quaternius

- Name: Roof Radar (from Ultimate Space Kit)
- Author: Quaternius (https://quaternius.com, via poly.pizza)
- URL: https://poly.pizza/m/V7XQDxF8JC
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Model: 972 triangles, 1 groups, 1 material(s); up +Y, front dish faces +Z; conversion rotation: none; units: metres (approx.).
- Note: Small dish radar on a pedestal (2.7 m), can sit next to the dome. Low-poly palette texture.
- Files:
  - `targets/radar_dish_quaternius/radar_dish.mtl` (218 bytes)
  - `targets/radar_dish_quaternius/radar_dish.obj` (96 KB)
  - `targets/radar_dish_quaternius/radar_dish_diffuse_0.jpg` (5 KB)

## Airfield props

### airfield/shelter_quonset_hut

- Name: Quonset Hut
- Author: John (gold-experience) (https://sketchfab.com/gold-experience)
- URL: https://sketchfab.com/3d-models/quonset-hut-5213606fbe334f64b929dfedffcf71af
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "Quonset Hut" by John, https://sketchfab.com/3d-models/quonset-hut-5213606fbe334f64b929dfedffcf71af, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 3473 triangles, 3 groups, 3 material(s); up +Y, front opening at +Z/-Z ends; conversion rotation: none; units: normalised, 2 units long.
- Note: Arched hut, used as a stand-in for the hardened aircraft shelter: scale it up and tint it concrete. 3 materials, diffuse + normal + metallic/roughness (1024, occlusion packed in the R channel of map_Pr). Normalised to 2 units long.
- Files:
  - `airfield/shelter_quonset_hut/shelter_quonset_hut.mtl` (1 KB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut.obj` (0.57 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_diffuse_0.jpg` (0.40 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_diffuse_3.jpg` (0.37 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_diffuse_6.jpg` (0.34 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_metalRough_1.jpg` (0.43 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_metalRough_4.jpg` (0.40 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_metalRough_7.jpg` (0.36 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_normal_2.jpg` (0.14 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_normal_5.jpg` (0.18 MB)
  - `airfield/shelter_quonset_hut/shelter_quonset_hut_normal_8.jpg` (0.14 MB)

### airfield/missile_cart_r27

- Name: cart with "R-27ET" A-A soviet missiles
- Author: Tiltow (https://sketchfab.com/Tiltow)
- URL: https://sketchfab.com/3d-models/cart-with-r-27et-a-a-soviet-missiles-fadb1f9ef8cd4425aeada7d918255200
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "cart with "R-27ET" A-A soviet missiles" by Tiltow, https://sketchfab.com/3d-models/cart-with-r-27et-a-a-soviet-missiles-fadb1f9ef8cd4425aeada7d918255200, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 8250 triangles, 2 groups, 2 material(s); up +Y, front tow bar at -X; conversion rotation: none; units: metres.
- Note: Weapons trolley with two R-27 missiles and a tow bar (along -X), metres. Group 1 is the cart, group 2 the missiles, so the missiles can be hidden and ours placed instead.
- Files:
  - `airfield/missile_cart_r27/missile_cart_r27.mtl` (390 bytes)
  - `airfield/missile_cart_r27/missile_cart_r27.obj` (1.60 MB)
  - `airfield/missile_cart_r27/missile_cart_r27_diffuse_0.jpg` (0.16 MB)
  - `airfield/missile_cart_r27/missile_cart_r27_diffuse_1.jpg` (0.17 MB)

### airfield/maintenance_ladder

- Name: Aircraft Maintenance Ladder
- Author: himanshurawathr0987 (https://sketchfab.com/himanshurawathr0987)
- URL: https://sketchfab.com/3d-models/aircraft-maintenance-ladder-4b943d26fa1e4c4ab0f0a99931ab8371
- License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)
- Credit: "Aircraft Maintenance Ladder" by himanshurawathr0987, https://sketchfab.com/3d-models/aircraft-maintenance-ladder-4b943d26fa1e4c4ab0f0a99931ab8371, licensed under CC BY 4.0. Converted to OBJ and resized textures.
- Model: 5816 triangles, 1 groups, 1 material(s); up +Y, front n/a; conversion rotation: none; units: metres.
- Note: Boarding ladder, 2.26 m tall. Diffuse with alpha (map_d) + normal (1024). Originally spec/gloss material, converted to plain diffuse.
- Files:
  - `airfield/maintenance_ladder/maintenance_ladder.mtl` (334 bytes)
  - `airfield/maintenance_ladder/maintenance_ladder.obj` (0.62 MB)
  - `airfield/maintenance_ladder/maintenance_ladder_diffuse_0.png` (0.80 MB)
  - `airfield/maintenance_ladder/maintenance_ladder_normal_2.jpg` (68 KB)

## Ground textures

All tileable PBR sets. Diffuse is 2048 px, normal (OpenGL / +Y convention, "GL") and roughness (grey) are 1024 px to stay inside the size budget; they tile the same way, so sample them with the same UV.

### textures/sand_dunes_Ground097

- Name: Ground 097 (desert sand with wind ripples)
- Author: ambientCG (Lennart Demes)
- URL: https://ambientcg.com/view?id=Ground097
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Primary desert sand.
- Files:
  - `textures/sand_dunes_Ground097/Ground097_diffuse_2k.jpg` (0.32 MB)
  - `textures/sand_dunes_Ground097/Ground097_normal_gl_1k.jpg` (0.18 MB)
  - `textures/sand_dunes_Ground097/Ground097_roughness_1k.jpg` (0.20 MB)

### textures/sand_flat_Ground092A

- Name: Ground 092 A (pale fine desert sand)
- Author: ambientCG (Lennart Demes)
- URL: https://ambientcg.com/view?id=Ground092A
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Second, flatter and paler sand.
- Files:
  - `textures/sand_flat_Ground092A/Ground092A_diffuse_2k.jpg` (0.47 MB)
  - `textures/sand_flat_Ground092A/Ground092A_normal_gl_1k.jpg` (0.23 MB)
  - `textures/sand_flat_Ground092A/Ground092A_roughness_1k.jpg` (0.23 MB)

### textures/sand_windblown_alt_Ground093C

- Name: Ground 093 C (windblown dune sand)
- Author: ambientCG (Lennart Demes)
- URL: https://ambientcg.com/view?id=Ground093C
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Alternate sand, 1K only.
- Files:
  - `textures/sand_windblown_alt_Ground093C/Ground093C_diffuse_1k.jpg` (92 KB)
  - `textures/sand_windblown_alt_Ground093C/Ground093C_normal_gl_1k.jpg` (0.15 MB)
  - `textures/sand_windblown_alt_Ground093C/Ground093C_roughness_1k.jpg` (0.22 MB)

### textures/rocky_ground_rocky_trail_02

- Name: Rocky Trail 02
- Author: Amal Kumar (Poly Haven)
- URL: https://polyhaven.com/a/rocky_trail_02
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Dusty stony ground.
- Files:
  - `textures/rocky_ground_rocky_trail_02/rocky_trail_02_diffuse_2k.jpg` (1.31 MB)
  - `textures/rocky_ground_rocky_trail_02/rocky_trail_02_normal_gl_1k.jpg` (0.69 MB)
  - `textures/rocky_ground_rocky_trail_02/rocky_trail_02_roughness_1k.jpg` (0.21 MB)

### textures/rocky_ground_alt_Ground110

- Name: Ground 110 (gravel, dry dirt)
- Author: ambientCG (Lennart Demes)
- URL: https://ambientcg.com/view?id=Ground110
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Alternate rocky ground, 1K only, greyer.
- Files:
  - `textures/rocky_ground_alt_Ground110/Ground110_diffuse_1k.jpg` (0.47 MB)
  - `textures/rocky_ground_alt_Ground110/Ground110_normal_gl_1k.jpg` (0.60 MB)
  - `textures/rocky_ground_alt_Ground110/Ground110_roughness_1k.jpg` (0.43 MB)

### textures/dry_grass_withered_grass

- Name: Withered Grass
- Author: Charlotte Baglioni (Poly Haven)
- URL: https://polyhaven.com/a/withered_grass
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Files:
  - `textures/dry_grass_withered_grass/withered_grass_diffuse_2k.jpg` (2.10 MB)
  - `textures/dry_grass_withered_grass/withered_grass_normal_gl_1k.jpg` (0.73 MB)
  - `textures/dry_grass_withered_grass/withered_grass_roughness_1k.jpg` (0.38 MB)

### textures/green_grass_Grass004

- Name: Grass 004
- Author: ambientCG (Lennart Demes)
- URL: https://ambientcg.com/view?id=Grass004
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: For the greener north.
- Files:
  - `textures/green_grass_Grass004/Grass004_diffuse_2k.jpg` (1.77 MB)
  - `textures/green_grass_Grass004/Grass004_normal_gl_1k.jpg` (0.69 MB)
  - `textures/green_grass_Grass004/Grass004_roughness_1k.jpg` (0.50 MB)

### textures/rock_cliff_Rock029

- Name: Rock 029 (orange-red desert cliff)
- Author: ambientCG (Lennart Demes)
- URL: https://ambientcg.com/view?id=Rock029
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: For slopes and hills.
- Files:
  - `textures/rock_cliff_Rock029/Rock029_diffuse_2k.jpg` (1.33 MB)
  - `textures/rock_cliff_Rock029/Rock029_normal_gl_1k.jpg` (0.70 MB)
  - `textures/rock_cliff_Rock029/Rock029_roughness_1k.jpg` (0.27 MB)

### textures/apron_concrete_concrete_pavement

- Name: Concrete Pavement
- Author: Charlotte Baglioni (Poly Haven)
- URL: https://polyhaven.com/a/concrete_pavement
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Large light concrete slabs with joints, the closest free match to an airfield apron. Real-world size of one tile: 1.8 x 1.8 m.
- Files:
  - `textures/apron_concrete_concrete_pavement/concrete_pavement_diffuse_2k.jpg` (1.53 MB)
  - `textures/apron_concrete_concrete_pavement/concrete_pavement_normal_gl_1k.jpg` (0.53 MB)
  - `textures/apron_concrete_concrete_pavement/concrete_pavement_roughness_1k.jpg` (0.14 MB)

### textures/runway_asphalt_clean_asphalt

- Name: Clean Asphalt
- Author: Dimitrios Savva (Poly Haven)
- URL: https://polyhaven.com/a/clean_asphalt
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Runway surface without markings; draw the markings in the shader or as decals.
- Files:
  - `textures/runway_asphalt_clean_asphalt/clean_asphalt_diffuse_2k.jpg` (1.40 MB)
  - `textures/runway_asphalt_clean_asphalt/clean_asphalt_normal_gl_1k.jpg` (0.25 MB)
  - `textures/runway_asphalt_clean_asphalt/clean_asphalt_roughness_1k.jpg` (0.25 MB)

## Sky (equirectangular panoramas)

`*_2k.hdr` is Radiance RGBE 2048x1024 (stb_image: stbi_loadf). `*_4k_tonemapped.jpg` is the Poly Haven tonemapped LDR version downscaled to 4096x2048 for direct use as a sky texture.

### sky/belfast_sunset_puresky

- Name: Belfast Sunset (Pure Sky)
- Author: Dimitrios Savva (photography), Greg Zaal (processing), Jarod Guest (sky edits) (Poly Haven)
- URL: https://polyhaven.com/a/belfast_sunset_puresky
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Recommended skybox: ground removed, open horizon, low golden sun, soft clouds.
- Files:
  - `sky/belfast_sunset_puresky/belfast_sunset_puresky_2k.hdr` (4.56 MB)
  - `sky/belfast_sunset_puresky/belfast_sunset_puresky_4k_tonemapped.jpg` (0.38 MB)

### sky/goegap_road

- Name: Goegap Road
- Author: Greg Zaal, James Ray Cock (Poly Haven)
- URL: https://polyhaven.com/a/goegap_road
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Arid Namaqualand sunrise, high contrast; lower half shows a gravel road and scrub (hidden by terrain in flight).
- Files:
  - `sky/goegap_road/goegap_road_2k.hdr` (6.55 MB)
  - `sky/goegap_road/goegap_road_4k_tonemapped.jpg` (2.70 MB)

### sky/klippad_sunrise_2

- Name: Klippad Sunrise 2
- Author: Greg Zaal (Poly Haven)
- URL: https://polyhaven.com/a/klippad_sunrise_2
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Rocky desert sunrise. JPG only (the 2K .hdr was dropped to fit 100 MB; it is at https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/2k/klippad_sunrise_2_2k.hdr).
- Files:
  - `sky/klippad_sunrise_2/klippad_sunrise_2_4k_tonemapped.jpg` (2.06 MB)

## Vegetation cutouts (PNG with alpha)

### foliage/desert_shrubs_trigger_rally_onsemeliot

- Name: Vaious vegetation sprites (Trigger Rally)
- Author: Onsemeliot (https://opengameart.org/users/onsemeliot)
- URL: https://opengameart.org/content/vaious-vegetation-sprites
- License: CC0 1.0, chosen from the multi-license list on the page (CC-BY 4.0/3.0, CC-BY-SA, GPL, OGA-BY, CC0)
- Note: Painted, semi-realistic billboards 1024x1024. Desert-looking ones: dusty-bush, dry-slim-bush1/2, dry-grass, thorn-bush; bush-low and olive-bush for the greener zone. The author asks for credit "Onsemeliot from Trigger Rally" (not required under CC0, but nice).
- Files:
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/bush-low.png` (0.30 MB)
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/dry-grass.png` (0.62 MB)
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/dry-slim-bush1.png` (0.28 MB)
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/dry-slim-bush2.png` (0.43 MB)
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/dusty-bush.png` (0.28 MB)
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/olive-bush.png` (0.59 MB)
  - `foliage/desert_shrubs_trigger_rally_onsemeliot/thorn-bush.png` (0.46 MB)

### foliage/acacia_standin_trigger_rally_onsemeliot

- Name: wide-tree from Vaious vegetation sprites (Trigger Rally)
- Author: Onsemeliot
- URL: https://opengameart.org/content/vaious-vegetation-sprites
- License: CC0 1.0 (multi-license, see above)
- Note: NOT an acacia: a wide, flat-topped tree used as a stand-in. No free CC0/CC-BY realistic acacia cutout was found.
- Files:
  - `foliage/acacia_standin_trigger_rally_onsemeliot/wide-tree.png` (0.74 MB)

### foliage/palm_trigger_rally_onsemeliot

- Name: palm-tree and palm-tree2 from Vaious vegetation sprites (Trigger Rally)
- Author: Onsemeliot
- URL: https://opengameart.org/content/vaious-vegetation-sprites
- License: CC0 1.0 (multi-license, see above)
- Note: Two palm billboards (1024 and 1000 px).
- Files:
  - `foliage/palm_trigger_rally_onsemeliot/palm-tree.png` (0.79 MB)
  - `foliage/palm_trigger_rally_onsemeliot/palm-tree2.png` (0.31 MB)

### foliage/conifer_trigger_rally_onsemeliot

- Name: fir from Vaious vegetation sprites (Trigger Rally)
- Author: Onsemeliot
- URL: https://opengameart.org/content/vaious-vegetation-sprites
- License: CC0 1.0 (multi-license, see above)
- Note: Painted fir, matches the other Trigger Rally sprites.
- Files:
  - `foliage/conifer_trigger_rally_onsemeliot/fir.png` (1.11 MB)

### foliage/conifers_photo_rubberduck

- Name: high-res tree textures
- Author: rubberduck (https://opengameart.org/users/rubberduck)
- URL: https://opengameart.org/content/high-res-tree-textures
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Photo-based spruces with matching billboard normal maps, cut out of trees_1K.zip (tree_collection_1K.png); 4 of the 7 trees kept to save space.
- Files:
  - `foliage/conifers_photo_rubberduck/spruce_1.png` (0.74 MB)
  - `foliage/conifers_photo_rubberduck/spruce_1_normal.png` (0.52 MB)
  - `foliage/conifers_photo_rubberduck/spruce_2.png` (0.46 MB)
  - `foliage/conifers_photo_rubberduck/spruce_2_normal.png` (0.33 MB)
  - `foliage/conifers_photo_rubberduck/spruce_3.png` (0.45 MB)
  - `foliage/conifers_photo_rubberduck/spruce_3_normal.png` (0.33 MB)
  - `foliage/conifers_photo_rubberduck/spruce_4.png` (0.33 MB)
  - `foliage/conifers_photo_rubberduck/spruce_4_normal.png` (0.22 MB)

### foliage/dry_grass_tufts_photo_rubberduck

- Name: 60 CC0 Vegetation textures (plant_36, plant_37, plant_38)
- Author: rubberduck (https://opengameart.org/users/rubberduck)
- URL: https://opengameart.org/content/60-cc0-vegetation-textures
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Photo cutouts: two dry grass tufts and a leafless shrub, good for the desert floor.
- Files:
  - `foliage/dry_grass_tufts_photo_rubberduck/plant_36.png` (0.78 MB)
  - `foliage/dry_grass_tufts_photo_rubberduck/plant_37.png` (0.60 MB)
  - `foliage/dry_grass_tufts_photo_rubberduck/plant_38.png` (0.28 MB)

## Water

### water/sea_waves_keith333

- Name: Water - Batch of 15 Seamless Textures with normalmaps (SeaWaves, SeaWavesB)
- Author: Keith333 (https://opengameart.org/users/keith333)
- URL: https://opengameart.org/content/water-batch-of-15-seamless-textures-with-normalmaps
- License: CC BY 3.0 (https://creativecommons.org/licenses/by/3.0/)
- Credit: "Water - Batch of 15 Seamless Textures with normalmaps (SeaWaves, SeaWavesB)" by Keith333, https://opengameart.org/content/water-batch-of-15-seamless-textures-with-normalmaps, licensed under CC BY 3.0.
- Note: `*_N.jpg` is the tileable normal map, `*_S.jpg` the matching surface/colour image. 2048x1365 (not square, NPOT; fine with ARB_texture_non_power_of_two).
- Files:
  - `water/sea_waves_keith333/SeaWavesB_N.jpg` (0.66 MB)
  - `water/sea_waves_keith333/SeaWavesB_S.jpg` (0.31 MB)
  - `water/sea_waves_keith333/SeaWaves_N.jpg` (0.92 MB)
  - `water/sea_waves_keith333/SeaWaves_S.jpg` (0.47 MB)

## Effects

### effects/kenney_smoke_particles

- Name: Smoke Particles
- Author: Kenney (https://kenney.nl)
- URL: https://kenney.nl/assets/smoke-particles
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: Frame sequences: explosion00-08, flash00-08, blackSmoke00-24, whitePuff00-24. Palettized PNG with transparency (stb_image returns RGBA with req_comp = 4).
- Files:
  - `effects/kenney_smoke_particles/License.txt` (486 bytes)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke00.png` (52 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke01.png` (60 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke02.png` (55 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke03.png` (57 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke04.png` (59 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke05.png` (59 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke06.png` (60 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke07.png` (62 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke08.png` (60 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke09.png` (55 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke10.png` (58 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke11.png` (54 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke12.png` (62 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke13.png` (58 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke14.png` (62 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke15.png` (62 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke16.png` (61 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke17.png` (55 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke18.png` (57 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke19.png` (60 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke20.png` (59 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke21.png` (62 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke22.png` (62 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke23.png` (60 KB)
  - `effects/kenney_smoke_particles/black_smoke/blackSmoke24.png` (57 KB)
  - `effects/kenney_smoke_particles/explosion/explosion00.png` (0.11 MB)
  - `effects/kenney_smoke_particles/explosion/explosion01.png` (0.12 MB)
  - `effects/kenney_smoke_particles/explosion/explosion02.png` (0.11 MB)
  - `effects/kenney_smoke_particles/explosion/explosion03.png` (0.11 MB)
  - `effects/kenney_smoke_particles/explosion/explosion04.png` (0.12 MB)
  - `effects/kenney_smoke_particles/explosion/explosion05.png` (0.12 MB)
  - `effects/kenney_smoke_particles/explosion/explosion06.png` (0.11 MB)
  - `effects/kenney_smoke_particles/explosion/explosion07.png` (0.11 MB)
  - `effects/kenney_smoke_particles/explosion/explosion08.png` (0.12 MB)
  - `effects/kenney_smoke_particles/flash/flash00.png` (92 KB)
  - `effects/kenney_smoke_particles/flash/flash01.png` (87 KB)
  - `effects/kenney_smoke_particles/flash/flash02.png` (0.10 MB)
  - `effects/kenney_smoke_particles/flash/flash03.png` (92 KB)
  - `effects/kenney_smoke_particles/flash/flash04.png` (88 KB)
  - `effects/kenney_smoke_particles/flash/flash05.png` (0.10 MB)
  - `effects/kenney_smoke_particles/flash/flash06.png` (94 KB)
  - `effects/kenney_smoke_particles/flash/flash07.png` (0.10 MB)
  - `effects/kenney_smoke_particles/flash/flash08.png` (99 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff00.png` (58 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff01.png` (55 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff02.png` (59 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff03.png` (58 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff04.png` (60 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff05.png` (56 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff06.png` (57 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff07.png` (59 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff08.png` (62 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff09.png` (56 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff10.png` (60 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff11.png` (62 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff12.png` (56 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff13.png` (62 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff14.png` (60 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff15.png` (59 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff16.png` (62 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff17.png` (58 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff18.png` (58 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff19.png` (62 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff20.png` (59 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff21.png` (56 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff22.png` (63 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff23.png` (60 KB)
  - `effects/kenney_smoke_particles/white_puff/whitePuff24.png` (54 KB)

### effects/kenney_particle_pack

- Name: Particle Pack
- Author: Kenney (https://kenney.nl)
- URL: https://kenney.nl/assets/particle-pack
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: 45 of 80 single 512x512 sprites: fire, flame, flare, muzzle (missile exhaust), smoke, spark, trace (smoke trail), scorch (ground marks), light. White/grey on transparent, tint in the shader, additive blending works well.
- Files:
  - `effects/kenney_particle_pack/License.txt` (651 bytes)
  - `effects/kenney_particle_pack/circle_05.png` (65 KB)
  - `effects/kenney_particle_pack/fire_01.png` (100 KB)
  - `effects/kenney_particle_pack/fire_02.png` (88 KB)
  - `effects/kenney_particle_pack/flame_01.png` (54 KB)
  - `effects/kenney_particle_pack/flame_02.png` (68 KB)
  - `effects/kenney_particle_pack/flame_03.png` (50 KB)
  - `effects/kenney_particle_pack/flame_04.png` (71 KB)
  - `effects/kenney_particle_pack/flame_05.png` (13 KB)
  - `effects/kenney_particle_pack/flame_06.png` (16 KB)
  - `effects/kenney_particle_pack/flare_01.png` (43 KB)
  - `effects/kenney_particle_pack/light_01.png` (93 KB)
  - `effects/kenney_particle_pack/light_02.png` (93 KB)
  - `effects/kenney_particle_pack/light_03.png` (0.10 MB)
  - `effects/kenney_particle_pack/muzzle_01.png` (82 KB)
  - `effects/kenney_particle_pack/muzzle_02.png` (58 KB)
  - `effects/kenney_particle_pack/muzzle_03.png` (57 KB)
  - `effects/kenney_particle_pack/muzzle_04.png` (63 KB)
  - `effects/kenney_particle_pack/muzzle_05.png` (54 KB)
  - `effects/kenney_particle_pack/scorch_01.png` (61 KB)
  - `effects/kenney_particle_pack/scorch_02.png` (74 KB)
  - `effects/kenney_particle_pack/scorch_03.png` (92 KB)
  - `effects/kenney_particle_pack/smoke_01.png` (97 KB)
  - `effects/kenney_particle_pack/smoke_02.png` (97 KB)
  - `effects/kenney_particle_pack/smoke_03.png` (40 KB)
  - `effects/kenney_particle_pack/smoke_04.png` (98 KB)
  - `effects/kenney_particle_pack/smoke_05.png` (83 KB)
  - `effects/kenney_particle_pack/smoke_06.png` (62 KB)
  - `effects/kenney_particle_pack/smoke_07.png` (78 KB)
  - `effects/kenney_particle_pack/smoke_08.png` (90 KB)
  - `effects/kenney_particle_pack/smoke_09.png` (88 KB)
  - `effects/kenney_particle_pack/smoke_10.png` (90 KB)
  - `effects/kenney_particle_pack/spark_01.png` (95 KB)
  - `effects/kenney_particle_pack/spark_02.png` (0.10 MB)
  - `effects/kenney_particle_pack/spark_03.png` (80 KB)
  - `effects/kenney_particle_pack/spark_04.png` (84 KB)
  - `effects/kenney_particle_pack/spark_05.png` (65 KB)
  - `effects/kenney_particle_pack/spark_06.png` (46 KB)
  - `effects/kenney_particle_pack/spark_07.png` (41 KB)
  - `effects/kenney_particle_pack/trace_01.png` (38 KB)
  - `effects/kenney_particle_pack/trace_02.png` (39 KB)
  - `effects/kenney_particle_pack/trace_03.png` (42 KB)
  - `effects/kenney_particle_pack/trace_04.png` (47 KB)
  - `effects/kenney_particle_pack/trace_05.png` (42 KB)
  - `effects/kenney_particle_pack/trace_06.png` (36 KB)
  - `effects/kenney_particle_pack/trace_07.png` (33 KB)

### effects/explosion_sheets_stumpystrust

- Name: Explosion Sheet + More Explosions
- Author: StumpyStrust (https://opengameart.org/users/stumpystrust)
- URL: https://opengameart.org/content/explosion-sheet
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: explosion_sheet_boom3.png: 8x8 frames of 128 px. more_explosions_*.png (from https://opengameart.org/content/more-explosions, CC0): 10 columns of 100 px frames, fire turning into smoke, the last rows may be empty.
- Files:
  - `effects/explosion_sheets_stumpystrust/explosion_sheet_boom3.png` (0.37 MB)
  - `effects/explosion_sheets_stumpystrust/more_explosions_test.png` (0.63 MB)
  - `effects/explosion_sheets_stumpystrust/more_explosions_test3.png` (0.59 MB)
  - `effects/explosion_sheets_stumpystrust/more_explosions_tests.png` (0.25 MB)

### effects/flame_sheet_tauran

- Name: Animated flame texture
- Author: tauran (https://opengameart.org/users/tauran)
- URL: https://opengameart.org/content/animated-flame-texture
- License: CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
- Note: 1024x512 sheet, 12 columns x 6 rows (about 85 px frames), with alpha.
- Files:
  - `effects/flame_sheet_tauran/flame_sheet_12x6.png` (0.31 MB)

## HUD font

### fonts/b612_mono

- Name: B612 Mono (Regular, Bold)
- Author: The B612 Project Authors (Nicolas Chauveau, Thomas Paillot, Jonathan Favre-Lamarine, Jean-Luc Vinot; Airbus / Intactile Design)
- URL: https://github.com/google/fonts/tree/main/ofl/b612mono (upstream https://github.com/polarsys/b612)
- License: SIL Open Font License 1.1 (https://openfontlicense.org)
- Note: Monospace cockpit-display font, version 1.008. OFL.txt must ship with the font files.
- Files:
  - `fonts/b612_mono/B612Mono-Bold.ttf` (0.14 MB)
  - `fonts/b612_mono/B612Mono-Regular.ttf` (0.14 MB)
  - `fonts/b612_mono/OFL.txt` (4 KB)

## Markings

### markings

- Name: Israeli Air Force roundel (blue Star of David on a white disc)
- Author: drawn for this project (not downloaded), from reference photos and the public description of the insignia
- License: our own file; the insignia itself is a national emblem, not a copyrighted artwork of a third party
- Note: disc radius 100, star circumradius 81, fill #0038B8 (blue of the Israeli flag; real aircraft paint ranges to a darker navy, change the fill if needed). The PNG is a 1024 px anti-aliased raster of the same geometry with a transparent background, ready for stb_image.
- Files:
  - `markings/iaf-roundel.svg` (9 KB)
  - `markings/iaf-roundel_1024.png` (67 KB)

## Rejected or left out

- Sketchfab "F-16 NL FF version NATO Standard" by cloudhub (CC BY 4.0 on the page): its source archive is named "F-16_Fighting_Falcon_-_Fighter_Jet_-_Free.usdz", which is bohmerang's standalone F-16 released under CC BY-NC-SA. The textures most likely come from that NC model, so the CC BY label is doubtful. Not used; bohmerang's own CC BY collection copy is used instead (f16_bohmerang_lowpoly).
- Sketchfab "USSR 5N63S S-300 SAM Radar (War Thunder AF!)" by KojfDiscord (CC BY 4.0 on the page): the title points to a War Thunder game asset, so the uploader may not own it. Not used.
- Sketchfab "Hardened Aircraft Shelter HAS -DRAFT-" by samuelbrunner (CC BY 4.0): the only real HAS found, but a draft, about 50k triangles, and part of the mesh has no UVs. Not used; the Quonset hut is the stand-in. URL: https://sketchfab.com/3d-models/hardened-aircraft-shelter-has-draft-ccc398978c0f4bb3955ada9d9a1dcf3c
- bohmerang F-35 from the same collection: converted (41k triangles, groups like the F-16, no texture) but not included for size; can be regenerated from the collection.
- Everything under CC BY-NC(-SA), ND, Sketchfab Standard or store licenses (for example bohmerang "Missile & Bomb Collection", Civorsky AIM-9M, 42manako S-400) was skipped.

