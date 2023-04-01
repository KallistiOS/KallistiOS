/* Tsunami for KallistiOS ##version##

   banner.cpp
   (c)2002 Megan Potter

   The included font (typewriter.txf) was pulled from the dcplib example
   in ../../cpp/dcplib.

*/

/*

This is a somewhat complex example of what can be done with the Tsunami
library with a little effort.

This sets up a simple scene with two text labels and a texture banner
with an animation path, and user control over the animation. All elements
animate onto the screen, and all elements animate off. All the transitions
are 100% smooth.

One will notice that a lot of the code below is for the custom animation
(which can be created once and used for all of your drawables) and because
I've written the code to show off a lot of tweakable features, and thus it
needs to keep references to everything. The larger and more complex your
scenes become, the bigger of a win it becomes to have all your objects
draw and animate themselves autonomously in this way. There is also a bit
of repeated code below for the "main loop", which is generally placed in
its own function/method. I'm planning to provide something to assist with
even the main loop and user input eventually.

*/

#include <kos.h>
#include <math.h>
#include <tsu/texture.h>
#include <tsu/drawables/scene.h>
#include <tsu/drawables/banner.h>
#include <tsu/drawables/label.h>
#include <tsu/anims/logxymover.h>
#include <tsu/anims/expxymover.h>
#include <tsu/anims/alphafader.h>
#include <tsu/triggers/chainanim.h>
#include <tsu/triggers/death.h>

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

// Simple animation class which rotates in a circle. This shows how
// you can create your own animations very simply.
class Circler : public Animation {
public:
    Circler(float cx, float cy, float cz, float size, float spd)
        : m_cx(cx), m_cy(cy), m_cz(cz),
          m_size(size), m_sizeto(size), m_spd(spd), m_mag(0.0f), m_frame(0),
          m_finishing(false) { }
    virtual ~Circler() { }

    virtual void nextFrame(Drawable * t) {
        Vector v;

        // Proscribe a circular path around the center point
        v.x = m_cx + m_mag * m_size * fsin(m_frame * 2 * M_PI / m_spd);
        v.y = m_cy + m_mag * m_size * fcos(m_frame * 2 * M_PI / m_spd);
        v.z = m_cz;
        t->setTranslate(v);

        // Increase our frame counter
        m_frame++;

        // Increase or decrease the size of the circle as necessary
        if(fabs(m_size - m_sizeto) > 1.0f) {
            m_size += (m_sizeto - m_size) / (m_spd / 2);
        }
        else {
            m_size = m_sizeto;
        }

        // Increase or decrease the magnitude of the circle as necessary
        if(!m_finishing && m_mag < 1.0f)
            m_mag += 1.0f / m_spd;

        if(m_finishing) {
            if(m_mag > 0.0f)
                m_mag -= 1.0f / m_spd;
            else {
                // If we're done, then trigger and die
                complete(t);
            }
        }
    }

    // Tells our animation to finish up for exit
    void finishUp() {
        m_finishing = true;
    }

    // Change the size of the circle
    void changeSize(float s) {
        m_sizeto = s;
    }

private:
    float   m_cx, m_cy, m_cz;
    float   m_size, m_sizeto, m_spd;
    float   m_mag;
    int m_frame;
    bool    m_finishing;
};

int main(int argc, char **argv) {
    int done = 0, done2 = 0;

    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (void (*)(unsigned char, long  unsigned int))arch_exit);

    pvr_init_defaults();

    // Load a texture for our banner
    RefPtr<Texture> txr = new Texture("/rd/logo.png", true);

    // Setup a scene and place a banner in it
    RefPtr<Scene> sc = new Scene();
    RefPtr<Banner> b = new Banner(PVR_LIST_TR_POLY, txr);
    sc->subAdd(b);

    // Put the banner off-screen to begin with, and attach a LogXYMover
    // animation to move it to 320,240
    b->setTranslate(Vector(800, 600, 10));
    RefPtr<LogXYMover> mover = new LogXYMover(320, 240);
    b->animAdd(mover);

    // Add a trigger to the LogXYMover to chain to our Circler animation.
    RefPtr<Circler> circ = new Circler(320, 240, 10, 75, 30);
    mover->triggerAdd(new ChainAnimation(circ));

    // Add a couple of labels with explanation. Set each one's
    // initial alpha to 0 and fade them in over 30 frames.
    RefPtr<Font> fnt = new Font("/rd/typewriter.txf");
    RefPtr<Label> lbl1 = new Label(fnt, "Use the joystick to control the size", 24, true, true);
    lbl1->setTranslate(Vector(320, 360, 20));
    sc->subAdd(lbl1);
    lbl1->setAlpha(0.0f);
    RefPtr<AlphaFader> fader = new AlphaFader(1.0f, 1.0f / 30.0f);
    lbl1->animAdd(fader);

    RefPtr<Label> lbl2 = new Label(fnt, "of the circle. Press START to quit.", 24, true, true);
    lbl2->setTranslate(Vector(320, 360 + 24, 20));
    sc->subAdd(lbl2);
    lbl2->setAlpha(0.0f);
    lbl2->animAdd(fader);

    pvr_set_bg_color(0.2f, 0.0f, 0.4f);

    // Enter the main loop -- user interaction would normally happen here.
    while(!done) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);

        // Draw the scene's opaque poly list
        sc->draw(PVR_LIST_OP_POLY);

        pvr_list_begin(PVR_LIST_TR_POLY);

        // Draw the scene's transparent poly list
        sc->draw(PVR_LIST_TR_POLY);

        pvr_scene_finish();

        // Advance any animations
        sc->nextFrame();

        // Check for the user pressing START
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)

        if(t->buttons & CONT_START)
            done = 1;

        if(t->joyx != 0) {
            circ->changeSize(30 + t->joyx / 6.0);
        }
        else {
            circ->changeSize(30);
        }

        MAPLE_FOREACH_END()
    }

    // The user has pressed start, but instead of just exiting we will
    // attach some more animations to make a nice exit.

    printf("Starting exodus...\n");

    // Create an ExpXYMover heading off-screen and chain it to the Circler
    RefPtr<ExpXYMover> mover2 = new ExpXYMover(-1.0f, -1.0f, 320 - 800, 240 - 600);
    circ->triggerAdd(new ChainAnimation(mover2));

    // Add a trigger to the ExpXYMover that will signal that the banner
    // is finished once it reaches the animation endpoint.
    mover2->triggerAdd(new Death());

    // Add faders with triggers; note we use two separate alpha faders
    // this time so we have two triggers (one per label)
    fader = new AlphaFader(0.0f, -1.0f / 30.0f);
    fader->triggerAdd(new Death());
    lbl1->animAdd(fader);

    fader = new AlphaFader(0.0f, -1.0f / 30.0f);
    fader->triggerAdd(new Death());
    lbl2->animAdd(fader);

    // Tell the circler we're about to finish up
    circ->finishUp();

    // Ok, do another drawing loop, and quit when the scene says it's done
    while(!sc->isFinished() && !done2) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);

        // Draw the scene's opaque poly list
        sc->draw(PVR_LIST_OP_POLY);

        pvr_list_begin(PVR_LIST_TR_POLY);

        // Draw the scene's transparent poly list
        sc->draw(PVR_LIST_TR_POLY);

        pvr_scene_finish();

        // Advance any animations
        sc->nextFrame();

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)

        if(!(t->buttons & CONT_START))
            done = 0;

        if(!done && t->buttons & CONT_START)
            done2 = 1;

        MAPLE_FOREACH_END()
    }

    // Ok, we're all done! The RefPtrs will take care of mem cleanup.

    return 0;
}


