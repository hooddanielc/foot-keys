/* main.c
 * Created in 2010 by Ulrik Sverdrup <ulrik.sverdrup@gmail.com>
 *
 * This work is placed in the public domain.
 */

#include <stdio.h>

#include <gtk/gtk.h>
#include <keybinder.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <iostream>

// The key code to be sent.
// A full list of available codes can be found in /usr/include/X11/keysymdef.h
#define KEYCODE XK_Down

// Function to create a keyboard event
XKeyEvent createKeyEvent(Display *display, Window &win,
                           Window &winRoot, bool press,
                           int keycode, int modifiers)
{
   XKeyEvent event;

   event.display     = display;
   event.window      = win;
   event.root        = winRoot;
   event.subwindow   = None;
   event.time        = CurrentTime;
   event.x           = 1;
   event.y           = 1;
   event.x_root      = 1;
   event.y_root      = 1;
   event.same_screen = True;
   event.keycode     = XKeysymToKeycode(display, keycode);
   event.state       = modifiers;

   if(press)
      event.type = KeyPress;
   else
      event.type = KeyRelease;

   return event;
}

#define EXAMPLE_KEY "<Ctrl>F"

void handler (const char *keystring, void *user_data) {
  printf("Handle %s (%p)!\n", keystring, user_data);
  //keybinder_unbind(keystring, handler);
  //gtk_main_quit();

  // Obtain the X11 display.
   Display *display = XOpenDisplay(0);
   if (display == NULL) {
      std::cout << "didn't work" << std::endl;
      return;
   }

// Get the root window for the current display.
   Window winRoot = XDefaultRootWindow(display);

// Find the window which has the current keyboard focus.
   Window winFocus;
   int    revert;
   XGetInputFocus(display, &winFocus, &revert);

// Send a fake key press event to the window.
   XKeyEvent event_f = createKeyEvent(display, winFocus, winRoot, true, XK_Control_L, 0);
   XSendEvent(event_f.display, event_f.window, True, KeyPressMask, (XEvent *)&event_f);

   XKeyEvent event_ctrl = createKeyEvent(display, winFocus, winRoot, true, XK_F, 0);
   XSendEvent(event_ctrl.display, event_ctrl.window, True, KeyPressMask, (XEvent *)&event_ctrl);

// Send a fake key release event to the window.
   event_f = createKeyEvent(display, winFocus, winRoot, false, XK_F, 0);
   XSendEvent(event_f.display, event_f.window, True, KeyPressMask, (XEvent *)&event_f);

   event_ctrl = createKeyEvent(display, winFocus, winRoot, false, XK_Control_L, 0);
   XSendEvent(event_ctrl.display, event_ctrl.window, True, KeyPressMask, (XEvent *)&event_ctrl);

// Done.
   XCloseDisplay(display);
}

int main (int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  keybinder_init();
  keybinder_bind(EXAMPLE_KEY, handler, NULL);
  printf("Press " EXAMPLE_KEY " to activate keybinding and quit\n");

  gtk_main();
  return 0;
}
