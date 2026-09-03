// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO CABEÇALHO — src/tui/cabecalho.hpp
// ══════════════════════════════════════════════════════════════════════════
// A linha do alto: as tres abas, os tres botões do transporte, o nome do que
// sôa, e á direita o tempo, o volume e os dous modos, em segmentos powerline.
// Toma o logar da barra lateral: o menu d'esta Casa é fita de abas.
//
// DOMÍNIO ......... o Retracto do instante, a aba corrente e a largura.
// CONTRA-DOMÍNIO .. `ftxui::Element`, e as taboadas puras das teclas.
// INVARIANTE ...... peça alguma se apara ao meio: a que não cabe sahe INTEIRA,
//                   e sómente o nome se corta, com «…». E cada peça enche a
//                   sua caixa, para que o dedo ache o que o olho vê.
// Q.E.D. .......... sendo a linha funcção de valores, a bateria pinta-a em
//                   écran de papel e afere-a cella a cella, sem terminal.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "tui/navegador.hpp"
#include "tui/rato.hpp"
#include "tui/transporte.hpp"

namespace mysong::tui {

// As TRES abas, na ordem em que a fita as põe. São as palavras D'ELLE, e não
// as secções do navegador: ARTISTAS e ÁLBUNS não sobem ao cabeçalho, que a
// linha é uma só e o agrupamento continua a um `o` de distancia.
enum class Aba { MySong, Playlists, Download };

// secao_da_aba — a secção em que cada aba abre: MY SONG é o acervo plano,
// PLAYLISTS é a lista das listas, DOWNLOAD é a busca na rede.
Secao secao_da_aba(Aba aba) noexcept;

// aba_da_secao — o caminho de volta, para a fita accender onde se ESTÁ. Dentro
// de uma lista accende PLAYLISTS, e no catalogo accende DOWNLOAD: são degraus
// de DENTRO da aba, e não salas á parte.
Aba aba_da_secao(Secao secao) noexcept;

// aba_seguinte — o Tab: MY SONG, PLAYLISTS, DOWNLOAD, e torna ao principio.
Aba aba_seguinte(Aba corrente) noexcept;

// vista_seguinte — o `o` dentro das MY SONG: faixas, artistas, albuns, e torna
// ás faixas. Os ÁLBUNS pedem artista na trilha, que a bibliotheca lista os
// albuns D'ELLE e não os do acervo inteiro; sem artista o cyclo salta-os, que
// vista sem chão seria tecla a não fazer nada.
Secao vista_seguinte(Secao corrente, bool ha_artista) noexcept;

// nome_da_vista — a palavra que a chapa diz da vista: FAIXAS, ARTISTAS,
// ÁLBUNS. Vazia fóra das MY SONG, que lá a vista se não cycla.
std::string nome_da_vista(Secao secao);

// Os GESTOS que as teclas do cabeçalho pedem. O Alheio é toda tecla que elle
// não conhece, e quem o recebe deixa-a seguir á taboada de sempre.
enum class GestoDaAba { Alheio, Vai, Cycla, CyclaVista };

struct OrdemDaAba {
  GestoDaAba gesto = GestoDaAba::Alheio;
  Aba aba = Aba::MySong;  // sómente no Vai; nas demais fica no principio
};

// ordem_da_aba — a taboada: `1` `2` `3` vão á aba, o Tab e o Shift+Tab cyclam
// as abas, e o `o` cycla a vista. Nenhuma d'estas teclas estava tomada, e o
// Tab abria a barra que esta issue apaga.
OrdemDaAba ordem_da_aba(const ftxui::Event& tecla) noexcept;

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
