#include "../../RetroEngine.hpp"

static int gl_init(const char *gameTitle)
{
#if RETRO_USING_OPENGL
    // Init GL
    Engine.m_glContext = SDL_GL_CreateContext(Engine.window);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    // glew Setup
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printLog("glew init error:");
        printLog((const char *)glewGetErrorString(err));
        return false;
    }

    glViewport(0, 0, SCREEN_XSIZE * Engine.windowScale, SCREEN_YSIZE * Engine.windowScale);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(-2.0, 2.0, -2.0, 2.0, -20.0, 20.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glShadeModel(GL_SMOOTH);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_LIGHTING);
    glDisable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    SetupPolygonLists();

    for (int i = 0; i < TEXTURE_LIMIT; i++) {
        glGenTextures(1, &gfxTextureID[i]);
        glBindTexture(GL_TEXTURE_2D, gfxTextureID[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, texBuffer);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    // Allows for texture locations in pixels instead of from 0.0 to 1.0, saves us having to do this every time we set UVs
    glScalef(1.0 / TEXTURE_SIZE, 1.0 / TEXTURE_SIZE, 1.0f);
    glMatrixMode(GL_PROJECTION);

    glClear(GL_COLOR_BUFFER_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glClear(GL_COLOR_BUFFER_BIT);

    framebufferId = 0;
    fbTextureId   = 0;

    SetScreenDimensions(SCREEN_XSIZE, SCREEN_YSIZE, Engine.windowScale);
#endif
    return 1;
}

void gl_flipScreen() {
#if RETRO_USING_OPENGL
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

    glLoadIdentity();

    glOrtho(0, orthWidth, SCREEN_YSIZE << 4, 0.0, 0.0f, 100.0f);
    if (texPaletteNum >= TEXTURE_LIMIT) {
        glBindTexture(GL_TEXTURE_2D, gfxTextureID[texPaletteNum % TEXTURE_LIMIT]);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, gfxTextureID[texPaletteNum]);
    }
    glEnableClientState(GL_COLOR_ARRAY);

    if (render3DEnabled) {
        // Non Blended rendering
        glVertexPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].x);
        glTexCoordPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(DrawVertex), &gfxPolyList[0].colour);
        glDrawElements(GL_TRIANGLES, gfxIndexSizeOpaque, GL_UNSIGNED_SHORT, gfxPolyListIndex);
        glEnable(GL_BLEND);

        // Init 3D Plane
        glViewport(0, 0, viewWidth, viewHeight);
        glPushMatrix();
        glLoadIdentity();
        CalcPerspective(1.8326f, viewAspect, 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glScalef(1.0f, -1.0f, -1.0f);
        glRotatef(180.0f + floor3DAngle, 0, 1.0f, 0);
        glTranslatef(floor3DXPos, floor3DYPos, floor3DZPos);
        glVertexPointer(3, GL_FLOAT, sizeof(DrawVertex3D), &polyList3D[0].x);
        glTexCoordPointer(2, GL_SHORT, sizeof(DrawVertex3D), &polyList3D[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(DrawVertex3D), &polyList3D[0].colour);
        glDrawElements(GL_TRIANGLES, indexSize3D, GL_UNSIGNED_SHORT, gfxPolyListIndex);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);

        // Return for blended rendering
        glViewport(0, 0, bufferWidth, bufferHeight);
        glPopMatrix();

        int numBlendedGfx = (int)(gfxIndexSize - gfxIndexSizeOpaque);
        glVertexPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].x);
        glTexCoordPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(DrawVertex), &gfxPolyList[0].colour);
        glDrawElements(GL_TRIANGLES, numBlendedGfx, GL_UNSIGNED_SHORT, &gfxPolyListIndex[gfxIndexSizeOpaque]);
    }
    else {
        glVertexPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].x);
        glTexCoordPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(DrawVertex), &gfxPolyList[0].colour);
        glDrawElements(GL_TRIANGLES, gfxIndexSizeOpaque, GL_UNSIGNED_SHORT, gfxPolyListIndex);

        int blendedGfxCount = gfxIndexSize - gfxIndexSizeOpaque;

        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glVertexPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].x);
        glTexCoordPointer(2, GL_SHORT, sizeof(DrawVertex), &gfxPolyList[0].u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(DrawVertex), &gfxPolyList[0].colour);
        glDrawElements(GL_TRIANGLES, blendedGfxCount, GL_UNSIGNED_SHORT, &gfxPolyListIndex[gfxIndexSizeOpaque]);
    }
    glDisableClientState(GL_COLOR_ARRAY);

    // Render the framebuffer now
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(virtualX, virtualY, virtualWidth, virtualHeight);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fbTextureId);
    glVertexPointer(2, GL_SHORT, 0, &screenVerts);
    glTexCoordPointer(2, GL_FLOAT, 0, &fbTexVerts);
    glColorPointer(4, GL_FLOAT, 0, &pureLight);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glViewport(0, 0, bufferWidth, bufferHeight);
#endif
}

void gl_release() {
#if RETRO_USING_OPENGL
    if (Engine.m_glContext)
        SDL_GL_DeleteContext(Engine.m_glContext);
#endif
}

drawing_driver_t drawing_GL = {
    gl_init,
    gl_flipScreen,
    gl_release,
};
