// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA CAPA — src/nucleo/capa.hpp
// ══════════════════════════════════════════════════════════════════════════
// Acha a capa do album e converte-a em texto de terminal pelo `chafa`, em
// MEIO-BLOCO. Meio-bloco é o TECTO d'esta machina, e não escolha preguiçosa: o
// operador corre Alacritty dentro de tmux, e protocolo de imagem algum atravessa o
// tmux. Escolher sixel ou kitty seria pintar o que elle não veria.
//
// DOMÍNIO ......... o caminho de uma faixa, e a geometria do painel.
// CONTRA-DOMÍNIO .. linhas de texto com escapes de cor, prontas a pintar; ou
//                   nada, e ahi quem chama desenha o marcador.
// INVARIANTE ...... converter é CARO, e por isso o resultado guarda-se por album
//                   E por tamanho. Duas faixas do mesmo album pedem a mesma capa
//                   uma vez só; e redimensionar o terminal pede outra, porque a
//                   arte tem de encher o painel novo.
// Q.E.D. .......... sendo puras a busca do arquivo ao lado e a chave do cache, a
//                   bateria afere-as sem chamar o chafa e sem imagem alguma.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// Os nomes de arquivo de capa que se procuram ao lado do album, na ORDEM em que se
// preferem. A lista é fechada, e a ordem é a convenção que os ripadores usam.
const std::vector<std::string>& nomes_de_capa();

// capa_ao_lado — o arquivo de capa que estiver na pasta da faixa, pela ordem dos
// nomes preferidos. Vazio quando não ha nenhum. Não desce nem sobe: capa de album
// vive junto das faixas d'elle, e ir buscá-la mais longe traria a do album vizinho.
std::filesystem::path capa_ao_lado(const std::filesystem::path& faixa);

// arte_embutida — os OCTETOS do PRIMEIRO quadro APIC da etiqueta. Vazio quando a
// faixa não tem etiqueta, não tem APIC, ou não é MP3 de todo: arquivo que a taglib
// não reconheça sahe vazio, e não lança.
//
// O PRIMEIRO, e não «a capa da frente»: o typo do quadro não se consulta. Para o que
// esta Casa baixa é o mesmo, que o yt-dlp grava um APIC só; mas faixa vinda de
// ripador traz ás vezes dous (capa e contra-capa, ou foto do artista), e ahi sahe o
// que estiver á frente na etiqueta. Preferir o `FrontCover` é emenda de outra issue.
//
// Devolve os octetos, e não um caminho, de proposito (issue #81): assim a bateria
// afere-a contra uma etiqueta lavrada á mão, sem chafa, sem rede e sem temporario
// para limpar. Quem precisa de arquivo é o chafa, que lê disco e não memoria, e
// essa gravação fica do lado de cá, na Galeria.
//
// A ORDEM está declarada e é esta: o arquivo ao lado GANHA da etiqueta. É o que o
// operador pode trocar sem reescrever o MP3, e por isso é o que elle manda.
std::string arte_embutida(const std::filesystem::path& faixa);

// embute_arte — o ESPELHO de escripta do arte_embutida (issue #83): assenta
// os octetos como quadro APIC de capa da FRENTE, com o mime lido do CONTEUDO
// (JPEG ou PNG pelos octetos de guarda; outro formato recusa-se sem tocar o
// arquivo, que gravar a pagina de erro de um servidor dentro da etiqueta é
// pior que não gravar). O resto da etiqueta FICA: o save() da taglib preserva
// os quadros, propriedade que a prova da issue #81 prende. Falso quando os
// octetos não são imagem ou a taglib recusa o arquivo. ESCREVE em disco, e é
// por isso que quem chama só o faz em faixa que não tinha capa alguma.
bool embute_arte(const std::filesystem::path& faixa, std::string_view octetos);

// chave_do_cache — o que identifica um render. É a PASTA da faixa mais o tamanho,
// e não o caminho da faixa: as faixas de um album partilham a capa, e usar o
// caminho faria converter uma vez por faixa em vez de uma por album.
std::string chave_do_cache(const std::filesystem::path& faixa,
                           std::size_t collunas, std::size_t linhas);

// argumentos_do_chafa — o que se corre. Meio-bloco fixado, e a geometria em
// collunhas por linhas.
std::vector<std::string> argumentos_do_chafa(const std::filesystem::path& imagem,
                                             std::size_t collunas,
                                             std::size_t linhas);

// Uma CORRIDA de célullas da mesma tinta: o texto, e as duas côres. Menos um em
// qualquer componente quer dizer «sem côr», que é o que o `ESC[39m` e o `ESC[49m` do
// chafa dizem.
//
// Guarda-se em corridas, e NÃO em cadeia com os escapes dentro. A razão foi medida:
// pondo-se a cadeia crua n'um `ftxui::text`, o FTXUI conta os octetos do escape como
// LARGURA, donde a capa reclamava oitenta collunhas onde pintava quinze e esmagava a
// barra lateral e a tabella. Lê-se «ARTISTS» como « A».
struct Corrida {
  std::string texto;
  int r_frente = -1, g_frente = -1, b_frente = -1;
  int r_fundo = -1, g_fundo = -1, b_fundo = -1;
};

// A CAPA renderizada: as linhas, cada uma em corridas de côr.
struct CapaPintada {
  std::vector<std::vector<Corrida>> linhas;
  bool achada = false;
};

// analysa_sgr — parte uma linha de sahida do chafa em corridas. Funcção PURA, e por
// isso aferivel contra linhas escriptas á mão sem chamar o chafa.
std::vector<Corrida> analysa_sgr(std::string_view linha);

// ── E AGORA O QUE TOCA O MUNDO.

// A GALERIA: guarda os renders já feitos, para que converter aconteça uma vez por
// album e por tamanho. Não é optimização gratuita: o chafa leva dezenas de
// milesimos, e o pintor corre vinte vezes por segundo.
class Galeria {
 public:
  // Devolve a capa da faixa no tamanho pedido. Achando-a em cache, não corre nada.
  // Capa ausente devolve `achada` falso, e isso tambem se guarda: sem guardar a
  // AUSENCIA, um album sem capa faria a Casa procurar o arquivo a cada quadro.
  const CapaPintada& capa(const std::filesystem::path& faixa,
                          std::size_t collunas, std::size_t linhas);

  std::size_t quantos_renders() const noexcept;  // serve á prova do cache

 private:
  std::map<std::string, CapaPintada> guardadas_;
  std::size_t renders_ = 0;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
