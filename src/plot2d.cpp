/*******************************************************
 * Copyright (c) 2015-2019, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <fg/plot2d.h>
#include <fg/exception.h>
#include <common.hpp>
#include <err_common.hpp>

#include <math.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

namespace fg
{

Plot::Plot(GLuint pNumPoints, GLenum pDataType)
    : Chart(), mNumPoints(pNumPoints), mDataType(pDataType),
      mMainVAO(0), mMainVBO(0), mMainVBOsize(0)
{
    MakeContextCurrent();

    unsigned total_points = 2*mNumPoints;
    // buffersubdata calls on mMainVBO
    // will only update the points data
    switch(mDataType) {
        case GL_FLOAT:
            mMainVBO = createBuffer<float>(total_points, NULL, GL_DYNAMIC_DRAW);
            mMainVBOsize = total_points*sizeof(float);
            break;
        case GL_INT:
            mMainVBO = createBuffer<int>(total_points, NULL, GL_DYNAMIC_DRAW);
            mMainVBOsize = total_points*sizeof(int);
            break;
        case GL_UNSIGNED_INT:
            mMainVBO = createBuffer<unsigned>(total_points, NULL, GL_DYNAMIC_DRAW);
            mMainVBOsize = total_points*sizeof(unsigned);
            break;
        case GL_UNSIGNED_BYTE:
            mMainVBO = createBuffer<unsigned char>(total_points, NULL, GL_DYNAMIC_DRAW);
            mMainVBOsize = total_points*sizeof(unsigned char);
            break;
        default: fg::TypeError("Plot::Plot", __LINE__, 1, mDataType);
    }

    GLuint pointIndex = borderProgramPointIndex();

    //create vao
    glGenVertexArrays(1, &mMainVAO);
    glBindVertexArray(mMainVAO);
    // attach plot vertices
    glBindBuffer(GL_ARRAY_BUFFER, mMainVBO);
    glVertexAttribPointer(pointIndex, 2, mDataType, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pointIndex);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Plot::~Plot()
{
    MakeContextCurrent();
    glDeleteBuffers(1, &mMainVBO);
    glDeleteVertexArrays(1, &mMainVAO);
}

void Plot::setColor(float r, float g, float b)
{
    mLineColor[0] = clampTo01(r);
    mLineColor[1] = clampTo01(g);
    mLineColor[2] = clampTo01(b);
    mLineColor[3] = 1.0f;
}

GLuint Plot::vbo() const { return mMainVBO; }

size_t Plot::size() const { return mMainVBOsize; }

void Plot::render(int pVPW, int pVPH) const
{
    float graph_scale_x = 1/(xmax() - xmin());
    float graph_scale_y = 1/(ymax() - ymin());

    CheckGL("Begin Plot::render");
    /* Enavle scissor test to discard anything drawn beyond viewport.
     * Set scissor rectangle to clip fragments outside of viewport */
    glScissor(leftMargin()+tickSize(), bottomMargin()+tickSize(),
            pVPW - (leftMargin()+rightMargin()+tickSize()),
            pVPH - (bottomMargin()+topMargin()+tickSize()));
    glEnable(GL_SCISSOR_TEST);

    bindBorderProgram();
    glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(graph_scale_x, graph_scale_y, 1));
    glUniformMatrix4fv(borderMatIndex(), 1, GL_FALSE, glm::value_ptr(transform));
    glUniform4fv(borderColorIndex(), 1, mLineColor);

    /* render the plot data */
    glBindVertexArray(mMainVAO);
    glDrawArrays(GL_LINE_STRIP, 0, mNumPoints);
    glBindVertexArray(0);

    /* Stop clipping and reset viewport to window dimensions */
    glDisable(GL_SCISSOR_TEST);
    unbindBorderProgram();

    /* render graph border and axes */
    renderChart(pVPW, pVPH);

    CheckGL("End Plot::render");
}

}