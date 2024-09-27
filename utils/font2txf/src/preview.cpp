/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#ifdef DISPLAY

#include "preview.h"

// Uncomment to debug
#undef GL_EXT_paletted_texture


GLuint _tid;


void build_font_texture( FT_Bitmap* img )
{
    //glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    glGenTextures( 1, &_tid );
    glBindTexture( GL_TEXTURE_2D, _tid );

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

#ifdef GL_EXT_paletted_texture
    if( glutExtensionSupported( "GL_EXT_paletted_texture" ) )
    {
        GLubyte cmap[ 256 * 4 ];
        GLubyte* cp = cmap;
        int i;
        for( i = 0; i < 256; i++ )
        {
            *cp++ = i;
            *cp++ = i;
            *cp++ = i;
            *cp++ = i;
        }

        glColorTableEXT( GL_TEXTURE_2D, GL_RGBA, 256,
                         GL_RGBA, GL_UNSIGNED_BYTE, cmap );

        glTexImage2D( GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT,
                      img->width, img->rows, 0,
                      GL_COLOR_INDEX, GL_UNSIGNED_BYTE, img->buffer );
    }
    else
#endif
    {
        /*
        // Doesn't work!
        GLfloat fmap[ 256 ];
        GLfloat* fp = fmap;
        GLfloat i;
        for( i = 0; i < 256; i++ )
        {
            *fp++ = i / 255.0;
        }

        glPixelMapfv( GL_PIXEL_MAP_I_TO_R, 256, fmap );
        glPixelMapfv( GL_PIXEL_MAP_I_TO_G, 256, fmap );
        glPixelMapfv( GL_PIXEL_MAP_I_TO_B, 256, fmap );
        glPixelMapfv( GL_PIXEL_MAP_I_TO_A, 256, fmap );

        glTexImage2D( GL_TEXTURE_2D, 0, 1, img->width, img->rows, 0,
                      GL_COLOR_INDEX, GL_UNSIGNED_BYTE, img->buffer );
        */
#if 1
        int i;
        long size = img->width * img->rows;
        GLubyte* rgba = (GLubyte*) malloc( size * 2 );
        GLubyte* src = img->buffer;
        GLubyte* dst = rgba;
        for( i = 0; i < size; i++ )
        {
            *dst++ = *src;
            *dst++ = *src++;
        }
        glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                      img->width, img->rows, 0,
                      GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, rgba );
#else
        int i;
        long size = img->width * img->rows;
        GLubyte* rgba = (GLubyte*) malloc( size * 4 );
        GLubyte* src = img->buffer;
        GLubyte* dst = rgba;
        for( i = 0; i < size; i++ )
        {
            *dst++ = *src;
            *dst++ = *src;
            *dst++ = *src;
            *dst++ = *src;
            ++src;
        }

        glTexImage2D( GL_TEXTURE_2D, 0, 4, img->width, img->rows, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, rgba );
        free( rgba );
#endif
    }
}


void on_display()
{
    glClear( GL_COLOR_BUFFER_BIT );

    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
    glBegin( GL_QUADS );
    glColor3ub( 255, 0, 0 );   glVertex2f( 0, 0 );
    glColor3ub( 0, 255, 0 );   glVertex2f( 0, g_txf.rows );
    glColor3ub( 0, 0, 255 );   glVertex2f( g_txf.width, g_txf.rows );
    glColor3ub( 0, 0, 0 );     glVertex2f( g_txf.width, 0 );
    glEnd();

    glBindTexture( GL_TEXTURE_2D, _tid );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_TEXTURE_2D );
    glColor3ub( 255, 255, 255 );

    glBegin( GL_QUADS );
    glTexCoord2f( 0.0, 1.0 );   glVertex2f( 0, 0 );
    glTexCoord2f( 0.0, 0.0 );   glVertex2f( 0, g_txf.rows );
    glTexCoord2f( 1.0, 0.0 );   glVertex2f( g_txf.width, g_txf.rows );
    glTexCoord2f( 1.0, 1.0 );   glVertex2f( g_txf.width, 0 );
    glEnd();

    glutSwapBuffers();
}


void on_keyFunc( unsigned char key, int, int )
{
    if( key == 'q' || key == 27 ) // ESC
    {
        exit( 0 );
    }
}


void on_reshape( int w, int h )
{
    glViewport(0, 0, w, h);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluOrtho2D( 0, w, 0, h );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    /*glTranslatef(0, h - g_txf.rows, 0);
    glRasterPos2i(0, 0);
    glBitmap(0, 0, 0, 0, 256, -256, NULL);*/
}


void do_preview_txf( int argc, char* argv[] )
{
    glutInit( &argc, argv );
    // GLUT_SINGLE does not seem to work with NVidia's OpenGL on GeForce2.
    //glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE );

    glutInitWindowSize( g_txf.width, g_txf.rows );
    glutCreateWindow( PROGRAM_NAME );
    glutReshapeFunc( on_reshape );
    glutDisplayFunc( on_display );
    glutKeyboardFunc( on_keyFunc );

    glClearColor( 0.2, 0.2, 0.2, 1.0 );

    build_font_texture( &g_txf );    

    glutMainLoop();
}

#endif
