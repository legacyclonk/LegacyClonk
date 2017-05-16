#include "png.h"
#include <iostream>

// LoadPNG_A
/*! routine for loading png files, can't load paletted png files
// \return	data:	pointer to data to read
// \return	width:	image width
// \return	height:	image height
// \param	bands:	bands of image
// \return	0: fail,
// \return	1: success */
// 
void PNGAPI user_read_data_PNG(png_structp png_ptr, png_bytep data, png_size_t length)
{
	fread(data,1,length,(FILE*)png_ptr->io_ptr);	
}

int loadPng(const char *strFilename, 
						unsigned char*& data, 
						int& width, int& height, 
						const int bands, const bool swaprgb)
{
	FILE *pFile;

	// check # bands >>>>>>>>>>>>>>>>>>>>>>>>>>>>
	if ((bands != 1) & (bands != 3) & (bands != 4))
	{
#ifdef DEBUG_PNG
			cout << "LoadPNG: error: illegal # of bands (neither 1, 3 or 4)" << endl;
#endif
		return 0;
	}
	// check # bands <<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// get filepointer if needed >>>>>>>>>>>>>>>>
	if (!(pFile = fopen(strFilename,"rb")))
	{
#ifdef DEBUG_PNG
			cout << "LoadPNG: error: can't open file" << endl;
#endif
		return 0;
	}

	unsigned char buf[8];		// 8 bytes to check
	fread(buf, 1, 8, pFile);

	if (png_sig_cmp(buf, (png_size_t)0, 8))	// png_sig_cmp returns !=0 if no png
	{
	#ifdef DEBUG_PNG
			cout << "LoadPNG: error: no png-file" << endl;
	#endif
			fclose(pFile);
			return 0;	// no png file
	};

	// get fp and check if it points to an png file <<

	// create png_structp >>>>>>>>>>>>>>>>>>>>>>>
	png_structp	png_read_ptr = 0;
	png_read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (png_read_ptr == NULL)
	{
#ifdef DEBUG_PNG
		cout << "LoadPNG: error: can't create png_structp" << endl;
#endif
		fclose(pFile);
		return 0;
	};
	// create png_structp <<<<<<<<<<<<<<<<<<<<<<<

	// tell pnglib we have checked the first >>>>
	// 8 byte of the file to verify filetype >>>>

	png_set_sig_bytes(png_read_ptr, 8);
	// using mempointer, set custom io fct. >>>>>

	png_set_read_fn(png_read_ptr, pFile, user_read_data_PNG);
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// create png_infop >>>>>>>>>>>>>>>>>>>>>>>>>
	png_infop	info_ptr = 0; 
	info_ptr = png_create_info_struct(png_read_ptr);
	if (info_ptr == NULL)
	{
#ifdef DEBUG_PNG
		cout << "LoadPNG: error: can't create png_infop" << endl;
#endif
		fclose(pFile);
		png_destroy_read_struct(&png_read_ptr, (png_infopp)0, (png_infopp)0);
		return 0;
	};
	// create png_infop <<<<<<<<<<<<<<<<<<<<<<<<<


	// png_write_ptr and info_ptr are initialised
	// now we can declare the png error handler >
	if (setjmp(png_jmpbuf(png_read_ptr)))
	{
#ifdef DEBUG_PNG
		cout << "LoadPNG: error: unknown error (libpng aborted)" << endl;
#endif
		png_destroy_read_struct(&png_read_ptr, &info_ptr, (png_infopp)0);
		fclose(pFile);
		return 0;
	};
	// png error handler <<<<<<<<<<<<<<<<<<<<<<<<
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// read the fileheader >>>>>>>>>>>>>>>>>>>>>>
	png_read_info(png_read_ptr, info_ptr);

	png_uint_32	widthL;
	png_uint_32	heightL;
	int			bit_depth;
	int			color_type;
	//int			interlace_type;

	png_get_IHDR(png_read_ptr,
				info_ptr,
				&widthL,
				&heightL,
				&bit_depth,
				&color_type,
				0, //&interlace_type,
				0,
				0);
	// fileheader <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// prepare the data >>>>>>>>>>>>>>>>>>>>>>>>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


	// get the # of bytes per row needed to store
	// the image >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	png_uint_32	bprow = png_get_rowbytes(png_read_ptr, info_ptr);	// pnglibs call png_get_rowbytes doesn't care
																	// if we change the imageformat using png_set_filler,
																	// png_set_rgb_to_gray or anything similar. 
																	// so we have to get the bytes per row here and
																	// apply any changes to the # of bytes per row manualy

	// tell libpng to strip 16 bit/color files >>
	// down to 8 bits/color >>>>>>>>>>>>>>>>>>>>>
	if (bit_depth == 16)
	{
		png_set_strip_16(png_read_ptr);
		bit_depth = 8;
	};

	// Extract multiple pixels with bit depths of 1, 2, and 4
	// from a single byte into separate bytes >>>
	png_set_packing(png_read_ptr);

	// Expand paletted colors into true >>>>>>>>>
	// RGB triplets >>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_read_ptr);
		//png_set_palette_to_rgb(png_read_ptr);

	// Expand grayscale images to the full 8 bits
	// from 1, 2, or 4 bits/pixel >>>>>>>>>>>>>>>
	if (((color_type == PNG_COLOR_TYPE_GRAY) ||
		 (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) &&
		 (bit_depth < 8))
	    png_set_expand(png_read_ptr);
		//png_set_gray_1_2_4_to_8(png_read_ptr);

	// Expand paletted or RGB images with >>>>>>>
	// transparency to full alpha channels >>>>>>
	if (png_get_valid(png_read_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_read_ptr);
		//png_set_tRNS_to_alpha(png_read_ptr);

	// flip the RGB pixels to BGR (or RGBA to BGRA)
	if (/*(color_type & PNG_COLOR_TYPE_RGB) &&*/ swaprgb)
		png_set_bgr(png_read_ptr);

	// rgb -> greyscale >>>>>>>>>>>>>>>>>>>>>>>>>
	if ((color_type & PNG_COLOR_TYPE_RGB) && (bands == 1))
	{
		if (color_type & PNG_COLOR_MASK_ALPHA)
		{
			png_set_strip_alpha(png_read_ptr);
			bprow = bprow * 3 / 4;	// remove alpha
		};
		png_set_rgb_to_gray(png_read_ptr, 1, 0.299, 0.587);	
		bprow /= 3;					// switch to 1 band
	};
	
	// greyscale -> rgb >>>>>>>>>>>>>>>>>>>>>>>>>
	if ((color_type == PNG_COLOR_TYPE_GRAY) && (bands > 1))
	{
		png_set_gray_to_rgb(png_read_ptr);
		bprow *= 3;					// expand to rgb
		color_type = PNG_COLOR_TYPE_RGB;
	};
	
	// if we try to read a 3 band image into an >
	// Image4X add an alpha chanel >>>>>>>>>>>>>>
	if ((color_type == PNG_COLOR_TYPE_RGB) && (bands == 4))
	{
		png_set_filler(png_read_ptr, 0x0, PNG_FILLER_AFTER);
		bprow = bprow * 4 / 3;	// we have just added an alpha channel
	};

	// if we try to read a 4 band image into an >
	// Image3X strip the alpha chanel >>>>>>>>>>>
  if ((color_type == PNG_COLOR_TYPE_RGB_ALPHA) && (bands == 3))
	{
		png_set_strip_alpha(png_read_ptr);
		bprow = bprow * 3 / 4;	// we have just removed the alpha channel
	};

	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	// prepare the data <<<<<<<<<<<<<<<<<<<<<<<<<
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


	// allocate memory to store the image >>>>>>>
	// bprow holds now the # of bytes per row >>>
	// needed to store in the format we want >>>>
	png_bytep*	row_pointers = new png_bytep[heightL];	// png_bytep is simple unsigned char*
	png_uint_32 i;
	for (i = 0; i < heightL; i++)
	{
		row_pointers[i] = new unsigned char[bprow];
	};
	// allcoate memory <<<<<<<<<<<<<<<<<<<<<<<<<<	
	
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// read the image >>>>>>>>>>>>>>>>>>>>>>>>>>>
	png_read_image(png_read_ptr, row_pointers);
	// read rest of file >>>>>>>>>>>>>>>>>>>>>>>>
	png_read_end(png_read_ptr, info_ptr);
	// file read <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


	// clean up after read, and free any memory allocated
	png_destroy_read_struct(&png_read_ptr, &info_ptr, (png_infopp)0);

	// close the file
	fclose(pFile);
	
	// now we have the rowpointers that hold the image
	// and can fill the retunr values >>>>>>>>>>>
	width	= widthL;
	height	= heightL;

	/*if (!data)*/
	data	= new unsigned char[width * height * bands];

	png_uint_32 j;
	for (i = 0; i < (png_uint_32) height; i++)
	{
		//for (j = 0; j < bprow; j++) ...was this - bprow might be larger than width*bands ?!
		for (j = 0; j < (png_uint_32) (width*bands); j++)
		{
			data[i * width * bands + j] = row_pointers[height-1-i][j];
		}
	}
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// get rid of the row pointers >>>>>>>>>>>>>>
	for (i = 0; i < (png_uint_32) height; i++)
	{
		delete row_pointers[i];
	}
	delete	row_pointers;
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	return 1;
};








int getPngInfo(const char *strFilename, 
							 int *ipWidth, int *ipHeight, 
							 int *ipBands)
{
	FILE *pFile;
	// get filepointer if needed >>>>>>>>>>>>>>>>
	if (!(pFile = fopen(strFilename,"rb")))
	{
#ifdef DEBUG_PNG
			cout << "LoadPNG: error: can't open file" << endl;
#endif
		return 0;
	}

	unsigned char buf[8];		// 8 bytes to check
	fread(buf, 1, 8, pFile);

	if (png_sig_cmp(buf, (png_size_t)0, 8))	// png_sig_cmp returns !=0 if no png
	{
	#ifdef DEBUG_PNG
			cout << "LoadPNG: error: no png-file" << endl;
	#endif
			fclose(pFile);
			return 0;	// no png file
	};

	// get fp and check if it points to an png file <<

	// create png_structp >>>>>>>>>>>>>>>>>>>>>>>
	png_structp	png_read_ptr = 0;
	png_read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (png_read_ptr == NULL)
	{
#ifdef DEBUG_PNG
		cout << "LoadPNG: error: can't create png_structp" << endl;
#endif
		fclose(pFile);
		return 0;
	};
	// create png_structp <<<<<<<<<<<<<<<<<<<<<<<

	// tell pnglib we have checked the first >>>>
	// 8 byte of the file to verify filetype >>>>

	png_set_sig_bytes(png_read_ptr, 8);
	// using mempointer, set custom io fct. >>>>>

	png_set_read_fn(png_read_ptr, pFile, user_read_data_PNG);
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// create png_infop >>>>>>>>>>>>>>>>>>>>>>>>>
	png_infop	info_ptr = 0; 
	info_ptr = png_create_info_struct(png_read_ptr);
	if (info_ptr == NULL)
	{
#ifdef DEBUG_PNG
		cout << "LoadPNG: error: can't create png_infop" << endl;
#endif
		fclose(pFile);
		png_destroy_read_struct(&png_read_ptr, (png_infopp)0, (png_infopp)0);
		return 0;
	};
	// create png_infop <<<<<<<<<<<<<<<<<<<<<<<<<


	// png_write_ptr and info_ptr are initialised
	// now we can declare the png error handler >
	if (setjmp(png_jmpbuf(png_read_ptr)))
	{
#ifdef DEBUG_PNG
		cout << "LoadPNG: error: unknown error (libpng aborted)" << endl;
#endif
		png_destroy_read_struct(&png_read_ptr, &info_ptr, (png_infopp)0);
		fclose(pFile);
		return 0;
	};
	// png error handler <<<<<<<<<<<<<<<<<<<<<<<<
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// read the fileheader >>>>>>>>>>>>>>>>>>>>>>
	png_read_info(png_read_ptr, info_ptr);

	png_uint_32	widthL;
	png_uint_32	heightL;
	int			bit_depth;
	int			color_type;
	//int			interlace_type;

	png_get_IHDR(png_read_ptr,
				info_ptr,
				&widthL,
				&heightL,
				&bit_depth,
				&color_type,
				0, //&interlace_type,
				0,
				0);
	// fileheader <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	
	// close the file
	fclose(pFile);

	// Return info
	if (ipWidth) *ipWidth	= widthL;
	if (ipHeight) *ipHeight	= heightL;

	// Don't really know what 'bit_depth' is (8 or 16?)
	// Calculate bands from color type
	if (ipBands)
		switch (color_type)
		{
			case PNG_COLOR_TYPE_GRAY: *ipBands = 1;	break;
			case PNG_COLOR_TYPE_RGB: *ipBands = 3;	break;
			case PNG_COLOR_TYPE_RGB_ALPHA: *ipBands = 4;	break;
			case PNG_COLOR_TYPE_GRAY_ALPHA: *ipBands = 2;	break; // ?
			default: *ipBands=0; break; // unknown ?!
		}
		
	return true;
};
