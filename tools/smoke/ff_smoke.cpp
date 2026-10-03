// ff_smoke: phase 0 dependency check for the Fire Fight port.
//
// - Video: opens a 640x400 window and presents an 8-bit paletted framebuffer the way
//   the port will (6-bit palette scaled x4 -> ARGB8888 streaming texture, letterboxed).
// - Audio: decodes a FLAC soundtrack track and an original 8-bit 11 kHz WAV with SDL2_mixer.
// - Network: completes an ENet handshake and delivers a reliable packet over loopback.
//
// Exit code 0 means every check passed. --headless uses SDL's dummy video/audio drivers.

#include <SDL.h>
#include <SDL_mixer.h>
#include <enet/enet.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 400;

struct Options {
  int frames = 120;
  bool headless = false;
  const char* music = nullptr;
  const char* wav = nullptr;
};

int g_failures = 0;

void check(bool ok, const char* what, const char* detail = nullptr) {
  std::printf("[%s] %s%s%s\n", ok ? " ok " : "FAIL", what, detail ? ": " : "", detail ? detail : "");
  if (!ok) ++g_failures;
}

bool parse_args(int argc, char** argv, Options& o) {
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    const bool has_value = i + 1 < argc;
    if (std::strcmp(a, "--headless") == 0) {
      o.headless = true;
    } else if (std::strcmp(a, "--frames") == 0 && has_value) {
      o.frames = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--music") == 0 && has_value) {
      o.music = argv[++i];
    } else if (std::strcmp(a, "--wav") == 0 && has_value) {
      o.wav = argv[++i];
    } else {
      std::fprintf(stderr,
                   "usage: ff_smoke [--headless] [--frames N] [--music file.flac] [--wav file.wav]\n");
      return false;
    }
  }
  return o.frames > 0;
}

// Same scheme as the original present path: 6-bit components, output = value * 4.
void build_palette(std::uint32_t (&lut)[256]) {
  for (int i = 0; i < 256; ++i) {
    const std::uint32_t r6 = static_cast<std::uint32_t>(i >> 2);
    const std::uint32_t g6 = static_cast<std::uint32_t>((255 - i) >> 2);
    const std::uint32_t b6 = static_cast<std::uint32_t>((i * 3) >> 2) & 63u;
    lut[i] = 0xFF000000u | (r6 * 4) << 16 | (g6 * 4) << 8 | (b6 * 4);
  }
}

std::uint8_t pattern(int x, int y, int frame) {
  return static_cast<std::uint8_t>((x + y + frame) & 0xFF);
}

void run_video(const Options& o) {
  Uint32 window_flags = SDL_WINDOW_RESIZABLE;
  if (o.headless) window_flags |= SDL_WINDOW_HIDDEN;
  SDL_Window* window = SDL_CreateWindow("Fire Fight - SDL smoke test", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, kWidth, kHeight, window_flags);
  check(window != nullptr, "create 640x400 window", window ? nullptr : SDL_GetError());
  if (!window) return;

  SDL_Renderer* renderer = nullptr;
  if (!o.headless)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  check(renderer != nullptr, "create renderer", renderer ? nullptr : SDL_GetError());

  SDL_Texture* texture = nullptr;
  if (renderer) {
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) == 0) std::printf("       renderer: %s\n", info.name);
    SDL_RenderSetLogicalSize(renderer, kWidth, kHeight);  // letterbox, square pixels
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                kWidth, kHeight);
    check(texture != nullptr, "create ARGB8888 streaming texture", texture ? nullptr : SDL_GetError());
  }

  if (renderer && texture) {
    std::uint32_t lut[256];
    build_palette(lut);
    std::vector<std::uint8_t> framebuffer(static_cast<size_t>(kWidth) * kHeight);

    int presented = 0;
    bool quit = false;
    bool readback_ok = !o.headless;  // pixel readback is only reliable with the software renderer
    for (int frame = 0; frame < o.frames && !quit; ++frame) {
      SDL_Event ev;
      while (SDL_PollEvent(&ev))
        if (ev.type == SDL_QUIT) quit = true;

      for (int y = 0; y < kHeight; ++y)
        for (int x = 0; x < kWidth; ++x)
          framebuffer[static_cast<size_t>(y) * kWidth + x] = pattern(x, y, frame);

      void* pixels = nullptr;
      int pitch = 0;
      if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
        check(false, "lock texture", SDL_GetError());
        break;
      }
      for (int y = 0; y < kHeight; ++y) {
        auto* row = reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(pixels) + y * pitch);
        const std::uint8_t* src = &framebuffer[static_cast<size_t>(y) * kWidth];
        for (int x = 0; x < kWidth; ++x) row[x] = lut[src[x]];
      }
      SDL_UnlockTexture(texture);

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);

      if (o.headless && frame == o.frames - 1) {
        // The window is exactly 640x400, so logical and physical pixels coincide.
        Uint32 px = 0;
        const SDL_Rect probe{123, 45, 1, 1};
        if (SDL_RenderReadPixels(renderer, &probe, SDL_PIXELFORMAT_ARGB8888, &px, 4) == 0)
          readback_ok = px == lut[pattern(probe.x, probe.y, frame)];
      }
      SDL_RenderPresent(renderer);
      ++presented;
      if (!o.headless) SDL_Delay(16);
    }
    check(quit || presented == o.frames, "present paletted frames");
    check(readback_ok, "palette->ARGB conversion reaches the screen");
  }

  if (texture) SDL_DestroyTexture(texture);
  if (renderer) SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void run_audio(const Options& o) {
  const int inited = Mix_Init(MIX_INIT_FLAC);
  check((inited & MIX_INIT_FLAC) != 0, "SDL_mixer FLAC decoder available",
        (inited & MIX_INIT_FLAC) ? nullptr : Mix_GetError());

  if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) != 0) {
    check(false, "open audio device", Mix_GetError());
    Mix_Quit();
    return;
  }
  check(true, "open audio device");
  Mix_AllocateChannels(16);

  if (o.wav) {
    Mix_Chunk* chunk = Mix_LoadWAV(o.wav);
    check(chunk != nullptr, "load original 8-bit 11 kHz WAV", chunk ? nullptr : Mix_GetError());
    if (chunk) {
      const int channel = Mix_PlayChannel(-1, chunk, 0);
      check(channel >= 0, "play WAV on a mixer channel", channel >= 0 ? nullptr : Mix_GetError());
      if (channel >= 0) Mix_SetPanning(channel, 255, 96);
      SDL_Delay(150);
      Mix_HaltChannel(-1);
      Mix_FreeChunk(chunk);
    }
  }

  if (o.music) {
    Mix_Music* music = Mix_LoadMUS(o.music);
    check(music != nullptr, "load FLAC soundtrack track", music ? nullptr : Mix_GetError());
    if (music) {
      check(Mix_GetMusicType(music) == MUS_FLAC, "music decoder is FLAC");
      check(Mix_PlayMusic(music, -1) == 0, "play music (looping)");
      SDL_Delay(250);
      check(Mix_PlayingMusic() == 1, "music is playing");
      Mix_HaltMusic();
      Mix_FreeMusic(music);
    }
  }

  Mix_CloseAudio();
  Mix_Quit();
}

void run_enet() {
  if (enet_initialize() != 0) {
    check(false, "initialise ENet");
    return;
  }
  check(true, "initialise ENet");

  ENetAddress listen_at{};
  enet_address_set_host(&listen_at, "127.0.0.1");
  listen_at.port = ENET_PORT_ANY;
  ENetHost* server = enet_host_create(&listen_at, 4, 2, 0, 0);
  ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
  check(server && client, "create ENet hosts");

  if (server && client) {
    ENetAddress bound{};
    enet_socket_get_address(server->socket, &bound);
    ENetAddress target = listen_at;
    target.port = bound.port;
    ENetPeer* peer = enet_host_connect(client, &target, 2, 0);

    static const char kPayload[] = "fire fight";
    bool server_connected = false, client_connected = false, delivered = false;
    const Uint32 deadline = SDL_GetTicks() + 5000;
    while (!delivered && !SDL_TICKS_PASSED(SDL_GetTicks(), deadline)) {
      ENetEvent ev;
      while (enet_host_service(client, &ev, 0) > 0) {
        if (ev.type == ENET_EVENT_TYPE_CONNECT && !client_connected) {
          client_connected = true;
          enet_peer_send(peer, 0,
                         enet_packet_create(kPayload, sizeof kPayload, ENET_PACKET_FLAG_RELIABLE));
        }
      }
      while (enet_host_service(server, &ev, 0) > 0) {
        if (ev.type == ENET_EVENT_TYPE_CONNECT) server_connected = true;
        if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
          delivered = ev.packet->dataLength == sizeof kPayload &&
                      std::memcmp(ev.packet->data, kPayload, sizeof kPayload) == 0;
          enet_packet_destroy(ev.packet);
        }
      }
      SDL_Delay(1);
    }
    check(server_connected && client_connected, "ENet loopback handshake");
    check(delivered, "ENet reliable packet delivered");
  }

  if (client) enet_host_destroy(client);
  if (server) enet_host_destroy(server);
  enet_deinitialize();
}

}  // namespace

int main(int argc, char* argv[]) {
  Options opt;
  if (!parse_args(argc, argv, opt)) return 2;

  if (opt.headless) {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
  }

  SDL_version linked;
  SDL_GetVersion(&linked);
  const SDL_version* mixer = Mix_Linked_Version();
  std::printf("SDL %d.%d.%d, SDL_mixer %d.%d.%d, ENet %d.%d.%d, platform %s\n", linked.major,
              linked.minor, linked.patch, mixer->major, mixer->minor, mixer->patch,
              ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH, SDL_GetPlatform());

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER |
               SDL_INIT_GAMECONTROLLER) != 0) {
    check(false, "initialise SDL", SDL_GetError());
    return 1;
  }
  std::printf("       video driver: %s, audio driver: %s, joysticks: %d\n",
              SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver(), SDL_NumJoysticks());

  run_video(opt);
  run_audio(opt);
  run_enet();

  SDL_Quit();
  std::printf("%s (%d failed check%s)\n", g_failures ? "FAILED" : "PASSED", g_failures,
              g_failures == 1 ? "" : "s");
  return g_failures ? 1 : 0;
}
