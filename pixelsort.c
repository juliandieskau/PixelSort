/* pixelsort.c
  Sort pixels in chunks in an image
  open png image as FILE
  TODO: look at chunks of the image with user-defined size
  TODO: sort pixels in a given chunk
  write chunk back into FILE
  save FILE to disk

  Opening and saving png files
  https://gist.github.com/niw/5963798
*/

#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define ERROR -1
#define NOT_PNG -2

size_t CHUNK_SIZE = 16;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers = NULL;

// open png image as FILE
int read_png(char *file_name, FILE *img) {
  char header[8];
  // open png as binary file stream
  img = fopen(file_name, "rb");
  if (!img) {
    printf("File could not be opened.\n");
    return ERROR;
  }
  // read header to check if is png
  fread(header, 1, 8, img);
  int is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png){
    printf("File is not a png.\n");
    return NOT_PNG;
  }
  
  printf("Read png successfully.\n");
}

int initialize_png(FILE *png) {
  // initialize png struct
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)png_get_error_ptr(png_ptr), NULL, NULL);
  if (!png_ptr) return (ERROR);
  
  // initialize png info
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,
                            (png_infopp)NULL, (png_infopp)NULL);
    return (ERROR);
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr,
                            (png_infopp)NULL);
    return (ERROR);
  }

  // set callback for libpng to destroy png struct if error occurs
  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr,
                            &end_info);
    fclose(png);
    return (ERROR);
  }

  // link file to png_ptr
  png_init_io(png_ptr, png);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // read whole png image into file, automatically allocating nessecary memory
  //(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  // get image data
  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth   = png_get_bit_depth(png_ptr, info_ptr);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png_ptr);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  if (row_pointers) return ERROR;

  // allocate and read image pixels
  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y=0; y<height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));
  }
  png_read_image(png_ptr, row_pointers);

  // close file stream, not needed anymore
  fclose(png);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
}

int sort_image() {
  // do nothing
}

int save_image(char *filename) {
  int y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) return ERROR;

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) return ERROR;

  png_infop info = png_create_info_struct(png);
  if (!info) return ERROR;

  if (setjmp(png_jmpbuf(png))) return ERROR;

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  if (!row_pointers) return ERROR;

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);

  png_destroy_write_struct(&png, &info);
}

int main() {
  // open png image as file
  FILE *img;
  int err = read_png("C:\\Users\\traum\\Documents\\Code\\C\\Pixelsort\\jinping.png", img);
  if (err<0) return EXIT_FAILURE;

  // allocate and initialize png_struct and png_info
  err = initialize_png(img);
  if (err<0) {
    printf("Could not initialize image data");
    return EXIT_FAILURE;
  }

  // edit/sort image (in row_pointers)
  err = sort_image();
  if (err<0) {
    printf("Could not sort image data");
    return EXIT_FAILURE;
  }

  // write png to a new file and save it to disk
  err = save_image("C:\\Users\\traum\\Documents\\Code\\C\\Pixelsort\\sorted.png");
  if (err<0) {
    printf("Could not save image data");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}