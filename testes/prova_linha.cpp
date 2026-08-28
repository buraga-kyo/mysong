// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA LINHA DE COMMANDO — testes/prova_linha.cpp
// ══════════════════════════════════════════════════════════════════════════
// Julga a leitura da linha SEM abrir terminal, motor nem som: é para isso que
// a decisão sahiu do main() e foi morar no núcleo. Os casos escrevem á mão o
// que se espera, e nenhum pergunta á obra o que ella acha que devia dar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "nucleo/linha.hpp"

using mysong::nucleo::Invocacao;
using mysong::nucleo::ler_linha;
using mysong::nucleo::Modo;

namespace {

// Monta o argv como o systema o entrega, com o nome do programa em [0]. A
// linha vem por valor: os ponteiros apontam para dentro d'ella, e ella ha de
// viver até a leitura acabar.
Invocacao ler(std::vector<std::string> linha) {
  std::vector<const char*> argv{"mysong"};
  for (const std::string& arg : linha) argv.push_back(arg.c_str());
  return ler_linha(static_cast<int>(argv.size()), argv.data());
}

}  // namespace

TEST_CASE("cada opção longa dá o seu modo, e o dedo escreve as duas formas") {
  CHECK(ler({"--versao"}).modo == Modo::Versao);
  CHECK(ler({"--version"}).modo == Modo::Versao);
  CHECK(ler({"--ajuda"}).modo == Modo::Ajuda);
  CHECK(ler({"--help"}).modo == Modo::Ajuda);
  CHECK(ler({"--sonda"}).modo == Modo::Sonda);
  CHECK(ler({}).modo == Modo::Tocar);
}

TEST_CASE("as faixas entram na ordem em que vieram") {
  const Invocacao duas = ler({"faixa.mp3", "outra.flac"});
  CHECK(duas.modo == Modo::Tocar);
  REQUIRE(duas.faixas.size() == 2);
  CHECK(duas.faixas[0] == "faixa.mp3");
  CHECK(duas.faixas[1] == "outra.flac");
}
