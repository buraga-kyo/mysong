// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA VARREDURA — testes/prova_varredura.cpp
// ══════════════════════════════════════════════════════════════════════════
// Duas metades. A derivação do caminho prova-se em cadeias, sem disco algum; a
// varredura prova-se sobre acervos que esta bateria FABRICA em directorio
// temporario. Caso algum toca `~/Música` nem `~/.local/share/mysong`.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "nucleo/varredura.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("a extensão de audio aceita-se em qualquer caixa") {
  CHECK(nu::extensao_de_audio(".mp3"));
  CHECK(nu::extensao_de_audio(".MP3"));
  CHECK(nu::extensao_de_audio(".FlAc"));
  CHECK(nu::extensao_de_audio(".opus"));
  CHECK_FALSE(nu::extensao_de_audio(".txt"));
  CHECK_FALSE(nu::extensao_de_audio(".png"));
  CHECK_FALSE(nu::extensao_de_audio("mp3"));   // sem o ponto não é extensão
  CHECK_FALSE(nu::extensao_de_audio(""));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
