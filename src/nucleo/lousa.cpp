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

#include "nucleo/aquisicao.hpp"  // corre(): o fork e o exec sem shell

#include <cstdlib>

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

Parecer parecer_da_lousa(ModoDaLousa modo, bool ha_display, bool ha_programa) {
  if (modo == ModoDaLousa::Nao)
    return {false, "desligada pelo ajuste: lousa = nao"};
  // A falta do PROGRAMA vem antes da do DISPLAY, e não é ordem gratuita: quem
  // não o installou ha de ler o remedio, e não «sem DISPLAY», que o mandaria
  // caçar defeito no servidor graphico por causa de um apt que falta.
  if (!ha_programa) return {false, "falta o ueberzugpp; ficam os symbolos"};
  if (!ha_display && modo != ModoDaLousa::Sim)
    return {false, "sem DISPLAY; a janella d'ella é de X11"};
  return {true, "X11"};
}

bool ha_display() {
  const char* const tela = std::getenv("DISPLAY");
  return tela != nullptr && tela[0] != '\0';
}

std::string versao_da_lousa() {
  // UMA chamada responde ás duas perguntas do diagnostico: se o programa está,
  // e qual é. Perguntar ao PATH á mão daria a primeira e não a segunda, e o
  // --sonda ha de dizer a versão, que é o que muda o protocolo por baixo de nós.
  std::string colhido;
  if (corre({"ueberzugpp", "--version"}, &colhido) != 0) return {};
  const std::size_t fim = colhido.find('\n');
  if (fim != std::string::npos) colhido.resize(fim);
  while (!colhido.empty() && colhido.back() == '\r') colhido.pop_back();
  return colhido;
}

std::string texto_da_lousa(const Parecer& parecer, std::string_view versao) {
  std::string texto = "\n  lousa: ";
  if (!parecer.de_pe) return texto + parecer.razao + "\n";
  // A versão vae CRUA como o programa a deu («ueberzugpp 2.9.8»): recortar-lhe
  // o nome para o tornar a escrever seria duas verdades a divergirem no dia em
  // que elle mudar a linha.
  texto += versao.empty() ? "ueberzugpp" : std::string(versao);
  return texto + ", " + parecer.razao + "\n";
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
