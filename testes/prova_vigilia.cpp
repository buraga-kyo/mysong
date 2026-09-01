// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA VIGILIA — testes/prova_vigilia.cpp
// ══════════════════════════════════════════════════════════════════════════
// A machina de dormir e acordar o desenho (issue #82), sem terminal: somno
// cala a batida, despertar arma o quadro completo UMA vez que se consome.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "tui/vigilia.hpp"

namespace tui = mysong::tui;

TEST_CASE("a vigilia nasce sem noticia e pedindo batida como hoje") {
  tui::Vigilia v;
  CHECK(v.pede_batida());
  CHECK_FALSE(v.ha_noticia());
  CHECK_FALSE(v.acordou());
}

TEST_CASE("a vigilia adormece ao perder o foco e conta a noticia") {
  tui::Vigilia v;
  v.perde();
  CHECK_FALSE(v.pede_batida());
  CHECK(v.ha_noticia());
  CHECK_FALSE(v.acordou());  // dormir não é despertar
}

TEST_CASE("a vigilia acordada do somno arma o quadro completo que se consome") {
  tui::Vigilia v;
  v.perde();
  v.ganha();
  CHECK(v.pede_batida());
  CHECK(v.acordou());
  CHECK_FALSE(v.acordou());  // consumida na leitura, como o colheu
}

TEST_CASE("a vigilia que ganha sem somno previo não arma despertar") {
  tui::Vigilia v;
  v.ganha();
  CHECK_FALSE(v.acordou());  // a tela nunca dormiu: nada ha a repintar
  v.ganha();  // já desperta: ganhar outra vez é inerte
  CHECK_FALSE(v.acordou());
  CHECK(v.ha_noticia());
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
