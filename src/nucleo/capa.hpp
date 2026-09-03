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

#include "nucleo/ajustes.hpp"  // Sextantes: a alavanca do operador

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

// A MEDIDA de uma imagem, em PIXEIS. Serve tambem á célulla do terminal, que é
// rectangulo com largura e altura como qualquer outro.
struct Medida {
  std::size_t largura = 0, altura = 0;
};

// medida_da_imagem — a largura e a altura lidas do CABEÇALHO dos octetos, sem
// se decodificar imagem alguma. Conhece o JPEG e o PNG, que são os dous que o
// APIC d'este acervo traz e os que a Casa embute; de todo o mais devolve zero,
// e quem chama toma isso por «não sei» e não por «vazia». PURA, e é d'ahi que
// a bateria a afere contra cabeçalhos escriptos á mão, sem imagem no disco.
Medida medida_da_imagem(std::string_view octetos);

// O RECTANGULO que a capa toma, em CÉLULLAS.
struct Retangulo {
  std::size_t collunas = 0, linhas = 0;
};

// A CÉLULLA d'esta machina, em pixeis: nove por vinte, medido no Alacritty do
// operador com a JetBrainsMono NF de corpo onze. Entra por PARAMETRO na conta,
// e este é sómente o padrão da Casa: quem trocar de fonte troca um numero.
inline constexpr Medida CELLULA_DA_CASA{9, 20};

// rectangulo_da_capa — quantas célullas a imagem toma dentro do tecto, GUARDADA
// A PROPORÇÃO. A célulla é mais alta que larga, e sem essa razão na conta a
// capa quadrada pediria o dobro das linhas que toma. Medida por ler (formato
// que este modulo não conhece) toma o tecto inteiro, que o Überzug++ encolhe a
// imagem por dentro e o que se perde é sómente a fileira que sobraria.
//
// PURA, e sem parametro de omissão pela razão do argumentos_do_chafa: com
// padrão, o valor avaliar-se-ia no logar da chamada e a bateria deixaria de
// alcançar a machina de célulla differente.
Retangulo rectangulo_da_capa(Medida imagem, std::size_t tecto_collunas,
                             std::size_t tecto_linhas, Medida cellula);

// somma_dos_octetos — o FNV-1a de sessenta e quatro bits, em hexadecimal. Não é
// criptographia e não precisa de ser: o que se quer é que duas capas
// differentes não caiam no mesmo arquivo do cache. Vae aqui, e não em
// bibliotheca, porque nenhuma d'esta Casa o traz e são seis linhas.
std::string somma_dos_octetos(std::string_view octetos);

// extensao_da_capa — «jpg» ou «png», pelo CONTEUDO e por extensão nenhuma, pela
// regra do embute_arte: o APIC declara um mime que ninguem afere. Vazio de todo
// o mais, e ahi a capa não vae ao cache.
std::string_view extensao_da_capa(std::string_view octetos);

// caminho_da_capa_em_cache — `$XDG_CACHE_HOME/mysong/capas/<somma>.<extensão>`,
// e sem a variavel `~/.cache/mysong/capas/`, pelo precedente exacto do
// caminho_da_configuracao. Vazio sem HOME, e vazio quando os octetos não são
// imagem que se conheça. Directorio algum se cria aqui: quem escreve é quem
// cria, e esta funcção sómente diz ONDE.
std::filesystem::path caminho_da_capa_em_cache(std::string_view octetos);

// ha_sextante_na_fonte — diz se a fonte d'esta machina desenha o SEXTANTE
// (U+1FB00), o glypho de duas por tres sub-célullas com que o chafa dobra os
// degraus por célulla. Pergunta-se ao fontconfig pela classe «nerd», que é a
// que a sonda já exige; sem glypho, o sextante sahiria quadrículo vazio, e ahi
// o remedio seria peor que o mal.
//
// A resposta GUARDA-SE: o fontconfig lê a taboa das fontes do systema, e o
// pintor corre vinte vezes por segundo. UMA consulta por processo, e não uma
// por render.
bool ha_sextante_na_fonte();

// sextante_de — resolve a alavanca do operador n'um bool. `Auto` é a regra da
// Casa, que pergunta á fonte; `Sim` e `Nao` são a vontade d'elle, e essa não se
// discute: quem olha o terminal é elle, e a fonte de substituição pode desenhar
// o sextante muito bem sem que o fontconfig o saiba dizer.
//
// Chama-se `_de` pelo precedente do fonte_de e do volume_de, e NÃO
// `com_sextante`: aquelle é o nome do parametro que quatro funcções d'este
// modulo carregam, e funcção homonyma do parametro fica sombreada dentro
// d'ellas. Sem -Wshadow isso passa calado até ao dia em que alguem escrever o
// nome esperando a funcção e obtiver o bool, ou o contrario.
bool sextante_de(Sextantes ajuste);

// argumentos_do_chafa — o que se corre. Os symbolos, a geometria em collunhas
// por linhas, e o trabalho no maximo.
//
// O `com_sextante` entra por PARAMETRO, e sem valor padrão: com padrão, esta
// funcção deixaria de ser pura (o padrão avaliar-se-ia no logar da chamada e
// iria ao fontconfig), e é a pureza que deixa a bateria aferir as DUAS listas
// sem fonte, sem chafa e sem imagem alguma. Quem sabe a resposta é quem chama.
std::vector<std::string> argumentos_do_chafa(const std::filesystem::path& imagem,
                                             std::size_t collunas,
                                             std::size_t linhas,
                                             bool com_sextante);

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

// pinta_imagem — renderiza um ARQUIVO no tamanho pedido, e é o que a Galeria
// faz depois de achar a imagem. Sahe d'ella (issue #94) para o fita_capa a
// alcançar sem faixa, sem etiqueta e sem cache. Tamanho zero, chafa ausente e
// imagem recusada devolvem `achada` falso; nada lança.
CapaPintada pinta_imagem(const std::filesystem::path& imagem,
                         std::size_t collunas, std::size_t linhas,
                         bool com_sextante);

// O ARQUIVO de capa de uma faixa: o caminho que a lousa ha de abrir, e a medida
// d'elle em pixeis. Caminho vazio quer dizer «esta faixa não tem capa alguma».
struct ArquivoDaCapa {
  std::filesystem::path caminho;
  Medida medida;
};

// ── E AGORA O QUE TOCA O MUNDO.

// O ARQUIVARIO: o arquivo de capa de cada faixa, achado uma vez. Existe porque
// a lousa quer CAMINHO e não octetos (o Überzug++ lê disco), e porque procurar
// no disco a cada quadro seriam vinte aberturas por segundo.
//
// Guarda por FAIXA, e não por pasta como a Galeria: aquella guarda o RENDER,
// que é caro e é do album; este guarda o caminho, e a arte embutida é de cada
// arquivo. Acervo de pasta unica teria uma capa só se a chave fosse a pasta.
class Arquivario {
 public:
  const ArquivoDaCapa& de(const std::filesystem::path& faixa);

  std::size_t quantos_escriptos() const noexcept;  // serve á prova do cache

 private:
  std::map<std::filesystem::path, ArquivoDaCapa> guardados_;
  std::size_t escriptos_ = 0;
};

// A GALERIA: guarda os renders já feitos, para que converter aconteça uma vez por
// album e por tamanho. Não é optimização gratuita: o chafa leva dezenas de
// milesimos, e o pintor corre vinte vezes por segundo.
class Galeria {
 public:
  // DOUS constructores, e nunca um parametro com valor padrão: o padrão
  // avaliar-se-ia no logar da chamada, e cada Galeria da bateria iria ao
  // fontconfig. O vazio segue a regra da Casa; o de um argumento toma o que os
  // ajustes do operador resolveram.
  Galeria() : com_sextante_(ha_sextante_na_fonte()) {}
  explicit Galeria(bool com_sextante) : com_sextante_(com_sextante) {}

  // Devolve a capa da faixa no tamanho pedido. Achando-a em cache, não corre nada.
  // Capa ausente devolve `achada` falso, e isso tambem se guarda: sem guardar a
  // AUSENCIA, um album sem capa faria a Casa procurar o arquivo a cada quadro.
  const CapaPintada& capa(const std::filesystem::path& faixa,
                          std::size_t collunas, std::size_t linhas);

  std::size_t quantos_renders() const noexcept;  // serve á prova do cache

 private:
  const bool com_sextante_;
  std::map<std::string, CapaPintada> guardadas_;
  std::size_t renders_ = 0;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
