#pragma once
#include<X11/Xlib.h>
#include<memory>
#include<map>
#include<fstream>
#include<vector>
#include"workspace.hpp"

using namespace std;

// Return values, for improved readability
enum ExVal{

  SUCCESS=0,
  ERROR=-1

};

enum Color {

  RED = 0xff0000U,
  LIGHT_BLUE = 0x8be5efU,
  WHITE = 0xffffffU

};


class WindowManager {
public:

  static bool wm_present;
  int screen_height;
  int screen_width;
  static int window_count;
  const unsigned int border_width = 2;

  // The constructor is invoked by the start() method. Attempts to open the display and returns a WindowManager object.
  static unique_ptr<WindowManager> start();

  // Checks whether another WM is present and handles various errors.
  static int XError_handler(Display* display, XErrorEvent* x);

  static string timestamp();

  // Destructor. Closes the display.
  ~WindowManager();

  // Commences WM operations
  void run();

  static void log(string msg){
    output << timestamp() <<" "<< msg << endl;
  };

private:

  // Constructor. Initializes root window.
  WindowManager(Display* display): display(display), root_window(DefaultRootWindow(display)){

    Screen* scr = XDefaultScreenOfDisplay(display);
    screen_height = scr->height;
    screen_width = scr->width;
    parent_map = workspaces[current_workspace].get_windows();
  }

  Display* display;

  const Window root_window;

  static ofstream output;

  map<Window, Window> parent_map;

  vector<Workspace> workspaces = vector<Workspace>(8);

  int current_workspace = 0;


  void handle_ConfigureRequest(const XConfigureRequestEvent& ev);

  void handle_MapRequest(const XMapRequestEvent& ev);

  void handle_UnmapNotify(const XUnmapEvent& ev);

  void handle_KeyPress(const XKeyPressedEvent& ev);

};
