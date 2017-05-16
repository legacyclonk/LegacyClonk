#include "png.h"
#include "common.h"

int  complevel = 6;
int  interlace = 0;

static int write_png( char *fn, FILE *fp, IMAGE *img )
{
	png_structp png_ptr;
	png_infop info_ptr;
	int bit_depth;
	int color_type;
	int interlace_type;

	//set_status( "Writing %s", bname(fn) );

	/* ------------------------------------------------------ */

	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, fn, png_my_error, png_my_warning );
	if( png_ptr==NULL ){
		//xprintf( err_outofmemory, fn );
		return 0;
	}
	info_ptr = png_create_info_struct( png_ptr );
	if( info_ptr==NULL ){
		png_destroy_write_struct( &png_ptr, NULL );
		//xprintf( err_outofmemory, fn );
		return 0;
	}
	if( setjmp(png_ptr->jmpbuf) ){
		/* If we get here, we had a problem reading the file */
		png_destroy_write_struct( &png_ptr, &info_ptr );
		return 0;
	}
	png_init_io( png_ptr, fp );
	png_set_compression_level( png_ptr, complevel );
/*	png_set_compression_mem_level( png_ptr, MAX_MEM_LEVEL );*/

	/* ------------------------------------------------------ */

	if (img->pixdepth==24)
	{
		bit_depth  = 8;
		color_type = PNG_COLOR_TYPE_RGB;
	}
	else if (img->pixdepth==32)
	{
		bit_depth  = 8; // ...correct for RGBA?!
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	}
	else
	{
		bit_depth  = img->pixdepth;
		color_type = PNG_COLOR_TYPE_PALETTE;
		png_set_PLTE( png_ptr, info_ptr, img->palette, img->palnum );
	}

	interlace_type = (interlace)? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE ;

	png_set_IHDR( png_ptr, info_ptr, img->width, img->height, bit_depth,
	              color_type, interlace_type, PNG_COMPRESSION_TYPE_DEFAULT,
	              PNG_FILTER_TYPE_DEFAULT );

	png_write_info( png_ptr, info_ptr );

	/* ------------------------------------------------------ */

	if ((img->pixdepth==24) || (img->pixdepth==32))
		png_set_bgr(png_ptr);

	/* ------------------------------------------------------ */

	png_set_write_status_fn( png_ptr, row_callback );
	init_row_callback( png_ptr, img->width, img->height );

	png_write_image( png_ptr, img->rowptr );

	png_write_end( png_ptr, info_ptr );
	png_destroy_write_struct( &png_ptr, &info_ptr );

	/* ------------------------------------------------------ */

	//set_status( "OK      %s", bname(fn) );
	feed_line();

	return 1;
}


int SavePNG(const char *strFilename, unsigned char* data, int width, int height, const int bands)
{	
	// Create and fill image structure
	IMAGE img;

	img.width = width;
	img.height = height;
	img.pixdepth = bands*8;
	img.palnum = 0;
	img.topdown = FALSE;

	imgalloc(&img);

	memcpy( img.bmpbits, data, width*height*bands );


	// Write file
	FILE *fp;
	int iResult = 0;
	if (fp = fopen( strFilename, "wb" ))
	{
		iResult = write_png((char*)strFilename, fp, &img);
		fclose( fp );
	}

	// Destroy image structure
	imgfree(&img);

	// Done
	return iResult;
}

