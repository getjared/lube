<div align="center">

```ascii

███████████████████████████████████████████████████████████████
█░░░░░░█████████░░░░░░██░░░░░░█░░░░░░░░░░░░░░███░░░░░░░░░░░░░░█
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀▄▀▄▀▄▀▄▀░░███░░▄▀▄▀▄▀▄▀▄▀░░█
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀░░░░░░▄▀░░███░░▄▀░░░░░░░░░░█
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀░░██░░▄▀░░███░░▄▀░░█████████
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀░░░░░░▄▀░░░░█░░▄▀░░░░░░░░░░█
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀▄▀▄▀▄▀▄▀▄▀░░█░░▄▀▄▀▄▀▄▀▄▀░░█
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀░░░░░░░░▄▀░░█░░▄▀░░░░░░░░░░█
█░░▄▀░░█████████░░▄▀░░██░░▄▀░░█░░▄▀░░████░░▄▀░░█░░▄▀░░█████████
█░░▄▀░░░░░░░░░░█░░▄▀░░░░░░▄▀░░█░░▄▀░░░░░░░░▄▀░░█░░▄▀░░░░░░░░░░█
█░░▄▀▄▀▄▀▄▀▄▀░░█░░▄▀▄▀▄▀▄▀▄▀░░█░░▄▀▄▀▄▀▄▀▄▀▄▀░░█░░▄▀▄▀▄▀▄▀▄▀░░█
█░░░░░░░░░░░░░░█░░░░░░░░░░░░░░█░░░░░░░░░░░░░░░░█░░░░░░░░░░░░░░█
███████████████████████████████████████████████████████████████
```

**[ motion amplification for still images ]**

[![License: Unlicense](https://img.shields.io/badge/License-Unlicense-pink.svg)](http://unlicense.org/)
[![Made with C](https://img.shields.io/badge/Made%20with-C-purple.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

</div>

## ✧ features

- 🎯 interactive region selection
- 🎨 advanced color quantization
- 🌊 smooth motion interpolation
- 🎬 high quality gif output
- 🎮 customizable motion parameters

## ✧ examples

| still  | motion |
|--------|-------|
| ![before](https://imgur.com/a/gAvL7Iw) | ![after](https://jmp.sh/zFbu69J9) |

| type | link |
|------|------|
| usage demo | [view](https://jmp.sh/IYc13kqN) |

## ✧ installation

```bash
git clone https://github.com/getjared/lube.git
cd lube
make
sudo make install
```

## ✧ dependencies

- 📝 c compiler (gcc or clang)
- 🔧 make
- 📸 libjpeg (jpeg handling)
- 🎬 giflib (gif creation)
- 🎮 sdl2 (region selection)

## ✧ quick start guide

### basic usage

```bash
# create animation with defaults
./lube input.jpg output.gif

# use 30 frames
./lube -f 30 input.jpg output.gif

# slower animation
./lube -t 5 input.jpg output.gif

# horizontal only
./lube -m 0 input.jpg output.gif
```

### motion types

| flag | effect |
|------|--------|
| `-m 0` | horizontal motion only |
| `-m 1` | vertical motion only |
| `-m 2` | both directions (default) |

### controls

| flag | description | range |
|------|-------------|--------|
| `-f <frames>` | frame count | 1-30 (default: 24) |
| `-t <delay>` | frame delay in 1/100s | (default: 3) |
| `-h` | show help | - |

## ✧ interactive usage

1. run with your desired options
2. in the window:
   - 🖱️ click and drag for motion areas
   - ⭕ bigger circles = more motion area
   - 🔢 up to 10 regions allowed
   - 🚪 close window or press esc when done
3. ⏳ wait for gif creation

## ✧ example effects

### nature effects
```bash
# gentle wave effect on water
./lube -m 0 -f 30 -t 8 lake.jpg wave.gif

# subtle tree movement
./lube -m 2 -f 24 -t 5 forest.jpg breeze.gif

# slow flowing clouds
./lube -m 0 -f 30 -t 10 sky.jpg clouds.gif

# ripple effect
./lube -m 2 -f 20 -t 3 pond.jpg ripple.gif
```

### object effects
```bash
# flag waving
./lube -m 0 -f 24 -t 4 flag.jpg wave.gif

# grass moving in wind
./lube -m 2 -f 30 -t 6 field.jpg wind.gif

# make leaves rustle
./lube -m 2 -f 24 -t 4 tree.jpg rustle.gif
```

### artistic effects
```bash
# create a dreamy effect
./lube -m 2 -f 30 -t 8 portrait.jpg dream.gif

# create subtle facial animation
./lube -m 2 -f 20 -t 6 face.jpg animate.gif

# water reflection movement
./lube -m 0 -f 30 -t 5 reflection.jpg shimmer.gif
```

## ✧ pro tips

- 🌊 use slower delays (-t 8-10) for subtle effects
- 🎯 combine multiple regions for complex motion
- 🎨 smaller regions = more precise control
- 🔄 try different motion modes for varied effects
- 🎬 experiment with frame counts for smoothness

## ✧ advanced examples

```bash
# super slow and subtle
./lube -m 2 -f 30 -t 15 input.jpg subtle.gif

# quick and bouncy
./lube -m 2 -f 15 -t 2 input.jpg quick.gif

# smooth horizontal drift
./lube -m 0 -f 30 -t 7 input.jpg drift.gif

# gentle vertical float
./lube -m 1 -f 24 -t 6 input.jpg float.gif

# combine both for complex motion
./lube -m 2 -f 30 -t 5 input.jpg complex.gif
```

## ✧ technical details

- 🎨 uses median cut for colors (up to 256)
- 🌊 smooth motion interpolation
- 📊 gaussian motion falloff
- 🔄 frame-by-frame processing
- 🎨 automatic palette generation

## ✧ limitations

- 📸 jpeg input only
- 🎬 gif output only
- ⚡ bigger images = slower processing
- 🎯 max 10 motion regions
- 🎬 max 30 frames per animation

## ✧ credits

- [libjpeg](http://libjpeg.sourceforge.net/)
- [giflib](http://giflib.sourceforge.net/)
- [sdl2](https://www.libsdl.org/)

<div align="center">

```ascii
╭─────────────────────────╮
│  made with ♥ by jared   │
╰─────────────────────────╯
```

</div>
