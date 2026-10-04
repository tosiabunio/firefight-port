// Port test tool: an input script (--input <file>) pushes SDL input events at given frames, so a
// test can play the game through the real input path (SDL -> Comm::input_proc -> Kbd, Mouse,
// Joy -> Eem). With --fast the simulation makes one step per frame, so a script replays the same
// way every time.
//
// One action per line; '#' starts a comment. <frame> is the number of frames shown so far
// (Video::frames, as in the frame_<frame>.bmp dumps); the action happens after that frame, before
// the next simulation step. Lines must come in frame order.
//
//   <frame> key <scancode> down|up         SDL scancode name: Return, Space, Up, A, F11, Left Ctrl...
//   <frame> mouse <x> <y>                  move to window coordinates (headless: framebuffer pixels)
//   <frame> mouse left|middle|right down|up
//   <frame> pad <button> down|up           a virtual game controller; SDL names: a, b, x, y, back,
//                                          start, leftshoulder, rightshoulder, dpup, dpdown, ...
//   <frame> pad <axis> <value>             leftx, lefty, rightx, righty (-32768..32767),
//                                          lefttrigger, righttrigger (0..32767)
//   <frame> log <text>                     write text to the log (marks for a test to look for)
//
// The virtual controller is attached before the engine starts, so headless runs open it too
// (they ignore real devices).

#include <string>
#include <vector>

#include "headers.h"
#include <SDL.h>

namespace {

enum Kind { key, mouse_move, mouse_button, pad_button, pad_axis, log_text };

struct Action
{
  int frame;
  Kind kind;
  int code;      // scancode, mouse button, controller button or axis
  int value;     // down (1) / up (0), axis value, or x
  int y;
  std::string text;
};

std::vector<Action> actions;
size_t next_action;
int pointer_x, pointer_y;
SDL_Joystick *pad;

void push(SDL_Event &event)
{
  if (SDL_PushEvent(&event) < 0)
  {
    WARNING("input script: event not queued: %s", (char *)SDL_GetError());
  }
}

void run(const Action &a)
{
  SDL_Event e;
  memset(&e, 0, sizeof(e));
  switch (a.kind)
  {
    case key:
      e.type = a.value ? SDL_KEYDOWN : SDL_KEYUP;
      e.key.timestamp = SDL_GetTicks();
      e.key.state = a.value ? SDL_PRESSED : SDL_RELEASED;
      e.key.keysym.scancode = (SDL_Scancode)a.code;
      e.key.keysym.sym = SDL_GetKeyFromScancode((SDL_Scancode)a.code);
      push(e);
      break;
    case mouse_move:
      e.type = SDL_MOUSEMOTION;
      e.motion.timestamp = SDL_GetTicks();
      e.motion.x = a.value;
      e.motion.y = a.y;
      e.motion.xrel = a.value - pointer_x;
      e.motion.yrel = a.y - pointer_y;
      pointer_x = a.value;
      pointer_y = a.y;
      push(e);
      break;
    case mouse_button:
      e.type = a.value ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
      e.button.timestamp = SDL_GetTicks();
      e.button.button = (Uint8)a.code;
      e.button.state = a.value ? SDL_PRESSED : SDL_RELEASED;
      e.button.clicks = 1;
      e.button.x = pointer_x;
      e.button.y = pointer_y;
      push(e);
      break;
    case pad_button:
      SDL_JoystickSetVirtualButton(pad, a.code, (Uint8)a.value);
      break;
    case pad_axis:
      SDL_JoystickSetVirtualAxis(pad, a.code, (Sint16)a.value);
      break;
    case log_text:
      MESSAGE("input script: %s", (char *)a.text.c_str());
      break;
  }
}

void frame_proc(int frame)
{
  while (next_action < actions.size() && actions[next_action].frame <= frame)
    run(actions[next_action++]);
}

// Triggers are raw axes on the virtual device: -32768 is released (0 for the controller).
int raw_trigger(int value)
{
  return SDL_clamp(value, 0, 32767) * 2 - 32768;
}

bool attach_pad()
{
  if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
    return false;
  SDL_VirtualJoystickDesc desc;
  memset(&desc, 0, sizeof(desc));
  desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
  desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
  desc.naxes = SDL_CONTROLLER_AXIS_MAX;
  desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
  desc.name = "Fire Fight input script";
  int index = SDL_JoystickAttachVirtualEx(&desc);
  if (index < 0 || !(pad = SDL_JoystickOpen(index)))
    return false;
  SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT, raw_trigger(0));
  SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, raw_trigger(0));
  return true;
}

bool fail(const char *file, int line, const char *what)
{
  fprintf(stderr, "firefight: %s:%d: %s\n", file, line, what);
  return false;
}

} // namespace

// Reads the script and installs it; false (with a message on stderr) if it can't be used.
bool input_script_load(const char *file)
{
  FILE *f = fopen(file, "r");
  if (!f)
    return fail(file, 0, "cannot open input script");
  char buf[512];
  int line = 0, last_frame = 0;
  bool uses_pad = false;
  while (fgets(buf, sizeof(buf), f))
  {
    line++;
    if (char *hash = strchr(buf, '#'))
      *hash = 0;
    char device[32], name[64], state[32];
    int frame, consumed = 0;
    if (sscanf(buf, " %d %31s %n", &frame, device, &consumed) < 2)
    {
      if (strspn(buf, " \t\r\n") != strlen(buf))
        return fail(file, line, "expected <frame> <action> ...");
      continue;
    }
    if (frame < last_frame)
      return fail(file, line, "frames must not decrease");
    last_frame = frame;
    Action a = {frame, key, 0, 0, 0, std::string()};
    std::string what = device;
    if (what == "log")
    {
      a.kind = log_text;
      a.text = buf + consumed;
      a.text.erase(a.text.find_last_not_of(" \t\r\n") + 1);
    }
    else if (what == "key")
    {
      // The scancode name may contain spaces ("Left Ctrl"): the state is the last word.
      std::string rest = buf + consumed;
      rest.erase(rest.find_last_not_of(" \t\r\n") + 1);
      size_t space = rest.find_last_of(" \t");
      if (space == std::string::npos)
        return fail(file, line, "expected key <scancode> down|up");
      std::string key_name = rest.substr(0, space), key_state = rest.substr(space + 1);
      key_name.erase(key_name.find_last_not_of(" \t") + 1);
      a.code = SDL_GetScancodeFromName(key_name.c_str());
      if (a.code == SDL_SCANCODE_UNKNOWN)
        return fail(file, line, "unknown scancode name");
      if (key_state != "down" && key_state != "up")
        return fail(file, line, "expected down or up");
      a.value = key_state == "down";
    }
    else if (what == "mouse" && sscanf(buf + consumed, "%d %d", &a.value, &a.y) == 2)
      a.kind = mouse_move;
    else if (what == "mouse" && sscanf(buf + consumed, "%63s %31s", name, state) == 2)
    {
      a.kind = mouse_button;
      std::string button = name, s = state;
      a.code = button == "left" ? SDL_BUTTON_LEFT : button == "middle" ? SDL_BUTTON_MIDDLE
             : button == "right" ? SDL_BUTTON_RIGHT : 0;
      if (!a.code || (s != "down" && s != "up"))
        return fail(file, line, "expected mouse left|middle|right down|up");
      a.value = s == "down";
    }
    else if (what == "pad" && sscanf(buf + consumed, "%63s %31s", name, state) == 2)
    {
      uses_pad = true;
      std::string s = state;
      SDL_GameControllerButton button = SDL_GameControllerGetButtonFromString(name);
      SDL_GameControllerAxis axis = SDL_GameControllerGetAxisFromString(name);
      if (button != SDL_CONTROLLER_BUTTON_INVALID && (s == "down" || s == "up"))
      {
        a.kind = pad_button;
        a.code = button;
        a.value = s == "down";
      }
      else if (axis != SDL_CONTROLLER_AXIS_INVALID)
      {
        a.kind = pad_axis;
        a.code = axis;
        a.value = atoi(state);
        if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
          a.value = raw_trigger(a.value);
        else
          a.value = SDL_clamp(a.value, -32768, 32767);
      }
      else
        return fail(file, line, "expected pad <button> down|up or pad <axis> <value>");
    }
    else
      return fail(file, line, "unknown action");
    actions.push_back(a);
  }
  fclose(f);
  if (uses_pad && !attach_pad())
    return fail(file, 0, "cannot attach a virtual game controller");
  Video::frame_proc = frame_proc;
  return true;
}
