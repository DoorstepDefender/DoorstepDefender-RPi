#pragma once

#include <string>
#include <thread>
#include <mutex>

class AudioHandler {
public:
  AudioHandler(std::string audio_file);
  ~AudioHandler();
  
  void play_audio();
  
  bool is_plaing();

  void terminate();
  
private:
  std::thread audio_thread;
  std::mutex audio_mutex;
  
  bool should_term = false;
  bool should_play = false;
  bool playing = false;
  
  void audio_thread_main(std::string audio_file);
};
