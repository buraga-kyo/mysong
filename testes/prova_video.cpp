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

TEST_CASE("o soquete leva o pid no nome, e mora na raiz que se disse") {
  const std::filesystem::path posto =
      nu::caminho_do_soquete("/run/user/1000", 4242);
  CHECK(posto.parent_path() == "/run/user/1000");
  // O PID no nome é o que impede duas corridas do mysong de disputarem o mesmo
  // soquete: a segunda ligaria á janella da primeira.
  CHECK(posto.filename() == "mysong-video-4242.sock");
  CHECK(nu::caminho_do_soquete("/run/user/1000", 1).filename() !=
        posto.filename());
}

TEST_CASE("os argumentos declaram a classe da janella nas duas fórmas") {
  const std::vector<std::string> ditos =
      nu::argumentos_do_projector("/acervo/A/B/01 - Tal.mkv", "/run/s.sock");
  const auto tem = [&ditos](const std::string& qual) {
    return std::find(ditos.begin(), ditos.end(), qual) != ditos.end();
  };
  // As DUAS fórmas: o X11 lê uma e o Wayland a outra, e quem corre não sabe qual
  // das duas o systema d'elle usa. É por este nome que o RADICAL-OS a governa.
  CHECK(tem("--x11-name=mysong-video"));
  CHECK(tem("--wayland-app-id=mysong-video"));
  CHECK(tem("--input-ipc-server=/run/s.sock"));
  // SEM terminal: elle é nosso, e a TUI está a pintar n'elle.
  CHECK(tem("--no-terminal"));
  CHECK(tem("--force-window=yes"));
  // O `--` fecha as opções, e a faixa vae DEPOIS d'elle: sem isso, faixa chamada
  // `--version` viraria opção do mpv.
  CHECK(ditos[ditos.size() - 2] == "--");
  CHECK(ditos.back() == "/acervo/A/B/01 - Tal.mkv");
  CHECK(ditos.front() == "mpv");
}

TEST_CASE("o escape do JSON cobre a aspa, a barra e o controle") {
  CHECK(nu::escapa_json("simples") == "simples");
  CHECK(nu::escapa_json("com \"aspa\"") == "com \\\"aspa\\\"");
  CHECK(nu::escapa_json("com \\ barra") == "com \\\\ barra");
  // Byte de controle vae na fórma longa: o JSON não o admitte crú, e nome de
  // arquivo com tabulação existe no disco.
  CHECK(nu::escapa_json("com\ttab") == "com\\u0009tab");
  CHECK(nu::escapa_json("com\nlinha") == "com\\u000alinha");
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
