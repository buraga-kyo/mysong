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

std::filesystem::path pasta_de_musica(const std::filesystem::path& casa,
                                    const std::filesystem::path& configuracao) {
  if (casa.empty()) return {};
  std::ifstream arquivo(configuracao / "user-dirs.dirs");
  std::string linha;
  while (std::getline(arquivo, linha)) {
    const auto pasta = le_pasta_xdg(linha, casa);
    if (!pasta.empty()) return pasta;
  }
  for (const auto* nome : {"Música", "Músicas", "Musicas", "Musics", "Music",
                           "musica", "musicas", "musics", "music"}) {
    std::error_code erro;
    if (std::filesystem::is_directory(casa / nome, erro)) return casa / nome;
  }
  return casa / "Música";
}

bool salva_acervo(const std::filesystem::path& arquivo,
                  const std::filesystem::path& acervo, std::string* razao) {
  const auto recusa = [razao](const std::string& motivo) {
    if (razao) *razao = motivo;
    return false;
  };
  const std::string caminho = acervo.string();
  if (arquivo.empty() || !acervo.is_absolute() || caminho.find_first_of("#\r\n") != std::string::npos ||
      caminho.find('\0') != std::string::npos || aparar(caminho) != caminho)
    return recusa("use caminho absoluto, sem #, quebras ou espaços nas pontas");
  std::error_code erro;
  std::filesystem::create_directories(acervo, erro);
  if (erro) return recusa("não foi possível criar a biblioteca: " + erro.message());
  Ajustes leitura;
  std::string anterior;
  if (ler_o_arquivo(arquivo, &anterior, &leitura) == EstadoDoArquivo::Illegivel ||
      !leitura.queixas.empty()) return recusa("configuração ilegível ou grande demais");
  std::string novo;
  std::size_t inicio = 0;
  while (inicio < anterior.size()) {
    const auto fim = anterior.find('\n', inicio);
    const auto linha = anterior.substr(inicio, fim == std::string::npos ? fim : fim - inicio);
    const auto igual = linha.find('=');
    if (igual == std::string::npos || aparar(linha.substr(0, igual)) != "acervo")
      novo += linha + "\n";
    inicio = fim == std::string::npos ? anterior.size() : fim + 1;
  }
  novo += "acervo = " + caminho + "\n";
  std::filesystem::create_directories(arquivo.parent_path(), erro);
  if (erro) return recusa(erro.message());
  std::string molde = arquivo.string() + ".XXXXXX";
  std::vector<char> nome(molde.begin(), molde.end());
  nome.push_back('\0');
  const int descritor = ::mkstemp(nome.data());
  if (descritor < 0) return recusa("não foi possível criar a configuração temporária");
  const bool gravou = ::write(descritor, novo.data(), novo.size()) == static_cast<ssize_t>(novo.size()) &&
                      ::fsync(descritor) == 0;
  const bool fechou = ::close(descritor) == 0;
  if (gravou && fechou) std::filesystem::rename(nome.data(), arquivo, erro);
  if (!gravou || !fechou || erro) {
    std::filesystem::remove(nome.data(), erro);
    return recusa("não foi possível salvar a configuração");
  }
  return true;
}
}  // namespace mysong::nucleo
