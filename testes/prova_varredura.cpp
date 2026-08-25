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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>

#include <sqlite3.h>
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

namespace {

// corre_ate_o_fim — chama passo() até elle dizer que acabou, e devolve QUANTOS
// passos foram. A conta serve ao aceite: passos maiores ou eguaes ao numero de
// arquivos prova que passo algum engoliu o acervo inteiro.
std::size_t corre_ate_o_fim(nu::Varredura& varredura) {
  std::size_t passos = 0;
  while (varredura.passo()) ++passos;
  return passos + 1;  // o passo que devolveu falso tambem foi um passo
}

}  // namespace

// Etiqueta COMPLETA: tudo entra pela etiqueta, e a máscara sahe em ZERO. O
// caminho diz outra cousa de proposito, para que a prova distinga as duas fontes.
TEST_CASE("etiqueta completa entra pela etiqueta, e deduzido sahe em zero") {
  const Cova cova;
  const std::filesystem::path faixa =
      cova.acervo() / "Caminho Diz Isto" / "E Isto" / "99 - Nome Do Caminho.wav";
  faz_wav(faixa, 3);
  poe_etiqueta(faixa, "Ada Lovelace", "Máquina Analítica", "Tear", 3);

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  corre_ate_o_fim(varredura);
  CHECK(varredura.desfecho() == nu::Desfecho::Concluido);
  CHECK(varredura.progresso().lidas == 1u);
  CHECK(varredura.progresso().reaproveitadas == 0u);

  const nu::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.total() == 1u);
  nu::Faixa achada;
  REQUIRE(livraria.acha_por_caminho(faixa.string(), achada));
  CHECK(achada.artista == "Ada Lovelace");
  CHECK(achada.album == "Máquina Analítica");
  CHECK(achada.titulo == "Tear");
  CHECK(achada.numero == 3);
  CHECK(achada.duracao == 3);
  CHECK(achada.deduzido == nu::kDeduziuNada);
}

// Sem etiqueta ALGUMA: tudo entra pelo caminho, e a máscara nomeia os quatro.
TEST_CASE("sem etiqueta, tudo entra pelo caminho e deduzido nomeia os quatro") {
  const Cova cova;
  const std::filesystem::path faixa =
      cova.acervo() / "Bach" / "Cravo Bem Temperado" / "02 - Fuga.wav";
  faz_wav(faixa, 2);  // e NÃO se põe etiqueta alguma

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  corre_ate_o_fim(varredura);
  REQUIRE(varredura.desfecho() == nu::Desfecho::Concluido);
  const nu::Biblioteca livraria(cova.banco());
  nu::Faixa achada;
  REQUIRE(livraria.acha_por_caminho(faixa.string(), achada));
  CHECK(achada.artista == "Bach");
  CHECK(achada.album == "Cravo Bem Temperado");
  CHECK(achada.titulo == "Fuga");
  CHECK(achada.numero == 2);
  CHECK(achada.deduzido == (nu::kDeduziuArtista | nu::kDeduziuAlbum |
                            nu::kDeduziuTitulo | nu::kDeduziuNumero));
}

// Etiqueta PARCIAL: sómente o titulo. Um bit apagado, tres acesos, e o campo da
// etiqueta preservado ao lado dos tres que vieram do caminho.
TEST_CASE("etiqueta parcial toma da etiqueta o que ella diz") {
  const Cova cova;
  const std::filesystem::path faixa =
      cova.acervo() / "Bach" / "Suites" / "05 - Nome Do Caminho.wav";
  faz_wav(faixa, 2);
  poe_etiqueta(faixa, "", "", "Sarabanda", 0);

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  corre_ate_o_fim(varredura);
  const nu::Biblioteca livraria(cova.banco());
  nu::Faixa achada;
  REQUIRE(livraria.acha_por_caminho(faixa.string(), achada));
  CHECK(achada.titulo == "Sarabanda");  // da etiqueta
  CHECK(achada.artista == "Bach");      // do caminho
  CHECK(achada.album == "Suites");      // do caminho
  CHECK(achada.numero == 5);            // do caminho
  CHECK(achada.deduzido == (nu::kDeduziuArtista | nu::kDeduziuAlbum |
                            nu::kDeduziuNumero));
}

// O INCREMENTAL, provado de FÓRA da obra. Depois da primeira corrida tira-se toda
// permissão dos arquivos, o que NÃO altera hora nem tamanho. Chamada a taglib, a
// leitura falharia; ella não é chamada, e por isso a segunda corrida sahe egual á
// primeira com `lidas` em zero. Não se pergunta á obra se ella releu: tira-se-lhe
// a possibilidade de o fazer, e vê-se se o resultado sobrevive.
TEST_CASE("segunda corrida não relê etiqueta, provado por chmod zero") {
  const Cova cova;
  for (int i = 1; i <= 3; ++i) {
    const std::filesystem::path faixa =
        cova.acervo() / "Ada" / "Notas" / ("0" + std::to_string(i) + " - N.wav");
    faz_wav(faixa, i);
    poe_etiqueta(faixa, "Ada Lovelace", "Notas", "Nota " + std::to_string(i),
                 static_cast<unsigned>(i));
  }
  {
    nu::Varredura primeira(cova.banco(), {cova.acervo()});
    corre_ate_o_fim(primeira);
    REQUIRE(primeira.desfecho() == nu::Desfecho::Concluido);
    CHECK(primeira.progresso().lidas == 3u);
    CHECK(primeira.progresso().reaproveitadas == 0u);
  }
  std::vector<nu::Faixa> antes;
  {
    const nu::Biblioteca livraria(cova.banco());
    REQUIRE(livraria.total() == 3u);
    antes = livraria.faixas_do_album("Ada Lovelace", "Notas");
  }

  // Toda permissão fóra. Hora e tamanho ficam; sómente a LEITURA morre.
  for (const auto& entrada :
       std::filesystem::recursive_directory_iterator(cova.acervo()))
    if (entrada.is_regular_file())
      std::filesystem::permissions(entrada.path(), std::filesystem::perms::none);

  nu::Varredura segunda(cova.banco(), {cova.acervo()});
  corre_ate_o_fim(segunda);
  CHECK(segunda.desfecho() == nu::Desfecho::Concluido);
  CHECK(segunda.progresso().lidas == 0u);
  CHECK(segunda.progresso().reaproveitadas == 3u);

  const nu::Biblioteca livraria(cova.banco());
  const std::vector<nu::Faixa> depois =
      livraria.faixas_do_album("Ada Lovelace", "Notas");
  REQUIRE(depois.size() == antes.size());
  for (std::size_t i = 0; i < depois.size(); ++i) {
    CHECK(depois[i].caminho == antes[i].caminho);
    CHECK(depois[i].titulo == antes[i].titulo);
    CHECK(depois[i].numero == antes[i].numero);
    CHECK(depois[i].duracao == antes[i].duracao);
    CHECK(depois[i].deduzido == antes[i].deduzido);
  }
}

// A CONDUCÇÃO por passos: passos maiores que arquivos, trabalho do chamador entre
// dous passos com a corrida inacabada, e o destino AUSENTE até ao ultimo passo.
TEST_CASE("a varredura conduz-se por passos, e o destino não existe antes do fim") {
  const Cova cova;
  for (int i = 1; i <= 5; ++i)
    faz_wav(cova.acervo() / "A" / "B" / ("0" + std::to_string(i) + " - T.wav"), 1);

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  std::size_t passos = 0, trabalho_do_chamador = 0;
  while (varredura.passo()) {
    ++passos;
    // O destino NÃO existe em passo algum antes do ultimo.
    REQUIRE_FALSE(std::filesystem::exists(cova.banco()));
    ++trabalho_do_chamador;  // o chamador corre o seu proprio trabalho aqui
  }
  CHECK(passos >= 5u);                       // passo algum engoliu o acervo
  CHECK(trabalho_do_chamador == passos);     // e o chamador correu entre todos
  CHECK(varredura.desfecho() == nu::Desfecho::Concluido);
  CHECK(std::filesystem::exists(cova.banco()));  // sómente agora
}

// Abandonar a meio: o índice anterior fica byte a byte, e resíduo algum sobra.
TEST_CASE("abandonar a meio conserva o índice anterior, e nada sobra") {
  const Cova cova;
  faz_wav(cova.acervo() / "A" / "B" / "01 - T.wav", 1);
  {
    nu::Varredura primeira(cova.banco(), {cova.acervo()});
    corre_ate_o_fim(primeira);
    REQUIRE(primeira.desfecho() == nu::Desfecho::Concluido);
  }
  std::ifstream fonte(cova.banco(), std::ios::binary);
  const std::string antes((std::istreambuf_iterator<char>(fonte)),
                          std::istreambuf_iterator<char>());
  REQUIRE_FALSE(antes.empty());

  faz_wav(cova.acervo() / "A" / "B" / "02 - Nova.wav", 1);
  {
    nu::Varredura segunda(cova.banco(), {cova.acervo()});
    REQUIRE(segunda.passo());  // lista
    REQUIRE(segunda.passo());  // e um arquivo
    segunda.abandona();
    CHECK(segunda.desfecho() == nu::Desfecho::Abandonado);
  }
  std::ifstream fonte2(cova.banco(), std::ios::binary);
  const std::string depois((std::istreambuf_iterator<char>(fonte2)),
                           std::istreambuf_iterator<char>());
  CHECK(depois == antes);
  // Resíduo algum: nenhum arquivo fóra do banco no directorio d'elle.
  std::size_t vizinhos = 0;
  for (const auto& entrada : std::filesystem::directory_iterator(cova.raiz()))
    if (entrada.is_regular_file()) ++vizinhos;
  CHECK(vizinhos == 1u);
}

// Múltiplas raízes: as boas entram TODAS, e as ruins contam-se sem abortar.
TEST_CASE("raiz ausente e raiz sem permissão contam-se, e não abortam") {
  const Cova cova;
  const std::filesystem::path outra = cova.raiz() / "acervo2";
  const std::filesystem::path trancada = cova.raiz() / "trancada";
  faz_wav(cova.acervo() / "A" / "B" / "01 - Um.wav", 1);
  faz_wav(outra / "C" / "D" / "01 - Dous.wav", 1);
  std::filesystem::create_directories(trancada / "E");
  faz_wav(trancada / "E" / "01 - Tres.wav", 1);
  std::filesystem::permissions(trancada, std::filesystem::perms::none);

  nu::Varredura varredura(cova.banco(),
                          {cova.acervo(), outra, cova.raiz() / "nao-existe",
                           trancada});
  corre_ate_o_fim(varredura);
  CHECK(varredura.desfecho() == nu::Desfecho::Concluido);
  CHECK(varredura.progresso().raizes_falhadas >= 1u);
  // As DUAS raízes boas entraram inteiras, que é o que importa.
  const nu::Biblioteca livraria(cova.banco());
  CHECK(livraria.total() == 2u);
  CHECK(livraria.busca_faixa("Um").size() == 1u);
  CHECK(livraria.busca_faixa("Dous").size() == 1u);
  std::filesystem::permissions(trancada, std::filesystem::perms::owner_all);
}

// As duas recusas: extensão alheia, que nem se offerece á taglib; e o arquivo com
// extensão de audio que não é audio, que a taglib recusa abrir.
TEST_CASE("extensão alheia e falso audio não entram no índice") {
  const Cova cova;
  faz_wav(cova.acervo() / "A" / "B" / "01 - Boa.wav", 1);
  std::ofstream(cova.acervo() / "A" / "B" / "capa.png") << "nao e imagem";
  std::ofstream(cova.acervo() / "A" / "B" / "leia.txt") << "nem texto de audio";
  std::ofstream(cova.acervo() / "A" / "B" / "02 - Falsa.mp3") << "isto e texto";

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  corre_ate_o_fim(varredura);
  CHECK(varredura.desfecho() == nu::Desfecho::Concluido);
  CHECK(varredura.progresso().recusadas == 3u);  // dous por extensão, um pela taglib
  const nu::Biblioteca livraria(cova.banco());
  CHECK(livraria.total() == 1u);
  CHECK(livraria.busca_faixa("Falsa").empty());
  CHECK(livraria.busca_faixa("Boa").size() == 1u);
}

// O LAÇO de ligações symbólicas. Sem a guarda, a varredura giraria sem fim; com
// ella, termina em numero FINITO de passos. O caso tem relogio de guarda proprio:
// não se confia no `while` para terminar, que é justamente o que se está a provar.
TEST_CASE("laço de ligação symbólica não faz a varredura girar") {
  const Cova cova;
  faz_wav(cova.acervo() / "A" / "B" / "01 - Um.wav", 1);
  std::error_code erro;
  std::filesystem::create_directory_symlink(cova.acervo(),
                                            cova.acervo() / "A" / "volta", erro);
  REQUIRE_FALSE(erro);

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  std::size_t passos = 0;
  while (varredura.passo()) {
    ++passos;
    REQUIRE(passos < 1000u);  // relogio de guarda: girar seria passar d'aqui
  }
  CHECK(varredura.desfecho() == nu::Desfecho::Concluido);
  CHECK(varredura.progresso().ligacoes_saltadas >= 1u);
  const nu::Biblioteca livraria(cova.banco());
  CHECK(livraria.total() == 1u);  // a faixa entrou UMA vez
}

// Raízes que se SOBREPÕEM: o mesmo arquivo alcançavel por duas entra UMA vez,
// pelo caminho canónico.
TEST_CASE("arquivo alcançavel por duas raízes entra uma vez") {
  const Cova cova;
  faz_wav(cova.acervo() / "A" / "B" / "01 - Um.wav", 1);
  nu::Varredura varredura(cova.banco(),
                          {cova.acervo(), cova.acervo() / "A"});
  corre_ate_o_fim(varredura);
  CHECK(varredura.desfecho() == nu::Desfecho::Concluido);
  CHECK(varredura.progresso().vistas == 1u);
  const nu::Biblioteca livraria(cova.banco());
  CHECK(livraria.total() == 1u);
}

// Esquema mais NOVO em disco: nada se toca. Nem se abre escriba, donde
// temporario algum chega a existir, e o arquivo fica byte a byte.
TEST_CASE("banco de esquema mais novo não se sobrescreve") {
  const Cova cova;
  faz_wav(cova.acervo() / "A" / "B" / "01 - Um.wav", 1);
  {
    nu::Varredura primeira(cova.banco(), {cova.acervo()});
    corre_ate_o_fim(primeira);
    REQUIRE(primeira.desfecho() == nu::Desfecho::Concluido);
  }
  // Envelhece-se a Casa por SQL propria, e não por knob novo na obra: dar ao
  // Escriba um parametro de versão sómente para esta prova poria na obra uma
  // porta por onde alguem escreveria a versão errada de verdade.
  sqlite3* punho = nullptr;
  REQUIRE(sqlite3_open(cova.banco().c_str(), &punho) == SQLITE_OK);
  REQUIRE(sqlite3_exec(punho, "UPDATE esquema SET versao = 99;", nullptr,
                       nullptr, nullptr) == SQLITE_OK);
  sqlite3_close(punho);
  std::ifstream fonte(cova.banco(), std::ios::binary);
  const std::string antes((std::istreambuf_iterator<char>(fonte)),
                          std::istreambuf_iterator<char>());
  REQUIRE_FALSE(antes.empty());

  nu::Varredura varredura(cova.banco(), {cova.acervo()});
  CHECK_FALSE(varredura.passo());  // não ha passo algum a dar
  CHECK(varredura.desfecho() == nu::Desfecho::EsquemaMaisNovo);
  std::ifstream fonte2(cova.banco(), std::ios::binary);
  const std::string depois((std::istreambuf_iterator<char>(fonte2)),
                           std::istreambuf_iterator<char>());
  CHECK(depois == antes);
  std::size_t vizinhos = 0;
  for (const auto& entrada : std::filesystem::directory_iterator(cova.raiz()))
    if (entrada.is_regular_file()) ++vizinhos;
  CHECK(vizinhos == 1u);
}

// Disco cheio a meio da escripta, injectado por limite de paginas baixo: a falta
// cahe no TEMPORARIO, o índice anterior sobrevive idêntico, e o temporario some.
TEST_CASE("disco cheio a meio não estraga o índice anterior") {
  const Cova cova;
  for (int i = 1; i <= 40; ++i)
    faz_wav(cova.acervo() / "A" / "B" /
                ("0" + std::to_string(i) + " - Faixa Com Nome Comprido.wav"), 1);
  {
    nu::Varredura primeira(cova.banco(), {cova.acervo()});
    corre_ate_o_fim(primeira);
    REQUIRE(primeira.desfecho() == nu::Desfecho::Concluido);
  }
  std::ifstream fonte(cova.banco(), std::ios::binary);
  const std::string antes((std::istreambuf_iterator<char>(fonte)),
                          std::istreambuf_iterator<char>());
  REQUIRE_FALSE(antes.empty());

  nu::Varredura apertada(cova.banco(), {cova.acervo()}, 1);  // uma pagina addicional
  corre_ate_o_fim(apertada);
  CHECK(apertada.desfecho() == nu::Desfecho::ErroDeEscripta);
  std::ifstream fonte2(cova.banco(), std::ios::binary);
  const std::string depois((std::istreambuf_iterator<char>(fonte2)),
                           std::istreambuf_iterator<char>());
  CHECK(depois == antes);
  std::size_t vizinhos = 0;
  for (const auto& entrada : std::filesystem::directory_iterator(cova.raiz()))
    if (entrada.is_regular_file()) ++vizinhos;
  CHECK(vizinhos == 1u);
}

// A bateria não escreve nos caminhos do operador. Afere-se AQUI, dentro da
// bateria, para que a garantia corra em toda corrida e não sómente quando alguem
// se lembra de olhar á mão.
TEST_CASE("caso algum da bateria toca os caminhos do operador") {
  const char* casa = ::getenv("HOME");
  REQUIRE(casa != nullptr);
  const std::filesystem::path indice =
      std::filesystem::path(casa) / ".local" / "share" / "mysong";
  CHECK_FALSE(std::filesystem::exists(indice));
  const std::filesystem::path musica = std::filesystem::path(casa) / "Música";
  if (std::filesystem::exists(musica)) {
    // Existindo, ella é do OPERADOR: o que se afere é que a bateria não lhe
    // acrescentou banco algum, e não que ella esteja vazia.
    CHECK_FALSE(std::filesystem::exists(musica / "indice.sqlite3"));
  }
}

// A comparação é de DOUS campos, e este caso é o que o prova. Reescreve-se o
// arquivo com outro tamanho e REPÕE-SE a hora de modificação: comparando-se
// sómente a hora, a linha velha seria reaproveitada e a duração nova nunca
// entraria no índice. Mutação corrida nesta Casa: annullando a comparação do
// tamanho, este caso morre, e sómente elle.
TEST_CASE("hora egual e tamanho diverso faz reler a etiqueta") {
  const Cova cova;
  const std::filesystem::path faixa = cova.acervo() / "A" / "B" / "01 - T.wav";
  faz_wav(faixa, 2);
  {
    nu::Varredura primeira(cova.banco(), {cova.acervo()});
    corre_ate_o_fim(primeira);
    REQUIRE(primeira.desfecho() == nu::Desfecho::Concluido);
  }
  {
    const nu::Biblioteca livraria(cova.banco());
    nu::Faixa antes;
    REQUIRE(livraria.acha_por_caminho(faixa.string(), antes));
    REQUIRE(antes.duracao == 2);
  }

  const auto hora = std::filesystem::last_write_time(faixa);
  faz_wav(faixa, 9);  // outro tamanho, e outra duração
  std::filesystem::last_write_time(faixa, hora);  // e a MESMA hora

  nu::Varredura segunda(cova.banco(), {cova.acervo()});
  corre_ate_o_fim(segunda);
  CHECK(segunda.progresso().lidas == 1u);
  CHECK(segunda.progresso().reaproveitadas == 0u);
  const nu::Biblioteca livraria(cova.banco());
  nu::Faixa depois;
  REQUIRE(livraria.acha_por_caminho(faixa.string(), depois));
  CHECK(depois.duracao == 9);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
