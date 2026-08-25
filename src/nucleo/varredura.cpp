// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA VARREDURA — src/nucleo/varredura.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. A taglib mora aqui e sómente aqui, atras do punho oculto.
//
// DOMÍNIO ......... raízes de acervo em disco, e o índice antigo se houver.
// CONTRA-DOMÍNIO .. o índice novo, e o progresso.
// INVARIANTE ...... funcção alguma d'aqui deixa escapar excepção do
//                   std::filesystem: acervo é disco alheio, e disco alheio
//                   falha. Toda falha conta-se e a corrida segue.
// Q.E.D. .......... o passo é a unidade, e o estado inteiro vive no punho: donde
//                   se pode parar entre dous passos sem perder cousa alguma.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/varredura.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace mysong::nucleo {

namespace {

// As extensões que se offerecem á taglib, em minuscula e com o ponto. A lista é
// FECHADA de proposito: extensão nova é decisão, e não descuido.
constexpr std::string_view kExtensoes[] = {".mp3", ".flac", ".ogg",  ".oga",
                                           ".opus", ".m4a", ".mp4", ".wav",
                                           ".wma", ".aac", ".ape", ".wv"};

std::string minuscula(std::string_view crua) {
  std::string baixa;
  baixa.reserve(crua.size());
  for (const char letra : crua)
    baixa += static_cast<char>(
        std::tolower(static_cast<unsigned char>(letra)));
  return baixa;
}


// numero_e_titulo — parte `NN - Titulo` no numero e no titulo. Sem numero á
// frente, o titulo é o nome inteiro e o numero fica zero, que quer dizer «sem
// numero» e não «faixa zero». O separador aceita-se com ou sem espaços, e tanto
// hyphen como ponto: `01 - Tear`, `01-Tear` e `01. Tear` sahem eguaes.
void numero_e_titulo(const std::string& talo, int* numero, std::string* titulo) {
  *numero = 0;
  *titulo = talo;
  std::size_t i = 0;
  while (i < talo.size() && std::isdigit(static_cast<unsigned char>(talo[i])))
    ++i;
  if (i == 0 || i > 3) return;  // sem digitos á frente, ou numero improvavel

  std::size_t j = i;
  while (j < talo.size() && talo[j] == ' ') ++j;
  if (j < talo.size() && (talo[j] == '-' || talo[j] == '.')) ++j;
  else if (j == i) return;  // digitos collados a letra: é nome, e não numero
  while (j < talo.size() && talo[j] == ' ') ++j;
  if (j >= talo.size()) return;  // sómente o numero, sem titulo depois

  *numero = std::stoi(talo.substr(0, i));
  *titulo = talo.substr(j);
}

}  // namespace

bool extensao_de_audio(std::string_view extensao) {
  const std::string baixa = minuscula(extensao);
  return std::find(std::begin(kExtensoes), std::end(kExtensoes), baixa) !=
         std::end(kExtensoes);
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
