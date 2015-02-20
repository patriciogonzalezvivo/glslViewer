#pragma once

#include <string>
#include <stdio.h>
#include <iostream>

#include <FreeImage.h>      //  FreeImage type is GL_BGR
#include "gl.h"

// static variable for freeImage initialization:
void InitFreeImage(bool deinit=false){
    // need a new bool to avoid c++ "deinitialization order fiasco":
    // http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.15
    static bool * bFreeImageInited = new bool(false);
    if(!*bFreeImageInited && !deinit){
        FreeImage_Initialise();
        *bFreeImageInited = true;
    }
    if(*bFreeImageInited && deinit){
        FreeImage_DeInitialise();
    }
}

FIBITMAP* loadImage(std::string _path){
    InitFreeImage();

    //  From http://www.mbsoftworks.sk/index.php?page=tutorials&series=1&tutorial=9
    //
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN; 
    FIBITMAP* dib(0); 
    fif = FreeImage_GetFileType(_path.c_str(), 0); // Check the file signature and deduce its format

    if(fif == FIF_UNKNOWN) // If still unknown, try to guess the file format from the file extension
        fif = FreeImage_GetFIFFromFilename(_path.c_str()); 

    if(fif == FIF_UNKNOWN){
        // If still unkown, return failure
        std::cerr << "Texture: Unknown type" << std::endl; 
        return NULL;
    }

    if(FreeImage_FIFSupportsReading(fif)) // Check if the plugin has reading capabilities and load the file
      dib = FreeImage_Load(fif, _path.c_str()); 
    if(!dib){
        std::cerr << "Texture: unhable to open" << std::endl;
        return NULL;
    }

    FIBITMAP *image = FreeImage_ConvertTo32Bits(dib);
    FreeImage_Unload(dib);

    return image;
}

void saveImage(const std::string& _path, unsigned char * _pixels, int _width, int _height, bool _dither = false) {
    InitFreeImage();

    FREE_IMAGE_FORMAT fif = FIF_PNG;//FreeImage_GetFileType(_path.c_str());
    // if (fif != FIF_UNKNOWN)
    {
        int bpp = 32;
        int pitch = ((((bpp * _width) + 31) / 32) * 4);
        
        FIBITMAP* bmp = FreeImage_ConvertFromRawBits(_pixels, _width, _height, pitch, bpp, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK, false);
        
        if(_dither){
            FIBITMAP* ditherImg = FreeImage_Dither(bmp,FID_FS);
            FreeImage_Save(fif,ditherImg,_path.c_str());
            if (ditherImg != NULL){
                FreeImage_Unload(ditherImg);
            }  
        } else {
            FreeImage_Save(fif,bmp,_path.c_str());
        }

        if (bmp != NULL){
            FreeImage_Unload(bmp);
        }
    }

    std::cout << "Saving image to " << _path << std::endl;
}