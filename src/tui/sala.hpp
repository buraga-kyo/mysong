// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SALA — src/tui/sala.hpp
// ══════════════════════════════════════════════════════════════════════════
// A sala em DUAS METADES (issue #102): a pauta á esquerda, o painel á direita,
// e o cabeçalho e o trilho por cima das duas. Aqui moram a chapa, o divisor, o
// painel, e os numeros da composição toda. Funcções PURAS de VALORES: nem
// Tocador, nem janella.
//
// DOMÍNIO ......... a chapa, a ficha do que toca, e a tela em collunhas e
//                   linhas.
// CONTRA-DOMÍNIO .. cadeias e `ftxui::Element`.
// INVARIANTE ...... numero algum da composição mora no pintor: sahem todos
//                   d'uma funcção só, que a bateria interroga sem terminal.
// Q.E.D. .......... sendo a sala funcção do estado, redimensionar e voltar dá
//                   a mesma sala, e a prova afere-a cella a cella em papel.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "nucleo/capa.hpp"
#include "nucleo/fila.hpp"
#include "tui/navegador.hpp"

namespace mysong::tui {

// O que se conta na linha da conta. A ESPECIE, e não a secção: MINHAS MÚSICAS,
// um album e uma lista contam a MESMA cousa, e tres nomes dariam tres erros.
enum class Especie { Faixas, Artistas, Albuns, Listas, Achados };

// texto_da_duracao — a somma POR EXTENSO: `1h23`, `23min`, `45s`, e vazia no
// que não é positivo. `MM:SS` ao lado de «4 FAIXAS» lê-se como o tempo D'ELLA.
std::string texto_da_duracao(int segundos);

// texto_da_conta — «4 FAIXAS, 14min». Um sahe no singular e sem o `s`.
std::string texto_da_conta(std::size_t quantas, Especie especie, int duracao);

// especie_da_secao — o que se conta em cada secção da aba corrente.
Especie especie_da_secao(Secao secao);

// onde_da_chapa — ONDE se está, na palavra da aba e nos degraus de dentro,
// apartados por «▸»: `MY SONG`, `PLAYLISTS ▸ Funk`, `ARTISTAS ▸ MXZI`. O
// vocabulario FIXO vae em caixa alta; o nome vindo do acervo sahe VERBATIM,
// que caixa alta byte a byte estragaria o UTF-8 acentuado do portuguez
// («Canção» sahiria «CANçãO»), e taboa de caixa Unicode esta Casa não carrega.
std::string onde_da_chapa(Secao secao, const std::vector<std::string>& trilha,
                          const std::string& nome_do_catalogo);

// A CHAPA de uma linha por cima da pauta (issue #102): onde se está, a conta, e
// a vista. Toma o logar do cabeçalho da colleção, que gastava seis linhas para
// dizer o que a aba já diz, e o da trilha do topo.
struct Chapa {
  std::string onde;
  std::size_t quantas = 0;
  Especie especie = Especie::Faixas;
  int duracao = 0;
  std::string vista;   // FAIXAS, ARTISTAS, ÁLBUNS; vazia onde não se cycla
  // As ENCOMMENDAS (colhidas, falhadas, duvidosas). Campo PROPRIO, e não mais
  // um pedaço do recado: a issue pede-as «por cima da lista» na DOWNLOAD, e no
  // fim da cadeia o primeiro aviso comprido comia-lhes o logar. Pintam-se logo
  // á direita do texto, e recado algum as empurra.
  std::string encommendas;
  // O CONSELHO da pauta VAZIA (issue #111): o que falta, e a tecla que o
  // desfaz. Vem á chapa, e não ao meio da folha, porque conselho pintado onde
  // as linhas se lêem toma-se por linha da lista, e o dedo tenta elegel-o.
  std::string conselho;
  std::string recado;  // o aviso da rede, o filtro posto, a varredura
};

// espaco_do_recado — quantas collunhas sobram á direita depois do texto e das
// encommendas. Quem monta o recado precisa d'esta conta ANTES de o montar:
// juntar sem ella dá cadeia que a chapa corta a meio da palavra, e foi o que
// se mediu nos dumps («busca prime», e o «(s)» perdido).
std::size_t espaco_do_recado(const Chapa& chapa, std::size_t largura);

// texto_da_chapa — «MY SONG, 42 FAIXAS, 1h29, FAIXAS». O recado NÃO entra: elle
// vae á direita da linha, e n'outra tinta.
std::string texto_da_chapa(const Chapa& chapa);

// elemento_da_chapa — a linha inteira: o texto á esquerda, o recado á direita
// quando ha, e o fundo do painel por baixo dos dous.
ftxui::Element elemento_da_chapa(const Chapa& chapa, std::size_t largura);

// Um RECTANGULO da sala: onde começa e quanto mede. O canto conta-se da tela
// INTEIRA, e não de dentro de peça alguma: a tela nova não leva orla, e canto
// contado de dentro obrigaria quem o lê a sommar por fóra o que a orla comia.
struct Rectangulo {
  std::size_t x = 0, y = 0, largura = 0, altura = 0;
  bool vazio() const noexcept { return largura == 0 || altura == 0; }
};

// A SALA (issue #102): o cabeçalho, o trilho, e duas metades. Todo numero da
// composição sahe d'aqui, e em campo NOMEADO: as issues irmãs pendem d'estes
// rectangulos, e indice n'um vector desloca-se á primeira peça nova.
struct Sala {
  Rectangulo cabecalho;  // UMA linha, no alto, de largura inteira
  Rectangulo trilho;     // UMA linha, logo abaixo d'elle
  Rectangulo campo;      // a linha do prompt; vazia com elle fechado
  Rectangulo chapa;      // UMA linha por cima da pauta
  Rectangulo pauta;      // a lista das musicas, á esquerda
  Rectangulo divisor;    // a collunha que aparta as duas metades
  Rectangulo painel;     // a metade direita; vazia abaixo de cem collunhas
  Rectangulo capa;       // no alto do painel, ATÉ quarenta e cinco por cento
  Rectangulo espectro;   // o que sobra do painel, abaixo da capa
  Rectangulo rodape;     // UMA linha de dicas, no pé
};

// sala_da_tela — todos os numeros da composição, n'uma conta só, para que a
// bateria os interrogue sem terminal. `campo_aberto` é o prompt de digitar, que
// pede linha propria e empurra o corpo uma para baixo.
Sala sala_da_tela(std::size_t largura, std::size_t altura, bool campo_aberto);

// espectro_abaixo_da — o rectangulo do espectro depois de se saber quantas
// linhas a capa tomou DE FACTO. O `capa` da sala é TECTO: a capa de 16 por 9
// sahe mais baixa que elle, e o que ella deixa pertence ao espectro. A conta
// mora aqui, e não no pintor, pela regra da sala: numero algum da composição
// se resolve em janella.cpp, que é o que não se prova.
Rectangulo espectro_abaixo_da(const Sala& sala, std::size_t linhas_da_capa);

// A FICHA da faixa que toca. Valores, e não punho para o tocador nem para o
// indice: assim a bateria arma-a á mão, sem motor e sem banco.
struct Ficha {
  std::string titulo;
  std::string artista;
  std::string album;
};

// ficha_da_faixa — a ficha do caminho, com as etiquetas do indice quando as ha.
// Sem etiqueta, o nome do arquivo serve de titulo: buraco na ficha faria o
// painel dizer que nada toca na hora em que alguma cousa toca.
Ficha ficha_da_faixa(const std::string& caminho, const std::string& titulo,
                     const std::string& artista, const std::string& album);

// A ARTE: a capa já pintada, cingida ao TECTO que se pediu á Galeria.
// `linhas_da_arte` diz quantas linhas ella toma de facto (as do chafa quando
// achada, as do marcador quando não), para que quem compõe saiba o que sobra.
//
// O desconto da orla mora AQUI, e não no `elemento_da_capa`: aquelle põe
// `border` POR FÓRA do marcador, duas linhas e duas collunhas a mais do que se
// lhe pede, e é lavra que a tarefa irmã do rato tambem edita.
std::size_t linhas_da_arte(const nucleo::CapaPintada& capa, std::size_t tecto);
ftxui::Element elemento_da_arte(const nucleo::CapaPintada& capa,
                                std::size_t largura, std::size_t linhas);

// elemento_do_divisor — a collunha que aparta as duas metades, em `line_dim`. É
// collunha PROPRIA, e não orla do painel: orla custaria duas de cada lado, e
// n'este painel ellas sahiriam todas da arte.
ftxui::Element elemento_do_divisor(std::size_t altura);

// elemento_do_painel — a metade direita: a arte no alto, CENTRADA na largura,
// e o que o chamador
// quizer por baixo (o espectro, ou a letra quando o `l` a pede). Os dous entram
// já compostos, para que esta peça não conheça nem um nem outra e a bateria
// lh'os arme á mão. O titulo e a ficha sahiram: o nome do que sôa vive agora na
// linha do alto, e repeti-lo aqui gastaria quatro linhas da arte.
ftxui::Element elemento_do_painel(ftxui::Element arte, ftxui::Element baixo,
                                  std::size_t largura);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
