# Vision Engine OpenGL Patch

Open source release of the fan-made OpenGL renderer for EA's **Vision Engine** games:

- **Wing Commander: Prophecy**
- **Wing Commander: Secret Ops**

This patch was originally proposed and requested for the fan-developed [Wing Commander: Standoff](https://standoff.solsector.net/) project to add enhancements such as high resolution backgrounds and specular maps with little focus on matching the original visuals, however over time the complete set of original 3DFX effects have been added to support a more classic look.

This renderer does not modify the original game in any way and instead provides an alternative to the original Software/Direct3D/Glide graphics library dlls. 

Now decades since the projects inception it relies on a blend of the fixed function pipeline and the now defunct [CGFX](https://developer.nvidia.com/cg-toolkit). This repository is released as-is for reference for the community and is not actively maintained and the code quality will be reflective of this fact.

> **Important:** This project is an unofficial fan-made optional renderer. It is not affiliated with, endorsed by, or sponsored by Electronic Arts or any of the original game developers or publishers. The original games, their code, artwork, data, trademarks, and other intellectual property remain the property of their respective owners. This project does not claim any ownership of, or rights to, those works.

---

## Legal and licensing

The original source code contained in this repository is released under the **MIT License**, except where otherwise stated.

This repository does **not** grant any rights to the original *Wing Commander* games or to any other third-party intellectual property. You must own and obtain the original games through legitimate means in order to use this renderer with them.

The repository does not include the original game executable, game data, artwork, movies, or other copyrighted game assets.

The license for this repository does not extend to versions earlier than 1.4.


### Third-party components

This project uses third-party software which is subject to its own licensing terms:

- **NVIDIA Cg Toolkit** — required to build the project but **not included** in this repository. It must be obtained separately from NVIDIA and is subject to NVIDIA's licensing terms.

The MIT License applies only to the portions of this project for which the author has the right to grant that license. Third-party components remain subject to their respective licenses.

### Trademarks

*Wing Commander*, *Wing Commander: Prophecy*, *Wing Commander: Secret Ops*, and related names and marks are the property of their respective owners.

Use of these names in this project is solely for identification and compatibility purposes.

---

## Requirements

### Build environment

- [Microsoft Visual Studio 2026](https://visualstudio.microsoft.com/downloads/)
- [NVIDIA Cg Toolkit](https://developer.nvidia.com/cg-toolkit)

### NVIDIA Cg Toolkit

Download and install the Cg Toolkit separately.

The build currently expects the following libraries to be available in the repository root:

```text
cg.lib
cgGL.lib
```

The Cg Toolkit is no longer actively developed by NVIDIA, so obtaining a compatible copy may require using an archived distribution.

---

## Features

### Original Vision Engine effects

The renderer supports the original effects used by the games, including:

- Textured space
- Lens flares
- Fog
- Coloured lights
- Translucency

### Optional enhancements

The OpenGL renderer additionally supports:

- Arbitrary resolutions and aspect ratios
- Widescreen rendering
- Bloom
- Automatic specular lighting
- Cubemap-based starfields
- FSAA
- Per-pixel lighting effects

Gameplay remains internally 4:3 unless the game data is modified to support widescreen gameplay, as done by projects such as *Wing Commander: Standoff*.

### Enhanced materials

With suitably modified game assets, the renderer supports:

- Specular maps
- Normal maps
- Emissive maps
- Iridescence effects

No modified game assets are included with this project.

---

## Configuration

Most runtime options are controlled by `GLOpenGL.cfg`.

| Option | Description |
|---|---|
| `width` | Screen width in pixels |
| `height` | Screen height in pixels |
| `fullscreen` | Use fullscreen or windowed mode |
| `fastScaling` | Use direct pixel drawing for scaling. Lower quality; generally not recommended on modern systems |
| `starbox` | Use the built-in cubemap for stars instead of the original 2D starfield |
| `scaleBackground` | Scale background images. Normally leave this set to `true` |
| `maxAnisotropy` | Maximum texture anisotropy |
| `FSAASamples` | Number of FSAA samples |
| `shaderMaterials` | Enable shader-based materials with per-pixel lighting |
| `treFileName` | Override the TRE file to load |
| `treFileNameEX0-treFileNameEX9` | Additional TRE files to load
| `depthBits` | Requested depth buffer size; currently clamped to 24-bit |
| `useHDR` | Enable HDR effects such as bloom |
| `forceATIHDRFix` | Workaround for older ATI drivers and non-power-of-two textures; generally no longer necessary |
| `forceAutoSpec` | Enable automatic specular lighting regardless of material overrides |
| `obeyScreenPolyAlpha` | Whether to obey the alpha multiplier on HUD elements. `false` approximates Direct3D behaviour; `true` approximates 3dfx behaviour |
| `fogDensityScale` | Multiplier applied to the fog density requested by the game |
| `screenPolySafetyOffset` | A shift applied to the UVs to work around the texel alignment issue |
| `refreshRate` | Override the refresh rate of your monitor when full screen is true (`0` indicates use the current dekstop value)
| `windowOffset` | Hack for wcphr to allow the movie video to render on top. On some intel devices it was noted that a window matching the display resolution would behave differently and the game window would remain in focus. A value of 1 indicates make the screen 1 pixel wider than the display to prevent this issue. If this option is missing from the cfg file it will default to 1 for Intel cards and 0 otherwise. Only experiment with this if you are having issues with hi-res movies in Prophecy. 


---

# Developer documentation

The renderer contains a number of optional asset extensions. These are encoded using the first palette entry of a bitmap.

Palette entry `0` has a special meaning in the original engine: it represents transparency. Consequently, the actual colour value stored in that entry is otherwise irrelevant and can be repurposed for these extensions.

## Bitmap overrides

To enable bitmap overrides, compile with:

```c
#define SUPPORT_HI_RES_BGS 1
```

unfortunately due to the animated nature of the backgrounds there isn't a good way to just redirect the dll to external files.
You must encode high resolution backgrounds with a 256 color palette

### Palette entry tags

The following palette entry values identify the intended placement of a bitmap:

| Tag | Meaning |
|---|---|
| `1280960` | Image is intended for a 1280x960 coordinate system and is centred (only version available without SUPPORT_HI_RES_BGS) |
| `1024768` | Image is intended for a 1024×768 coordinate system and is centred |
| `800600` | Image is intended for an 800×600 coordinate system and is centred |
| `0x004F4654` | Image is intended for a 640×480 coordinate system and is centred |


---

## Material extensions

A palette entry beginning with `0x2D` is used for additional 2D material effects, primarily particles.

If the first byte of palette entry `0` is `0x2D`, the following flags are checked in the second byte.

| Flag | Meaning |
|---|---|
| `0x40` | Bloom |
| `0x80` | Additive blending |

When HDR is enabled, the third byte of palette entry `0` controls the brightness multiplier for the bloom material:

```text
brightness = byte3 / 10
```

For example, a value of `255` produces a brightness multiplier of `25.5`.

This is useful for extremely bright objects such as engine flames and the sun.

The `0x80` additive flag causes translucent materials to use additive blending, allowing overlapping particles to become progressively brighter.

---

## Texture overrides

If palette entry `0` begins with `0x5F` or `0x61`, the renderer checks for texture overrides.

In this format:

- The first four bits of byte 1 contain the texture flags.
- The remaining four bits of byte 1, together with byte 2, form the texture ID.

### Texture override flags

| Flag | Meaning | Directory |
|---|---|---|
| `0x1` | Specular map | `specmaps` |
| `0x2` | Texture override | `overrides` |
| `0x4` | Normal map | `normalmaps` |
| `0x8` | Emissive map | `emissivemaps` |

For example:

```text
0x5F 0x10 0x02
```

requests a specular map with texture ID `0x002`, corresponding to:

```text
specmaps/00000002.tga
```

Note that override textures can be placed loosely in the file structure or in one of the TRE files

### Automatic specular

A specular texture ID of `0` has a special meaning.

Instead of loading a separate specular texture, the renderer derives the specular intensity from the **average of the color channels of the base colour texture**.

This provides a convenient way to add specular lighting without requiring a separate texture.

---

## Iridescence materials

When byte `0` is `0x61`, the material can additionally use the iridescence effect originally used for the Nephilim in the MUP.

The normal texture override flags continue to apply, and the renderer also looks for:

```text
lookupmaps/
noisemaps/
iridescencemaps/
```

These provide:

- A lookup texture
- A noise texture
- An iridescence map

---

## Packaging files

You can package files into the default GLOpenGL.tre. The most essential files are included in this source release.
You can specify additional TRE files using 'treFileNameEX0', 'treFileNameEX1' etc in GLOpenGL.cfg



# Changelog

## v1.4

- Fixed 2D starfields rendering incorrectly when bloom was enabled.
- Improved 2D starfield rendering
- Fixed screen polygons rendering over the in-game menu.
- Added support for 3DFX style fog to the shader based rendering path
- Fixed rendering of translucent HUD elements
- Sort briefing lines by distance (line smooth doesn't play too well with depth testing)
- Graceful handling of invalid resolutions
- Refresh rate override support
- Resolved compatibility issues with wcphr external movie window
- Fixed window creation code on Intel when FSAA is enabled
- Added `windowOffset` to combat an window exclusivity issue on intel machines

## v1.3

### All modes

- Added support for Grim's movie playback.
  - The ODVS high-resolution DVD pack is recommended.
- Reduced briefing line width on high-resolution displays.
- Fixed the original FMV aspect ratio on super-wide displays.
- Fixed pixel-based stars when the starfield is disabled.
- Matched 3dfx HUD transparency behaviour.
- Removed cockpit shake.
- Fixed target rendering on the radar.

### HDR enabled

- Re-enabled lens flares.
  - These had previously been disabled for *Standoff* because of its very bright assets.
- Reduced the intensity of briefing bloom.
- Fixed a missile VDU bug that could cause the screen to flash.

### HDR disabled

- Ignored additive and super-bright flags on overridden assets, which are primarily balanced for HDR rendering.
- Fixed a navigation-map rendering bug.

## v1.2.2

- Fixed an API bug which could patch out the first files in `DATA.TRE`, causing *Wing Commander: Prophecy* to crash when launching from the Midway with the super-alien gun.
- Added a retroactive fix for repairing affected `DATA.TRE` files in existing installations.

## v1.2.1

- Added compatibility with HCl's Enhancement Pack DVD patch, with assistance from HCl.

## v1.2

- Fixed compatibility with Intel integrated graphics and some ATI graphics cards.
- Fixed additional compatibility issues.
- Added lighting effects:
  - Bloom
  - Automatic specular lighting
- Added support for high-resolution artwork, including artwork created for *Standoff* and artwork created specifically for this patch.
- Added support for iridescence maps.

---

# Credits

### Development

**Alex Barnfield (Pedro)**  
**Pierre Deshaies (PopsiclePete)**  

### Supporting assets (not included on this GitHub)

**Robbie Fowler (Filler)**  
**Eder Vieito**  

### Thanks

Special thanks to:

- **HCl** 
- **Quarto**
- **Eder**
- **Defiance Industries**
- **Dundradal**
- **Grim**
- **AD**

Thanks to **Grim** for helping add support for his HD movie patch and `wcpunl`, and to **HCl** for making his enhancement pack compatible through the various iterations of this renderer.

---

# Troubleshooting

## Launching from the CD

Launching the game directly from the original CD may not work on modern versions of Windows.

If necessary, manually set the compatibility mode for the following executables **on the CD drive**:

```text
launcher.exe
setup.exe
prophecy.exe
```

Set them to **Windows 98 compatibility mode**.


## Returning to DirectX or Glide

To switch *Wing Commander: Prophecy* or *Secret Ops* back to another renderer, run `Launcher.exe` and select **Select Video Card**.

---

# Project status

This is a historical fan-made graphics project originally developed to extend the life of the Vision Engine and support modern hardware.

The code is being released publicly so that the work can be preserved, studied, maintained, and potentially replaced with a more modern API by the community.

The original games and their assets are **not part of this project** and remain under the control of their respective rights holders.

---

## License

This project is licensed under the **MIT License**, subject to the third-party licensing terms described above.

See `LICENSE` for the complete license text.

**Wing Commander and all related game content are trademarks and/or copyrighted works of their respective owners. This project is an unofficial fan project and makes no claim of ownership over those works.**