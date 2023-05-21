#include <DoorstepDefender/Audio/AudioHandler.h>
#include <mpg123.h>
#include <ao/ao.h>

AudioHandler::AudioHandler(std::string audio_file)
: audio_thread([&]() { this->audio_thread_main(audio_file); }) {
}

AudioHandler::~AudioHandler() {
  terminate();
}

void AudioHandler::play_audio() {
  std::lock_guard<std::mutex> lk(audio_mutex);
  should_play = true;
}

bool AudioHandler::is_plaing() {
  std::lock_guard<std::mutex> lk(audio_mutex);
  return playing;
}

void AudioHandler::terminate() {
  std::lock_guard<std::mutex> lk(audio_mutex);
  should_term = true;
}

void AudioHandler::audio_thread_main(std::string audio_file) {
  mpg123_init();
  mpg123_handle *mh = mpg123_new(nullptr, nullptr);
  mpg123_open(mh, audio_file.c_str());
  
  // Get audio format information
  int channels, encoding;
  long rate;
  mpg123_getformat(mh, &rate, &channels, &encoding);
  
  // Initialize the audio output
  ao_initialize();
  ao_sample_format format;
  format.bits = mpg123_encsize(encoding) * 8;
  format.rate = rate;
  format.channels = channels;
  format.byte_format = AO_FMT_NATIVE;
  format.matrix = 0;
  ao_device *device = ao_open_live(ao_default_driver_id(), &format, NULL);
  
  while (true) {
    bool play = false;
    {
      std::lock_guard<std::mutex> lk(audio_mutex);
      play = should_play;
      should_play = false;
      if (should_term) break;
      playing = true;
    }
    
    if (play) {
      // Play the audio
      uint8_t buffer[1024];
      size_t rd;
      while (mpg123_read(mh, &buffer, sizeof(buffer), &rd) == MPG123_OK) {
        ao_play(device, reinterpret_cast<char*>(buffer), rd);
      }
      
      mpg123_seek(mh, 0, SEEK_SET);
      
      std::lock_guard<std::mutex> lk(audio_mutex);
      playing = false;
    }
  }
  
  // Clean up resources
  mpg123_close(mh);
  mpg123_delete(mh);
  mpg123_exit();
  ao_close(device);
  ao_shutdown();
}
