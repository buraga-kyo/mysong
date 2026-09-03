// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA ONDA — testes/prova_onda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A conta da envolvente, a chave e o formato do cache, a dobra e a linha em
// écran de PAPEL. O ffmpeg não corre aqui: afere-se o argv que se HA DE
// correr. O caso VIVO dorme sem a variavel MYSONG_PROVA_MP3.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "nucleo/onda.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("a linha de commando do ffmpeg sahe exacta, argumento a argumento") {
  CHECK(nu::linha_de_commando_da_onda("/casa/Musica/faixa.mp3") ==
        std::vector<std::string>{"ffmpeg", "-v", "error", "-nostdin", "-i",
                                 "/casa/Musica/faixa.mp3", "-vn", "-ac", "1",
                                 "-ar", "8000", "-f", "s16le", "pipe:1"});
}
