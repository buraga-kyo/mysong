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

#include <string>

#include "nucleo/libmpv.hpp"

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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
