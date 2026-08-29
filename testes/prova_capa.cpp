// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA CAPA — testes/prova_capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A busca do arquivo e a chave do cache provam-se sem imagem alguma. O render pelo
// chafa prova-se á mão, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <unistd.h>

#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

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

// Um PNG de um pixel, escripto octeto a octeto. Chamar o ffmpeg aqui seria trocar a
// prova da capa embutida por uma prova do ffmpeg: a issue #81 pede que ella corra sem
// ferramenta alheia e sem rede, e é este arranjo que o cumpre.
const std::string& png_de_um_pixel() {
  static const unsigned char kCrus[] = {
      0x89, 'P',  'N',  'G',  0x0D, 0x0A, 0x1A, 0x0A, 0,    0,    0,    13,
      'I',  'H',  'D',  'R',  0,    0,    0,    1,    0,    0,    0,    1,
      8,    2,    0,    0,    0,    0x90, 0x77, 0x53, 0xDE, 0,    0,    0,
      12,   'I',  'D',  'A',  'T',  0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00,
      0x00, 0x03, 0x01, 0x01, 0x00, 0x18, 0xDD, 0x8D, 0xB0, 0,    0,    0,
      0,    'I',  'E',  'N',  'D',  0xAE, 0x42, 0x60, 0x82};
  static const std::string kPng(reinterpret_cast<const char*>(kCrus), sizeof kCrus);
  return kPng;
}

// lavra_a_etiqueta — põe um APIC na faixa, pela MESMA taglib com que a obra o lê.
// O corpo do arquivo é um quadro de MPEG minimo: basta para a taglib o aceitar, e
// faixa que toque não é o que este caso afere.
bool lavra_a_etiqueta(const std::filesystem::path& faixa, const std::string& arte) {
  { std::ofstream(faixa, std::ios::binary) << "\xFF\xFB\x90\x00"; }
  TagLib::MPEG::File arquivo(faixa.c_str());
  if (!arquivo.isValid()) return false;
  auto* quadro = new TagLib::ID3v2::AttachedPictureFrame();
  quadro->setMimeType("image/png");
  quadro->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
  quadro->setPicture(
      TagLib::ByteVector(arte.data(), static_cast<unsigned>(arte.size())));
  arquivo.ID3v2Tag(true)->addFrame(quadro);
  return arquivo.save();
}

}  // namespace

// A CAPA EMBUTIDA. Este caminho existe desde a issue #44 e nunca teve prova: andou
// certo por sorte. Corre sem chafa e sem rede, que é o que a issue #81 pede.
TEST_CASE("a capa embutida lê-se da etiqueta lavrada á mão") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  const std::string arte = png_de_um_pixel();
  REQUIRE(lavra_a_etiqueta(faixa, arte));
  // Os MESMOS octetos voltam. Aferir sómente que «veio alguma cousa» deixaria passar
  // uma leitura que truncasse a arte, e arte truncada o chafa recusa.
  CHECK(nu::arte_embutida(faixa) == arte);
}

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

// A ORDEM, provada e não sómente declarada. Os dous estão presentes, e a etiqueta
// leva arte PODRE de proposito: ganhando ella, o chafa engasgaria e `achada` viria
// falso. Vindo verdadeiro, foi o arquivo ao lado que se tomou, que é a ordem que o
// cabeçalho promette.
TEST_CASE("com capa ao lado e na etiqueta ganha a do lado") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  REQUIRE(lavra_a_etiqueta(faixa, "isto não é imagem alguma"));
  REQUIRE_FALSE(nu::arte_embutida(faixa).empty());

  const std::filesystem::path imagem = cova.raiz() / "cover.png";
  const std::string commando =
      "ffmpeg -y -f lavfi -i color=c=teal:s=64x64 -frames:v 1 '" +
      imagem.string() + "' >/dev/null 2>&1";
  if (std::system(commando.c_str()) != 0 || !std::filesystem::exists(imagem)) {
    WARN("sem ffmpeg: o caso da preferencia não corre");
    return;
  }
  nu::Galeria galeria;
  CHECK(galeria.capa(faixa, 10, 5).achada);
}

// O `.mkv` do video, e todo arquivo que a taglib não leia como MP3. Medido: ella dá
// `isValid()` verdadeiro e etiqueta NÃO nula para todos estes, que a cria a pedido;
// o que vem vazio é a lista de quadros. Sahem quietos, e é isso que segura o pintor
// contra uma excepção quando o operador põe um video no acervo.
TEST_CASE("arquivo que não é mp3 não dá capa alguma nem lança") {
  const Cova cova;
  cova.poe("video.mkv");
  cova.poe("lixo.mp3");
  std::ofstream(cova.raiz() / "vazio.mp3", std::ios::binary);
  CHECK(nu::arte_embutida(cova.raiz() / "video.mkv").empty());
  CHECK(nu::arte_embutida(cova.raiz() / "lixo.mp3").empty());
  CHECK(nu::arte_embutida(cova.raiz() / "vazio.mp3").empty());
  // E o que nem existe sahe quieto tambem, que o pintor pergunta por faixa que o
  // acervo pode ter perdido entre a varredura e o quadro.
  CHECK(nu::arte_embutida(cova.raiz() / "nao-existe.mp3").empty());
}

TEST_CASE("album sem capa devolve a ausencia, e guarda-a") {
  const Cova cova;
  nu::Galeria galeria;
  const std::filesystem::path faixa = cova.raiz() / "01 - Um.mp3";
  const nu::CapaPintada& nada = galeria.capa(faixa, 10, 5);
  CHECK_FALSE(nada.achada);
  CHECK(nada.linhas.empty());
  CHECK(galeria.quantos_renders() == 0u);
  // E pedir outra vez não volta ao disco: a ausencia tambem está guardada. Prova-se
  // pondo a capa AGORA: estando a ausencia em cache, ella não se vê.
  cova.poe("cover.jpg");
  CHECK_FALSE(galeria.capa(faixa, 10, 5).achada);
}

// A ANALYSE do SGR, contra linhas escriptas á mão com a fórma que o chafa produz.
// Foi este parser que curou o defeito da largura, e é aqui que elle se prende.
TEST_CASE("o SGR do chafa parte-se em corridas com as suas côres") {
  // Uma linha de verdade, copiada da sahida do chafa e escripta aqui á mão.
  const std::string linha =
      "\x1b[0m\x1b[38;2;167;0;167;48;2;173;0;173m\u2580"
      "\x1b[38;2;163;0;163;48;2;170;0;170m\u2580\x1b[0m";
  const std::vector<nu::Corrida> corridas = nu::analysa_sgr(linha);
  REQUIRE(corridas.size() == 2u);
  CHECK(corridas[0].texto == "\u2580");
  CHECK(corridas[0].r_frente == 167);
  CHECK(corridas[0].g_frente == 0);
  CHECK(corridas[0].b_frente == 167);
  CHECK(corridas[0].r_fundo == 173);
  CHECK(corridas[1].texto == "\u2580");
  CHECK(corridas[1].r_frente == 163);
  CHECK(corridas[1].r_fundo == 170);

  // O `39` e o `49` apagam a côr, e é assim que o chafa diz «sem côr».
  const std::vector<nu::Corrida> apagada =
      nu::analysa_sgr("\x1b[38;2;1;2;3m a \x1b[39m\x1b[49m b ");
  REQUIRE(apagada.size() == 2u);
  CHECK(apagada[0].texto == " a ");
  CHECK(apagada[0].r_frente == 1);
  CHECK(apagada[1].texto == " b ");
  CHECK(apagada[1].r_frente == -1);
  CHECK(apagada[1].r_fundo == -1);

  // Texto sem escape algum dá UMA corrida sem côr.
  const std::vector<nu::Corrida> nua = nu::analysa_sgr("abc");
  REQUIRE(nua.size() == 1u);
  CHECK(nua[0].texto == "abc");
  CHECK(nua[0].r_frente == -1);
  // Linha vazia dá corrida alguma, e não uma corrida vazia.
  CHECK(nu::analysa_sgr("").empty());
  // Escape que não se conheça ignora-se, e o texto passa.
  const std::vector<nu::Corrida> alheio = nu::analysa_sgr("\x1b[7mx");
  REQUIRE(alheio.size() == 1u);
  CHECK(alheio[0].texto == "x");
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
