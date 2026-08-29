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
    // A JANELLA do video ganha do motor. Estando ella de pé, o motor está calado, e
    // ler o estado d'elle daria Ordem::Nada com a janella a tocar: a tecla não
    // pausava nada, e o operador teclava duas vezes a pensar que falhara.
    if (retracto.video)
      return {retracto.video_pausada ? Verbo::Retomar : Verbo::Pausar, 0.0};
    if (retracto.estado == nucleo::Estado::Tocando) return {Verbo::Pausar, 0.0};
    if (retracto.estado == nucleo::Estado::Pausado) return {Verbo::Retomar, 0.0};
    return {Verbo::Nada, 0.0};  // parado: não ha o que pausar nem retomar
  }
  if (tecla == ftxui::Event::Character('n')) return {Verbo::Proxima, 0.0};
  if (tecla == ftxui::Event::Character('p')) return {Verbo::Anterior, 0.0};
  if (tecla == ftxui::Event::Character('q')) return {Verbo::Sahir, 0.0};

  // A BUSCA passou de `←`/`→` para `,`/`.` (issue #48). O operador tentou voltar nos
  // menus com a seta esquerda e o que ella fazia era buscar no som; navegar por seta é o
  // que a mão espera n'uma arvore de tres degraus, e buscar acha-se em `,` e `.` sem
  // sahir da linha de casa.
  //
  // Havendo janella de video, a busca sahe RELATIVA: d'ella não se sabe a posição
  // sem lhe perguntar pelo soquete e esperar resposta, e o que a tecla quer dizer
  // é «cinco segundos adeante». No motor, absoluta como sempre.
  if (tecla == ftxui::Event::Character('.')) {
    if (retracto.video) return {Verbo::Buscar, PASSO_DA_BUSCA, true};
    return {Verbo::Buscar,
            aparar_busca(retracto.posicao + PASSO_DA_BUSCA, retracto.duracao)};
  }
  if (tecla == ftxui::Event::Character(',')) {
    if (retracto.video) return {Verbo::Buscar, -PASSO_DA_BUSCA, true};
    return {Verbo::Buscar,
            aparar_busca(retracto.posicao - PASSO_DA_BUSCA, retracto.duracao)};
  }

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
  if (tecla == ftxui::Event::Return || tecla == ftxui::Event::ArrowRight)
    return {Verbo::Entra, 0.0};
  if (tecla == ftxui::Event::Escape || tecla == ftxui::Event::Backspace ||
      tecla == ftxui::Event::ArrowLeft)
    return {Verbo::Volta, 0.0};
  if (tecla == ftxui::Event::Character('/')) return {Verbo::AbreBusca, 0.0};
  if (tecla == ftxui::Event::Character('r')) return {Verbo::Varre, 0.0};
  if (tecla == ftxui::Event::Character('b')) return {Verbo::AbreBaixa, 0.0};
  if (tecla == ftxui::Event::Character('l')) return {Verbo::TrocaLetra, 0.0};
  // O `s` de «search»: a busca na REDE, que é differente da busca no que ha. O `/`
  // filtra a lista que está á vista; o `s` pergunta ao YouTube.
  if (tecla == ftxui::Event::Character('s')) return {Verbo::AbreProcura, 0.0};

  // ── As teclas das listas (issue #10) ────────────────────────────────────
  // As MAIUSCULAS são de proposito para as tres que estragam cousa: renomear,
  // apagar e mover. Tecla que muda o que está gravado não ha de ficar debaixo do
  // dedo de quem anda na lista com as minusculas do vi.
  if (tecla == ftxui::Event::Character('P')) return {Verbo::AbreRois, 0.0};
  if (tecla == ftxui::Event::Character('c')) return {Verbo::CriaRol, 0.0};
  if (tecla == ftxui::Event::Character('R')) return {Verbo::RenomeiaRol, 0.0};
  if (tecla == ftxui::Event::Character('D')) return {Verbo::ApagaRol, 0.0};
  if (tecla == ftxui::Event::Character('a')) return {Verbo::JuntaAoRol, 0.0};
  if (tecla == ftxui::Event::Character('t')) return {Verbo::RetiraDoRol, 0.0};
  if (tecla == ftxui::Event::Character('K')) return {Verbo::SobeNoRol, 0.0};
  if (tecla == ftxui::Event::Character('J')) return {Verbo::DesceNoRol, 0.0};

  // O `v` de video. Minuscula porque não estraga cousa gravada: abre janella, e
  // fechá-la não perde nada.
  if (tecla == ftxui::Event::Character('v')) return {Verbo::AbreVideo, 0.0};

  // ── As duas do catalogo do Spotify (issue #13) ───────────────────────────
  // O `I` de importar, e o `T` de todas. MAIUSCULAS as duas: a primeira abre porta
  // de rede, e a segunda encommenda cincoenta baixas de uma vez. Tecla que gasta
  // rede em quantidade não ha de ficar debaixo do dedo de quem anda na lista.
  if (tecla == ftxui::Event::Character('I')) return {Verbo::AbreCatalogo, 0.0};
  if (tecla == ftxui::Event::Character('T')) return {Verbo::BaixaTudo, 0.0};

  // O `f` de fonte (issue #56): cicla de onde a busca vem. Minuscula, que trocar
  // de fonte não estraga cousa gravada; a guarda da secção fica na janella.
  if (tecla == ftxui::Event::Character('f')) return {Verbo::TrocaFonte, 0.0};

  // ── As duas do modo de reprodução (issue #62) ────────────────────────────
  // O `z` é a convenção do ncmpcpp e do mpd, e quem vem de lá não reaprende o
  // dedo; o `x` é o visinho d'elle no teclado, e as duas estavam livres.
  // Minusculas, que trocar de modo não estraga cousa gravada.
  if (tecla == ftxui::Event::Character('z')) return {Verbo::Embaralhar, 0.0};
  if (tecla == ftxui::Event::Character('x')) return {Verbo::Repetir, 0.0};

  return {Verbo::Nada, 0.0};
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
