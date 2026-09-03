// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LOUSA — src/nucleo/lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o filho e o cano no fim.
//
// DOMÍNIO ......... o rectangulo em célullas, e o caminho de uma imagem.
// CONTRA-DOMÍNIO .. linhas de JSON, e um filho que morre com o tocador.
// INVARIANTE ...... funcção alguma d'aqui lança nem espera pelo filho.
// Q.E.D. .......... a composição do JSON não conhece o filho, d'onde a prova do
//                   protocolo corre sem X11 vivo.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/lousa.hpp"

namespace mysong::nucleo {

std::vector<std::string> argumentos_da_lousa() {
  return {"ueberzugpp", "layer", "--silent", "-o", "x11"};
}

std::string escapado_em_json(std::string_view texto) {
  std::string sahida;
  for (const char letra : texto) {
    const auto octeto = static_cast<unsigned char>(letra);
    if (letra == '"' || letra == '\\') {
      sahida += '\\';
      sahida += letra;
    } else if (octeto < 0x20) {
      // Os de controle vão TODOS na fórma longa: uma só cobre os trinta e
      // dous, e caminho com um d'elles dentro não merece taboa á parte.
      static const char kAlgarismos[] = "0123456789abcdef";
      sahida += "\\u00";
      sahida += kAlgarismos[octeto >> 4];
      sahida += kAlgarismos[octeto & 0x0F];
    } else {
      sahida += letra;
    }
  }
  return sahida;
}

std::string ordem_de_por(std::string_view identidade,
                         const std::filesystem::path& imagem, int collunha,
                         int linha, std::size_t largura, std::size_t altura) {
  return "{\"action\":\"add\",\"identifier\":\"" + escapado_em_json(identidade) +
         "\",\"x\":" + std::to_string(collunha) +
         ",\"y\":" + std::to_string(linha) +
         ",\"max_width\":" + std::to_string(largura) +
         ",\"max_height\":" + std::to_string(altura) + ",\"path\":\"" +
         escapado_em_json(imagem.string()) + "\"}\n";
}

std::string ordem_de_tirar(std::string_view identidade) {
  return "{\"action\":\"remove\",\"identifier\":\"" +
         escapado_em_json(identidade) + "\"}\n";
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
