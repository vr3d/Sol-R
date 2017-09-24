/* Copyright (c) 2011-2017, Cyrille Favreau
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 * This file is part of Sol-R <https://github.com/cyrillefavreau/Sol-R>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
IMPORTANT: This code is based on the Hypercube dephi code by cs_Forman
http://codes-sources.commentcamarche.net/source/33735-hypercubes
*/

#include "HypercubeScene.h"

#include <opengl/rtgl.h>

#ifdef WIN32
#include <windows.h>
#else
#include <math.h>
#endif

using namespace solr;

const float animationAngles[MaxDimensions] = {3.3f, 5.6f, 6.4f, 2.7f, 3.2f, 7.4f, 8.2f, 6.5f, 3.1f, 4.f};

HypercubeScene::HypercubeScene(const std::string& name)
    : Scene(name)
    , _primitiveIndex(0)
{
}

HypercubeScene::~HypercubeScene(void)
{
}

float HypercubeScene::powerOfTwo(const size_t n)
{
    return round(exp(log(2.f) * n));
}

void HypercubeScene::makeMatrix()
{
    const float ratio = 2.f * static_cast<float>(M_PI) / 3000.f;
    _matrix = identityMatrix();
    _matrix = multiplyMatrices(_matrix, makeRotationMatrix(1, 2, _angles[0] * ratio));
    _matrix = multiplyMatrices(_matrix, makeRotationMatrix(2, 0, _angles[1] * ratio));
    _matrix = multiplyMatrices(_matrix, makeRotationMatrix(0, 1, _angles[2] * ratio));
    for (size_t a = 0; a < 3 * (_dimension - 3); ++a)
        _matrix = multiplyMatrices(_matrix, makeRotationMatrix((a % 3), (a / 3) + 3, _angles[a + 3] * ratio));
}

Matrix HypercubeScene::makeRotationMatrix(const size_t ID, const size_t IDD, const float angle)
{
    Matrix matrix = identityMatrix();
    matrix.values[ID][ID] = cos(angle);
    matrix.values[IDD][IDD] = matrix.values[ID][ID];
    matrix.values[IDD][ID] = sin(angle);
    matrix.values[ID][IDD] = -matrix.values[IDD][ID];
    return matrix;
}

Matrix HypercubeScene::identityMatrix()
{
    Matrix matrix;
    memset(&matrix, 0, sizeof(Matrix));
    for (size_t a = 0; a < MaxDimensions; ++a)
        matrix.values[a][a] = 1.f;
    return matrix;
}

Matrix HypercubeScene::multiplyMatrices(const Matrix& matrix1, const Matrix& matrix2)
{
    Matrix matrix;
    for (size_t a = 0; a < MaxDimensions; ++a)
        for (size_t b = 0; b < MaxDimensions; ++b)
        {
            float s = 0.f;
            for (size_t c = 0; c < MaxDimensions; ++c)
                s += matrix1.values[a][c] * matrix2.values[c][b];
            matrix.values[a][b] = s;
        }
    return matrix;
}

Vector HypercubeScene::cross(const Vector& u, const Vector& v)
{
    Vector vector(3, 0);
    vector[0] = u[1] * v[2] - u[2] * v[1];
    vector[1] = u[2] * v[0] - u[0] * v[2];
    vector[2] = u[0] * v[1] - u[1] * v[0];
    return vector;
}

void HypercubeScene::makeSubFaces(const size_t a, const size_t b, size_t& c, const size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        const size_t index = c + 4 * i;
        _vertices[index + 0].resize(MaxDimensions, 0.f);
        _vertices[index + 1].resize(MaxDimensions, 0.f);
        _vertices[index + 2].resize(MaxDimensions, 0.f);
        _vertices[index + 3].resize(MaxDimensions, 0.f);
        float v;
        size_t u = 1;
        for (size_t j = 0; j < _dimension; ++j)
        {
            if ((j == a) || (j == b))
                continue;
            if ((u & i) == u)
                v = 1.f;
            else
                v = -1.f;
            _vertices[index + 0][j] = v;
            _vertices[index + 1][j] = v;
            _vertices[index + 2][j] = v;
            _vertices[index + 3][j] = v;
            u *= 2;
        }
        _vertices[index + 0][a] = 1.f;
        _vertices[index + 1][a] = 1.f;
        _vertices[index + 2][a] = -1.f;
        _vertices[index + 3][a] = -1.f;
        _vertices[index + 0][b] = 1.f;
        _vertices[index + 1][b] = -1.f;
        _vertices[index + 2][b] = -1.f;
        _vertices[index + 3][b] = 1.f;
    }

    c += 4 * n;
}

void HypercubeScene::createGeometry()
{
    _dimension = 4 + m_currentModel % (MaxDimensions - 3);
    LOG_INFO(2, "Geometry dimension: " << _dimension);

    const size_t nbVertices = (_dimension - 1) * _dimension * powerOfTwo(_dimension - 1);
    LOG_INFO(2, "Number of faces   : " << nbVertices / 4);

    const float transparency = 0.5f;
    const float reflection = 0.f;
    const float refraction = 1.f;
    for (unsigned int i = 0; i < (nbVertices + 4); ++i)
        m_gpuKernel->setMaterial(i, 0.5f + 0.5f * cos(i), 0.5f, 0.5f + 0.5f * cos(i) * sin(i), 0.f, reflection,
                                 refraction, false, false, 0, transparency, 0.f, TEXTURE_NONE, TEXTURE_NONE,
                                 TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE, TEXTURE_NONE, 0.f, 100.f, 0.f,
                                 0.f, 10.f, 10000.f, false);

    _vertices.clear();
    size_t c = 0;
    const size_t n = powerOfTwo(_dimension - 2);
    for (size_t a = 0; a < _dimension; ++a)
        for (size_t b = a + 1; b < _dimension + 1; ++b)
            makeSubFaces(a, b, c, n);
    makeMatrix();
}

Vector HypercubeScene::multiplyVector(const Matrix& m, const Vector& v)
{
    Vector vector(MaxDimensions, 0.f);
    for (size_t a = 0; a < MaxDimensions; ++a)
    {
        float s = 0.f;
        for (size_t b = 0; b < MaxDimensions; ++b)
            s += m.values[a][b] * v[b];
        vector[a] = s;
    }
    return vector;
}

Vector HypercubeScene::subtractVector(const Vector& u, const Vector& v)
{
    Vector vector(MaxDimensions, 0.f);
    for (size_t a = 0; a < MaxDimensions; ++a)
        vector[a] = u[a] - v[a];
    return vector;
}

void HypercubeScene::drawFace(const size_t Id)
{
    Vector t = multiplyVector(_matrix, _vertices[Id + 0]);
    Vector u = multiplyVector(_matrix, _vertices[Id + 1]);
    Vector v = multiplyVector(_matrix, _vertices[Id + 2]);
    Vector w = multiplyVector(_matrix, _vertices[Id + 3]);

    const float scale = 1000.f;
    for (size_t i = 0; i < MaxDimensions; ++i)
    {
        t[i] *= scale;
        u[i] *= scale;
        v[i] *= scale;
        w[i] *= scale;
    }

    const Vector n = cross(subtractVector(t, v), subtractVector(u, w));
    vec3f normal = make_vec3f(n[0], n[1], n[2]);

    m_gpuKernel->setCurrentMaterial(Id);
    glBegin(GL_TRIANGLES);
    glVertex3f(t[0], t[1], t[2]);
    glVertex3f(u[0], u[1], u[2]);
    glVertex3f(v[0], v[1], v[2]);
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex3f(v[0], v[1], v[2]);
    glVertex3f(w[0], w[1], w[2]);
    glVertex3f(t[0], t[1], t[2]);
    glEnd();

    const float radius = scale / 50.f;
    _primitiveIndex = m_gpuKernel->addPrimitive(ptCylinder);
    m_gpuKernel->setPrimitive(_primitiveIndex, t[0], t[1], t[2], u[0], u[1], u[2], radius, 0.f, 0.f, WHITE_MATERIAL);
    _primitiveIndex = m_gpuKernel->addPrimitive(ptCylinder);
    m_gpuKernel->setPrimitive(_primitiveIndex, u[0], u[1], u[2], v[0], v[1], v[2], radius, 0.f, 0.f, WHITE_MATERIAL);
    _primitiveIndex = m_gpuKernel->addPrimitive(ptCylinder);
    m_gpuKernel->setPrimitive(_primitiveIndex, v[0], v[1], v[2], w[0], w[1], w[2], radius, 0.f, 0.f, WHITE_MATERIAL);
    _primitiveIndex = m_gpuKernel->addPrimitive(ptCylinder);
    m_gpuKernel->setPrimitive(_primitiveIndex, w[0], w[1], w[2], t[0], t[1], t[2], radius, 0.f, 0.f, WHITE_MATERIAL);
}

void HypercubeScene::drawFaces()
{
    for (size_t a = 0; a < (_vertices.size() + 1) / 4; ++a)
        drawFace(4 * a);
}

void HypercubeScene::doInitialize()
{
    _angles.resize(MaxDimensions, 0.f);
    for (size_t i = 0; i < MaxDimensions; ++i)
        _angles[i] = animationAngles[i];
    createGeometry();
    drawFaces();
}

void HypercubeScene::doAnimate()
{
    for (size_t i = 0; i < MaxDimensions; ++i)
        _angles[i] += animationAngles[i];

    _primitiveIndex = 0;
    m_gpuKernel->resetFrame();
    createGeometry();
    drawFaces();
    doAddLights();
    addCornellBox(m_cornellBoxType);
    m_gpuKernel->compactBoxes(true);
}

void HypercubeScene::doAddLights()
{
    // lights
    m_nbPrimitives = m_gpuKernel->addPrimitive(ptSphere);
    m_gpuKernel->setPrimitive(m_nbPrimitives, 0.f, 10000.f, -10000.f, 1000.f, 0.f, 0, DEFAULT_LIGHT_MATERIAL);
    m_gpuKernel->setPrimitiveIsMovable(m_nbPrimitives, false);
}
