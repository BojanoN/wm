#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<fstream>
#include<cstring>
#include<unistd.h>
#include"wm.hpp"


using namespace std;

// Static member initialization

bool WindowManager::wm_present = false;

int WindowManager::window_count = 0;

ofstream WindowManager::output{"log.txt",ios::app};


// Class methods

unique_ptr<WindowManager> WindowManager::start(){

  Display* disp = XOpenDisplay(nullptr);

  if(disp==nullptr)
    return nullptr;

  return unique_ptr<WindowManager>(new WindowManager(disp));
}

WindowManager::~WindowManager(){
  XCloseDisplay(this->display);
  delete &workspaces;
}

string WindowManager::timestamp(){

  time_t t;
  tm* x;
  t = time(NULL);
  x=localtime(&t);
  return string("["+to_string(x->tm_hour)+":"+to_string(x->tm_min)+":"+to_string(x->tm_sec)+"]");

}

int WindowManager::XError_handler(Display* display, XErrorEvent* x){

  // If another WM is present, once we make the XSelectInput request and flush the buffer using XSync, the XErrorEvent error_code struct member will contain
  // a BadAccess code;

  if(static_cast<int> (x->error_code) == BadAccess){
    log("Another WM is already present! ;_;");
    wm_present=true;
    return ExVal::ERROR;
  }

  log("Error code: "+ to_string(static_cast<int> (x->error_code)));
  log("From request: "+ to_string(x->request_code));

}

void WindowManager::handle_ConfigureRequest(const XConfigureRequestEvent& ev){

  XWindowChanges ch;

  // Copy the requested window parameters
  ch.x = ev.x;
  ch.y = ev.y;
  ch.width = ev.width;
  ch.height = ev.height;
  ch.border_width = ev.border_width;
  ch.sibling = ev.above;
  ch.stack_mode = ev.detail;

  // Resize the frame if the parent window contains one
  if(parent_map.count(ev.window)){

    Window frame = parent_map[ev.window];
    XConfigureWindow(display, frame, ev.value_mask, &ch);

  }

  XConfigureWindow(display, ev.window, ev.value_mask, &ch);
  XSync(this->display, false);
  }

void WindowManager::handle_MapRequest(const XMapRequestEvent& ev){

  XWindowAttributes attrs;
  XWindowAttributes tmp_attrs;
  XWindowChanges changes;
  Window win;
  //  map<Window, Window> tmp = workspaces[this->current_workspace-1].get_windows();

  int i;
  int width_quantum;

  if(parent_map.count(ev.window)){
    log("Window already framed.");
    return;
  }

  XGetWindowAttributes(this->display, ev.window, &attrs);

  // First window fits to screen
  if(parent_map.empty()){

    attrs.x = 0;
    attrs.y = 0;
    attrs.width = this->screen_width;
    attrs.height = this->screen_height - this->border_width;

    XResizeWindow(this->display, ev.window, attrs.width, attrs.height);
    log("First window.");

  }else{
   // Tiling algorithm
    i = 0;
    width_quantum = this->screen_width / (workspaces[current_workspace].window_count+1);
    changes.y = 0;
    changes.height = this->screen_height - this->border_width;
    changes.border_width = border_width;
  
    // Resize existing windows
    for(auto& w : parent_map ){
      log(to_string(i));
      XMoveResizeWindow(this->display, w.second, width_quantum * i, 0, width_quantum - 10, this->screen_height);
      i++;
    }

    // Place the new window last
    XMoveResizeWindow(this->display, ev.window, width_quantum * i, 0, width_quantum, this->screen_height -  this->border_width);
    XGetWindowAttributes(this->display, ev.window, &attrs);
  }
  
  win = XCreateSimpleWindow(this->display, this->root_window, attrs.x, attrs.y, attrs.width,
                            attrs.height,
                            border_width,
                            Color::LIGHT_BLUE,
                            Color::WHITE
                            );
  XSelectInput(this->display, win, SubstructureRedirectMask | SubstructureNotifyMask
                                   | KeyPressMask | KeyReleaseMask);
  XAddToSaveSet(this->display, ev.window);
  XReparentWindow(this->display, ev.window, win, 0, 0);
  XMapWindow(this->display, win);

  parent_map[ev.window] = win;

  // Kill window with $mod(currently LSHIFT) and q
  XGrabKey(display, XKeysymToKeycode(this->display, XK_F4), Mod1Mask, ev.window, true,
              GrabModeAsync,
              GrabModeAsync
              );

  XGrabKey(display, XKeysymToKeycode(this->display, XK_Return), Mod1Mask, ev.window, true,
           GrabModeAsync,
           GrabModeAsync
           );

  // TODO XGrabKeys, bind keys for basic functionalities
  XMapWindow(this->display, ev.window);
  this->workspaces[current_workspace].window_count++;
}

void WindowManager::handle_UnmapNotify(const XUnmapEvent& ev){
  int i = 0;
  int width_quantum;

  if(!parent_map.count(ev.window)){
    log("Unmanaged window recieved, ignoring...");
    return;
  }

  // Delete associated frame and window
  Window fr = parent_map[ev.window];
  XUnmapWindow(this->display, fr);
  XReparentWindow(this->display, ev.window, this->root_window, 0, 0);
  XRemoveFromSaveSet(this->display, ev.window);
  XDestroyWindow(this->display, ev.window);
  parent_map.erase(ev.window);

  this->workspaces[current_workspace].window_count--;

  // Resize remaining windows
  width_quantum = this->screen_width / this->workspaces[current_workspace].window_count;
  for(auto& w : parent_map){
    if(i == window_count - 1)
      XMoveResizeWindow(this->display, w.second, width_quantum * i, 0, width_quantum - 10, this->screen_height - this->border_width);
    else
      XMoveResizeWindow(this->display, w.second, width_quantum * i, 0, width_quantum - 10, this->screen_height);
    i++; 
  }
}

void WindowManager::handle_KeyPress(const XKeyEvent& ev){

  if ((ev.state == Mod1Mask) &&(ev.keycode == XKeysymToKeycode(this->display, XK_F4))){
    XEvent del;
    memset(&del, 0, sizeof(del));
    del.xclient.type = ClientMessage;
    del.xclient.message_type = XInternAtom(this->display, "WM_PROTOCOLS", true);
    del.xclient.format = 32;
    del.xclient.window = ev.window;
    del.xclient.data.l[0] = XInternAtom(this->display, "WM_DELETE_WINDOW", false);
    XSendEvent(this->display, ev.window, false, NoEventMask, &del);
  }

  if ((ev.state == Mod1Mask) && (ev.keycode == XKeysymToKeycode(this->display, XK_Return))){
    if(fork() == 0){
      system("xterm");
      log("Child process done");
      exit(0);
    }
  }

}

void WindowManager::run(){

  // Initialization. Makes initial request to check for present WMs.
  XEvent* ev;

  XSetErrorHandler(&WindowManager::XError_handler);
  XSelectInput(this->display, this->root_window, SubstructureRedirectMask | SubstructureNotifyMask
               | KeyPressMask | KeyReleaseMask);
  XSync(this->display, false);

  if (this->wm_present)
    return;

  log("Window manager initialized");
  log("Screen dimensions: "+to_string(this->screen_width)+"x"+to_string(this->screen_height));

  // Event loop.
  while (true){

    XNextEvent(this->display, ev);
    log("Recieved event "+to_string(ev->type));

    switch (ev->type){

       case CreateNotify:
         break;

       case ConfigureRequest:
         handle_ConfigureRequest(ev->xconfigurerequest);
         break;

       case DestroyNotify:
         break;

       case MapRequest:
         handle_MapRequest(ev->xmaprequest);
         break;

       case ConfigureNotify:
         break;

       case ReparentNotify:
         break;

       case MapNotify:
         break;

       case UnmapNotify:
         handle_UnmapNotify(ev->xunmap);
         break;

       case ResizeRequest:
         break;

       case KeyPress:
         handle_KeyPress(ev->xkey);
         break;


         // TODO: implement other events and their handlers

       default:
        log("Event not yet implemented: "+to_string(ev->type));
    }
  }
}


int main(){

  unique_ptr<WindowManager> wm(WindowManager::start());

  if(wm == nullptr){
    WindowManager::log("Unable to open display!");
    WindowManager::log("-----------------------");
    return ExVal::ERROR;
  }

  wm->run();
}
