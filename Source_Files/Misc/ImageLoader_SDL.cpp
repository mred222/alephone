/*
 *  ImageLoader_SDL.cpp - Image file loading, SDL implementation
 *
 *  Written in 2001 by Christian Bauer
 */

#include "ImageLoader.h"
#include "FileHandler.h"

#ifdef HAVE_SDL_IMAGE_H
#include <SDL_image.h>
#endif


/*
 *  Load specified image file
 */

bool LoadImageFromFile(ImageDescriptor& Img, FileSpecifier& File, int ImgMode)
{
	// Don't load opacity if there is no color component:
	switch(ImgMode) {
		case ImageLoader_Colors:
			break;
		
		case ImageLoader_Opacity:
			if (!Img.IsPresent())
				return false;
			break;
		
		default:
			vassert(false, csprintf(temporary,"Bad image mode for loader: %d",ImgMode));
	}

	// Load image to surface
#ifdef HAVE_SDL_IMAGE
	SDL_Surface *s = IMG_Load(File.GetPath());
#else
	SDL_Surface *s = SDL_LoadBMP(File.GetPath());
#endif
	if (s == NULL)
		return false;

	// Get image dimensions and set its size
	int Width = s->w, Height = s->h;
	switch (ImgMode) {
		case ImageLoader_Colors:
			Img.Resize(Width, Height);
			break;

		case ImageLoader_Opacity:
			// If the wrong size, then bug out
			if (Width != Img.GetWidth() || Height != Img.GetHeight()) {
				SDL_FreeSurface(s);
				return false;
			}
			break;
	}

	// Convert to 32-bit OpenGL-friendly RGBA surface
#ifdef ALEPHONE_LITTLE_ENDIAN
	SDL_Surface *rgba = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#else
	SDL_Surface *rgba = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#endif
	if (rgba == NULL) {
		SDL_FreeSurface(s);
		return false;
	}
	SDL_BlitSurface(s, NULL, rgba, NULL);
	SDL_FreeSurface(s);

	// Convert surface to RGBA texture
	switch (ImgMode) {
		case ImageLoader_Colors:
			memcpy(Img.GetPixelBasePtr(), rgba->pixels, Width * Height * 4);
			break;

		case ImageLoader_Opacity: {
			uint8 *p = (uint8 *)rgba->pixels;
			uint8 *q = (uint8 *)Img.GetPixelBasePtr();
			for (int h=0; h<Height; h++) {
				for (int w=0; w<Width; w++) {
					// RGB to greyscale value, and then to the opacity
					float Red = float(*p++);
					float Green = float(*p++);
					float Blue = float(*p++);
					p++;
					float Opacity = (Red + Green + Blue) / 3.0;
					q[3] = PIN(int(Opacity + 0.5), 0, 255);
					q += 4;
				}
			}
			break;
		}
	}

	SDL_FreeSurface(rgba);
	return true;
}
