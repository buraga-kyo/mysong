// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA VIGILIA — src/tui/vigilia.hpp
// ══════════════════════════════════════════════════════════════════════════
// A machina que dorme e acorda o DESENHO (issue #82). A issue #78 mediu que
// repintar n'um painel sem foco arrasta o cursor do painel do OPERADOR
// quarenta vezes por segundo. Perdido o foco, dorme o pedido de repintura e
// NADA mais; tocador, fila, baixas, MPRIS e socket nem sabem d'isto.
//
// DOMÍNIO ......... o foco do painel como o terminal o conta, e a batida.
// CONTRA-DOMÍNIO .. pedir ou não repintura; e UMA ordem de quadro completo.
// INVARIANTE ...... sem noticia alguma pede-se batida SEMPRE, como hoje.
// Q.E.D. .......... escreve o fio da tela; lê o do relogio. Atomicos, e
//                   tranca nenhuma: cada methodo toca UM atomo só.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <atomic>

#include <ftxui/component/event.hpp>

namespace mysong::tui {

class Vigilia {
 public:
  // ganha — os olhos voltaram; vindo de adormecida arma-se o despertar.
  void ganha() {
    if (estado_.exchange(Estado::Desperta) == Estado::Adormecida)
      acordou_.store(true);
  }

  // perde — ninguem olha; o relogio deixa de pedir repintura.
  void perde() { estado_.store(Estado::Adormecida); }

  // pede_batida — o relogio pergunta antes de conferir a assignatura.
  bool pede_batida() const { return estado_.load() != Estado::Adormecida; }

  // acordou — VERDADEIRO uma vez por despertar; consome-se na leitura, como o
  // colheu() do estaleiro: o que mudou dormindo não se pintou.
  bool acordou() { return acordou_.exchange(false); }

  // ha_noticia — se evento de foco algum chegou. A falta declara-se ao sahir.
  bool ha_noticia() const { return estado_.load() != Estado::SemNoticia; }

 private:
  enum class Estado { SemNoticia, Desperta, Adormecida };
  std::atomic<Estado> estado_{Estado::SemNoticia};
  std::atomic<bool> acordou_{false};
};

// O GESTO do foco, tal como o FTXUI v7.0.3 o entrega. O parser d'elle não
// conhece o CSI do modo 1004: o «ESC [ I» chega CRU n'um Event::Special, mas o
// «ESC [ O» cahe no g_uniformize, que o reescreve para «ESC O R» por o tomar
// pelo F3 de terminal velho. Aceitam-se pois as DUAS formas por perda: tecla F
// alguma existe na taboada d'esta Casa, e um F3 physico que adormeça por
// engano desfaz-se na tecla seguinte, que tecla de gente acorda (abaixo).
enum class GestoDoFoco { Alheio, Ganha, Perde };

inline GestoDoFoco gesto_do_foco(const ftxui::Event& evento) {
  if (evento.input() == "\x1b[I") return GestoDoFoco::Ganha;
  if (evento.input() == "\x1b[O" || evento == ftxui::Event::F3)
    return GestoDoFoco::Perde;
  return GestoDoFoco::Alheio;
}

// eh_tecla_de_gente — tecla que terminal e tmux só entregam a painel FOCADO:
// chegar uma é prova de foco, e a vigilia adormecida acorda por ella. É a rede
// de segurança do F3 disfarçado. A lista é o vocabulario inteiro da taboada; o
// Event::Custom fica DE FORA, que é a batida do proprio relogio, e batida que
// acordasse faria o somno impossivel.
inline bool eh_tecla_de_gente(const ftxui::Event& evento) {
  using ftxui::Event;
  if (evento == Event::Custom) return false;
  // O RATO conta (issue #95): terminal e tmux só entregam evento de rato a
  // painel FOCADO, donde chegar um é a mesma prova que uma tecla dá.
  if (evento.is_mouse()) return true;
  if (evento.is_character()) return true;
  return evento == Event::ArrowUp || evento == Event::ArrowDown ||
         evento == Event::ArrowLeft || evento == Event::ArrowRight ||
         evento == Event::Return || evento == Event::Escape ||
         evento == Event::Tab || evento == Event::TabReverse ||
         evento == Event::Backspace || evento == Event::Home ||
         evento == Event::End;
}

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
