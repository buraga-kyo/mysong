// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA LINHA DE COMMANDO — testes/prova_linha.cpp
// ══════════════════════════════════════════════════════════════════════════
// Julga a leitura da linha SEM abrir terminal, motor nem som: é para isso que
// a decisão sahiu do main() e foi morar no núcleo. Os casos escrevem á mão o
// que se espera, e nenhum pergunta á obra o que ella acha que devia dar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <sys/wait.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "nucleo/linha.hpp"

using mysong::nucleo::Invocacao;
using mysong::nucleo::ler_linha;
using mysong::nucleo::Modo;
using mysong::nucleo::texto_da_ajuda;
using mysong::nucleo::texto_da_versao;

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

// As duas metades da MESMA guarda: a recusa, e a sahida por onde quem tem
// arquivo de nome torto continua a passar. Provar a primeira sem a segunda
// deixaria de fóra o preço classico de se pôr guarda na linha de commando.
TEST_CASE("o traço duplo encerra as opções, e o traço sozinho é caminho") {
  const Invocacao com_traco = ler({"--", "--arquivo-com-traco.mp3"});
  CHECK(com_traco.modo == Modo::Tocar);
  REQUIRE(com_traco.faixas.size() == 1);
  CHECK(com_traco.faixas[0] == "--arquivo-com-traco.mp3");
  REQUIRE(ler({"-"}).faixas.size() == 1);
  CHECK(ler({"-"}).faixas[0] == "-");
}

TEST_CASE("a opção desconhecida recusa NOMEANDO-a, e nada toca") {
  const Invocacao torta = ler({"--coisa-errada"});
  CHECK(torta.modo == Modo::Recusa);
  CHECK(torta.razao.find("--coisa-errada") != std::string::npos);
  CHECK(torta.razao.find("--ajuda") != std::string::npos);
  CHECK(torta.faixas.empty());
  CHECK(ler({"-h"}).modo == Modo::Recusa);  // opção curta alguma existe
  CHECK(ler({"--versao", "--coisa-errada"}).modo == Modo::Recusa);
}

// O FORMATO, e não a concordancia. Os casos do binario, abaixo, comparam-no
// com a MESMA funcção que elle chama: concordariam ainda que o
// @PROJECT_VERSION@ deixasse de se substituir, e o `mysong @PROJECT_VERSION@`
// passaria verde. Aqui escreve-se á mão o que se ha de VER.
TEST_CASE("o numero tem forma de numero, e a ajuda nomeia o que existe") {
  const std::string dito = texto_da_versao();
  REQUIRE(dito.rfind("mysong ", 0) == 0);
  REQUIRE(dito.size() > 7);
  CHECK(std::isdigit(static_cast<unsigned char>(dito[7])));

  const std::string ajuda = texto_da_ajuda();
  CHECK(ajuda.find("--versao") != std::string::npos);
  CHECK(ajuda.find("--ajuda") != std::string::npos);
  CHECK(ajuda.find("--sonda") != std::string::npos);
  CHECK(ajuda.find("\n  -- ") != std::string::npos);
}

// ── O BINARIO, e não a bibliotheca: o que a issue #66 promette é o que o
// programa ESCREVE e o codigo com que elle SAHE, e afere-se correndo-o.
namespace {

std::string colher(const std::string& commando, int* codigo) {
  std::string colhido;
  FILE* cano = ::popen(commando.c_str(), "r");
  if (cano == nullptr) return colhido;
  char pedaco[256];
  while (std::fgets(pedaco, sizeof pedaco, cano) != nullptr) colhido += pedaco;
  *codigo = WEXITSTATUS(::pclose(cano));
  return colhido;
}
const std::string BINARIO = "'" MYSONG_BINARIO "'";

}  // namespace

TEST_CASE("o binario diz o nome e o numero, e sahe com zero") {
  int codigo = -1;
  CHECK(colher(BINARIO + " --versao", &codigo) == texto_da_versao());
  CHECK(codigo == 0);
}

TEST_CASE("a versão sahe ainda que falte o requisito que impede o tocador") {
  int codigo = -1;  // a falta força-se pela chave da sonda, sem tocar o systema
  CHECK(colher("MYSONG_SONDA_FORCA=fonte " + BINARIO + " --versao", &codigo) ==
        texto_da_versao());
  CHECK(codigo == 0);
}

TEST_CASE("o binario escreve a ajuda, e sahe com zero") {
  int codigo = -1;
  CHECK(colher(BINARIO + " --ajuda", &codigo) == texto_da_ajuda());
  CHECK(codigo == 0);
}

// A queixa vae ao stderr: quem encana o mysong não a recebe no que pediu.
TEST_CASE("o binario recusa a opção torta pelo stderr, e sahe com dous") {
  int codigo = -1;
  CHECK(colher(BINARIO + " --coisa-errada 2>/dev/null", &codigo).empty());
  CHECK(codigo == 2);
  CHECK(colher(BINARIO + " --coisa-errada 2>&1", &codigo).find(
            "--coisa-errada") != std::string::npos);
}
