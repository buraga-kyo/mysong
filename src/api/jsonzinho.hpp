// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO JSONZINHO — src/api/jsonzinho.hpp
// ══════════════════════════════════════════════════════════════════════════
// O JSON d'esta Casa, e SÓ o d'esta Casa. Não é bibliotheca geral: é o
// subconjunto PLANO que o protocolo do socket usa — um objecto de UM nivel, com
// valores escalares — e mais nada. Declara-se só-cabeçalho, á maneira do
// aparar_volume que mora no tractado do motor, para que unidade alguma se
// compile por causa d'elle.
//
// DOMÍNIO ......... de um lado, cadeias cruas do systema de arquivos, que
//                   trazem aspas, contra-barra, mudança de linha e UTF-8 de
//                   mais de um byte; do outro, uma linha que chegou pelo socket
//                   e que pode ser qualquer cousa, inclusive lixo.
// CONTRA-DOMÍNIO .. JSON valido de UMA linha, e uma Mensagem que ou é valida ou
//                   traz a razão por que não é.
// INVARIANTE ...... o escape NUNCA deixa passar byte que parta o enquadramento.
//                   Uma mudança de linha crua no nome de uma faixa partiria a
//                   mensagem em duas, e o cliente do outro lado leria metade de
//                   uma e metade da seguinte: é este o defeito que o escape
//                   existe para impedir, e não a elegancia.
// Q.E.D. .......... o parser é ESTRICTO de proposito: rejeita tudo o que sae do
//                   subconjunto. Rejeitar não é falhar — «json_malformado» é
//                   resposta legitima e prevista, e parser permissivo é que
//                   seria o risco, porque acceitaria por adivinhação o que o
//                   contracto não promette. Sendo ambas as bandas funcções
//                   puras de cadeia para cadeia, provam-se sem socket algum.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::api {

// ESCAPA a cadeia crua no corpo de uma cadeia JSON (SEM as aspas de fóra). O
// UTF-8 passa INTACTO, byte a byte: o JSON o admitte cru, e transcrevê-lo em
// \u seria inflar a mensagem para nada. O que NÃO passa é byte de controle,
// que vae em \u00XX; e no meio d'elles está a mudança de linha, que é a que
// partiria o enquadramento de uma-mensagem-por-linha.
inline std::string escapa(std::string_view cru) {
  std::string obra;
  obra.reserve(cru.size() + 8);
  for (const char bruto : cru) {
    const unsigned char byte = static_cast<unsigned char>(bruto);
    switch (byte) {
      case '"':  obra += "\\\""; continue;
      case '\\': obra += "\\\\"; continue;
      case '\b': obra += "\\b";  continue;
      case '\f': obra += "\\f";  continue;
      case '\n': obra += "\\n";  continue;
      case '\r': obra += "\\r";  continue;
      case '\t': obra += "\\t";  continue;
      default: break;
    }
    if (byte < 0x20u) {
      char cifra[7];
      std::snprintf(cifra, sizeof(cifra), "\\u%04x", static_cast<unsigned>(byte));
      obra += cifra;
      continue;
    }
    obra += bruto;
  }
  return obra;
}


// ─── A EMISSÃO. Cada escalar sahe já pontuado, e quem os junta não repontua ──

inline std::string texto(std::string_view v) { return "\"" + escapa(v) + "\""; }
inline std::string inteiro(long long v) { return std::to_string(v); }
inline std::string booleano(bool v) { return v ? "true" : "false"; }

// Tres casas, e não a precisão inteira do duplo: posição e duração são segundos
// para olho e para relogio de cliente, e «12.340000000000001» não serve a nenhum
// dos dous. Valor que não seja finito sahe ZERO, porque «nan» não é JSON.
inline std::string duplo(double v) {
  if (!(v > -1e18 && v < 1e18)) return "0.000";
  char cifra[32];
  std::snprintf(cifra, sizeof(cifra), "%.3f", v);
  return cifra;
}

inline std::string vector_de_textos(const std::vector<std::string>& itens) {
  std::string obra = "[";
  for (std::size_t i = 0; i < itens.size(); ++i) {
    if (i != 0) obra += ',';
    obra += texto(itens[i]);
  }
  obra += ']';
  return obra;
}

// O OBJECTO guarda a ORDEM em que os pares se juntaram, e não a alphabetica de
// um mapa: a resposta há de começar por «ok», que é o que o cliente lê primeiro,
// e ordem estavel é o que faz o documento poder trazer exemplo verbatim.
class Objecto {
 public:
  Objecto& par(std::string_view chave, const std::string& ja_emittido) {
    if (!corpo_.empty()) corpo_ += ',';
    corpo_ += texto(chave);
    corpo_ += ':';
    corpo_ += ja_emittido;
    return *this;
  }
  std::string fecha() const { return "{" + corpo_ + "}"; }

 private:
  std::string corpo_;
};

// ─── A LEITURA. Quatro typos, e não mais: é o que o contracto promette ──────

enum class Typo { Texto, Numero, Booleano, Nulo };

struct Valor {
  Typo typo = Typo::Nulo;
  std::string texto;      // já DESESCAPADO, e em UTF-8
  double numero = 0.0;
  bool booleano = false;
};

// A mensagem lida. Ou é valida, ou traz a RAZÃO por que não é: não há terceiro
// estado, e não há mensagem invalida sem razão dita, porque razão calada é o
// silêncio que a issue proibiu.
struct Mensagem {
  bool valida = false;
  std::string razao;
  std::map<std::string, Valor> pares;

  const Valor* acha(const std::string& chave) const {
    const auto assento = pares.find(chave);
    return assento == pares.end() ? nullptr : &assento->second;
  }
};
namespace intimo {

// O LEITOR: um cursor sobre a linha, e nada mais. Não copia a fonte.
class Leitor {
 public:
  explicit Leitor(std::string_view fonte) noexcept : fonte_(fonte) {}

  bool acabou() const noexcept { return i_ >= fonte_.size(); }
  char olha() const noexcept { return acabou() ? '\0' : fonte_[i_]; }
  char toma() noexcept { return acabou() ? '\0' : fonte_[i_++]; }

  void come_brancos() noexcept {
    while (!acabou() && (olha() == ' ' || olha() == '\t' || olha() == '\r' ||
                         olha() == '\n'))
      ++i_;
  }

  bool cadeia(std::string* fora, std::string* razao);
  bool numero(double* fora, std::string* razao);

 private:
  bool quatro_hexas(unsigned* fora, std::string* razao);
  bool ponto_de_codigo(std::string* fora, std::string* razao);

  std::string_view fonte_;
  std::size_t i_ = 0;
};

// EM_UTF8: o ponto de codigo sahe nos mesmos bytes em que o resto da mensagem
// já vae, e não numa segunda codificação: mensagem de duas codificações é
// mensagem que o cliente há de adivinhar.
inline void em_utf8(unsigned ponto, std::string* fora) {
  if (ponto < 0x80u) {
    *fora += static_cast<char>(ponto);
  } else if (ponto < 0x800u) {
    *fora += static_cast<char>(0xC0u | (ponto >> 6));
    *fora += static_cast<char>(0x80u | (ponto & 0x3Fu));
  } else if (ponto < 0x10000u) {
    *fora += static_cast<char>(0xE0u | (ponto >> 12));
    *fora += static_cast<char>(0x80u | ((ponto >> 6) & 0x3Fu));
    *fora += static_cast<char>(0x80u | (ponto & 0x3Fu));
  } else {
    *fora += static_cast<char>(0xF0u | (ponto >> 18));
    *fora += static_cast<char>(0x80u | ((ponto >> 12) & 0x3Fu));
    *fora += static_cast<char>(0x80u | ((ponto >> 6) & 0x3Fu));
    *fora += static_cast<char>(0x80u | (ponto & 0x3Fu));
  }
}

inline bool Leitor::quatro_hexas(unsigned* fora, std::string* razao) {
  unsigned somma = 0;
  for (int casa = 0; casa < 4; ++casa) {
    const char cifra = toma();
    unsigned valor = 0;
    if (cifra >= '0' && cifra <= '9') valor = static_cast<unsigned>(cifra - '0');
    else if (cifra >= 'a' && cifra <= 'f') valor = static_cast<unsigned>(cifra - 'a') + 10u;
    else if (cifra >= 'A' && cifra <= 'F') valor = static_cast<unsigned>(cifra - 'A') + 10u;
    else { *razao = "o escape \\u pede quatro cifras hexadecimaes"; return false; }
    somma = (somma << 4) | valor;
  }
  *fora = somma;
  return true;
}
// O PAR DE SUBSTITUTOS. Cliente que use bibliotheca de JSON manda o UTF-8 cru e
// nunca cae aqui; mas quem escapar tudo em \u há de ser lido igual, e substituto
// solto é recusa dita, e não byte torto emittido adiante.
inline bool Leitor::ponto_de_codigo(std::string* fora, std::string* razao) {
  unsigned alto = 0;
  if (!quatro_hexas(&alto, razao)) return false;
  unsigned ponto = alto;
  if (alto >= 0xD800u && alto <= 0xDBFFu) {
    if (toma() != '\\' || toma() != 'u') {
      *razao = "substituto alto sem o par que o completa";
      return false;
    }
    unsigned baixo = 0;
    if (!quatro_hexas(&baixo, razao)) return false;
    if (baixo < 0xDC00u || baixo > 0xDFFFu) {
      *razao = "substituto baixo fora da faixa";
      return false;
    }
    ponto = 0x10000u + ((alto - 0xD800u) << 10) + (baixo - 0xDC00u);
  } else if (alto >= 0xDC00u && alto <= 0xDFFFu) {
    *razao = "substituto baixo solto, sem o alto que o precede";
    return false;
  }
  em_utf8(ponto, fora);
  return true;
}
// A CADEIA. Byte de controle CRU dentro d'ella é recusa: é justamente o que o
// nosso escape nunca emitte, donde acceitá-lo na entrada seria acceitar o que
// nós mesmos não produzimos.
inline bool Leitor::cadeia(std::string* fora, std::string* razao) {
  if (toma() != '"') { *razao = "esperava-se cadeia entre aspas"; return false; }
  fora->clear();
  for (;;) {
    if (acabou()) { *razao = "cadeia sem a aspa de fecho"; return false; }
    const unsigned char byte = static_cast<unsigned char>(toma());
    if (byte == '"') return true;
    if (byte < 0x20u) {
      *razao = "byte de controle cru dentro de cadeia";
      return false;
    }
    if (byte != '\\') { *fora += static_cast<char>(byte); continue; }
    switch (toma()) {
      case '"':  *fora += '"';  break;
      case '\\': *fora += '\\'; break;
      case '/':  *fora += '/';  break;
      case 'b':  *fora += '\b'; break;
      case 'f':  *fora += '\f'; break;
      case 'n':  *fora += '\n'; break;
      case 'r':  *fora += '\r'; break;
      case 't':  *fora += '\t'; break;
      case 'u':  if (!ponto_de_codigo(fora, razao)) return false; break;
      default:
        *razao = "escape que este subconjunto nao conhece";
        return false;
    }
  }
}
}  // namespace intimo
}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
