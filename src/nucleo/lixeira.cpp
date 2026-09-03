// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LIXEIRA, LAVRA — src/nucleo/lixeira.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabeçalho. As tres regras da especificação freedesktop vivem aqui, e
// sómente aqui: o nome do par, o percent-encoding do `Path=`, e a data local.
//
// DOMÍNIO ......... um caminho, e a raiz da lixeira.
// CONTRA-DOMÍNIO .. o par arquivo e bilhete, e o desfecho por escripto.
// INVARIANTE ...... funcção alguma d'este arquivo lança pela borda: falta de
//                   disco e falta de permissão sahem por `razao`.
// Q.E.D. .......... o caminho lê-se do ambiente n'um logar só, donde a lixeira
//                   que a tela usa e a que a sonda diria são a mesma.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/lixeira.hpp"

#include <cstdlib>
#include <ctime>

namespace mysong::nucleo {

// A barra CONSERVA-SE, e sómente ella entre as reservadas: o campo guarda um
// caminho, e escapá-la faria o gerenciador de arquivos ler um nome de arquivo
// com barras dentro. Espaço e acento, que é o que o acervo d'elle tem aos
// montes, sahem em `%20` e em dous octetos de `%XX` cada.
std::string escapa_o_caminho(std::string_view cru) {
  static constexpr char kAlgarismos[] = "0123456789ABCDEF";
  std::string url;
  url.reserve(cru.size());
  for (const char bruto : cru) {
    const unsigned char letra = static_cast<unsigned char>(bruto);
    if ((letra >= 'A' && letra <= 'Z') || (letra >= 'a' && letra <= 'z') ||
        (letra >= '0' && letra <= '9') || letra == '-' || letra == '_' ||
        letra == '.' || letra == '~' || letra == '/') {
      url += bruto;
      continue;
    }
    url += '%';
    url += kAlgarismos[letra >> 4];
    url += kAlgarismos[letra & 0x0Fu];
  }
  return url;
}

// Sem fuso escripto, que é o que a especificação pede: a hora é a do relogio de
// quem apagou, e é assim que o gerenciador de arquivos a mostra.
std::string data_da_exclusao(std::time_t quando) {
  std::tm partido = {};
  ::localtime_r(&quando, &partido);
  char linha[32] = {0};
  std::strftime(linha, sizeof(linha), "%Y-%m-%dT%H:%M:%S", &partido);
  return std::string(linha);
}

// O mesmo desenho do caminho_do_indice da janella, e de proposito: uma Casa que
// leia `$XDG_DATA_HOME` de duas maneiras teria dous logares de dados no dia em
// que o operador a assentasse. Directorio algum se cria aqui: quem cria é quem
// manda á lixeira, que é quem sabe se ha alguma cousa a mandar.
std::filesystem::path caminho_da_lixeira() {
  const char* dados = std::getenv("XDG_DATA_HOME");
  if (dados != nullptr && dados[0] != '\0')
    return std::filesystem::path(dados) / "Trash";
  const char* casa = std::getenv("HOME");
  if (casa == nullptr || casa[0] == '\0') return {};
  return std::filesystem::path(casa) / ".local" / "share" / "Trash";
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
