// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA LIXEIRA — testes/prova_lixeira.cpp
// ══════════════════════════════════════════════════════════════════════════
// Corre inteira em directorio temporario: a raiz da lixeira entra por
// parâmetro, donde prova alguma manda cousa á lixeira de quem nos usa. O que
// ella afere é o PAR da especificação freedesktop: arquivo em `files` e
// bilhete em `info`, com o mesmo nome, e o bilhete a dizer d'onde a faixa veio.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ctime>
#include <string>

#include "nucleo/lixeira.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("o caminho do bilhete escapa o espaço e o acento, e guarda a barra") {
  // O acervo d'elle é todo de espaço e acento: sem o escape, o bilhete sahia
  // quebrado na primeira faixa que se apagasse.
  CHECK(nu::escapa_o_caminho("/tmp/a b/Água.mp3") == "/tmp/a%20b/%C3%81gua.mp3");
  // A barra CONSERVA-SE, que o campo guarda caminho e não nome de arquivo.
  CHECK(nu::escapa_o_caminho("/casa/musica") == "/casa/musica");
  // O til, o ponto, o traço e o sublinhado são livres pela norma.
  CHECK(nu::escapa_o_caminho("a-b_c.d~e") == "a-b_c.d~e");
  CHECK(nu::escapa_o_caminho("50%+1") == "50%25%2B1");
}

TEST_CASE("a data da exclusão sahe em ISO 8601 na hora local") {
  // Pelo mktime, e não por um carimbo escripto á mão: assim a prova diz a
  // mesma cousa em qualquer fuso, que é onde uma comparação crua falharia.
  std::tm partido = {};
  partido.tm_year = 126;  // 2026
  partido.tm_mon = 8;     // setembro
  partido.tm_mday = 3;
  partido.tm_hour = 4;
  partido.tm_min = 5;
  partido.tm_sec = 6;
  partido.tm_isdst = -1;
  CHECK(nu::data_da_exclusao(std::mktime(&partido)) == "2026-09-03T04:05:06");
}
