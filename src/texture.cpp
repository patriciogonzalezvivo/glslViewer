#include "texture.h"

#include "utils.h"
#include "omx_image.h"

Texture::Texture():m_id(0),m_width(0),m_height(0){
}

Texture::~Texture(){

}

bool Texture::load(const std::string& _path){
	std::string extension = getExtention(_path);
	int type = 0;
    if(extension == ".png" || extension == ".PNG") {
    	type = 1;
    }

    doctor_chompers_t buffer;
   	omx_image_fill_buffer_from_disk(_path.c_str(), &buffer);
   	glGenTextures(1, &m_id);
   	omx_image_loader(m_id, &state->display, &state->context, 0, 0, 0, 0, type, &buffer);
   	free(buffer.data);
}

void Texture::bind(){

}

void Texture::unbind(){

}