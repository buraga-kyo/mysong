// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO FOCO — src/tui/foco.hpp
// ══════════════════════════════════════════════════════════════════════════
// Que peça da tela tem o foco, e para onde a seta o leva (issue #107). As
// setas deixam de voltar e de entrar: ellas ANDAM PELO LAYOUT, e o Enter e o
// Espaço apertam a peça que o foco encontrou, pelo MESMO gesto do clique.
//
// DOMÍNIO ......... as caixas que o `reflect` encheu no ultimo quadro, a peça
//                   corrente e a direcção da seta.
// CONTRA-DOMÍNIO .. a peça que passa a ter o foco, e o Alvo do rato que o
//                   Enter ha de apertar n'ella.
// INVARIANTE ...... o salto NUNCA sahe da tela: não havendo candidata na
//                   direcção, o foco FICA. E caixa por pintar não é candidata
//                   alguma, que peça que se não vê não recebe foco.
// Q.E.D. .......... sendo tudo funcção de valores, a bateria arma a tela em
//                   caixas escriptas á mão e afere cada seta de cada peça.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "tui/rato.hpp"

namespace mysong::tui {

// As peças FOCAVEIS, na ordem em que a tela se lê. A ordem não é enfeite: é
// ella que desempata dous candidatos á mesma distancia, e a PAUTA vem primeiro
// por ser o estado de nascença, que é onde o foco começa.
//
// O nome, o tempo, a chapa e o espectro não entram: elles DIZEM, e não fazem.
// Peça que não responde ao clique não ha de prender o foco pelo caminho.
enum class Focavel {
  Pauta,
  AbaMySong, AbaPlaylists, AbaDownload,
  Tocar, Anterior, Seguinte,
  Volume, Embaralhar, Repetir,
  Trilho,
  Capa,
};

// A direcção da seta. `Nenhuma` é toda tecla que não é seta, e não é queda de
// taboada: é o que faz a tecla seguir ao seu fluxo de sempre.
enum class Direcao { Nenhuma, Cima, Baixo, Esquerda, Dextra };

// rumo_da_tecla — a taboada das quatro setas, e nada mais. O `j` e o `k` não
// entram: elles andam na LISTA, e quem os cumpre é a taboada do commando.
Direcao rumo_da_tecla(const ftxui::Event& tecla) noexcept;

// caixa_da_peca — a caixa de cada peça, colhida das MESMAS que o rato lê. Uma
// só verdade sobre onde a peça está: caixa colhida a parte divergiria da do
// dedo na primeira issue que mudasse a composição.
ftxui::Box caixa_da_peca(const CaixasDaTela& caixas, Focavel qual) noexcept;

// O PESO da travessia. O custo do salto é a distancia na direcção MAIS o dobro
// da distancia de través: assim a seta anda no seu eixo, e sómente sahe d'elle
// quando não ha por onde seguir. Medido em 167 por 67 com peso um: o `↓` do
// botão seguinte cahia no trilho e o do botão anterior cahia na pauta, que é a
// mesma seta a fazer duas cousas em duas collunhas de distancia.
inline constexpr int PESO_DE_TRAVES = 2;

// salto — a peça mais proxima na direcção pedida, pela geometria dos CENTROS
// das caixas, e SÓMENTE entre as que cruzam a corrente no eixo de través: a
// seta anda no seu corredor, que peça posta ACIMA não está á esquerda ainda
// que o centro d'ella caia mais á esquerda.
//
// Sem candidata, devolve a corrente: a tela não tem beira por onde
// o foco caia, nem volta ao principio, que dar a volta faria a seta levar o
// olho ao canto opposto d'onde elle olhava. Peça que SAHIU da tela devolve o
// foco á PAUTA: encolhido o terminal, o painel vae-se com a capa, e foco preso
// n'uma peça sem caixa deixaria as quatro setas mudas.
Focavel salto(const CaixasDaTela& caixas, Focavel corrente,
              Direcao rumo) noexcept;

// alvo_do_foco — a peça com foco DITA em Alvo do rato, para que o Enter e o
// Espaço desaguem na taboada `gesto_do_alvo` sem caminho proprio. Dous
// devolvem `Peca::Nada`, e é de proposito: a PAUTA, cujo Enter toca e cujo
// Espaço pausa pela taboada de sempre, e o TRILHO, cujo clique carrega a
// collunha em que o dedo pousou. Tecla alguma carrega collunha, e buscar o
// segundo zero seria affirmar o principio da faixa por um Enter que ninguem
// pediu.
Alvo alvo_do_foco(Focavel qual) noexcept;

// orla_do_foco — o quadro de glow_core em volta da capa que tem o foco. Vive
// aqui, e não na sala: é pintura do FOCO, e a sala não sabe que ha foco.
ftxui::Element orla_do_foco(ftxui::Element dentro);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
