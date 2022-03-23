#include "mirror.h"

#include <iostream>
using namespace std;

void mirror_point(glm::vec3 m_p, glm::vec3 normal, glm::vec3 p, glm::vec3 &out)
{
    normal = normalize(normal);
    float dist = dot(normal, m_p-p);
    if (dist < 0)
        dist = -dist;
    out = -normal * dist*2.0f + p;
}

//float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
//        // positions   // texCoords
//        -0.5f,  0.5f,  0.0f, 1.0f,
//        -0.5f, -0.5f,  0.0f, 0.0f,
//         0.5f, -0.5f,  1.0f, 0.0f,
//
//        -0.5f,  0.5f,  0.0f, 1.0f,
//         0.5f, -0.5f,  1.0f, 0.0f,
//         0.5f,  0.5f,  1.0f, 1.0f
//};

Mirror::Mirror(glm::vec3 pos, glm::vec3 nom, unsigned int scr_width, unsigned int scr_height) : 
    scrWidth(scr_width), scrHeight(scr_height)
{
    mirrorPosition = pos;
    mirrorNormal = nom;
    // framebuffer configuration
    // -------------------------
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scr_width, scr_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scr_width, scr_height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Mirror::~Mirror()
{

}

void Mirror::start_draw(Camera &camera)
{
    camPosition = camera.Position;
    front = camera.Front;
    glm::vec3 out;
    mirror_point(mirrorPosition, mirrorNormal, camPosition, out);
    glm::vec3 reflectedCamToMirror = reflect(mirrorPosition- camera.Position, mirrorNormal);
    float FOV = glm::clamp(((1.0f) / camToMirrorLen - 1) / 1, 0.0f, glm::half_pi<float>());
    //mat4 perspectiveMat;
    /*First Render Pass*/
    camera.ChangeView(out, reflectedCamToMirror);
    //perspectiveMat = perspective(FOV, 1.0f, 0.1f, 100.0f);
    // render
    // ------
    // bind to framebuffer and draw scene as we normally would to color texture 
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

    //// make sure we clear the framebuffer's content
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Mirror::end_draw(Camera& camera)
{
    // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    camera.ChangeView(camPosition, front);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
}
