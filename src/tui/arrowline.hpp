// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA FITA ARROWLINE — src/tui/arrowline.hpp
// ══════════════════════════════════════════════════════════════════════════
// Compõe a FITA: enfiada de segmentos de borda esquerda RETA e ponta AFIADA á
// direita, encaixados um no outro por um glifo de junção.
//
// ADVERTÊNCIA DE FIDELIDADE, que se leia antes de tudo: no RADICAL-OS o
// arrowline não é glifo, é polygono traçado em cairo por shapes.powerline, e o
// angulo da diagonal é grandeza derivada, atan(tip / (h/2)) — cerca de 41,6
// graus da vertical com os tokens vivos. Em terminal de célullas isso não se
// copia: ha o glifo U+E0B0, cujo angulo a fonte já decidiu, e a célulla é
// indivisivel, de sorte que o sobrepôr de -T nem se propõe. É TRADUCÇÃO, e não
// cópia. Não se promette fidelidade geometrica; promettem-se as tres REGRAS do
// systema de desenho que a traducção conserva:
//   (a) seta de UMA direcção: borda esquerda reta, ponta afiada á direita.
//       JAMAIS losango de duas pontas, que é erro pregresso registrado.
//   (b) a côr da seta é a côr do segmento que ella SEGUE: a junção sahe com a
//       tinta do anterior e o fundo do seguinte. É d'isto que a fita parece
//       continua, em vez de rectangulos costurados.
//   (c) o enchimento fica um degrau REBAIXADO em relação á orla.
//
// DOMÍNIO ......... a enfiada de segmentos, cada um com rotulo, fundo e tinta.
// CONTRA-DOMÍNIO .. a sequencia ordenada de PEDAÇOS, inspeccionavel sem
//                   terminal e sem a janella — o que torna a regra (b)
//                   asserção verificavel, em vez de boa intenção.
// INVARIANTE ...... fita de N segmentos emitte exactamente N menos um pedaços
//                   de junção INTERNA. A junção de CAUDA, que remata a fita em
//                   ponta, conta-se á parte e nunca entra n'aquelle N-1.
// Q.E.D. .......... a guarnição do rotulo e o córte por largura ficam com quem
//                   desenha, e não com este primitivo: cortar aqui obrigaria a
//                   cortar no meio de um par tinta/fundo, que é a emenda
//                   visivel que o aceite proscreve.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "tui/tokens.hpp"

namespace mysong::tui {

// Os glifos da fita, da JetBrainsMono Nerd Font. U+E0B0 aponta á dextra;
// U+E0B2, á esquerda. NUNCA os dous no mesmo sentido de fita.
// Escrevem-se por PONTO DE CODIGO, e não pelo glifo cru: moram na area de uso
// privado, onde editor, tubo e terminal os engolem sem dar signal, e o que
// resta é cadeia vazia — falha que passaria calada por toda a fita.
inline constexpr std::string_view kPontaDextra = "\ue0b0";
inline constexpr std::string_view kPontaEsquerda = "\ue0b2";

// O sentido em que a fita aponta. Um só por fita: misturar os dous lavraria o
// losango que a regra (a) proscreve.
enum class Sentido { Dextra, Esquerda };

// Um segmento: o rotulo que mostra, o fundo que o veste, a tinta que o
// escreve. O rotulo entra COMO SE HA DE MOSTRAR, guarnição inclusa: a folga
// dos flancos é do desenhista, e o primitivo não lha acrescenta ás escondidas.
struct Segmento {
  std::string rotulo;
  std::string_view fundo = tokens::v900;
  std::string_view tinta = tokens::text_bright;
};

// Um PEDAÇO já composto: o texto e o par de côres com que se emitte. Os dous
// predicados dizem o que o pedaço É, para que a prova conte sem adivinhar:
// `juncao` marca o glifo de encaixe, e `cauda` distingue o remate final das
// junções INTERNAS, que são as que valem no N menos um.
struct Pedaco {
  std::string texto;
  std::string_view fundo;
  std::string_view tinta;
  bool juncao = false;
  bool cauda = false;
};

// rebaixar — desce ao degrau de CENTENA seguinte da rampa violeta, que é dizer
// DOUS assentos do arranjo, e não um: os intermedios (v600, v800) o olho não
// distingue da vizinhança. É a regra (c), o enchimento sob a orla. Assim v500
// dá v700, e v700 dá v900. No degrau mais fundo satura, em vez de sahir da
// rampa, e côr de fóra da rampa devolve-se intacta.
std::string_view rebaixar(std::string_view degrau);

// A FITA: junta segmentos e compõe pedaços. Não pinta nem trunca.
class Fita {
 public:
  explicit Fita(Sentido sentido = Sentido::Dextra, bool cauda = true);
  Fita& junta(Segmento segmento);
  Fita& glifo(std::string outro);  // terminal sem Nerd Font troca aqui
  std::vector<Pedaco> compor() const;
  std::size_t largura_exigida() const;  // em CODEPOINTS, não em collunas
  const std::vector<Segmento>& segmentos() const noexcept { return segmentos_; }
 private:
  std::vector<Segmento> segmentos_;
  std::string glifo_;
  Sentido sentido_;
  bool cauda_;
};

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
