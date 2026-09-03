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
#include <string_view>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "nucleo/letreiro.hpp"
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

// Os tres degraus de pintura de uma aba. O `ComFoco` é da issue irmã das setas
// e nasce aqui por a chapa em XIROD precisar dos tres n'um logar só: a côr da
// chapa e a da cella debaixo d'ella, lidas em dous logares, divergiriam na
// primeira issue que mexesse n'uma.
enum class EstadoDaAba { Apagada, Corrente, ComFoco };

struct PinturaDaAba {
  std::string_view tinta;
  std::string_view fundo;
};

// pintura_da_aba — o par de côres de cada degrau, e o UNICO logar que o diz.
PinturaDaAba pintura_da_aba(EstadoDaAba estado) noexcept;

// palavra_da_aba — a palavra de MARCA sósinha, sem o glifo e sem a guarnição.
// É ella, e sómente ella, que sahe em XIROD: a chapa não cobre o icone nem as
// setas da fita, que aquelle é glifo da fonte do terminal e estas são junção.
std::string palavra_da_aba(Aba aba);

// caixa_da_palavra — as cellas da PALAVRA dentro da caixa do segmento. Tira o
// flanco que o `rotulo_da_aba` põe adeante (o espaço, o glifo, o espaço) e o
// espaço que põe atraz; caixa por pintar, ou segmento sem palavra que sobre,
// responde VAZIA, e ahi o pintor não tem chapa que pôr.
ftxui::Box caixa_da_palavra(const ftxui::Box& segmento) noexcept;

// rotulo_da_aba — a palavra da aba com o seu glifo e a guarnição dos flancos.
// UM logar só, e é de proposito: a chapa em XIROD da issue irmã troca a
// pintura d'esta palavra, e rotulo espalhado por dous ramos dar-lhe-hia duas
// verdades sobre o que a aba diz.
std::string rotulo_da_aba(Aba aba);

// elemento_da_aba — a palavra JÁ PINTADA, corrente ou não. Vive apartada da
// fita pela mesma razão: quem puzer imagem por cima da cella troca aqui, e a
// composição da linha não muda uma linha.
ftxui::Element elemento_da_aba(Aba aba, bool corrente);

// elemento_do_cabecalho — a linha inteira, com a caixa de cada peça. Largura
// zero dá elemento vazio, e nunca quadro roto. Punho nullo nas caixas quer
// dizer «esta chamada não quer saber», e a linha sahe a mesma, cella a cella.
// O `nome` é o que se MOSTRA, e não o caminho que o Retracto carrega: o titulo
// vem da etiqueta do indice, e caminho de arquivo na linha do alto diria a
// pasta do operador em vez de dizer a musica.
ftxui::Element elemento_do_cabecalho(const Retracto& retracto, Aba corrente,
                                     const std::string& nome,
                                     std::size_t largura,
                                     CaixasDoCabecalho* caixas = nullptr);

// A ORDEM que o pintor dá á lousa quanto á chapa de UMA aba. Sahem TRES de
// cada quadro, uma por aba e na ordem da fita, e nunca menos: aba que não tem
// chapa ha de dizer que a não tem, senão a do quadro anterior ficava na tela.
struct ChapaDaAba {
  Aba aba = Aba::MySong;
  EstadoDaAba estado = EstadoDaAba::Apagada;
  bool poe = false;  // falso é o Tira, e é o que o quadro sem caixa pede
  int collunha = 0;
  int linha = 0;
  std::size_t largura = 0;  // em cellas, e é d'ella que a proporção sahe
};

// identidade_da_chapa — o nome por que a lousa conhece a janella de cada aba.
std::string_view identidade_da_chapa(Aba aba) noexcept;

// ordens_das_chapas — a decisão, PURA pelo molde exacto do `ordem_da_capa`: o
// foco entra em TODO quadro, e não sómente no do evento, que o FTXUI desenha
// logo depois de correr os eventos e um tira_tudo no tratador desfaz-se no
// desenho seguinte. O `com_foco` é da issue irmã das setas: punho nullo quer
// dizer que aba alguma o tem, e é o que vale enquanto ella não chega.
std::vector<ChapaDaAba> ordens_das_chapas(const CaixasDoCabecalho& caixas,
                                          Aba corrente, bool letreiro_de_pe,
                                          bool foco_dentro,
                                          const Aba* com_foco = nullptr);

// pedido_da_chapa — o que se manda rasterizar: a palavra, as côres do degrau,
// e a largura em cellas. Aqui se casam a tinta da chapa e a da cella.
nucleo::PedidoDaChapa pedido_da_chapa(const ChapaDaAba& ordem);

// elemento_do_trilho — a linha do progresso, de largura inteira, logo abaixo
// do cabeçalho: v600 no andado e line_dim no que falta. A caixa d'elle é a do
// clique que busca, e a barra do pé morre porque os botões subiram.
ftxui::Element elemento_do_trilho(const Retracto& retracto,
                                  std::size_t largura,
                                  ftxui::Box* caixa = nullptr);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
