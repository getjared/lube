#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <gif_lib.h>
#include <jpeglib.h>
#include <errno.h>
#include <setjmp.h>
#include <SDL2/SDL.h>

#define MAX_FRAMES 30
#define DEFAULT_FRAME_COUNT 24
#define DEFAULT_DELAY_TIME 3
#define COLOR_DEPTH 256
#define MAX_REGIONS 10

typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} Image;

typedef struct {
    int x, y;
    int radius;
    float dx, dy;
    float frequency;
    float falloff;
} MotionRegion;

typedef struct {
    unsigned char r, g, b;
} Color;

typedef struct {
    Color *colors;
    int count;
    Color average;
} ColorBox;

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static Image load_jpeg(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening input JPEG file");
        exit(EXIT_FAILURE);
    }

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        fprintf(stderr, "Error during JPEG decompression\n");
        exit(EXIT_FAILURE);
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);

    int header_result = jpeg_read_header(&cinfo, TRUE);
    if (header_result != 1) {
        fprintf(stderr, "Error reading JPEG header\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    jpeg_start_decompress(&cinfo);

    Image img = {
        .width = cinfo.output_width,
        .height = cinfo.output_height,
        .channels = cinfo.output_components,
        .data = malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components)
    };

    if (!img.data) {
        fprintf(stderr, "Error allocating memory for image data\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    while (cinfo.output_scanline < cinfo.output_height) {
        unsigned char *row_ptr = img.data + cinfo.output_scanline * img.width * img.channels;
        jpeg_read_scanlines(&cinfo, &row_ptr, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    return img;
}

static SDL_Surface* image_to_sdl_surface(const Image *img) {
    if (img->channels != 3) {
        fprintf(stderr, "Unsupported number of channels: %d. Only RGB images are supported.\n", img->channels);
        return NULL;
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
        img->data,
        img->width,
        img->height,
        img->channels * 8,
        img->width * img->channels,
        0x0000FF,
        0x00FF00,
        0xFF0000,
        0
    );

    if (!surface) {
        fprintf(stderr, "SDL_CreateRGBSurfaceFrom Error: %s\n", SDL_GetError());
        return NULL;
    }

    return surface;
}

static int select_regions(SDL_Surface *image, MotionRegion *regions, int max_regions, int motion_mode) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window *win = SDL_CreateWindow("Select Motion Regions", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, image->w, image->h, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL) {
        SDL_DestroyWindow(win);
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, image);
    if (tex == NULL) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    int running = 1;
    SDL_Event e;
    int selecting = 0;
    int start_x = 0, start_y = 0, end_x = 0, end_y = 0;
    SDL_Rect current_rect = {0, 0, 0, 0};
    int region_count = 0;

    printf("Instructions:\n");
    printf("  - Click and drag the mouse to select a circular region.\n");
    printf("  - Repeat to select up to %d regions.\n", max_regions);
    printf("  - Press ESC or close the window to finish selection.\n");

    while (running && region_count < max_regions) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                selecting = 1;
                start_x = e.button.x;
                start_y = e.button.y;
                current_rect.x = start_x;
                current_rect.y = start_y;
                current_rect.w = 0;
                current_rect.h = 0;
            }
            else if (e.type == SDL_MOUSEMOTION && selecting) {
                end_x = e.motion.x;
                end_y = e.motion.y;
                current_rect.w = end_x - start_x;
                current_rect.h = end_y - start_y;
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && selecting) {
                selecting = 0;
                end_x = e.button.x;
                end_y = e.button.y;

                int center_x = (start_x + end_x) / 2;
                int center_y = (start_y + end_y) / 2;
                int radius = (int)sqrt((start_x - end_x) * (start_x - end_x) + (start_y - end_y) * (start_y - end_y)) / 2;

                if (radius <= 0) {
                    printf("Invalid region selected. Please select a larger area.\n");
                    continue;
                }

                regions[region_count].x = center_x;
                regions[region_count].y = center_y;
                regions[region_count].radius = radius;

                switch (motion_mode) {
                    case 0:
                        regions[region_count].dx = 15.0f;
                        regions[region_count].dy = 0.0f;
                        break;
                    case 1:
                        regions[region_count].dx = 0.0f;
                        regions[region_count].dy = 15.0f;
                        break;
                    case 2:
                    default:
                        regions[region_count].dx = 15.0f;
                        regions[region_count].dy = 10.0f;
                        break;
                }

                regions[region_count].frequency = 1.0f;
                regions[region_count].falloff = 2.0f;
                region_count++;

                printf("Selected Region %d: Center=(%d, %d), Radius=%d\n", region_count, center_x, center_y, radius);
            }
        }

        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);

        if (selecting) {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
            SDL_Rect rect = current_rect;
            if (rect.w < 0) {
                rect.x += rect.w;
                rect.w = -rect.w;
            }
            if (rect.h < 0) {
                rect.y += rect.h;
                rect.h = -rect.h;
            }
            SDL_RenderDrawRect(ren, &rect);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return region_count;
}

static float calculate_displacement(int x, int y, const MotionRegion *region, float phase) {
    float dx = (float)(x - region->x);
    float dy = (float)(y - region->y);
    float distance = sqrtf(dx * dx + dy * dy);

    if (distance > region->radius) return 0.0f;

    float influence = 1.0f - powf(distance / region->radius, region->falloff);
    float motion = sinf(phase * region->frequency);

    return influence * motion;
}

static Image *amplify_motion(const Image *src, int frame_count, const MotionRegion *regions, int num_regions) {
    Image *frames = calloc(frame_count, sizeof(Image));
    if (!frames)
        die("Error allocating memory for frames");

    for (int f = 0; f < frame_count; f++) {
        float phase = (2.0f * M_PI * f) / frame_count;
        frames[f].width = src->width;
        frames[f].height = src->height;
        frames[f].channels = src->channels;
        frames[f].data = malloc(src->width * src->height * src->channels);
        if (!frames[f].data)
            die("Error allocating memory for frame data");

        for (int y = 0; y < src->height; y++) {
            for (int x = 0; x < src->width; x++) {
                float total_dx = 0.0f;
                float total_dy = 0.0f;

                for (int r = 0; r < num_regions; r++) {
                    float disp = calculate_displacement(x, y, &regions[r], phase);
                    total_dx += disp * regions[r].dx;
                    total_dy += disp * regions[r].dy;
                }

                int src_x = x + (int)(total_dx + 0.5f);
                int src_y = y + (int)(total_dy + 0.5f);

                src_x = src_x < 0 ? 0 : (src_x >= src->width ? src->width - 1 : src_x);
                src_y = src_y < 0 ? 0 : (src_y >= src->height ? src->height - 1 : src_y);

                int dst_offset = (y * src->width + x) * src->channels;
                int src_offset = (src_y * src->width + src_x) * src->channels;
                memcpy(&frames[f].data[dst_offset], &src->data[src_offset], src->channels);
            }
        }
    }
    return frames;
}

static int compare_r(const void *a, const void *b) {
    return ((Color *)a)->r - ((Color *)b)->r;
}

static int compare_g(const void *a, const void *b) {
    return ((Color *)a)->g - ((Color *)b)->g;
}

static int compare_b(const void *a, const void *b) {
    return ((Color *)a)->b - ((Color *)b)->b;
}

static void find_color_range(ColorBox *box, int *r_min, int *r_max, int *g_min, int *g_max, int *b_min, int *b_max) {
    *r_min = *g_min = *b_min = 255;
    *r_max = *g_max = *b_max = 0;

    for (int i = 0; i < box->count; i++) {
        if (box->colors[i].r < *r_min) *r_min = box->colors[i].r;
        if (box->colors[i].r > *r_max) *r_max = box->colors[i].r;
        if (box->colors[i].g < *g_min) *g_min = box->colors[i].g;
        if (box->colors[i].g > *g_max) *g_max = box->colors[i].g;
        if (box->colors[i].b < *b_min) *b_min = box->colors[i].b;
        if (box->colors[i].b > *b_max) *b_max = box->colors[i].b;
    }
}

static int split_box(ColorBox *input_box, ColorBox *box1, ColorBox *box2) {
    int r_min, r_max, g_min, g_max, b_min, b_max;
    find_color_range(input_box, &r_min, &r_max, &g_min, &g_max, &b_min, &b_max);

    int r_range = r_max - r_min;
    int g_range = g_max - g_min;
    int b_range = b_max - b_min;

    int sort_channel = 0;
    if (g_range >= r_range && g_range >= b_range)
        sort_channel = 1;
    else if (b_range >= r_range && b_range >= g_range)
        sort_channel = 2;

    if (sort_channel == 0)
        qsort(input_box->colors, input_box->count, sizeof(Color), compare_r);
    else if (sort_channel == 1)
        qsort(input_box->colors, input_box->count, sizeof(Color), compare_g);
    else
        qsort(input_box->colors, input_box->count, sizeof(Color), compare_b);

    int median = input_box->count / 2;

    box1->count = median;
    box1->colors = malloc(box1->count * sizeof(Color));
    if (!box1->colors) {
        fprintf(stderr, "Error allocating memory for color box 1\n");
        return -1;
    }
    memcpy(box1->colors, input_box->colors, box1->count * sizeof(Color));

    box2->count = input_box->count - median;
    box2->colors = malloc(box2->count * sizeof(Color));
    if (!box2->colors) {
        fprintf(stderr, "Error allocating memory for color box 2\n");
        free(box1->colors);
        return -1;
    }
    memcpy(box2->colors, input_box->colors + median, box2->count * sizeof(Color));

    return 0;
}

static void compute_average(ColorBox *box) {
    unsigned long r_total = 0, g_total = 0, b_total = 0;
    for (int i = 0; i < box->count; i++) {
        r_total += box->colors[i].r;
        g_total += box->colors[i].g;
        b_total += box->colors[i].b;
    }
    box->average.r = (unsigned char)(r_total / box->count);
    box->average.g = (unsigned char)(g_total / box->count);
    box->average.b = (unsigned char)(b_total / box->count);
}

static ColorMapObject* median_cut(const Image *frame, int color_depth) {
    Color *all_colors = malloc(frame->width * frame->height * sizeof(Color));
    if (!all_colors) {
        fprintf(stderr, "Error allocating memory for all_colors\n");
        return NULL;
    }

    int total_pixels = frame->width * frame->height;
    for (int i = 0; i < total_pixels; i++) {
        all_colors[i].r = frame->data[i * frame->channels];
        all_colors[i].g = frame->data[i * frame->channels + 1];
        all_colors[i].b = frame->data[i * frame->channels + 2];
    }

    ColorBox initial_box;
    initial_box.count = total_pixels;
    initial_box.colors = all_colors;

    ColorBox *boxes = malloc(color_depth * sizeof(ColorBox));
    if (!boxes) {
        fprintf(stderr, "Error allocating memory for color boxes\n");
        free(all_colors);
        return NULL;
    }

    boxes[0] = initial_box;
    int box_count = 1;

    while (box_count < color_depth) {
        int max_range = -1;
        int box_to_split = -1;
        for (int i = 0; i < box_count; i++) {
            int r_min, r_max, g_min, g_max, b_min, b_max;
            find_color_range(&boxes[i], &r_min, &r_max, &g_min, &g_max, &b_min, &b_max);
            int range = (r_max - r_min) > (g_max - g_min) ? (r_max - r_min) : (g_max - g_min);
            range = (range > (b_max - b_min)) ? range : (b_max - b_min);
            if (range > max_range) {
                max_range = range;
                box_to_split = i;
            }
        }

        if (box_to_split == -1) break;

        ColorBox box1, box2;
        if (split_box(&boxes[box_to_split], &box1, &box2) != 0) {
            for (int i = 0; i < box_count; i++) {
                free(boxes[i].colors);
            }
            free(boxes);
            return NULL;
        }

        boxes[box_to_split] = box1;
        boxes[box_count] = box2;
        box_count++;
    }

    for (int i = 0; i < box_count; i++) {
        compute_average(&boxes[i]);
    }

    ColorMapObject *colormap = GifMakeMapObject(box_count, NULL);
    if (!colormap) {
        for (int i = 0; i < box_count; i++) {
            free(boxes[i].colors);
        }
        free(boxes);
        return NULL;
    }

    for (int i = 0; i < box_count; i++) {
        colormap->Colors[i].Red = boxes[i].average.r;
        colormap->Colors[i].Green = boxes[i].average.g;
        colormap->Colors[i].Blue = boxes[i].average.b;
    }
    colormap->ColorCount = box_count;

    for (int i = 0; i < box_count; i++) {
        free(boxes[i].colors);
    }
    free(boxes);

    return colormap;
}

static GifByteType *create_color_index_buffer(const Image *frame, const ColorMapObject *colormap) {
    GifByteType *buffer = malloc(frame->width * frame->height);
    if (!buffer)
        die("Error allocating color index buffer");

    for (int i = 0; i < frame->width * frame->height; i++) {
        int offset = i * frame->channels;
        int minDist = INT32_MAX;
        int bestIndex = 0;

        for (int c = 0; c < colormap->ColorCount; c++) {
            int dr = frame->data[offset] - colormap->Colors[c].Red;
            int dg = frame->data[offset + 1] - colormap->Colors[c].Green;
            int db = frame->data[offset + 2] - colormap->Colors[c].Blue;
            int dist = dr * dr + dg * dg + db * db;

            if (dist < minDist) {
                minDist = dist;
                bestIndex = c;
            }
        }

        buffer[i] = bestIndex;
    }

    return buffer;
}

static void write_gif(const char *filename, const Image *frames, int frame_count, int delay_time) {
    int error;
    GifFileType *gif = EGifOpenFileName(filename, false, &error);
    if (!gif) {
        fprintf(stderr, "Error opening output GIF file: %s\n", GifErrorString(error));
        exit(EXIT_FAILURE);
    }

    ColorMapObject *colormap = median_cut(&frames[0], COLOR_DEPTH);
    if (!colormap) {
        fprintf(stderr, "Error performing median cut color quantization\n");
        EGifCloseFile(gif, &error);
        exit(EXIT_FAILURE);
    }

    if (EGifPutScreenDesc(gif, frames[0].width, frames[0].height, 8, 0, colormap) == GIF_ERROR) {
        fprintf(stderr, "Error writing screen descriptor: %s\n", GifErrorString(gif->Error));
        EGifCloseFile(gif, &error);
        GifFreeMapObject(colormap);
        exit(EXIT_FAILURE);
    }

    unsigned char app_ext[] = {
        11, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
        3, 1, 0, 0
    };
    if (EGifPutExtension(gif, APPLICATION_EXT_FUNC_CODE, sizeof(app_ext), app_ext) == GIF_ERROR) {
        fprintf(stderr, "Error writing application extension: %s\n", GifErrorString(gif->Error));
        EGifCloseFile(gif, &error);
        GifFreeMapObject(colormap);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < frame_count; i++) {
        unsigned char gce[] = {
            4,
            0x04,
            delay_time & 0xFF,
            (delay_time >> 8) & 0xFF,
            0
        };

        if (EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, sizeof(gce), gce) == GIF_ERROR) {
            fprintf(stderr, "Error writing graphics control extension: %s\n", GifErrorString(gif->Error));
            EGifCloseFile(gif, &error);
            GifFreeMapObject(colormap);
            exit(EXIT_FAILURE);
        }

        GifByteType *indexed = create_color_index_buffer(&frames[i], colormap);
        if (!indexed) {
            fprintf(stderr, "Error creating color index buffer\n");
            EGifCloseFile(gif, &error);
            GifFreeMapObject(colormap);
            exit(EXIT_FAILURE);
        }

        if (EGifPutImageDesc(gif, 0, 0, frames[i].width, frames[i].height, false, NULL) == GIF_ERROR) {
            fprintf(stderr, "Error writing image descriptor: %s\n", GifErrorString(gif->Error));
            free(indexed);
            EGifCloseFile(gif, &error);
            GifFreeMapObject(colormap);
            exit(EXIT_FAILURE);
        }

        for (int y = 0; y < frames[i].height; y++) {
            if (EGifPutLine(gif, &indexed[y * frames[i].width], frames[i].width) == GIF_ERROR) {
                fprintf(stderr, "Error writing image data: %s\n", GifErrorString(gif->Error));
                free(indexed);
                EGifCloseFile(gif, &error);
                GifFreeMapObject(colormap);
                exit(EXIT_FAILURE);
            }
        }

        free(indexed);
    }

    if (EGifCloseFile(gif, &error) == GIF_ERROR) {
        fprintf(stderr, "Error closing GIF file: %s\n", GifErrorString(error));
        GifFreeMapObject(colormap);
        exit(EXIT_FAILURE);
    }

    GifFreeMapObject(colormap);
}

static void usage(const char *prog_name) {
    fprintf(stderr,
        "Usage: %s [options] input.jpg output.gif\n"
        "Options:\n"
        "  -f <frames>      Number of frames for animation (default: 24, max: 30)\n"
        "  -t <delay>       Delay time between frames in hundredths of a second (default: 3)\n"
        "  -m <mode>        Motion mode: 0 for horizontal, 1 for vertical, 2 for both (default: 2)\n"
        "  -h               Show this help message\n",
        prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int frame_count = DEFAULT_FRAME_COUNT;
    int delay_time = DEFAULT_DELAY_TIME;
    int motion_mode = 2;
    int opt;

    while ((opt = getopt(argc, argv, "f:t:m:h")) != -1) {
        switch (opt) {
            case 'f':
                frame_count = atoi(optarg);
                if (frame_count <= 0 || frame_count > MAX_FRAMES) {
                    fprintf(stderr, "Frame count must be between 1 and %d\n", MAX_FRAMES);
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                delay_time = atoi(optarg);
                if (delay_time < 0) {
                    fprintf(stderr, "Delay time must be non-negative\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                motion_mode = atoi(optarg);
                if (motion_mode < 0 || motion_mode > 2) {
                    fprintf(stderr, "Motion mode must be 0 (horizontal), 1 (vertical), or 2 (both)\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    if (argc - optind != 2)
        usage(argv[0]);

    const char *input_file = argv[optind];
    const char *output_file = argv[optind + 1];

    Image src = load_jpeg(input_file);

    SDL_Surface *src_surface = image_to_sdl_surface(&src);
    if (!src_surface) {
        free(src.data);
        exit(EXIT_FAILURE);
    }

    MotionRegion regions[MAX_REGIONS];
    int num_regions = select_regions(src_surface, regions, MAX_REGIONS, motion_mode);
    if (num_regions <= 0) {
        fprintf(stderr, "No regions selected.\n");
        SDL_FreeSurface(src_surface);
        free(src.data);
        exit(EXIT_FAILURE);
    }

    SDL_FreeSurface(src_surface);

    Image *frames = amplify_motion(&src, frame_count, regions, num_regions);

    write_gif(output_file, frames, frame_count, delay_time);

    free(src.data);
    for (int i = 0; i < frame_count; i++)
        free(frames[i].data);
    free(frames);

    printf("Animated GIF '%s' created successfully with %d frame(s).\n", output_file, frame_count);

    return EXIT_SUCCESS;
}
