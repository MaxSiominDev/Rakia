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
int image_load(Image *image, const char *path, const char **reason);
void image_free(Image *image);

int image_load_hdr(HdrImage *image, const char *path, const char **reason);
void image_free_hdr(HdrImage *image);

#endif
