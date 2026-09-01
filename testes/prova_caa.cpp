// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO COVER ART ARCHIVE — testes/prova_caa.cpp
// ══════════════════════════════════════════════════════════════════════════
// SEM REDE, do principio ao fim: a Consulta entra de mentira, os corpos são
// recortes escriptos á mão sobre a fórma viva, e as faixas são fixtures
// lavradas em directorio temporario. A UNICA prova viva da issue corre fóra
// da bateria, pelo espia_capa, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>

#include "nucleo/caa.hpp"

namespace nu = mysong::nucleo;

// Nome SEM virgula nem ponto-e-virgula: a virgula quebra o -tc do doctest, e o
// ponto-e-virgula é separador de LISTA do CMake, que partiria a entrada do
// ctest em duas fantasmas que filtram nada e passam vazias. Foi medido aqui.
TEST_CASE("a URL da capa sahe pela release escapada e vazia sem MBID") {
  CHECK(nu::url_da_capa("bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f") ==
        "https://coverartarchive.org/release/"
        "bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f/front-500");
  // MBID não se interpola cru: byte fóra da taboa sahe por cento e hexa. Não
  // é caso do vivo, e é justamente por isso que a guarda se prova aqui.
  CHECK(nu::url_da_capa("a b") ==
        "https://coverartarchive.org/release/a%20b/front-500");
  CHECK(nu::url_da_capa("").empty());
}

TEST_CASE("a taboa dos desfechos do CAA reparte o Falhou pelo estado") {
  using D = nu::DesfechoDaCapa;
  // O que o MB já nomeou passa tal e qual.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Achado, 200) == D::Achada);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Recuo, 503) == D::Recuo);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Recuo, 429) == D::Recuo);
  // O 404 é o CAA a dizer «capa não ha»: definitivo, lembra-se.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 404) == D::SemCapa);
  // Rede muda (estado zero) e 5xx sem recuo respondem amanhã: não se lembram.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 0) == D::Transitoria);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 500) == D::Transitoria);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 403) == D::Transitoria);
}

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
