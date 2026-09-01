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

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
