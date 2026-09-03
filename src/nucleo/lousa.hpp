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

namespace mysong::nucleo {

// argumentos_da_lousa — o que se corre, pelo precedente do argumentos_do_chafa.
// `layer` é o modo de janella persistente; `--silent` manda o erro d'elle ao
// buraco, que esta Casa corre debaixo de uma tela do FTXUI e linha de aviso no
// meio do quadro estraga-o; `-o x11` é a sahida MEDIDA n'esta machina, a mesma
// que o yazi escolhe. A de wayland existe, e fica para quando houver Wayland.
std::vector<std::string> argumentos_da_lousa();

// escapado_em_json — as aspas, a barra invertida e os de controle. Á parte, e
// pura: aspa no nome do album partiria a linha ao meio, e o filho calava-se.
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

  const Parecer& parecer() const noexcept { return parecer_; }
  std::size_t descartadas() const noexcept { return descartadas_; }

 private:
  bool escreve(const std::string& ordem) noexcept;

  Parecer parecer_;
  bool vivo_ = false;
  int cano_ = -1;
  int filho_ = -1;
  std::size_t descartadas_ = 0;
  // O que está POSTO, e a ordem que o poz: é a comparação com ella que cala o
  // pintor quando nada mudou, e a lista das chaves que o tira_tudo percorre.
  std::map<std::string, std::string> postas_;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
