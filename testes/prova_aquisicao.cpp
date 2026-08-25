// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA AQUISIÇÃO — testes/prova_aquisicao.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. O que se afere é o que se HA DE correr,
// e o que se HA DE gravar: as cinco funcções puras. O `fork` e o `exec` provam-se
// á mão, contra o YouTube, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "nucleo/aquisicao.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("o saneamento tira o que o kernel proscreve, e mais o que engana") {
  // A barra TROCA-SE, e não se apaga: apagar collaria AC/DC em ACDC.
  CHECK(nu::saneia_nome("AC/DC") == "AC-DC");
  CHECK(nu::saneia_nome("a/b/c") == "a-b-c");
  // Ponto inicial faz arquivo occulto, e `..` subiria um degrau.
  CHECK(nu::saneia_nome(".occulto") == "occulto");
  CHECK(nu::saneia_nome("..") == "sem titulo");
  CHECK(nu::saneia_nome("...tres") == "tres");
  // Espaço das pontas sahe; do meio, fica.
  CHECK(nu::saneia_nome("  Tear  ") == "Tear");
  CHECK(nu::saneia_nome("Nota G") == "Nota G");
  // Nome que se reduz a nada tem resposta, e não erro.
  CHECK(nu::saneia_nome("") == "sem titulo");
  CHECK(nu::saneia_nome("   ") == "sem titulo");
  CHECK(nu::saneia_nome("/") == "-");
  // Acento atravessa intacto: elle é valido no ext4.
  CHECK(nu::saneia_nome("Máquina Analítica") == "Máquina Analítica");
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
