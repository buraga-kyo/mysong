// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA TABOA DA LIBMPV — testes/prova_libmpv.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova os modos de FALHAR do carregamento, e não só o caminho felix. Faz-se por
// carregar_taboa, e nunca por libmpv(), porque aquella se repete e esta abre uma
// vez por processo: prova que só sabe julgar o caminho felix é confirmação.
//
// Os alvos esperados escrevem-se Á MÃO aqui: o soname e o nome do symbolo.
// Nenhum caso pergunta á obra o que esperar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "nucleo/libmpv.hpp"
#include "nucleo/sonda.hpp"

using mysong::nucleo::carregar_taboa;
using mysong::nucleo::TaboaDaLibmpv;

TEST_CASE("a taboa ata os treze symbolos da libmpv desta machina") {
  TaboaDaLibmpv taboa;
  std::string razao = "(não tocada)";
  REQUIRE(carregar_taboa("libmpv.so.2", &taboa, &razao));
  CHECK(razao == "(não tocada)");
#define MYSONG_CONFERE(nome) CHECK(taboa.nome != nullptr);
  MYSONG_LIBMPV_FUNCCOES(MYSONG_CONFERE)
#undef MYSONG_CONFERE
}

TEST_CASE("soname que não existe recusa NOMEANDO o soname") {
  TaboaDaLibmpv taboa;
  std::string razao;
  CHECK_FALSE(carregar_taboa("libmpv-que-nao-existe.so.99", &taboa, &razao));
  CHECK(razao.find("libmpv-que-nao-existe.so.99") != std::string::npos);
}

// A bibliotheca existe e resolve, e não traz symbolo algum do mpv: é o caminho
// do dlsym, e não o do dlopen. Doze de treze subiria e morreria no primeiro uso
// do que faltou, e por isso a recusa tem de nomear o SYMBOLO.
TEST_CASE("bibliotheca sem os symbolos recusa NOMEANDO o symbolo") {
  TaboaDaLibmpv taboa;
  std::string razao;
  CHECK_FALSE(carregar_taboa("libm.so.6", &taboa, &razao));
  CHECK(razao.find("mpv_create") != std::string::npos);
}

TEST_CASE("a forçagem da sonda nomeia a chave da libmpv") {
  ::setenv("MYSONG_SONDA_FORCA", "libmpv", 1);
  CHECK(mysong::nucleo::nomeado_na_forcagem("libmpv"));
  ::unsetenv("MYSONG_SONDA_FORCA");
  CHECK_FALSE(mysong::nucleo::nomeado_na_forcagem("libmpv"));
}

namespace {

// Colhe a sahida inteira do commando. LC_ALL=C porque as ferramentas do binutils
// traduzem os seus rotulos, e prova que dependa do idioma da machina não é prova.
// Sahida vazia é FALHA nomeada no caso, e nunca caso saltado.
std::string colher(const std::string& commando) {
  std::string colhido;
  FILE* cano = ::popen(("LC_ALL=C " + commando + " 2>/dev/null").c_str(), "r");
  if (cano == nullptr) return colhido;
  char pedaco[512];
  while (std::fgets(pedaco, sizeof pedaco, cano) != nullptr) colhido += pedaco;
  ::pclose(cano);
  return colhido;
}

}  // namespace

// A PROVA DA LIGAÇÃO. Lê o BINÁRIO produzido, e nunca o CMakeLists nem a si
// mesma: a bateria não linka como o binario linka, e é essa differença que a
// issue #28 existe para apanhar. Duas metades, e ambas necessarias.
TEST_CASE("o binario da tela traz o motor e NÃO liga a libmpv") {
  const std::string binario(MYSONG_BINARIO);

  // Metade um: o motor ESTÁ no binario. Sem ella, a metade dous passaria por
  // ausencia de motor, que é o falso verde que esta prova existe para matar.
  const std::string symbolos = colher("nm -C '" + binario + "'");
  REQUIRE_MESSAGE(!symbolos.empty(), "binario sem taboa de symbolos");
  CHECK(symbolos.find("MotorMpv") != std::string::npos);

  // Metade dous: a libmpv NÃO é dependencia de ligação. Reintroduzida, este
  // binario morreria no carregador antes do main na machina sem ella, e a tela
  // das faltas da issue #22 nunca appareceria.
  const std::string dynamica = colher("readelf -d '" + binario + "'");
  REQUIRE_MESSAGE(dynamica.find("NEEDED") != std::string::npos, "sem taboa dynamica");
  CHECK(dynamica.find("mpv") == std::string::npos);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
