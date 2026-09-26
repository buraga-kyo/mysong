#pragma once
#include "nucleo/biblioteca.hpp"
#include "nucleo/rol.hpp"

namespace mysong::nucleo {
struct Renomeacao {
  bool feita = false;
  std::string caminho, titulo, razao;
};
Renomeacao renomeia_arquivo(const std::string& caminho, const std::string& titulo,
                            Biblioteca& biblioteca, Roleiro& listas);
}  // namespace mysong::nucleo
