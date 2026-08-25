// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COMMANDO — src/tui/commando.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada. Uma tecla, uma linha; e tecla que não está na taboada cahe em
// Ordem::Nada, que não chama cousa alguma.
//
// DOMÍNIO ......... a tecla e o Retracto.
// CONTRA-DOMÍNIO .. a Ordem, com o alvo já aparado.
// INVARIANTE ...... funcção pura: não toca tocador, não lê ambiente, não lança.
// Q.E.D. .......... a aparadura entra aqui e não no chamador, donde a tela e o
//                   motor recebem SEMPRE o mesmo numero.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/commando.hpp"

#include <cmath>

namespace mysong::tui {

namespace {

// aparar_busca — o alvo da busca dentro do arco da faixa. Duração que não presta
// dá zero: buscar n'uma faixa cuja duração o mpv ainda não sabe é pedir o
// principio, e não é pedir um numero de sorte.
double aparar_busca(double pedido, double duracao) {
  if (!std::isfinite(duracao) || duracao <= 0.0) return 0.0;
  if (!std::isfinite(pedido) || pedido < 0.0) return 0.0;
  return pedido > duracao ? duracao : pedido;
}

int aparar_volume(int pedido) {
  if (pedido < 0) return 0;
  return pedido > 100 ? 100 : pedido;
}

}  // namespace

Ordem ordem_da_tecla(const ftxui::Event& tecla, const Retracto& retracto,
                     bool digitando) {
  // A GUARDA DO MODO vem PRIMEIRO, antes de toda comparação: assim não ha tecla
  // alguma que se lhe escape por estar declarada acima d'ella.
  if (digitando) return {Verbo::Nada, 0.0};

  // O espaço alterna segundo o ESTADO, e não segundo uma lembrança propria: a
  // tela não guarda estado em duplicata, donde não ha como ella e o motor
  // discordarem sobre quem está a tocar.
  if (tecla == ftxui::Event::Character(' ')) {
    if (retracto.estado == nucleo::Estado::Tocando) return {Verbo::Pausar, 0.0};
    if (retracto.estado == nucleo::Estado::Pausado) return {Verbo::Retomar, 0.0};
    return {Verbo::Nada, 0.0};  // parado: não ha o que pausar nem retomar
  }
  if (tecla == ftxui::Event::Character('n')) return {Verbo::Proxima, 0.0};
  if (tecla == ftxui::Event::Character('p')) return {Verbo::Anterior, 0.0};
  if (tecla == ftxui::Event::Character('q')) return {Verbo::Sahir, 0.0};

  if (tecla == ftxui::Event::ArrowRight)
    return {Verbo::Buscar,
            aparar_busca(retracto.posicao + PASSO_DA_BUSCA, retracto.duracao)};
  if (tecla == ftxui::Event::ArrowLeft)
    return {Verbo::Buscar,
            aparar_busca(retracto.posicao - PASSO_DA_BUSCA, retracto.duracao)};

  if (tecla == ftxui::Event::Character('+'))
    return {Verbo::Volume, static_cast<double>(
                               aparar_volume(retracto.volume + DEGRAU_DO_VOLUME))};
  if (tecla == ftxui::Event::Character('-'))
    return {Verbo::Volume, static_cast<double>(
                               aparar_volume(retracto.volume - DEGRAU_DO_VOLUME))};

  // ── As teclas da navegação (issue #9) ──────────────────────────────────
  if (tecla == ftxui::Event::ArrowDown || tecla == ftxui::Event::Character('j'))
    return {Verbo::Desce, 0.0};
  if (tecla == ftxui::Event::ArrowUp || tecla == ftxui::Event::Character('k'))
    return {Verbo::Sobe, 0.0};
  if (tecla == ftxui::Event::Home || tecla == ftxui::Event::Character('g'))
    return {Verbo::AoPrincipio, 0.0};
  if (tecla == ftxui::Event::End || tecla == ftxui::Event::Character('G'))
    return {Verbo::AoFim, 0.0};
  if (tecla == ftxui::Event::Return) return {Verbo::Entra, 0.0};
  if (tecla == ftxui::Event::Escape || tecla == ftxui::Event::Backspace)
    return {Verbo::Volta, 0.0};
  if (tecla == ftxui::Event::Character('/')) return {Verbo::AbreBusca, 0.0};
  if (tecla == ftxui::Event::Character('r')) return {Verbo::Varre, 0.0};

  return {Verbo::Nada, 0.0};
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
