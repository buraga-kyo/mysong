// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO RATO — src/tui/rato.hpp
// ══════════════════════════════════════════════════════════════════════════
// A tradução de COORDENADA em alvo, e de alvo em GESTO (issue #95). Vive á
// parte da janella pela razão do commando: a janella abre terminal, motor e som
// e não se prova; isto é funcção de valores, e a bateria afere-a em papel.
//
// DOMÍNIO ......... as caixas que o `reflect` do FTXUI encheu no ultimo quadro,
//                   o ponto onde o botão desceu, o botão, o movimento, e o
//                   estado da tela (digitando, eleito, quantas, duração).
// CONTRA-DOMÍNIO .. um Alvo (que peça, que indice, que fracção) e um Gesto.
// INVARIANTE ...... caixa por pintar não casa com ponto algum. O `ftxui::Box`
//                   nasce {0,0,0,0}, e essa caixa CONTÉM o ponto (0,0): quem
//                   nascesse assim casaria com o clique no canto antes do
//                   primeiro quadro. Nasce-se pois VAZIO, e é estructural.
// Q.E.D. .......... sendo as taboadas puras, o rato prova-se em papel.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <vector>

#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>

namespace mysong::tui {

// caixa_por_pintar — a caixa de quem ainda se não pintou. `x_max` menor que
// `x_min` é o vazio que o FTXUI reconhece, e o `Contain` d'elle recusa tudo.
inline ftxui::Box caixa_por_pintar() noexcept { return {0, -1, 0, -1}; }

// As caixas do TRANSPORTE. As metades da barra de progresso guardam-se á parte
// porque o enchimento se pinta em DOUS elementos, o cheio e o vazio: no
// principio e no fim um d'elles tem largura zero e a caixa d'esse sahe vazia.
struct CaixasDoTransporte {
  ftxui::Box anterior = caixa_por_pintar();
  ftxui::Box pausa = caixa_por_pintar();
  ftxui::Box proxima = caixa_por_pintar();
  ftxui::Box barra_cheia = caixa_por_pintar();
  ftxui::Box barra_vazia = caixa_por_pintar();

  // progresso — a barra inteira, união das metades. Não se guarda em campo:
  // campo seria estado em duplicata, e o `reflect` só enche as metades.
  ftxui::Box progresso() const noexcept;
};

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
