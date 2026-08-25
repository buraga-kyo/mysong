// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO PROJECTOR — testes/prova_video.cpp
// ══════════════════════════════════════════════════════════════════════════
// Janella alguma se abre nesta bateria: o que se afere é o que se HA DE correr e o
// que se HA DE mandar. A janella de verdade prova-se á mão, com `pgrep` e `xprop`. E
// dous casos erguem processo, sem abrirem video: medem o processo, não o quadro.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>  // o codigo de erro do remove_all da cova
#include <vector>

#include "nucleo/video.hpp"

namespace nu = mysong::nucleo;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-video-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  const std::filesystem::path& raiz() const { return caminho_; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

TEST_CASE("o juizo do video é pela extensão, e a lista é fechada") {
  CHECK(nu::tem_video("/a/b/filme.mkv"));
  CHECK(nu::tem_video("/a/b/filme.mp4"));
  CHECK(nu::tem_video("/a/b/filme.webm"));
  // A CAIXA não importa: disco tras nome escripto de qualquer modo.
  CHECK(nu::tem_video("/a/b/FILME.MKV"));
  CHECK(nu::tem_video("/a/b/Filme.Mp4"));
  // Audio puro NÃO abre janella. É esta a assertiva que impede o operador de ver
  // janella preta a tocar um mp3.
  CHECK_FALSE(nu::tem_video("/a/b/faixa.mp3"));
  CHECK_FALSE(nu::tem_video("/a/b/faixa.flac"));
  CHECK_FALSE(nu::tem_video("/a/b/faixa.opus"));
  CHECK_FALSE(nu::tem_video("/a/b/faixa.ogg"));
  // Sem extensão alguma tambem não: lista fechada quer dizer fechada.
  CHECK_FALSE(nu::tem_video("/a/b/faixa"));
  CHECK_FALSE(nu::tem_video(""));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
