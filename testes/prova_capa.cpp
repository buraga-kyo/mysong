// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA CAPA — testes/prova_capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A busca do arquivo e a chave do cache provam-se sem imagem alguma. O render pelo
// chafa prova-se á mão, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "nucleo/capa.hpp"

namespace nu = mysong::nucleo;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-capa-" + std::to_string(::getpid()) + "-" +
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
  void poe(const std::string& nome) const {
    std::ofstream(caminho_ / nome) << "nao e imagem, mas existe";
  }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

TEST_CASE("a capa ao lado acha-se pela ordem de preferencia") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  // Pasta sem capa alguma: vazio, e não erro.
  CHECK(nu::capa_ao_lado(faixa).empty());

  // Pondo o menos preferido primeiro, elle sahe, que é o unico que ha.
  cova.poe("album.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "album.jpg");
  // Pondo o `folder`, elle ganha do `album`.
  cova.poe("folder.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "folder.jpg");
  // E o `cover` ganha de todos, que é o que o Picard grava.
  cova.poe("cover.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "cover.jpg");
  // Nome que não está na lista fechada não conta.
  cova.poe("arte.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "cover.jpg");
}

// A CHAVE é a PASTA mais o tamanho, e não a faixa: é ella que faz um album de vinte
// faixas pedir uma conversão, e não vinte.
TEST_CASE("a chave do cache é a pasta e o tamanho, e não a faixa") {
  const std::string uma = nu::chave_do_cache("/a/A/B/01 - Um.mp3", 20, 10);
  const std::string outra = nu::chave_do_cache("/a/A/B/02 - Dous.mp3", 20, 10);
  CHECK(uma == outra);  // mesmo album: a MESMA chave
  // Album differente dá chave differente.
  CHECK(uma != nu::chave_do_cache("/a/A/C/01 - Um.mp3", 20, 10));
  // Tamanho differente tambem, que a arte tem de encher o painel novo.
  CHECK(uma != nu::chave_do_cache("/a/A/B/01 - Um.mp3", 21, 10));
  CHECK(uma != nu::chave_do_cache("/a/A/B/01 - Um.mp3", 20, 11));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
