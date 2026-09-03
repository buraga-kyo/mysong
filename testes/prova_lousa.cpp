// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA LOUSA — testes/prova_lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// O protocolo do Überzug++, a conta do rectangulo e a decisão da alavanca,
// tudo sem X11 vivo e sem se erguer processo algum: as tres cousas são
// funcções puras, e é justamente para isto que ellas o são.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "nucleo/capa.hpp"
#include "nucleo/lousa.hpp"

namespace nu = mysong::nucleo;

namespace {

// Um JPEG de cabeçalho só, octeto a octeto: a guarda, um JFIF pelo meio, e o
// SOF0 com setecentos e vinte por mil duzentos e oitenta. Chamar o ffmpeg aqui
// seria trocar a prova da medida por uma prova do ffmpeg.
const std::string& jpeg_de_cabecalho() {
  static const unsigned char kCrus[] = {
      0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F',  'I',  'F',  0,   1, 1,
      0,    0,    1,    0,    1,    0,    0,   0xFF, 0xC0, 0x00, 0x11, 8, 0x02,
      0xD0, 0x05, 0x00, 3,    1,    0x22, 0,   2,    0x11, 1,    3,   0x11, 1};
  static const std::string kOctetos(reinterpret_cast<const char*>(kCrus),
                                    sizeof kCrus);
  return kOctetos;
}

// E um PNG de cabeçalho só: a guarda, e o IHDR com quinhentos por quatrocentos.
const std::string& png_de_cabecalho() {
  static const unsigned char kCrus[] = {
      0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 13,
      'I',  'H', 'D', 'R', 0,    0,    0x01, 0xF4, 0, 0, 1, 0x90};
  static const std::string kOctetos(reinterpret_cast<const char*>(kCrus),
                                    sizeof kCrus);
  return kOctetos;
}

}  // namespace

TEST_CASE("a ordem de pôr traz o rectangulo e o caminho n'uma linha de JSON") {
  CHECK(nu::ordem_de_por("capa", "/tmp/a.jpg", 10, 5, 40, 12) ==
        "{\"action\":\"add\",\"identifier\":\"capa\",\"x\":10,\"y\":5,"
        "\"max_width\":40,\"max_height\":12,\"path\":\"/tmp/a.jpg\"}\n");
}

TEST_CASE("a ordem de tirar nomeia sómente a identidade") {
  CHECK(nu::ordem_de_tirar("capa") ==
        "{\"action\":\"remove\",\"identifier\":\"capa\"}\n");
}

TEST_CASE("aspa no caminho escapa-se e não parte a linha ao meio") {
  const std::string ordem =
      nu::ordem_de_por("capa", "/tmp/o \"melhor\"/a.jpg", 0, 0, 1, 1);
  CHECK(ordem.find("\\\"melhor\\\"") != std::string::npos);
  CHECK(std::count(ordem.begin(), ordem.end(), '\n') == 1);
}

TEST_CASE("a barra invertida e o de controle vão na fórma que a norma pede") {
  CHECK(nu::escapado_em_json("a\\b") == "a\\\\b");
  CHECK(nu::escapado_em_json(std::string("a\x01"
                                         "b")) == "a\\u0001b");
  CHECK(nu::escapado_em_json("Música") == "Música");  // o UTF-8 passa inteiro
}

TEST_CASE("a capa de dezaseis por nove deixa fileiras para o espectro") {
  // A célulla é de nove por vinte pixeis: 1280 por 720 em quarenta collunhas
  // pede 40*720*9 / (1280*20), que é 10,125, e arredonda para onze linhas.
  const nu::Retangulo qual =
      nu::rectangulo_da_capa({1280, 720}, 40, 21, nu::CELLULA_DA_CASA);
  CHECK(qual.collunas == 40);
  CHECK(qual.linhas == 11);
}

TEST_CASE("a capa quadrada em tecto baixo encolhe a largura") {
  // 500 por 500 em quarenta collunhas pediria dezoito linhas, e cabem.
  CHECK(nu::rectangulo_da_capa({500, 500}, 40, 21, nu::CELLULA_DA_CASA).linhas ==
        18);
  // Com tecto de dez linhas, a altura manda: 10*500*20 / (500*9) dá 22,2, que
  // arredonda para vinte e tres collunhas, e a proporção fica guardada.
  const nu::Retangulo baixo =
      nu::rectangulo_da_capa({500, 500}, 40, 10, nu::CELLULA_DA_CASA);
  CHECK(baixo.collunas == 23);
  CHECK(baixo.linhas == 10);
}

TEST_CASE("medida por ler toma o tecto inteiro") {
  const nu::Retangulo qual =
      nu::rectangulo_da_capa({0, 0}, 40, 21, nu::CELLULA_DA_CASA);
  CHECK(qual.collunas == 40);
  CHECK(qual.linhas == 21);
}

TEST_CASE("painel de largura zero não pede rectangulo algum") {
  const nu::Retangulo qual =
      nu::rectangulo_da_capa({1280, 720}, 0, 21, nu::CELLULA_DA_CASA);
  CHECK(qual.collunas == 0);
  CHECK(qual.linhas == 0);
}

TEST_CASE("a medida do JPEG acha o SOF depois do JFIF") {
  const nu::Medida qual = nu::medida_da_imagem(jpeg_de_cabecalho());
  CHECK(qual.largura == 1280);
  CHECK(qual.altura == 720);
}

TEST_CASE("a medida do PNG lê-se do IHDR") {
  const nu::Medida qual = nu::medida_da_imagem(png_de_cabecalho());
  CHECK(qual.largura == 500);
  CHECK(qual.altura == 400);
}

TEST_CASE("octetos que não são imagem medem zero e não lançam") {
  CHECK(nu::medida_da_imagem("nao e imagem alguma").largura == 0);
  CHECK(nu::medida_da_imagem("").altura == 0);
  CHECK(nu::medida_da_imagem("\xFF\xD8truncado").largura == 0);
}

TEST_CASE("a chave do arquivo de capa sahe do conteudo") {
  CHECK(nu::somma_dos_octetos("a").size() == 16);
  CHECK(nu::somma_dos_octetos("a") == nu::somma_dos_octetos("a"));
  CHECK(nu::somma_dos_octetos("a") != nu::somma_dos_octetos("b"));
}

TEST_CASE("a extensão da capa sahe do formato e não do nome") {
  CHECK(nu::extensao_da_capa(jpeg_de_cabecalho()) == "jpg");
  CHECK(nu::extensao_da_capa(png_de_cabecalho()) == "png");
  CHECK(nu::extensao_da_capa("nao e imagem alguma").empty());
}

TEST_CASE("o arquivo da capa mora debaixo do cache do operador") {
  // A variavel repõe-se ao sahir: a bateria corre n'um processo só, e deixar
  // ambiente mudado por traz seria prova a governar prova.
  const char* const antes = std::getenv("XDG_CACHE_HOME");
  const std::string guardado = antes == nullptr ? std::string() : antes;
  ::setenv("XDG_CACHE_HOME", "/tmp/pa-cache-da-prova", 1);
  const std::filesystem::path onde =
      nu::caminho_da_capa_em_cache(jpeg_de_cabecalho());
  CHECK(onde.parent_path() ==
        std::filesystem::path("/tmp/pa-cache-da-prova/mysong/capas"));
  CHECK(onde.extension() == ".jpg");
  CHECK(nu::caminho_da_capa_em_cache("nao e imagem alguma").empty());
  if (antes == nullptr)
    ::unsetenv("XDG_CACHE_HOME");
  else
    ::setenv("XDG_CACHE_HOME", guardado.c_str(), 1);
}

TEST_CASE("a alavanca desligada vence o mundo inteiro") {
  const nu::Parecer qual =
      nu::parecer_da_lousa(nu::ModoDaLousa::Nao, true, true);
  CHECK_FALSE(qual.de_pe);
  CHECK(qual.razao == "desligada pelo ajuste: lousa = nao");
}

TEST_CASE("sem o programa a lousa não se ergue nem por vontade d'elle") {
  CHECK_FALSE(nu::parecer_da_lousa(nu::ModoDaLousa::Auto, true, false).de_pe);
  CHECK_FALSE(nu::parecer_da_lousa(nu::ModoDaLousa::Sim, true, false).de_pe);
}

TEST_CASE("o sim salta a pergunta do DISPLAY e o auto não") {
  CHECK_FALSE(nu::parecer_da_lousa(nu::ModoDaLousa::Auto, false, true).de_pe);
  CHECK(nu::parecer_da_lousa(nu::ModoDaLousa::Sim, false, true).de_pe);
  CHECK(nu::parecer_da_lousa(nu::ModoDaLousa::Auto, true, true).de_pe);
}

TEST_CASE("a linha do diagnostico diz a versão de pé e a razão deitada") {
  const nu::Parecer de_pe = nu::parecer_da_lousa(nu::ModoDaLousa::Auto, true, true);
  CHECK(nu::texto_da_lousa(de_pe, "ueberzugpp 2.9.8") ==
        "\n  lousa: ueberzugpp 2.9.8, X11\n");
  const nu::Parecer deitada =
      nu::parecer_da_lousa(nu::ModoDaLousa::Nao, true, true);
  CHECK(nu::texto_da_lousa(deitada, "ueberzugpp 2.9.8") ==
        "\n  lousa: desligada pelo ajuste: lousa = nao\n");
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
