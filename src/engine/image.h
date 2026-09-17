#ifndef IMAGE_H
#define IMAGE_H

typedef struct {
    unsigned char *pixels;
    int width;
    int height;
    int channels;
} Image;

// rgb floats, three per pixel
typedef struct {
    float *pixels;
    int width;
    int height;
} HdrImage;

// rows come out bottom-up, matching the OBJ texture convention of the collected models
int image_load(Image *image, const char *relative, const char **reason);
// the frames laid out in a grid of that many columns, each centered in a cell as wide as the widest of them;
// frame 0 goes into the bottom left cell, the way the rows come out of image_load
int image_atlas(Image *atlas, const char *const *paths, int count, int columns, const char **reason);
// the image must be 4 channels
void image_bleed(Image *image, int passes);
void image_free(Image *image);

int image_load_hdr(HdrImage *image, const char *relative, const char **reason);
void image_free_hdr(HdrImage *image);

#endif
