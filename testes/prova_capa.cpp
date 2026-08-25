// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA CAPA — testes/prova_capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A busca do arquivo e a chave do cache provam-se sem imagem alguma. O render pelo
// chafa prova-se á mão, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <cstdlib>
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

// A GALERIA e o CACHE, com imagem de verdade. A imagem faz-se aqui: um PNG de
// dezaseis por dezaseis escripto octeto a octeto seria trabalho de mais, e por isso
// se chama o ffmpeg UMA vez e se salta o caso se elle não estiver. O que se afere não
// é o chafa: é a CONTAGEM de conversões.
TEST_CASE("a galeria converte uma vez por album e por tamanho") {
  const Cova cova;
  const std::filesystem::path imagem = cova.raiz() / "cover.png";
  const std::string commando =
      "ffmpeg -y -f lavfi -i color=c=purple:s=64x64 -frames:v 1 '" +
      imagem.string() + "' >/dev/null 2>&1";
  if (std::system(commando.c_str()) != 0 ||
      !std::filesystem::exists(imagem)) {
    WARN("sem ffmpeg: o caso do cache não corre");
    return;
  }

  nu::Galeria galeria;
  const std::filesystem::path uma = cova.raiz() / "01 - Um.mp3";
  const std::filesystem::path outra = cova.raiz() / "02 - Dous.mp3";

  const nu::CapaPintada& primeira = galeria.capa(uma, 10, 5);
  REQUIRE(primeira.achada);
  CHECK(primeira.linhas.size() == 5u);
  CHECK(galeria.quantos_renders() == 1u);

  // A MESMA faixa outra vez: cache, e conversão alguma.
  galeria.capa(uma, 10, 5);
  CHECK(galeria.quantos_renders() == 1u);
  // OUTRA faixa do mesmo album: tambem cache. É a affirmação que a chave carrega.
  galeria.capa(outra, 10, 5);
  CHECK(galeria.quantos_renders() == 1u);
  // Tamanho novo: conversão nova, que a arte tem de encher o painel novo.
  galeria.capa(uma, 12, 6);
  CHECK(galeria.quantos_renders() == 2u);
  // E de volta ao tamanho de antes: cache outra vez.
  galeria.capa(uma, 10, 5);
  CHECK(galeria.quantos_renders() == 2u);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
