#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include "nucleo/catalogo.hpp"
int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "uso: espia <arquivo.html|url>\n"; return 2; }
  mysong::nucleo::Catalogo c;
  const std::string alvo = argv[1];
  if (alvo.rfind("http", 0) == 0 || alvo.rfind("spotify:", 0) == 0) {
    std::cout << "id: " << mysong::nucleo::id_da_playlist(alvo) << "\n";
    std::cout << "embed: " << mysong::nucleo::url_do_embed(
        mysong::nucleo::id_da_playlist(alvo)) << "\n";
    if (!mysong::nucleo::busca_catalogo(alvo, &c)) { std::cerr << "a rede nao respondeu\n"; return 1; }
  } else {
    std::ifstream a(alvo);
    std::stringstream b; b << a.rdbuf();
    c = mysong::nucleo::le_catalogo(b.str());
  }
  std::printf("lista: %s   (%zu faixas)\n", c.nome.c_str(), c.faixas.size());
  std::size_t quantas = 0;
  for (const auto& f : c.faixas) {
    if (quantas++ >= 5) break;
    std::printf("  %2d. %-42s | %-28s | %d ms\n", f.numero,
                f.titulo.c_str(), f.artista.c_str(), f.duracao_ms);
  }
  return 0;
}
