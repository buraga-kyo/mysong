// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO JSONZINHO, src/api/jsonzinho.hpp
// ══════════════════════════════════════════════════════════════════════════
// O JSON d'esta Casa, e SÓ o d'esta Casa. Não é bibliotheca geral: é o
// subconjunto PLANO que o protocolo do socket usa, um objecto de UM nivel, com
// valores escalares, e mais nada. Declara-se só-cabeçalho, á maneira do
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
//                   subconjunto. Rejeitar não é falhar, «json_malformado» é
//                   resposta legitima e prevista, e parser permissivo é que
//                   seria o risco, porque acceitaria por adivinhação o que o
//                   contracto não promette. Sendo ambas as bandas funcções
//                   puras de cadeia para cadeia, provam-se sem socket algum.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstdio>
#include <cstdlib>
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

// Os vectores de NUMERO, irmãos do de textos. Nascem porque as bandas do
// espectro, os numeros de faixa e as durações do acervo SÃO numero: emitti-los
// como texto obrigaria quem lê a converter de volta o que já era conta, e
// abriria a porta a duas escriptas do mesmo valor.
inline std::string vector_de_duplos(const std::vector<float>& itens) {
  std::string obra = "[";
  for (std::size_t i = 0; i < itens.size(); ++i) {
    if (i != 0) obra += ',';
    obra += duplo(static_cast<double>(itens[i]));
  }
  obra += ']';
  return obra;
}

inline std::string vector_de_inteiros(const std::vector<long long>& itens) {
  std::string obra = "[";
  for (std::size_t i = 0; i < itens.size(); ++i) {
    if (i != 0) obra += ',';
    obra += inteiro(itens[i]);
  }
  obra += ']';
  return obra;
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

// «Vector» é o vector de textos OU o de numeros, e nunca os dous no mesmo: são
// os unicos valores não escalares que este contracto emitte (as faixas da fila,
// as bandas do espectro, os numeros e as durações do acervo), e admitti-los na
// leitura é o que faz a nossa propria sahida voltar a entrar. Assymetria entre o
// que se emitte e o que se lê é armadilha para quem escrever cliente com esta
// mesma peça, e por isso a leitura do numero entrou junto com a emissão d'elle.
enum class Typo { Texto, Numero, Booleano, Nulo, Vector };

struct Valor {
  Typo typo = Typo::Nulo;
  std::string texto;      // já DESESCAPADO, e em UTF-8
  double numero = 0.0;
  bool booleano = false;
  std::vector<std::string> itens;    // sómente quando typo == Typo::Vector
  std::vector<double> numeros;       // idem, e o vector é HOMOGENEO: ou um, ou outro
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
// O NUMERO. Colhe-se a extensão dos caracteres que um numero pode ter, e
// entrega-se ao strtod, que é quem sabe as regras; mas exige-se que elle tenha
// consumido a extensão INTEIRA. Sem essa exigencia, «1.2.3» passaria como 1.2, e
// o cliente ficaria a crer que mandou o que não mandou.
inline bool Leitor::numero(double* fora, std::string* razao) {
  const std::size_t comeco = i_;
  bool houve_cifra = false;
  while (!acabou()) {
    const char letra = olha();
    const bool serve = (letra >= '0' && letra <= '9') || letra == '.' ||
                       letra == '-' || letra == '+' || letra == 'e' ||
                       letra == 'E';
    if (!serve) break;
    if (letra >= '0' && letra <= '9') houve_cifra = true;
    ++i_;
  }
  if (!houve_cifra) { *razao = "esperava-se um numero"; return false; }
  const std::string molde(fonte_.substr(comeco, i_ - comeco));
  char* fim = nullptr;
  const double valor = std::strtod(molde.c_str(), &fim);
  if (fim == nullptr || *fim != '\0') {
    *razao = "numero mal formado: " + molde;
    return false;
  }
  *fora = valor;
  return true;
}
// LE_VALOR: despacha pelo primeiro caracter, que no JSON basta. Palavra que não
// seja «true», «false» ou «null» recusa-se por nome, e não se lê adiante como se
// fosse cousa.
inline bool le_valor(Leitor* leitor, Valor* fora, std::string* razao) {
  const char guia = leitor->olha();
  if (guia == '"') {
    fora->typo = Typo::Texto;
    return leitor->cadeia(&fora->texto, razao);
  }
  if (guia == 't' || guia == 'f' || guia == 'n') {
    std::string palavra;
    while (leitor->olha() >= 'a' && leitor->olha() <= 'z') palavra += leitor->toma();
    if (palavra == "true" || palavra == "false") {
      fora->typo = Typo::Booleano;
      fora->booleano = (palavra == "true");
      return true;
    }
    if (palavra == "null") { fora->typo = Typo::Nulo; return true; }
    *razao = "palavra que nao e valor d'este subconjunto: " + palavra;
    return false;
  }
  if (guia == '[') {
    fora->typo = Typo::Vector;
    leitor->toma();  // o [ de abertura
    leitor->come_brancos();
    // O vector é HOMOGENEO: ou de textos, ou de numeros. Mistura RECUSA-SE, e
    // não se acolhe em silêncio: vector misto não é cousa que este contracto
    // emitta, e acceitá-lo na leitura seria acceitar o que nós não produzimos.
    bool primeiro = true;
    while (leitor->olha() != ']') {
      if (!primeiro) {
        if (leitor->toma() != ',') { *razao = "esperava-se , ou ] no vector"; return false; }
        leitor->come_brancos();
      }
      primeiro = false;
      if (leitor->olha() == '"') {
        if (!fora->numeros.empty()) { *razao = "vector de texto e numero misturados"; return false; }
        std::string item;
        if (!leitor->cadeia(&item, razao)) return false;
        fora->itens.push_back(std::move(item));
      } else {
        if (!fora->itens.empty()) { *razao = "vector de texto e numero misturados"; return false; }
        double item = 0.0;
        if (!leitor->numero(&item, razao)) return false;
        fora->numeros.push_back(item);
      }
      leitor->come_brancos();
    }
    leitor->toma();  // o ] de fecho
    return true;
  }
  fora->typo = Typo::Numero;
  return leitor->numero(&fora->numero, razao);
}
}  // namespace intimo

// ANALYSA. A linha inteira há de ser UM objecto plano, e nada mais: nem vector,
// nem escalar solto, nem dous objectos em fila, nem sobra depois do fecho. Chave
// repetida recusa-se, que de outra sorte a ultima venceria em silêncio e o
// cliente nunca saberia qual das duas o servidor obedeceu.
inline Mensagem analysa(std::string_view linha) {
  Mensagem obra;
  intimo::Leitor leitor(linha);
  leitor.come_brancos();
  if (leitor.toma() != '{') {
    obra.razao = "a mensagem ha de ser um objecto JSON, e comecar por {";
    return obra;
  }
  leitor.come_brancos();
  bool primeiro = true;
  while (leitor.olha() != '}') {
    if (!primeiro) {
      if (leitor.toma() != ',') {
        obra.razao = "esperava-se , ou } entre os pares";
        return obra;
      }
      leitor.come_brancos();
    }
    primeiro = false;
    std::string chave;
    if (!leitor.cadeia(&chave, &obra.razao)) return obra;
    if (obra.pares.count(chave) != 0) {
      obra.razao = "chave repetida: " + chave;
      return obra;
    }
    leitor.come_brancos();
    if (leitor.toma() != ':') {
      obra.razao = "esperava-se : depois da chave " + chave;
      return obra;
    }
    leitor.come_brancos();
    Valor valor;
    if (!intimo::le_valor(&leitor, &valor, &obra.razao)) return obra;
    obra.pares.emplace(std::move(chave), std::move(valor));
    leitor.come_brancos();
  }
  leitor.toma();  // o } de fecho
  leitor.come_brancos();
  if (!leitor.acabou()) {
    obra.razao = "ha sobra depois do } de fecho";
    return obra;
  }
  obra.valida = true;
  return obra;
}
// ── O RECORTE de JSON ANINHADO (issue #13) ──────────────────────────────────
// O leitor d'esta Casa lê objecto PLANO de um nivel, que é a fórma do socket e a do
// LRCLIB. O catalogo do Spotify não é plana: ella tras objecto dentro de objecto. As
// tres funcções abaixo não a analysam inteira; recortam o que se quer, respeitando
// aspas, contra-barra e FUNDO, que é o bastante e não pede leitor novo.

// objectos_do_arranjo, os objectos de fundo UM de um arranjo, cada um em texto.
// Vazio quando não ha arranjo, ou quando elle vem truncado: objecto meio não sahe.
//
// Vive AQUI, e não em quem o usa. A letra da issue #14 tinha o seu recorte, e o
// catalogo da #13 precisaria de outro egual: duas cópias da mesma conta dão duas
// verdades, e a que se corrigisse deixava a outra a errar.
inline std::vector<std::string> objectos_do_arranjo(std::string_view arranjo) {
  std::vector<std::string> achados;
  int fundo = 0;
  std::size_t principio = 0;
  bool dentro_de_aspas = false, escapado = false;
  for (std::size_t i = 0; i < arranjo.size(); ++i) {
    const char octeto = arranjo[i];
    // A ordem d'estas tres guardas é load-bearing. O escapado consome-se antes de
    // tudo; as aspas mudam o modo; e sómente FÓRA das aspas as chaves contam. Sem
    // isto, um titulo que traga `}` fecharia o objecto a meio.
    if (escapado) { escapado = false; continue; }
    if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
    if (octeto == '"') { dentro_de_aspas = !dentro_de_aspas; continue; }
    if (dentro_de_aspas) continue;
    if (octeto == '{') {
      if (fundo == 0) principio = i;
      ++fundo;
    } else if (octeto == '}' && fundo > 0 && --fundo == 0) {
      achados.push_back(std::string(arranjo.substr(principio, i - principio + 1)));
    }
  }
  return achados;
}

// recorta_arranjo, o arranjo que a chave `"<nome>":[` abre, com os cochetes. Vazio
// não havendo a chave, ou vindo o arranjo truncado. Acha a chave em QUALQUER fundo,
// que é o que permitte pescar `trackList` de dentro de dez niveis de embrulho.
inline std::string recorta_arranjo(std::string_view corpo, std::string_view nome) {
  const std::string agulha = "\"" + std::string(nome) + "\"";
  std::size_t onde = 0;
  while ((onde = corpo.find(agulha, onde)) != std::string_view::npos) {
    std::size_t i = onde + agulha.size();
    while (i < corpo.size() && (corpo[i] == ' ' || corpo[i] == ':')) ++i;
    if (i >= corpo.size() || corpo[i] != '[') {
      onde += agulha.size();
      continue;  // esta chave não abre arranjo: procura-se a proxima egual
    }
    int fundo = 0;
    bool dentro_de_aspas = false, escapado = false;
    for (std::size_t j = i; j < corpo.size(); ++j) {
      const char octeto = corpo[j];
      if (escapado) { escapado = false; continue; }
      if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
      if (octeto == '"') { dentro_de_aspas = !dentro_de_aspas; continue; }
      if (dentro_de_aspas) continue;
      if (octeto == '[') ++fundo;
      else if (octeto == ']' && --fundo == 0)
        return std::string(corpo.substr(i, j - i + 1));
    }
    return {};  // truncado
  }
  return {};
}

// recorta_objecto, o objecto que a chave `"<nome>":{` abre, com as chaves de
// fóra, achado em QUALQUER fundo. Vazio não havendo a chave, ou vindo truncado.
// Irmão do recorta_arranjo, e vive aqui pela mesma razão d'elle: o MusicBrainz da
// issue #57 tras a gravação embrulhada dentro da relação, e uma segunda cópia
// d'esta conta seria uma segunda verdade.
inline std::string recorta_objecto(std::string_view corpo, std::string_view nome) {
  const std::string agulha = "\"" + std::string(nome) + "\"";
  std::size_t onde = 0;
  while ((onde = corpo.find(agulha, onde)) != std::string_view::npos) {
    std::size_t i = onde + agulha.size();
    while (i < corpo.size() && (corpo[i] == ' ' || corpo[i] == ':')) ++i;
    if (i >= corpo.size() || corpo[i] != '{') {
      onde += agulha.size();
      continue;  // esta chave não abre objecto: procura-se a proxima egual
    }
    int fundo = 0;
    bool dentro_de_aspas = false, escapado = false;
    for (std::size_t j = i; j < corpo.size(); ++j) {
      const char octeto = corpo[j];
      if (escapado) { escapado = false; continue; }
      if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
      if (octeto == '"') { dentro_de_aspas = !dentro_de_aspas; continue; }
      if (dentro_de_aspas) continue;
      if (octeto == '{') ++fundo;
      else if (octeto == '}' && --fundo == 0)
        return std::string(corpo.substr(i, j - i + 1));
    }
    return {};  // truncado
  }
  return {};
}

// textos_do_arranjo, os TEXTOS de fundo um de um arranjo, já desescapados. Serve
// ao `isrcs` do MusicBrainz, que é arranjo de cadeias e não de objectos. Cadeia de
// dentro de objecto não sahe, e cadeia que não se deixe ler descarta-se sozinha.
inline std::vector<std::string> textos_do_arranjo(std::string_view arranjo) {
  std::vector<std::string> achados;
  int fundo = 0;
  bool dentro_de_aspas = false, escapado = false;
  std::size_t principio = 0;
  for (std::size_t i = 0; i < arranjo.size(); ++i) {
    const char octeto = arranjo[i];
    if (escapado) { escapado = false; continue; }
    if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
    if (octeto == '"') {
      if (!dentro_de_aspas) { dentro_de_aspas = true; principio = i; continue; }
      dentro_de_aspas = false;
      if (fundo != 1) continue;  // cadeia de dentro de objecto não é item
      intimo::Leitor leitor(arranjo.substr(principio, i - principio + 1));
      std::string valor, razao;
      if (leitor.cadeia(&valor, &razao)) achados.push_back(std::move(valor));
      continue;
    }
    if (dentro_de_aspas) continue;
    if (octeto == '{' || octeto == '[') ++fundo;
    else if (octeto == '}' || octeto == ']') --fundo;
  }
  return achados;
}

// texto_de_chave, o valor de texto de uma chave de FUNDO UM do objecto, já
// desescapado. Vazio quando ella falta, ou quando o valor não é texto. O fundo
// importa: `title` dentro de `audioPreview` não é o `title` da faixa.
inline std::string texto_de_chave(std::string_view objecto, std::string_view nome) {
  const std::string agulha = "\"" + std::string(nome) + "\"";
  int fundo = 0;
  bool dentro_de_aspas = false, escapado = false;
  for (std::size_t i = 0; i < objecto.size(); ++i) {
    const char octeto = objecto[i];
    if (escapado) { escapado = false; continue; }
    if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
    if (octeto == '"' && !dentro_de_aspas && fundo == 1 &&
        objecto.compare(i, agulha.size(), agulha) == 0) {
      std::size_t j = i + agulha.size();
      while (j < objecto.size() && (objecto[j] == ' ' || objecto[j] == ':')) ++j;
      if (j >= objecto.size() || objecto[j] != '"') return {};
      intimo::Leitor leitor(objecto.substr(j));
      std::string valor, razao;
      if (!leitor.cadeia(&valor, &razao)) return {};
      return valor;
    }
    if (octeto == '"') { dentro_de_aspas = !dentro_de_aspas; continue; }
    if (dentro_de_aspas) continue;
    if (octeto == '{' || octeto == '[') ++fundo;
    else if (octeto == '}' || octeto == ']') --fundo;
  }
  return {};
}

// numero_de_chave, o mesmo, para numero. `fóra` fica intacto não havendo chave.
inline bool numero_de_chave(std::string_view objecto, std::string_view nome,
                           double* fora) {
  const std::string agulha = "\"" + std::string(nome) + "\"";
  int fundo = 0;
  bool dentro_de_aspas = false, escapado = false;
  for (std::size_t i = 0; i < objecto.size(); ++i) {
    const char octeto = objecto[i];
    if (escapado) { escapado = false; continue; }
    if (octeto == '\\' && dentro_de_aspas) { escapado = true; continue; }
    if (octeto == '"' && !dentro_de_aspas && fundo == 1 &&
        objecto.compare(i, agulha.size(), agulha) == 0) {
      std::size_t j = i + agulha.size();
      while (j < objecto.size() && (objecto[j] == ' ' || objecto[j] == ':')) ++j;
      intimo::Leitor leitor(objecto.substr(j));
      std::string razao;
      return leitor.numero(fora, &razao);
    }
    if (octeto == '"') { dentro_de_aspas = !dentro_de_aspas; continue; }
    if (dentro_de_aspas) continue;
    if (octeto == '{' || octeto == '[') ++fundo;
    else if (octeto == '}' || octeto == ']') --fundo;
  }
  return false;
}

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
