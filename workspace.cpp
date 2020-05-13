#include "workspace.hpp"

using namespace std;

map<Window, Window> Workspace::get_windows(){
  return parent_map;
}

void Workspace::add_window(Window parent, Window child){
  parent_map[parent] = child;
  this->window_count++;
}

void Workspace::delete_window(Window parent){
  parent_map.erase(parent);
  this->window_count--;
}
