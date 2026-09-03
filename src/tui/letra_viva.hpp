// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA VIVA — src/tui/letra_viva.hpp
// ══════════════════════════════════════════════════════════════════════════
// O RIO da letra (issue #109). A letra deixa de se alternar com o espectro e
// passa a morar POR CIMA d'elle: cada linha assoma na base do painel ainda sem
// fórma, sobe até a LINHA DE LEITURA chegando lá no instante em que se canta, e
// d'ahi segue subindo, perdendo luz, até morrer na linha zero.
//
// DOMÍNIO ......... as linhas do `.lrc` com os instantes d'ellas, a posição da
//                   faixa em segundos, e a largura e a altura do rectangulo do
//                   espectro em CÉLULLAS.
// CONTRA-DOMÍNIO .. um QUADRO DA LETRA: de cada linha á vista, a linha da tela
//                   em que assenta, a collunha, o texto com a fracção de
//                   glyphos já resolvidos, e a tinta.
// INVARIANTE ...... funcção PURA, e sem relogio proprio. A mesma posição dá o
//                   MESMO quadro, e é assim que a bateria o interroga sem
//                   terminal, sem som e sem espera.
// Q.E.D. .......... letra alguma se inventa: faixa sem `.lrc` dá quadro vazio,
//                   e o painel mostra o espectro como se esta obra não houvesse.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "nucleo/letra.hpp"
#include "nucleo/letreiro.hpp"  // PedidoDaChapa: a linha corrente em XIROD
#include "tui/espectro.hpp"
#include "tui/sala.hpp"
#include "tui/tokens.hpp"

namespace mysong::tui {

// linha_de_leitura — o TERÇO do alto onde a linha chega no instante d'ella. Um
// terço, e não o meio: o que vem tem de caber por baixo, que é o vão da subida,
// e o que já passou por cima, que é o vão do apagar.
std::size_t linha_de_leitura(std::size_t altura);


// O NASCIMENTO. Quatro segundos antes do instante d'ella a linha assoma na
// base, ou o INTERVALLO desde a anterior quando elle é menor: letra apressada,
// de duas linhas no mesmo segundo, não ha de ter duas a subir sobrepostas.
inline constexpr double NASCIMENTO_MAXIMO = 4.0;

// E um PISO, que não é gosto: é a guarda da divisão. Carimbo repetido dá
// intervallo zero, e zero no denominador da subida daria fracção infinita. Um
// quarto de segundo faz a linha saltar da base á leitura, que é o que uma letra
// de carimbos eguaes de facto pede.
inline constexpr double NASCIMENTO_MINIMO = 0.25;

// nascimento_da_linha — quantos segundos ANTES do instante d'ella a linha
// `qual` assoma na base. A primeira conta o intervallo desde o zero da faixa,
// que é o instante anterior que ella tem: regra UNA para todas, e não um caso á
// parte que a bateria teria de provar duas vezes.
double nascimento_da_linha(const std::vector<nucleo::LinhaDaLetra>& linhas,
                           std::size_t qual);

// Uma LINHA do rio, já resolvida para a tela. `qual` é o indice d'ella na letra,
// e não no quadro: a irmã que ha de cristalizar a corrente em XIROD precisa de
// saber QUE verso é, e contar de novo pelo tempo daria duas verdades.
struct LinhaViva {
  std::size_t qual = 0;
  std::size_t linha_da_tela = 0;  // zero é o TOPO, como no Quadro do espectro
  std::size_t collunha = 0;       // onde o texto assenta, já centrado
  std::string texto;              // cortado á largura, e com o embaralho do instante
  // O VERSO como se LÊ: cortado á mesma largura, e sem embaralho algum. É elle
  // que a chapa em XIROD rasteriza (issue #110), e nunca o `texto`: imagem de
  // glyphos embaralhados seria lixo desenhado com esmero.
  std::string verso;
  std::string_view tinta = tokens::text_faint;
  double resolvida = 0.0;         // a fracção de glyphos já resolvidos, em [0,1]
  bool corrente = false;          // está na linha de leitura, e é a que se canta
  // SOBE — nasceu na base e ainda não chegou. UMA sómente em cada quadro, que o
  // nascimento se cinge ao intervallo desde a anterior: duas a subir juntas não
  // ha. É por ella que a chapa da PROXIMA se adianta.
  bool sobe = false;
};

// O QUADRO DA LETRA: as linhas Á VISTA n'uma posição, e mais nada. Linha que
// ainda não nasceu e linha que já morreu no alto não entram, de sorte que quem
// compõe não tem de as filtrar outra vez.
struct QuadroDaLetra {
  std::size_t largura = 0;
  std::size_t altura = 0;
  std::vector<LinhaViva> linhas;
  // O INDICE em `linhas` da que está na linha de leitura, ou menos um. Indice, e
  // não ponteiro, para que o quadro se copie sem cuidado.
  int corrente = -1;

  bool vazio() const noexcept { return linhas.empty(); }
};

// glifos_da_linha — a cadeia partida em pontos de codigo, um por CÉLULLA. Conta
// por CODEPOINT, e não por largura de columna: o glypho largo (CJK, emoji) sahe
// contado por um, e é o mesmo debito que a pauta d'esta Casa já carrega.
std::vector<std::string> glifos_da_linha(std::string_view texto);

// embaralha — os glyphos que ainda se não resolveram, tirados das PROPRIAS
// letras da linha. Os resolvidos são os do MEIO, e a fórma abre-se do centro
// para as pontas conforme a linha sobe. O branco CONSERVA-SE branco, que é o que
// deixa a fórma das palavras a ler-se antes das letras.
//
// `quadro` é a conta do embaralho (ver QUADROS_DO_EMBARALHO), e não o relogio:
// a mesma posição dá a mesma fita, sempre, e é isso que a bateria affirma.
std::string embaralha(const std::vector<std::string>& glifos, double resolvida,
                      std::size_t qual, long long quadro);

// Os QUADROS DO EMBARALHO por segundo. Oito, que é o bastante para o olho ler
// fervura e não pisca-pisca. Sahe da POSIÇÃO, e nunca de contador proprio: fosse
// contador, a mesma posição daria fitas differentes e a pureza cahia.
inline constexpr int QUADROS_DO_EMBARALHO = 8;

// quadro_da_letra — o rio n'uma posição. A linha `i` nasce na base
// `nascimento_da_linha` segundos antes do instante d'ella e sobe LINEARMENTE até
// a linha de leitura, onde chega no instante exacto; ahi fica em text_bright até
// que a seguinte chegue, e d'ahi sobe UMA linha por segundo, em text_muted e
// depois em text_faint, até sumir na linha zero.
//
// Linha de texto VAZIO não pinta nada, e é de proposito: é assim que o LRCLIB
// marca o silencio entre estrophes, e o carimbo d'ella continua a valer para
// expulsar a anterior da linha de leitura na hora certa.
QuadroDaLetra quadro_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                              double posicao, std::size_t largura,
                              std::size_t altura);

// linha_corrente_do_rio — a linha que se canta, ou nada. Funcção NOMEADA, e não
// campo a que se chegue por conta: a issue irmã da letra em XIROD pende d'ella.
const LinhaViva* linha_corrente_do_rio(const QuadroDaLetra& quadro);

// linha_que_sobe_do_rio — a que nasceu na base e ainda sobe, ou nada. Serve á
// chapa da PROXIMA (issue #110), que se rasteriza ao nascer d'ella para estar
// prompta no instante em que a voz a canta.
const LinhaViva* linha_que_sobe_do_rio(const QuadroDaLetra& quadro);

// caixa_da_corrente — o rectangulo que a linha corrente occupa, em coordenadas
// do RECTANGULO DO ESPECTRO e não da tela: quem a põe na tela somma-lhe o canto
// do painel, que é o unico que sabe onde o painel começa. Vazio quando não ha
// linha corrente, e por ahi se sabe que chapa alguma se ha de pôr.
Rectangulo caixa_da_corrente(const QuadroDaLetra& quadro);

// A IDENTIDADE por que a lousa conhece a janella da chapa em XIROD. FIXA, e uma
// só: o verso troca, a janella é a mesma, e assim o que sae não deixa fantasma
// por baixo do que chega.
inline constexpr std::string_view IDENTIDADE_DA_LETRA = "letra";

// A ORDEM que o pintor dá á lousa quanto á chapa da linha corrente. Sahe UMA de
// cada quadro, e sempre: quadro sem corrente ha de dizer que chapa não tem,
// senão a do quadro anterior ficava na tela por cima do verso novo.
struct ChapaDaLetra {
  bool poe = false;  // falso é o Tira, e é o que o quadro sem corrente pede
  int collunha = 0;  // em coordenadas da TELA: o canto do painel já sommado
  int linha = 0;
  std::size_t cellulas = 0;  // a largura do verso, e d'ella sae a proporção
  std::string verso;         // o texto já cortado, tal qual o rio o corta
  // O que se ADIANTA: o verso da linha que nasceu na base e ainda sobe. Não se
  // põe; rasteriza-se, para que no instante d'ella a chapa esteja em disco.
  std::string adiantado;
  std::size_t cellulas_adiantadas = 0;
};

// ordem_da_chapa_da_letra — a decisão, PURA pelo molde do `ordens_das_chapas`
// do cabeçalho: o foco e o `l` entram em TODO quadro, e não sómente no do
// evento, que o FTXUI desenha logo depois de correr os eventos. O `l` entra POR
// SI, e não pela via de o rio vir vazio: alavanca do operador é facto do mundo,
// como o foco, e facto que a bateria não interrogue á parte não se prova.
//
// `espectro` é o rectangulo d'elle na TELA (o da Sala, já descontada a capa): é
// a sommar-lh'o que a caixa do rio vira canto de janella.
ChapaDaLetra ordem_da_chapa_da_letra(const QuadroDaLetra& rio,
                                     const Rectangulo& espectro,
                                     bool letreiro_de_pe, bool foco_dentro,
                                     bool mostra_letra);

// pedido_da_chapa_da_letra — o que se manda rasterizar: o verso em text_bright
// sobre o fundo do painel, que é a tinta do brilho cheio da linha de leitura. O
// verso e as cellas vão SOLTOS, e não a ordem inteira, por a mesma funcção
// servir á chapa que se põe e á que se adianta.
nucleo::PedidoDaChapa pedido_da_chapa_da_letra(const std::string& verso,
                                               std::size_t cellulas);

// Uma CÉLULLA do rio: o espectro por baixo, a letra por cima. `letra` verdadeiro
// quer dizer que a célulla é de LETRA, e leva o FUNDO DO PAINEL por cama: é assim
// que ella esconde a barra que está debaixo d'ella, e sómente essa.
struct CelulaDoRio {
  std::string glifo = " ";
  tokens::Triade tinta;
  bool pinta = false;
  bool letra = false;
};

// tapete_do_rio — as célullas já compostas, em vector chato de largura vezes
// altura, pela medida do QUADRO DO ESPECTRO. A célulla com letra é a que tem
// glypho que não é branco: o branco entre as palavras deixa passar a barra, que
// é o que faz o rio parecer sahir do espectro em vez de assentar n'uma tarja.
//
// D'aqui sahem os DOUS consumidores, e por isso mora aqui: o ftxui::Element da
// janella e a sequencia SGR crua do exemplo. Duas composições dariam duas telas.
std::vector<CelulaDoRio> tapete_do_rio(const Quadro& espectro,
                                       const QuadroDaLetra& letra);

// sequencia_do_rio — os BYTES da célulla: o fundo, depois a tinta, depois o
// glypho, sem repouso pelo meio. A célulla que não é de letra não escreve fundo
// algum, que fundo escripto em toda a tela apagaria a transparencia do terminal.
std::string sequencia_do_rio(const CelulaDoRio& celula);

// elemento_do_rio — o espectro e a letra n'um só elemento.
//
// POR QUE NÃO `dbox`: o `text` do FTXUI escreve TODA célulla que o seu texto
// tem, e o espaço é célulla escripta. Posta a letra por cima em dbox, a linha
// inteira d'ella apagaria as barras de orla a orla, e não sómente as célullas
// das letras. Compõe-se pois célulla a célulla, que é a unica composição em que
// o que se esconde se pode NOMEAR, e a bateria afere-a por PixelAt.
ftxui::Element elemento_do_rio(const Quadro& espectro,
                               const QuadroDaLetra& letra);

// ── A LETRA PARADA (issue #157). Elle olhou o rio a subir e disse que o effeito
// era feio, e depois disse o que quer, letra por lettra: «apenas a Frase que
// esta cantando atualmente, depois aparece a proxima e a proxima, bem cru
// mesmo, com a fonte XIROD grande e laranja».
//
// Logo: nada de animação, e nada do verso que já passou. Ficam DOUS (issue
// #159): o que se canta, GRANDE e laranja no alto, e o SEGUINTE por baixo,
// miudo e apagado, para que o olho saiba o que vem. O bloco tem quatro fileiras,
// que a sala reserva: tres do corrente (que é o corpo grande da chapa que a
// lousa desenha por cima) e a ultima do seguinte, que cella de terminal já
// desenha e por isso não vae á lousa. Sem lousa, o corrente sae em mono, na
// mesma côr.
//
// Nada se anima aqui: o bloco é funcção da POSIÇÃO e de mais nada, d'onde a
// mesma posição dá sempre o mesmo bloco, e a bateria o afere sem relogio.
inline constexpr std::size_t FILEIRAS_DA_LETRA = 3;

// Quantas fileiras o verso CORRENTE toma: duas, que é o corpo grande da chapa.
// A terceira do bloco é do verso SEGUINTE (issue #159), miudo e apagado, e é
// por isso que elle fica LOGO por baixo (issue #161): tres fileiras entre um e
// outro era vão de mais, e o olho lia-os como duas cousas apartadas.
inline constexpr std::size_t FILEIRAS_DO_VERSO = 2;

// As duas fileiras que contam: a do que se canta, no alto, e a do que vem.
inline constexpr std::size_t FILEIRA_DO_CORRENTE = 0;
inline constexpr std::size_t FILEIRA_DO_SEGUINTE = FILEIRAS_DA_LETRA - 1;

// elemento_da_letra_parada — o bloco pintado, com UM verso. `corrente` é o que
// o `nucleo::linha_corrente` devolveu: menos um quer dizer «ainda não começou»,
// e ahi mostra-se o PRIMEIRO, que é o que vem a caminho. Faixa sem letra dá
// bloco VAZIO (fileiras em branco), e não recado algum: letra que não ha não se
// annuncia, que o painel não é logar de aviso.
ftxui::Element elemento_da_letra_parada(
    const std::vector<nucleo::LinhaDaLetra>& linhas, int corrente,
    std::size_t largura, std::size_t altura);

// verso_do_bloco — o texto do verso corrente, já cortado á largura, que é o que
// a lousa manda rasterizar. Vazio quando não ha verso a mostrar.
std::string verso_do_bloco(const std::vector<nucleo::LinhaDaLetra>& linhas,
                           int corrente, std::size_t largura);

// ordem_da_chapa_parada — a ordem que a lousa recebe para o bloco: a chapa do
// verso corrente, de DUAS fileiras, centrada na largura do bloco. As tres
// condições são as de sempre (letreiro de pé, foco dentro, letra á vista), e a
// quarta é haver verso. Sem ellas, manda-se TIRAR, que chapa esquecida na tela
// diria um verso que já passou.
// A `caixa` é a que o `reflect` pendurou no PROPRIO bloco no quadro anterior, e
// não o rectangulo da sala: aquelle conta a capa pelo tecto d'ella, e a capa de
// 16 por 9 sahe mais baixa, d'onde o bloco real sobe e a imagem cahia mais
// abaixo (issue #161). Caixa por pintar manda TIRAR, que é o que o primeiro
// quadro pede.
ChapaDaLetra ordem_da_chapa_parada(const std::vector<nucleo::LinhaDaLetra>& linhas,
                                   int corrente, const ftxui::Box& caixa,
                                   bool letreiro_de_pe, bool foco_dentro,
                                   bool mostra_letra);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
