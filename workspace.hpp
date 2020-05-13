#pragma once

#include<X11/Xlib.h>
#include<map>

using namespace std;


class Workspace{

public:
  Workspace(){};
  ~Workspace(){};
  int window_count = 0;
  map<Window,Window> get_windows();

  void add_window(Window p, Window c);
  void delete_window(Window p);

private:
  
  map<Window,Window> parent_map;

}; 
