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

namespace mysong::nucleo {

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
