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

#include "tui/foco.hpp"
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
// ás faixas. Os ÁLBUNS pedem artista A QUE DESCER, que a bibliotheca lista os
// albuns D'ELLE e não os do acervo inteiro: dos ARTISTAS entra-se no eleito, e
// é essa a terceira vista. Não havendo eleito, o cyclo salta-a, que vista sem
// chão seria tecla a não fazer nada.
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

// O ESTADO em que cada aba se pinta. Tres, e não dous booleanos ao lado: dous
// admittiriam o estado «corrente e apagada», que não existe. A irmã do letreiro
// (issue #108) lê este enum para saber que chapa em XIROD ha de pôr sobre a
// palavra, e é por isso que elle vive aqui e não dentro do pintor.
enum class EstadoDaAba { Apagada, Corrente, ComFoco };

// estado_da_aba — o estado de uma aba, dada a corrente e a peça com foco. O
// FOCO GANHA da corrente: quem anda com as setas ha de ver ONDE está a mão, e
// a aba corrente diz-se tambem pela chapa por cima da pauta.
EstadoDaAba estado_da_aba(Aba qual, Aba corrente, Focavel foco) noexcept;

// rotulo_da_aba — a palavra da aba com o seu glifo e a guarnição dos flancos.
// UM logar só, e é de proposito: a chapa em XIROD da issue irmã troca a
// pintura d'esta palavra, e rotulo espalhado por dous ramos dar-lhe-hia duas
// verdades sobre o que a aba diz.
std::string rotulo_da_aba(Aba aba);

// elemento_da_aba — a palavra JÁ PINTADA, corrente ou não. Vive apartada da
// fita pela mesma razão: quem puzer imagem por cima da cella troca aqui, e a
// composição da linha não muda uma linha.
ftxui::Element elemento_da_aba(Aba aba, EstadoDaAba estado);

// elemento_do_cabecalho — a linha inteira, com a caixa de cada peça. Largura
// zero dá elemento vazio, e nunca quadro roto. Punho nullo nas caixas quer
// dizer «esta chamada não quer saber», e a linha sahe a mesma, cella a cella.
// O `nome` é o que se MOSTRA, e não o caminho que o Retracto carrega: o titulo
// vem da etiqueta do indice, e caminho de arquivo na linha do alto diria a
// pasta do operador em vez de dizer a musica.
// O `foco` diz que PEÇA d'esta linha tem o foco (issue #107): ella veste-se de
// glow_core com texto panel, que é par distincto do da aba corrente (v600 com
// v50) sem sahir da familia. `Focavel::Pauta`, que é o padrão, quer dizer que o
// foco está fóra do cabeçalho, e ahi a linha sahe a mesma, cella a cella.
ftxui::Element elemento_do_cabecalho(const Retracto& retracto, Aba corrente,
                                     const std::string& nome,
                                     std::size_t largura,
                                     CaixasDoCabecalho* caixas = nullptr,
                                     Focavel foco = Focavel::Pauta);

// elemento_do_trilho — a linha do progresso, de largura inteira, logo abaixo
// do cabeçalho: v600 no andado e line_dim no que falta. A caixa d'elle é a do
// clique que busca, e a barra do pé morre porque os botões subiram.
// `com_foco` accende o andado em glow_core no logar do v600 (issue #107): o
// trilho é peça focavel como as outras, e o que elle tem para accender é o que
// já anda pintado.
ftxui::Element elemento_do_trilho(const Retracto& retracto,
                                  std::size_t largura,
                                  ftxui::Box* caixa = nullptr,
                                  bool com_foco = false);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
