// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LINHA DE COMMANDO, LAVRA — src/nucleo/linha.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabeçalho promette. Varre o argv uma vez, e a RECUSA tem
// precedencia: opção que a taboa não conhece pára a leitura ali mesmo, porque
// acceitar-lhe a companhia calada é o defeito que esta lavra veio corrigir.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/linha.hpp"

#include <string_view>

namespace mysong::nucleo {
namespace {

// Opção principia por traço e tem mais de um byte; traço sozinho é caminho.
bool e_opcao(std::string_view arg) { return arg.size() > 1 && arg.front() == '-'; }

}  // namespace

Invocacao ler_linha(int argc, const char* const* argv) {
  Invocacao invocacao;
  bool encerradas = false;
  bool quer_ajuda = false, quer_versao = false, quer_sonda = false;

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (encerradas || !e_opcao(arg)) invocacao.faixas.emplace_back(arg);
    else if (arg == "--") encerradas = true;
    else if (arg == "--ajuda" || arg == "--help") quer_ajuda = true;
    else if (arg == "--versao" || arg == "--version") quer_versao = true;
    else if (arg == "--sonda") quer_sonda = true;
    else {
      invocacao.modo = Modo::Recusa;
      invocacao.razao = "mysong: opcao desconhecida: " + std::string(arg) +
                        "\nCorra `mysong --ajuda` para ver as que existem.\n";
      return invocacao;
    }
  }

  if (quer_ajuda) invocacao.modo = Modo::Ajuda;
  else if (quer_versao) invocacao.modo = Modo::Versao;
  else if (quer_sonda) invocacao.modo = Modo::Sonda;
  return invocacao;
}

}  // namespace mysong::nucleo
