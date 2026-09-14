// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA VIGILIA — testes/prova_vigilia.cpp
// ══════════════════════════════════════════════════════════════════════════
// A machina de dormir e acordar o desenho (issue #82), sem terminal: somno
// cala a batida, despertar arma o quadro completo UMA vez que se consome.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "tui/vigilia.hpp"

namespace tui = mysong::tui;

using ftxui::Event;
using Gesto = tui::GestoDoFoco;

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

TEST_CASE("o gesto do foco na vigilia lê o escape cru e o disfarce de F3") {
  CHECK(tui::gesto_do_foco(Event::Special("\x1b[I")) == Gesto::Ganha);
  // O ESC [ O jamais chega cru do parser do FTXUI (vira F3 no g_uniformize),
  // mas aceita-se tambem: se um dia o parser sarar, nada aqui quebra.
  CHECK(tui::gesto_do_foco(Event::Special("\x1b[O")) == Gesto::Perde);
  CHECK(tui::gesto_do_foco(Event::F3) == Gesto::Perde);
  CHECK(tui::gesto_do_foco(Event::Custom) == Gesto::Alheio);
  CHECK(tui::gesto_do_foco(Event::Character("j")) == Gesto::Alheio);
  CHECK(tui::gesto_do_foco(Event::Return) == Gesto::Alheio);
}

TEST_CASE("tecla de gente na vigilia é a taboada e a batida não é gente") {
  CHECK(tui::eh_tecla_de_gente(Event::Character("j")));
  CHECK(tui::eh_tecla_de_gente(Event::ArrowDown));
  CHECK(tui::eh_tecla_de_gente(Event::Tab));
  CHECK(tui::eh_tecla_de_gente(Event::Escape));
  CHECK_FALSE(tui::eh_tecla_de_gente(Event::Custom));  // a batida do relogio
  CHECK_FALSE(tui::eh_tecla_de_gente(Event::Special("\x1b[I")));
  CHECK_FALSE(tui::eh_tecla_de_gente(Event::F3));  // perda disfarçada, não dedo
}

TEST_CASE("a vigilia guarda o despertar atravez de dormir entre batidas") {
  tui::Vigilia v;
  v.perde();
  v.ganha();
  v.perde();  // troca rapida de paineis: o relogio nem chegou a bater
  CHECK(v.acordou());            // a ordem do quadro completo ficou armada
  CHECK_FALSE(v.pede_batida());  // mas dormindo não se pinta
  v.ganha();
  CHECK(v.acordou());  // desperto de vez, o quadro completo sahe
  CHECK(v.pede_batida());
}

TEST_CASE("a vigilia configurada conserva a batida sem foco") {
  tui::Vigilia v(true);
  v.perde();
  CHECK(v.pede_batida());
  CHECK(v.ha_noticia() == false);
}

TEST_CASE("a trava manual congela e volta a liberar a animação") {
  tui::Vigilia v(true);
  CHECK(v.alterna_trava());
  CHECK(v.animacao_travada());
  CHECK_FALSE(v.pede_batida());
  CHECK_FALSE(v.alterna_trava());
  CHECK_FALSE(v.animacao_travada());
  CHECK(v.pede_batida());
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
