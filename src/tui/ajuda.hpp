// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA AJUDA — src/tui/ajuda.hpp
// ══════════════════════════════════════════════════════════════════════════
// O HELP (issue #133): a janella que diz TODOS os atalhos que esta Casa
// conhece e explica as côres do espectro. Flutua por cima do corpo, sem lhe
// tocar no estado: fechada, a lista está como estava. Abre-a o segmento HELP
// da fita, o `?` e o F1; fecham-na o Escape, o `?`, o F1, o Enter, o
// Backspace, o `q` e o clique fóra d'ella.
//
// DOMÍNIO ......... a largura e a altura da tela, o estado (aberta, rolada) e
//                   a tecla ou o clique que chega.
// CONTRA-DOMÍNIO .. a TABOADA da ajuda em valores (grupos, teclas, o que
//                   fazem), a LEGENDA do espectro em valores, e o
//                   `ftxui::Element` da janella, com a caixa d'ella.
// INVARIANTE ...... a taboada é FUNCÇÃO PURA e nomeia toda tecla que as
//                   taboadas do commando, do cabeçalho e do foco conhecem: a
//                   bateria percorre-as e procura aqui o rotulo de cada uma.
//                   Aberta, tecla alguma passa ao tocador nem á lista.
// Q.E.D. .......... sendo tudo funcção de valores, a janella pinta-se em écran
//                   de papel e afere-se cella a cella, e a legenda tira as
//                   côres de tui/espectro.hpp, que é d'onde a tela as tira.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "tui/tokens.hpp"

namespace mysong::tui {

// Uma LINHA da taboada: a tecla (ou as teclas, apartadas por dous espaços) e o
// que ella faz, em português de hoje, que é o que o operador lê.
struct LinhaDaAjuda {
  std::string tecla;
  std::string faz;
};

// Um GRUPO: o nome em caixa alta e as suas linhas, na ordem em que se lêem.
struct GrupoDaAjuda {
  std::string nome;
  std::vector<LinhaDaAjuda> linhas;
};

// taboada_da_ajuda — TODOS os atalhos, em grupos: TOCADOR, NAVEGAÇÃO, FAIXA,
// PLAYLISTS, DOWNLOAD, RATO. Funcção pura, e a UNICA verdade do que o HELP
// diz: o README transcreve-a, e a bateria confere-a contra as taboadas.
std::vector<GrupoDaAjuda> taboada_da_ajuda();

// rotulo_da_tecla — o nome por que a taboada chama cada tecla: a lettra
// mesma, «espaço», «Enter», «Esc», «Backspace», «Tab», «Shift+Tab»,
// «Delete», «F1» a «F12», as quatro setas, «Home», «End», «PgUp», «PgDn».
// Vazio para tecla que não tem nome, e é o que a bateria usa para achar cada
// tecla com officio dentro das linhas.
std::string rotulo_da_tecla(const ftxui::Event& tecla);

// Uma AMOSTRA da legenda do espectro: a tinta de verdade, o rotulo em caixa
// alta, a faixa em hertz e o que costuma morar ali.
struct AmostraDaAjuda {
  tokens::Triade tinta;
  std::string rotulo;
  std::string faixa;
  std::string nota;
};

// legenda_do_espectro — a barra violeta, o topo em rosa na batida forte, e os
// quatro registros com os hertz d'elles, SEM côr propria (issue #167): a fita
// tem UMA côr de batida, e é o rosa. Os hertz vêm das fronteiras de
// tui/espectro.hpp: legenda escripta á mão divergiria da tela no dia em que uma
// fronteira mudasse.
std::vector<AmostraDaAjuda> legenda_do_espectro();

// O ESTADO da ajuda: aberta ou não, e quanto se rolou. Valores, e não punho:
// a bateria arma-o á mão.
struct Ajuda {
  bool aberta = false;
  std::size_t rolagem = 0;
};

// A MEDIDA da janella para uma tela dada. As collunhas de conteudo (uma a
// tres, conforme a largura), as linhas UTEIS (as que cabem entre a orla de
// cima e o rodapé), o CONTEUDO (as linhas da collunha mais alta) e o canto.
struct MedidaDaAjuda {
  std::size_t largura = 0;    // da janella inteira, orla incluida
  std::size_t altura = 0;     // idem
  std::size_t collunhas = 0;  // quantas collunhas de conteudo
  std::size_t uteis = 0;      // linhas de conteudo que cabem
  std::size_t conteudo = 0;   // linhas de conteudo que ha
  int x = 0, y = 0;           // o canto de cima á esquerda, na tela
};

// A largura de cada collunha de conteudo: doze cellas de tecla, uma de vão e
// trinta e nove do que faz. Tres d'ellas cabem em cento e sessenta e sete
// collunhas, que é a tela d'elle, com o vão e a orla.
inline constexpr std::size_t LARGURA_DA_COLLUNHA = 52;
inline constexpr std::size_t VAO_ENTRE_COLLUNHAS = 2;
inline constexpr std::size_t MAXIMO_DE_COLLUNHAS = 3;

// medida_da_ajuda — a conta inteira, sem pintar. Tela sem largura ou com menos
// de quatro linhas dá medida vazia, e ahi não ha janella que pôr.
MedidaDaAjuda medida_da_ajuda(std::size_t largura_da_tela,
                              std::size_t altura_da_tela);

// rolagem_maxima — quanto se pode rolar: o conteudo que não cabe, ou zero.
std::size_t rolagem_maxima(const MedidaDaAjuda& medida) noexcept;

// O passo da pagina: quantas linhas o PgUp e o PgDn andam.
inline constexpr std::size_t PASSO_DA_PAGINA = 10;

// alterna_a_ajuda — abre fechada, fecha aberta; abrir começa do alto.
void alterna_a_ajuda(Ajuda& ajuda) noexcept;

// tecla_na_ajuda — com a ajuda ABERTA: fecha (Escape, `?`, F1, Enter,
// Backspace, `q`), rola (setas, `j`/`k`, PgUp/PgDn, Home/End) ou ENGOLE a
// tecla. Devolve VERDADEIRO sempre que a ajuda estava aberta, que é dizer
// «consumida»; fechada, devolve falso e não toca em nada.
bool tecla_na_ajuda(Ajuda& ajuda, const ftxui::Event& tecla,
                    std::size_t rolagem_maxima) noexcept;

// rato_na_ajuda — com a ajuda ABERTA: a roda rola, o clique FÓRA da caixa
// fecha, e o resto engole-se. Devolve verdadeiro quando estava aberta.
bool rato_na_ajuda(Ajuda& ajuda, const ftxui::Box& caixa,
                   const ftxui::Mouse& rato, std::size_t rolagem_maxima) noexcept;

// flutuante_da_ajuda — a janella, para o `dbox` por cima do corpo: vazia com a
// ajuda fechada. `caixa`, não nulla, recebe a caixa da janella, que é a que o
// clique de fóra consulta.
ftxui::Element flutuante_da_ajuda(const Ajuda& ajuda,
                                  std::size_t largura_da_tela,
                                  std::size_t altura_da_tela,
                                  ftxui::Box* caixa = nullptr);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
