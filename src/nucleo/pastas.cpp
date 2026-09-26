#include "nucleo/pastas.hpp"
#include "nucleo/ajustes.hpp"
#include <fstream>
#include <iterator>
#include <unistd.h>
#include <vector>

namespace mysong::nucleo {
namespace {
std::filesystem::path le_pasta_xdg(std::string linha,
                                  const std::filesystem::path& casa) {
  const auto igual = linha.find('=');
  if (igual == std::string::npos || aparar(linha.substr(0, igual)) != "XDG_MUSIC_DIR")
    return {};
  std::string valor(aparar(linha.substr(igual + 1)));
  if (valor.size() < 2 || valor.front() != '"' || valor.back() != '"') return {};
  valor = valor.substr(1, valor.size() - 2);
  if (valor.compare(0, 5, "$HOME") == 0 && (valor.size() == 5 || valor[5] == '/'))
    valor.replace(0, 5, casa.string());
  std::string caminho;
  for (std::size_t posicao = 0; posicao < valor.size(); ++posicao) {
    if (valor[posicao] == '\\' && posicao + 1 < valor.size()) ++posicao;
    else if (valor[posicao] == '$' || valor[posicao] == '`') return {};
    caminho += valor[posicao];
  }
  const std::filesystem::path pasta(caminho);
  return pasta.is_absolute() && pasta.lexically_normal() != casa.lexically_normal()
             ? pasta : std::filesystem::path{};
}
}  // namespace
