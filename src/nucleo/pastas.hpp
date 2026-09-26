#pragma once
#include <filesystem>
#include <string>

namespace mysong::nucleo {
std::filesystem::path pasta_de_musica(const std::filesystem::path& casa,
                                    const std::filesystem::path& configuracao);
bool salva_acervo(const std::filesystem::path& arquivo,
                  const std::filesystem::path& acervo, std::string* razao);
}  // namespace mysong::nucleo
