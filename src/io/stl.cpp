
#include "stl.h"

#include <iostream>
#include <fstream>
#include <string>

#include "../tools/geom.h"
#include "../tools/text.h"

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

bool loadSTL(WatchFileList& _files, Materials& _materials, Models& _models, int _index, bool _verbose) {
    std::string filename = _files[_index].path;
    std::string name = filename.substr(0, filename.size()-4);

    Mesh mesh;
    Material default_material;
    _materials[default_material.name] = default_material;

    FILE * stl_file = fopen(filename.c_str(),"rb");
    if (NULL == stl_file) {
        fprintf(stderr,"IOError: %s could not be opened...\n", filename.c_str());
        return false;
    }

     // Specifically 80 character header
    char header[80];
    char solid[80] = {0};
    bool is_ascii = true;
    if (fread(header, 1, 80, stl_file) != 80) {
        std::cerr << "IOError: too short (1)." << std::endl;
        goto close_false;
    }
    sscanf(header, "%79s", solid);
  
    if ( std::string("solid") != solid) 
        is_ascii = false;
    else {
        // might still be binary
        char buf[4];
        if ( fread(buf, 1, 4, stl_file) != 4) {
            std::cerr << "IOError: too short (3)." << std::endl;
            goto close_false;
        }
    
        size_t num_faces = *reinterpret_cast<unsigned int*>(buf);
        fseek(stl_file,0,SEEK_END);
        size_t file_size = ftell(stl_file);
        if (file_size == 80 + 4 + (4*12 + 2) * num_faces) is_ascii = false;
        else is_ascii = true;
    }

    if (is_ascii) {
        // Rewind to end of header
        //stl_file = fopen(filename.c_str(),"r");
        //stl_file = freopen(NULL,"r",stl_file);
        fseek(stl_file, 0, SEEK_SET);
        if (NULL == stl_file) {
            fprintf(stderr,"IOError: stl file could not be reopened as ascii ...\n");
            return false;
        }
        // Read 80 header
        // Eat file name

        char name[LINE_MAX];
        if ( NULL == fgets(name, LINE_MAX, stl_file)) {
            std::cerr << "IOError: ascii too short (2)." << std::endl;
            goto close_false;
        }

        // ascii
        while(true) {
            int ret;
            char facet[LINE_MAX];
            char normal[LINE_MAX];

            glm::vec3 n;

            double nd[3];
            ret = fscanf(stl_file,"%s %s %lg %lg %lg", facet, normal, nd, nd+1, nd+2);
            n.x = nd[0];
            n.y = nd[2];
            n.z = -nd[1];

            if (std::string("endsolid") == facet)
                break;
            
            if (ret != 5 || 
                !(  std::string("facet") == facet || 
                    std::string("faced") == facet) ||
                    std::string("normal") != normal) {
                std::cout << "facet: " << facet << std::endl;
                std::cout << "normal: " <<normal << std::endl;
                std::cerr << "IOError: bad format (1)." << std::endl;
                goto close_false;
            }
            // copy casts to Type
            n.x = nd[0]; 
            n.y = nd[2];
            n.z = -nd[1];

            mesh.addNormal(n);
            mesh.addNormal(n);
            mesh.addNormal(n);
            char outer[LINE_MAX], loop[LINE_MAX];
            ret = fscanf(stl_file,"%s %s",outer,loop);
            if (ret != 2 || std::string("outer") != outer || std::string("loop") != loop) {
                std::cerr << "IOError: bad format (2). " << std::endl;
                goto close_false;
            }
            
            while(true) {
                char word[LINE_MAX];
                int ret = fscanf(stl_file, "%s", word);
                if (ret == 1 && std::string("endloop") == word)
                    break;
                else if (ret == 1 && std::string("vertex") == word) {
                    glm::vec3 v;
                    double vd[3];
                    int ret = fscanf(stl_file, "%lg %lg %lg", vd, vd+1, vd+2);
                    if (ret != 3) {
                        std::cerr << "IOError: bad format (3)." << std::endl;
                        goto close_false;
                    }
                    mesh.addVertex(glm::vec3(vd[0], vd[2], -vd[1]));
                }
                else {
                    std::cerr << "IOError: bad format (4)." << std::endl;
                    goto close_false;
                }
            }

            char endfacet[LINE_MAX];
            ret = fscanf(stl_file,"%s",endfacet);
            if (ret != 1 || std::string("endfacet") != endfacet) {
                std::cerr << "IOError: bad format (5)." << std::endl;
                goto close_false;
            }
        }
        // read endfacet
        goto close_true;
    }
    else {
        // Binary
        // stl_file = freopen(NULL,"rb",stl_file);
        fseek(stl_file, 0, SEEK_SET);
        // Read 80 header
        char header[80];
        if (fread(header, sizeof(char), 80, stl_file) != 80) {
            std::cerr << "IOError: bad format (6)." << std::endl;
            goto close_false;
        }
        // Read number of triangles
        unsigned int num_tri;
        if (fread(&num_tri, sizeof(unsigned int), 1, stl_file) != 1) {
            std::cerr << "IOError: bad format (7)." << std::endl;
            goto close_false;
        }

        for(int t = 0; t < (int)num_tri; t++) {
            // Read normal
            glm::vec3 n;
            if (fread(&n.x, sizeof(float), 3, stl_file) != 3) {
                std::cerr << "IOError: bad format (8)." << std::endl;
                goto close_false;
            }
            mesh.addNormal(glm::vec3(n.x, n.z, -n.y));
            mesh.addNormal(glm::vec3(n.x, n.z, -n.y));
            mesh.addNormal(glm::vec3(n.x, n.z, -n.y));

            // Read each vertex
            for (size_t c = 0; c < 3; c++) {
                glm::vec3 v;
                if (fread (&v.x, sizeof(float), 3, stl_file) != 3) {
                    std::cerr << "IOError: bad format (9)." << std::endl;
                    goto close_false;
                }
                mesh.addVertex(glm::vec3(v.x, v.z, -v.y));
            }
            
            // Read attribute size
            unsigned short att_count;
            if (fread(&att_count, sizeof(unsigned short), 1, stl_file) != 1) {
                std::cerr << "IOError: bad format (10)." << std::endl;
                goto close_false;
            }
        }
        goto close_true;
    }

    close_false:
        fclose(stl_file);
        return false;
    close_true:
        fclose(stl_file);
        _models.push_back( new Model(name, mesh, default_material) );
        return true;

}