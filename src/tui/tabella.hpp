// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TABELLA — src/tui/tabella.hpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta o que o Navegador diz: a barra lateral, a tabella do meio com a fatia
// que a rolagem elegeu, e a trilha por titulo. Não decide nada; a decisão toda
// vive no navegador, e é lá que se prova.
//
// DOMÍNIO ......... o Navegador (por leitura), e a largura e a altura em
//                   collunhas e linhas.
// CONTRA-DOMÍNIO .. `ftxui::Element`.
// INVARIANTE ...... não chama ordem alguma do navegador: pinta e sahe. Uma
//                   funcção que pintasse E andasse na lista faria o quadro
//                   depender de quantas vezes se pintou.
// Q.E.D. .......... sendo a pintura funcção do estado, redesenhar não muda cousa
//                   alguma, que é o que um laço a vinte quadros por segundo pede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "nucleo/capa.hpp"
#include "nucleo/letra.hpp"
#include "tui/navegador.hpp"

namespace mysong::tui {

// A barra lateral do mockup. Marca a secção em que se está e, com o FOCO na
// barra (issue #80), marca tambem o degrau sob o dedo: o primeiro espaço do
// rotulo vira «▸» e o fundo é o v900 do eleito da tabella, que é a cor de
// cursor d'esta Casa. Os dous signaes convivem: o v700 diz onde se ESTÁ, o
// «▸» diz onde o dedo aponta e QUEM manda na tecla. Sem foco, os parametros
// novos dormem nos padrões e a pintura é a de sempre, byte por byte.
//
// As CAIXAS (issue #95) enchem-se AQUI, e não na composição da janella: quem
// mover os paineis não move os cliques. Uma por degrau, na ordem em que elles
// se pintam; punho nullo quer dizer «esta chamada não quer saber», e nesse caso
// a pintura sahe a mesma, cella por cella, que o `reflect` se não põe.
ftxui::Element elemento_da_barra(const Navegador& navegador,
                                 bool com_foco = false,
                                 std::size_t degrau_eleito = 0,
                                 std::vector<ftxui::Box>* caixas = nullptr);

// A tabella do meio, com a fatia que cabe em `altura` linhas. `primeira` é o que
// `primeira_a_mostrar` devolveu, e entra por parâmetro para que a pintura não
// guarde estado de rolagem que pudesse divergir da vista.
//
// As CAIXAS (issue #95) são UMA por linha que se pintou, e nunca por linha que
// se não pintou: a altura que sobra abaixo da lista não é alvo de clique algum,
// e vista vazia limpa o vector. Quem lê o vector somma-lhe a `primeira` para ir
// da linha visivel á linha da vista.
ftxui::Element elemento_da_tabella(const Navegador& navegador,
                                   std::size_t primeira, std::size_t altura,
                                   std::size_t largura,
                                   std::vector<ftxui::Box>* caixas = nullptr);

// A LETRA no painel (issue #15). Mostra a linha corrente em destaque, com as
// vizinhas apagadas em volta: `altura` linhas ao todo, e a corrente no meio d'ellas.
// `corrente` menos um quer dizer «antes do primeiro verso», e ahi mostram-se as
// primeiras linhas apagadas, para que o operador veja que ha letra a chegar.
ftxui::Element elemento_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                                 int corrente, std::size_t altura,
                                 std::size_t largura);

// A CAPA no painel (issue #16). Achada, pinta-se linha a linha, com os escapes que o
// chafa produziu passados intactos. Não achada, desenha-se o MARCADOR com os tokens
// d'esta Casa: um buraco não diz nada, e o marcador diz «este album não tem capa».
//
// A CAIXA (issue #95) é UMA, a do quadro inteiro: o clique n'ella pausa e
// retoma. Sem capa que caiba, ella sahe VAZIA, e não a do quadro anterior.
ftxui::Element elemento_da_capa(const nucleo::CapaPintada& capa,
                                std::size_t collunas, std::size_t linhas,
                                ftxui::Box* caixa = nullptr);

// caret_do_campo — a cella de UMA collunha onde o cursor do terminal pousa
// emquanto ha prompt aberto. É o UNICO logar d'esta obra que pede foco, e é de
// proposito: o `Render` do FTXUI elege UM nó focado por quadro e cala os outros
// sem aviso, donde dous pedidos seriam um pedido a perder-se em silencio.
//
// Barra QUIETA, e não a piscar: a queixa que abriu a issue #78 foi «o meu cursor
// fica piscando», e dar-lhe um caret que pisca seria responder á queixa com a
// queixa. O FTXUI usa o foco tambem para rolar dentro de um `frame`; esta obra
// não tem `frame` algum, e quem puser um ha de saber que herda esta linha.
ftxui::Element caret_do_campo();

// elemento_da_trilha — a linha do topo. Digitando-se, leva o caret no fim do
// texto, que é a unica hora em que o cursor ha de apparecer; parada, não pede
// foco algum, e ahi o FTXUI põe `Hidden` e o cursor some. `largura` é a que o
// pintor tem, e a trilha corta-se em `largura - 1` CODEPOINTS, não collunhas,
// para que o caret caiba DENTRO da tela: caret fóra d'ella faria o FTXUI
// mandar deslocamento negativo ao terminal do operador, que é escape mal
// formado. O glypho de duas collunhas (CJK, emoji) conta por um, donde o corte
// deixa passar até o dobro da largura pedida; o que se paga, medido em papel,
// é o caret espremido UMA collunha além da ultima, que a folga de quatro do
// pintor engole. Contar por `string_width` compraria a promessa inteira, mas
// esta linha muda de mãos na tarefa irmã 79; declara-se a divida em vez de a
// pagar duas vezes.
//
// O `sufixo` traz os appensos da linha (a lista alvo, o video, a varredura, o
// aviso da rede, o andamento das baixas). Parada a trilha, elles seguem-na;
// digitando-se, calam-se AQUI, que n'essa hora a linha é o prompt, e appenso
// depois do texto levaria o caret para o fim de palavras que o operador não
// escreveu.
ftxui::Element elemento_da_trilha(const std::string& trilha,
                                  const std::string& sufixo, bool digitando,
                                  std::size_t largura);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
