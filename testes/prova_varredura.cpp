// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA VARREDURA — testes/prova_varredura.cpp
// ══════════════════════════════════════════════════════════════════════════
// Duas metades. A derivação do caminho prova-se em cadeias, sem disco algum; a
// varredura prova-se sobre acervos que esta bateria FABRICA em directorio
// temporario. Caso algum toca `~/Música` nem `~/.local/share/mysong`.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include <taglib/fileref.h>
#include <taglib/tag.h>

#include "nucleo/biblioteca.hpp"
#include "nucleo/varredura.hpp"

namespace nu = mysong::nucleo;

namespace {

// Uma COVA propria por caso, para que corrida alguma veja o lixo de outra. O
// nome tras o pid, que a bateria pode correr em paralello.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-varredura-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_ / "acervo");
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::permissions(caminho_,
                                 std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::add, erro);
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;

  std::filesystem::path acervo() const { return caminho_ / "acervo"; }
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }
  const std::filesystem::path& raiz() const { return caminho_; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;


// faz_wav — um WAV valido escripto byte a byte. Não se chama ffmpeg: prova que
// depende de programma externo falha por razão que não é a sua. Quarenta e quatro
// octetos de cabeçalho, e silencio no corpo; a taglib abre-o e mede-lhe a
// duração, que é tudo o que esta bateria precisa d'elle.
void faz_wav(const std::filesystem::path& onde, int segundos) {
  std::filesystem::create_directories(onde.parent_path());
  const std::uint32_t taxa = 8000, canaes = 1, bits = 8;
  const std::uint32_t corpo = taxa * canaes * (bits / 8) * segundos;
  std::ofstream saida(onde, std::ios::binary);
  auto le32 = [&saida](std::uint32_t v) {
    for (int i = 0; i < 4; ++i) saida.put(static_cast<char>((v >> (8 * i)) & 0xFF));
  };
  auto le16 = [&saida](std::uint16_t v) {
    for (int i = 0; i < 2; ++i) saida.put(static_cast<char>((v >> (8 * i)) & 0xFF));
  };
  saida.write("RIFF", 4); le32(36 + corpo); saida.write("WAVE", 4);
  saida.write("fmt ", 4); le32(16); le16(1); le16(canaes);
  le32(taxa); le32(taxa * canaes * (bits / 8)); le16(canaes * (bits / 8)); le16(bits);
  saida.write("data", 4); le32(corpo);
  for (std::uint32_t i = 0; i < corpo; ++i) saida.put(static_cast<char>(0x80));
}

// poe_etiqueta — a etiqueta posta pela PROPRIA taglib. Cadeia vazia e numero zero
// querem dizer «não põe este campo», que é como se arma o caso da etiqueta
// parcial sem escrever um arquivo á mão para cada combinação.
void poe_etiqueta(const std::filesystem::path& onde, const std::string& artista,
                  const std::string& album, const std::string& titulo,
                  unsigned numero) {
  TagLib::FileRef arquivo(onde.c_str());
  REQUIRE_FALSE(arquivo.isNull());
  TagLib::Tag* etiqueta = arquivo.tag();
  REQUIRE(etiqueta != nullptr);
  // UTF8 EXPLICITO. `TagLib::String` construida de std::string assume LATIN-1, e
  // gravar «Máquina» como latin-1 fá-lo voltar «MÃ¡quina»: foi o que esta prova
  // apanhou de si mesma antes de julgar a obra.
  const auto utf8 = TagLib::String::UTF8;
  if (!artista.empty()) etiqueta->setArtist(TagLib::String(artista, utf8));
  if (!album.empty()) etiqueta->setAlbum(TagLib::String(album, utf8));
  if (!titulo.empty()) etiqueta->setTitle(TagLib::String(titulo, utf8));
  if (numero != 0) etiqueta->setTrack(numero);
  REQUIRE(arquivo.save());
}

}  // namespace

TEST_CASE("a extensão de audio aceita-se em qualquer caixa") {
  CHECK(nu::extensao_de_audio(".mp3"));
  CHECK(nu::extensao_de_audio(".MP3"));
  CHECK(nu::extensao_de_audio(".FlAc"));
  CHECK(nu::extensao_de_audio(".opus"));
  CHECK_FALSE(nu::extensao_de_audio(".txt"));
  CHECK_FALSE(nu::extensao_de_audio(".png"));
  CHECK_FALSE(nu::extensao_de_audio("mp3"));   // sem o ponto não é extensão
  CHECK_FALSE(nu::extensao_de_audio(""));
}

// A derivação, contra alvo ESCRIPTO Á MÃO. Nenhum caso pergunta á obra o que
// ella derivou para depois conferir que o derivou: os quatro campos vão
// escriptos aqui, letra por letra.
TEST_CASE("a derivação lê artista, album, numero e titulo do caminho") {
  const nu::Faixa cheia =
      nu::deriva_do_caminho("/acervo/Ada Lovelace/Máquina/03 - Tear.mp3",
                            "/acervo");
  CHECK(cheia.artista == "Ada Lovelace");
  CHECK(cheia.album == "Máquina");
  CHECK(cheia.numero == 3);
  CHECK(cheia.titulo == "Tear");
  CHECK(cheia.raiz == "/acervo");
  // Todos os quatro bits: a dedução é o piso, e a etiqueta apaga o que disser.
  CHECK(cheia.deduzido == (nu::kDeduziuArtista | nu::kDeduziuAlbum |
                           nu::kDeduziuTitulo | nu::kDeduziuNumero));
}

TEST_CASE("o separador do numero aceita-se nas tres fórmas") {
  CHECK(nu::deriva_do_caminho("/a/A/B/01 - Tear.mp3", "/a").numero == 1);
  CHECK(nu::deriva_do_caminho("/a/A/B/01 - Tear.mp3", "/a").titulo == "Tear");
  CHECK(nu::deriva_do_caminho("/a/A/B/07-Nota G.mp3", "/a").numero == 7);
  CHECK(nu::deriva_do_caminho("/a/A/B/07-Nota G.mp3", "/a").titulo == "Nota G");
  CHECK(nu::deriva_do_caminho("/a/A/B/12. Fuga.mp3", "/a").numero == 12);
  CHECK(nu::deriva_do_caminho("/a/A/B/12. Fuga.mp3", "/a").titulo == "Fuga");
}

TEST_CASE("sem numero á frente, o titulo é o nome inteiro") {
  const nu::Faixa sem = nu::deriva_do_caminho("/a/A/B/Tear.mp3", "/a");
  CHECK(sem.numero == 0);
  CHECK(sem.titulo == "Tear");
  // Digitos collados a letra são NOME, e não numero de faixa.
  const nu::Faixa pac = nu::deriva_do_caminho("/a/A/B/2Pac.mp3", "/a");
  CHECK(pac.numero == 0);
  CHECK(pac.titulo == "2Pac");
  // Mais de tres digitos não é numero de faixa.
  const nu::Faixa anno = nu::deriva_do_caminho("/a/A/B/1998 - Tear.mp3", "/a");
  CHECK(anno.numero == 0);
  CHECK(anno.titulo == "1998 - Tear");
  // Sómente o numero, sem titulo depois: o nome inteiro é o titulo.
  const nu::Faixa nu_ = nu::deriva_do_caminho("/a/A/B/05.mp3", "/a");
  CHECK(nu_.numero == 0);
  CHECK(nu_.titulo == "05");
}

// As duas bordas da hierarchia. Arquivo na raiz não tem artista nem album; um
// degrau só dá artista e deixa o album VAZIO, que dizer que o album se chama
// como o artista seria affirmar o que não se sabe; e mais fundo que o esperado
// toma o primeiro degrau por artista e o ultimo por album.
TEST_CASE("a hierarchia lê-se do primeiro degrau e do ultimo") {
  const nu::Faixa raiz = nu::deriva_do_caminho("/a/Tear.mp3", "/a");
  CHECK(raiz.artista.empty());
  CHECK(raiz.album.empty());
  CHECK(raiz.titulo == "Tear");
  const nu::Faixa um = nu::deriva_do_caminho("/a/Ada/Tear.mp3", "/a");
  CHECK(um.artista == "Ada");
  CHECK(um.album.empty());
  const nu::Faixa fundo =
      nu::deriva_do_caminho("/a/Ada/1843/Máquina/CD1/Tear.mp3", "/a");
  CHECK(fundo.artista == "Ada");
  CHECK(fundo.album == "CD1");
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
