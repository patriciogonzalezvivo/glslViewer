/*
Copyright (c) 2014, Patricio Gonzalez Vivo
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "glm/glm.hpp"
#include "types/mesh.h"
#include "types/rectangle.h"

Mesh line (const glm::vec3 &_a, const glm::vec3 &_b);
void drawLine(const glm::vec3 &_a, const glm::vec3 &_b);

Mesh cross(const glm::vec3 &_pos, float _width);
void drawCross(const glm::vec3 &_pos, float _width = 3.5);

Mesh rect (float _x, float _y, float _w, float _h);
Mesh rect (const Rectangle &_rect);

Mesh rectBorders(const Rectangle &_rect);
void drawBorders( const Rectangle &_rect );

Mesh rectCorners(const Rectangle &_rect, float _width = 4.);
void drawCorners(const Rectangle &_rect, float _width = 4.);

Mesh polyline (const std::vector<glm::vec3> &_pts );
void drawPolyline(const std::vector<glm::vec3> &_pts );