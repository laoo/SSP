# SSP
Suzy Sprite Packer is a command line tool operating on PNG images and generating packed sprites for Atari Lynx Suzy chip.

## Building

Use `cmake` to build the project or download 64-bit Windows binary from releases.

Requires `boost` uses `lode_png`

## Usage

Running without arguments will print the usage.

```
Suzy Sprite Packer:
  -h [ --help ]                     produce help message
  -i [ --input ] arg                input file, fist implicit agrument
  -o [ --output ] arg               output file, second implicit argument
                                    (default: input with .spr extension)
  -p [ --palette ] arg              image to compute palette from (default:
                                    input)
  -r [ --require-palette ]          report error if palette file does not exist
                                    (default: substitue input for palette if it
                                    does not exist)
  -s [ --save-optimal-palette ] arg saves optimal palette under given filename
                                    if it does not exist or is older than
                                    source palette (default: does nothing)
  -w [ --write-output-image ] arg   write PNG image with reduced palette
                                    (default: does nothing)
  -t [ --separate-outputs ]         write palette and sprite data separate
                                    files (default: one file)
  -f [ --frame-width ] arg          width of one frame of animation, must be a
                                    divisor of image width (default: image
                                    width)
  -c [ --max-colors ] arg           maximal number of colors in the palette
                                    (default: 16)
  -b [ --bpp ] arg                  forced bits per pixel (default: smallest
                                    possible)
  -l [ --literal ]                  do not compress sprite (default: off)
  -g [ --background ]               sprite is a background sprite (first color
                                    does not need to be black/transparent
                                    (default: off)
  -v [ --verbose ]                  write verbose information to standard
                                    output (default: off)
  -x [ --no-sprite-gen ]            do not generate output sprite. Useful if
                                    only optimal palette is needed (default:
                                    off)
```

### Basic Usage

Running
```
SSP -i sprite.png
```

or equivalently
```
SSP sprite.png
```

will create a packed sprite with at most 16 colors in file `sprite.spr`. Output name can be overriden using `-o` parameter (or just with additional filename, as `-o` if a default second parameter):
```
SSP sprite.png -o sprite.spr
```
or
```
SSP sprite.png sprite.spr
```

By default palette data and sprite data are merged in one file:
* 8 bytes of pen mapping at offset 0
* 16 bytes of green palette data at offset 8
* 16 bytes of blue and red palette data at offset 24
* raw sprite data starts at offset 40

Running with parameter `-t` will write palette and sprite data to separate files. The file `sprite.spr` will have raw sprite data and palette file named `sprite.pal` will have pen mappings and palette colors is in the textual format.

### Separate palette specification

By default SSP uses colors from input image but the palette can be specified in separate file. This is useful when you want to use the same palette for multiple sprites.
The palette is an ordinary PNG image that uses desired colors. It can be specified using `-p` parameter.
```
SSP sprite.png -p palette.png
```

When the palette file is not specified or if the palette file does not exist SSP will use input image as palette. This latter can be disabled using `-r` parameter.

The image of palette does not need to have 16 or less colors. SSP will reduce the palette to 16 colors by averaging colors that are close to each other.
The algorith in current version is trivial though ( O(n^3^) ) and it will deny to process palettes with more than 256 colors.

Additional useful feature of palette processing is that it can be saved to file using `-s` parameter. It should simplify the process of creating palette - just compose an image with the background and all sprites on the scene and specify it as input to SSP with parameters `-x` to disable writing output sprite and `-s` to write optimized palette.
```
SSP composition.png -xs palette.png
```

The palette is saved only if it does not exist or is older than the source palette, so running SSP as:
```
SSP sprite.png -p palette.png -s palette.png
```
will generate `palette.png` only if it does not exist or is older than `sprite.png` using colors from `sprite.png` and will be used henceforth (may be useful to reduce processing time if `sprite.png` has many colors).

### Color resolution

By default SSP will use at most 16 colors in the palette and 4 bits per pixel. This can be changed using `-c` and `-b` parameters respectively.
Number of colors can be reduced to less than 16 and the number of bits per pixel can be increased if desired.

By using `-w` option the source image can be saved with the reduced palette to the specified PNG image. This can be useful to see how the palette looks like and to check if the palette is correct.

### Animation

SSP has trivial animation support. The animation is specified by the width of one frame of animation using `-f` parameter. The width must be a divisor of the image width. Hence the frames of the animation must be laid out horizontally in the image.
The SSP will generate sprite data for each frame of animation to separate output files.

### Verbosity

Some verbose output can be printed to standard output by specifying `-v` parameter.

