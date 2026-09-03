// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LOUSA — src/nucleo/lousa.hpp
// ══════════════════════════════════════════════════════════════════════════
// A LOUSA é uma janella de X11 posta POR CIMA do terminal, e não desenho no
// terminal: quem a segura é o `ueberzugpp`, erguido em filho proprio, a quem
// se fala por um cano em JSON, uma ordem por linha. É assim que o yazi d'esta
// machina fica nitido, e é o unico caminho que ha: o Alacritty corre dentro do
// tmux, e nem o protocolo do kitty nem o sixel atravessam o tmux.
//
// DOMÍNIO ......... um identificador, o caminho de uma imagem, e o rectangulo
//                   em CÉLULLAS a contar do canto do TERMINAL.
// CONTRA-DOMÍNIO .. linhas de JSON no cano do filho; ou nada, e ahi quem chama
//                   pinta os symbolos do chafa como sempre pintou.
// INVARIANTE ...... funcção alguma d'aqui bloqueia nem lança. Cano cheio
//                   descarta a ordem e conta-a; filho morto responde ausente.
// Q.E.D. .......... sendo a composição do JSON funcção pura, a bateria afere o
//                   protocolo inteiro sem X11 vivo e sem erguer processo algum.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "nucleo/ajustes.hpp"  // ModoDaLousa: a alavanca do operador
#include "nucleo/capa.hpp"     // Medida: a chapa mede-se como a capa

namespace mysong::nucleo {

// argumentos_da_lousa — o que se corre, pelo precedente do argumentos_do_chafa.
// `layer` é o modo de janella persistente; `--silent` manda o erro d'elle ao
// buraco, que esta Casa corre debaixo de uma tela do FTXUI e linha de aviso no
// meio do quadro estraga-o; `-o x11` é a sahida MEDIDA n'esta machina, a mesma
// que o yazi escolhe. A de wayland existe, e fica para quando houver Wayland.
std::vector<std::string> argumentos_da_lousa();

// escapado_em_json — as aspas, a barra invertida e os de controle. Á parte, e
// pura: aspa no nome do album partiria a linha ao meio, e o filho calava-se.
//
// O LIMITE fica dito, pelo molde do `#` no mysong.conf: nome de arquivo em
// Linux é sequencia de octetos qualquer, e octeto alto que não seja UTF-8
// valido (album rasgado em Windows com CP-1252) passa cru, e o analysador do
// Überzug++ recusa a linha. Effeito: a capa d'esse album não apparece, e é
// tudo; o protocolo não se corrompe, que a linha continua a ser UMA.
std::string escapado_em_json(std::string_view texto);

// ordem_de_por e ordem_de_tirar — as DUAS ordens do protocolo, cada uma n'UMA
// linha, que é como o filho as lê. O canto é o do TERMINAL, e não o do painel:
// dentro do tmux o Überzug++ somma o deslocamento (elle lê o TMUX_PANE).
// MEDIDO em 03/09: contam de ZERO, e a imagem cabe guardando a proporção.
std::string ordem_de_por(std::string_view identidade,
                         const std::filesystem::path& imagem, int collunha,
                         int linha, std::size_t largura, std::size_t altura);
std::string ordem_de_tirar(std::string_view identidade);

// O PARECER sobre a lousa: se ella se ergue, e a razão em UMA linha. A razão
// vae sempre, ainda de pé, que é d'ella que a linha do --sonda vive.
struct Parecer {
  bool de_pe = false;
  std::string razao;
};

// parecer_da_lousa — a decisão, e é PURA: o mundo entra por dous bools, pela
// razão do Inquerito da sonda. Sem elles, o caminho da recusa não se observaria
// n'esta machina, que tem DISPLAY e tem o programa.
//
// `Sim` salta a pergunta do DISPLAY, e sómente ella: a variavel é um PALPITE
// sobre haver X11 ao alcance, e o operador pode saber melhor. A falta do
// programa não é palpite nenhum, e nem o `Sim` a atravessa: o exec falharia.
Parecer parecer_da_lousa(ModoDaLousa modo, bool ha_display, bool ha_programa);

// ha_display e versao_da_lousa — as duas perguntas ao MUNDO, e por isso á parte
// do parecer. A segunda corre `ueberzugpp --version` e devolve o que elle disse
// («ueberzugpp 2.9.8» n'esta machina), ou vazio quando o programa não está: uma
// chamada responde ás duas cousas que o diagnostico precisa de saber.
bool ha_display();
std::string versao_da_lousa();

// texto_da_lousa — a linha do --sonda, e pura pelo precedente do
// texto_dos_ajustes: escape algum sahe d'aqui.
std::string texto_da_lousa(const Parecer& parecer, std::string_view versao);

// COLLUNHA_DO_EMPURRAO — onde nasce a janella que nada mostra. Negativa para
// cahir fóra de todo terminal, e não MAIS negativa por uma razão medida: a
// coordenada de uma janella do X11 é inteiro de dezasseis bits com signal, e o
// producto d'esta collunha pela largura da cella tem de caber n'elle. Com
// quatro mil o producto dava menos trinta e seis mil, transbordava, e a janella
// nascia a trinta mil pixeis á DIREITA; n'esta tela ficou invisivel por acaso,
// e em tela mais larga apparecia. Mil e quinhentas cabem de sobra.
inline constexpr int COLLUNHA_DO_EMPURRAO = -1500;

// A CAIXA do empurrão, em célullas: larga e alta, e nunca de UMA. O Überzug++
// encolhe guardando a proporção, e chapa de uma linha é muito mais larga que
// alta: cabendo n'uma cella, a altura arredonda a ZERO e o OpenCV d'elle
// ABORTA na redimensão (asserção `inv_scale_x > 0`), levando comsigo a capa e
// as abas. Medido pela lavra da letra em 03/09. Sessenta e quatro por trinta e
// dous aguentam proporção de mil e cento e cinquenta para um, e a chapa mais
// larga que esta Casa faz é a da linha inteira, que não passa de setenta e
// cinco. A janella é invisivel, d'onde o tamanho d'ella nada custa ao olho.
inline constexpr std::size_t LARGURA_DO_EMPURRAO = 64;
inline constexpr std::size_t ALTURA_DO_EMPURRAO = 32;

// lados_do_empurrao — quantos pixeis a imagem toma DENTRO d'essa caixa, pela
// mesma reducção que o Überzug++ faz. PURA, e é o que a bateria afere: lado
// ZERO é o que o faz abortar, e nenhum dos dous ha de chegar lá.
Medida lados_do_empurrao(Medida imagem, Medida cellula) noexcept;

// A ORDEM que o pintor da a lousa quanto á capa, e as duas unicas que ha.
enum class OrdemDaCapa { Tira, Poe };

// ordem_da_capa — a decisão, e é PURA para que a bateria a afira sem tela. O
// `foco_dentro` manda em TODO quadro, e não sómente no quadro do evento, e é
// isso que a corrige: o FTXUI DESENHA logo depois de executar os eventos, d'onde
// um `tira_tudo` posto no tratador do foco desfaz-se no desenho seguinte, e a
// janella da lousa fica de pé por cima do que o operador foi ver. Medido no
// FTXUI v7.0.3, em `app.cpp`: o RunOnce corre as tarefas e chama o Draw.
OrdemDaCapa ordem_da_capa(bool lousa_de_pe, bool foco_dentro, bool ha_arquivo,
                          bool caixa_pintada) noexcept;

// ── E AGORA O QUE TOCA O MUNDO.

// A LOUSA: o filho vivo, e o cano por onde se lhe fala. Ergue-se no
// constructor, ou não se ergue, e a razão fica guardada para o diagnostico.
//
// Funcção alguma d'ella bloqueia: o pintor corre vinte vezes por segundo, e
// cano cheio descarta a ordem e conta-a em vez de segurar o quadro. E funcção
// alguma lança: sahida de imagem não é caminho por onde o tocador caia.
class Lousa {
 public:
  explicit Lousa(ModoDaLousa modo) noexcept;
  ~Lousa() noexcept;
  Lousa(const Lousa&) = delete;
  Lousa& operator=(const Lousa&) = delete;

  bool disponivel() const noexcept;

  // poe — a imagem no rectangulo, em célullas do canto do TERMINAL. Repetida
  // com os MESMOS numeros não manda ordem alguma: o Überzug++ redimensiona a
  // cada `add`, e o pintor pediria vinte por segundo de uma capa parada.
  bool poe(std::string_view identidade, const std::filesystem::path& imagem,
           int collunha, int linha, std::size_t largura,
           std::size_t altura) noexcept;

  // tira e tira_tudo — o `remove`. O segundo serve ao foco que se perde e á
  // sahida, e é elle que promette não deixar fantasma na tela.
  bool tira(std::string_view identidade) noexcept;
  void tira_tudo() noexcept;

  // empurra — uma ordem que NADA mostra, e que existe por uma MEDIÇÃO: o
  // Überzug++ (o 2.9.8 e o 2.9.10) não desenha a janella de UMA linha de
  // altura no instante em que a cria, e ella fica preta até que outro `add` o
  // faça redesenhar a tela toda. A de duas linhas desenha-se sósinha; a de uma
  // não, e a chapa do letreiro (issue #108) tem uma linha.
  //
  // Empurra-se com um `add` fóra da tela (a COLLUNHA_DO_EMPURRAO), que o X11
  // recorta inteiro seja qual for o canto em que o terminal esteja, e que troca
  // de logar a cada empurrão para o deduplicador do `poe` o deixar passar.
  void empurra(const std::filesystem::path& imagem) noexcept;

  // escritas — quantas ordens sahiram pelo cano. É por ella que quem chama
  // sabe se o quadro mexeu na lousa: sómente ahi o empurrão tem que fazer.
  std::size_t escritas() const noexcept { return escritas_; }

  const Parecer& parecer() const noexcept { return parecer_; }
  // descartadas — as ordens que o cano cheio engoliu. A janella di-las no
  // stderr ao sahir, pelo molde da linha da vigilia: o --sonda é outra corrida
  // do programa e lousa viva alguma tem, d'onde ali o numero seria sempre zero.
  std::size_t descartadas() const noexcept { return descartadas_; }

 private:
  bool escreve(const std::string& ordem) noexcept;

  Parecer parecer_;
  bool vivo_ = false;
  int cano_ = -1;
  int filho_ = -1;
  std::size_t descartadas_ = 0;
  std::size_t escritas_ = 0;
  bool empurrao_ = false;  // o lado do proximo empurrão
  // O que está POSTO, e a ordem que o poz: é a comparação com ella que cala o
  // pintor quando nada mudou, e a lista das chaves que o tira_tudo percorre.
  std::map<std::string, std::string> postas_;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
