// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA CAPA — testes/prova_capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A busca do arquivo e a chave do cache provam-se sem imagem alguma. O render pelo
// chafa prova-se á mão, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <unistd.h>

#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "nucleo/capa.hpp"

namespace nu = mysong::nucleo;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-capa-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  const std::filesystem::path& raiz() const { return caminho_; }
  void poe(const std::string& nome) const {
    std::ofstream(caminho_ / nome) << "nao e imagem, mas existe";
  }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

// Um PNG de um pixel, escripto octeto a octeto. Chamar o ffmpeg aqui seria trocar a
// prova da capa embutida por uma prova do ffmpeg: a issue #81 pede que ella corra sem
// ferramenta alheia e sem rede, e é este arranjo que o cumpre.
const std::string& png_de_um_pixel() {
  static const unsigned char kCrus[] = {
      0x89, 'P',  'N',  'G',  0x0D, 0x0A, 0x1A, 0x0A, 0,    0,    0,    13,
      'I',  'H',  'D',  'R',  0,    0,    0,    1,    0,    0,    0,    1,
      8,    2,    0,    0,    0,    0x90, 0x77, 0x53, 0xDE, 0,    0,    0,
      12,   'I',  'D',  'A',  'T',  0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00,
      0x00, 0x03, 0x01, 0x01, 0x00, 0x18, 0xDD, 0x8D, 0xB0, 0,    0,    0,
      0,    'I',  'E',  'N',  'D',  0xAE, 0x42, 0x60, 0x82};
  static const std::string kPng(reinterpret_cast<const char*>(kCrus), sizeof kCrus);
  return kPng;
}

// lavra_a_etiqueta — põe um APIC na faixa, pela MESMA taglib com que a obra o lê.
// O corpo do arquivo é um quadro de MPEG minimo: basta para a taglib o aceitar, e
// faixa que toque não é o que este caso afere.
bool lavra_a_etiqueta(const std::filesystem::path& faixa, const std::string& arte) {
  { std::ofstream(faixa, std::ios::binary) << "\xFF\xFB\x90\x00"; }
  TagLib::MPEG::File arquivo(faixa.c_str());
  if (!arquivo.isValid()) return false;
  auto* quadro = new TagLib::ID3v2::AttachedPictureFrame();
  quadro->setMimeType("image/png");
  quadro->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
  quadro->setPicture(
      TagLib::ByteVector(arte.data(), static_cast<unsigned>(arte.size())));
  arquivo.ID3v2Tag(true)->addFrame(quadro);
  return arquivo.save();
}

}  // namespace

// A CAPA EMBUTIDA. Este caminho existe desde a issue #44 e nunca teve prova: andou
// certo por sorte. Corre sem chafa e sem rede, que é o que a issue #81 pede.
TEST_CASE("a capa embutida lê-se da etiqueta lavrada á mão") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  const std::string arte = png_de_um_pixel();
  REQUIRE(lavra_a_etiqueta(faixa, arte));
  // Os MESMOS octetos voltam. Aferir sómente que «veio alguma cousa» deixaria passar
  // uma leitura que truncasse a arte, e arte truncada o chafa recusa.
  CHECK(nu::arte_embutida(faixa) == arte);
}

// A PROPRIEDADE DA TAGLIB de que a baixa inteira depende, e que até aqui vivia n'uma
// medição á mão e n'um commentario. Na aquisição a ordem é esta: o yt-dlp embute a
// capa, e SÓ DEPOIS o `escreve_etiqueta` abre o arquivo pela `FileRef` e grava
// artista e titulo. Deitasse esse `save()` a arte fóra, a capa sumia do painel com a
// bateria inteira verde, e a queixa do operador voltava sem que nada se queixasse.
//
// O `escreve_etiqueta` é anonymo e não se alcança d'aqui; o que se alcança é o que
// elle usa, e é isso que este caso prende.
TEST_CASE("gravar a etiqueta pela FileRef não deita fóra a capa embutida") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  const std::string arte = png_de_um_pixel();
  REQUIRE(lavra_a_etiqueta(faixa, arte));

  // O MESMO que a aquisição faz depois de baixar: FileRef, texto novo, e salvar.
  {
    TagLib::FileRef punho(faixa.c_str());
    REQUIRE_FALSE(punho.isNull());
    REQUIRE(punho.tag() != nullptr);
    punho.tag()->setArtist(TagLib::String("Quem Baixou", TagLib::String::UTF8));
    punho.tag()->setTitle(TagLib::String("Faixa Colhida", TagLib::String::UTF8));
    REQUIRE(punho.save());
  }

  // E a arte continua lá, octeto por octeto. Medi-a tambem sobre a faixa que o
  // mysong baixou de verdade: 23689 octetos antes do `save()` e 23689 depois.
  CHECK(nu::arte_embutida(faixa) == arte);
}

TEST_CASE("a capa ao lado acha-se pela ordem de preferencia") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  // Pasta sem capa alguma: vazio, e não erro.
  CHECK(nu::capa_ao_lado(faixa).empty());

  // Pondo o menos preferido primeiro, elle sahe, que é o unico que ha.
  cova.poe("album.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "album.jpg");
  // Pondo o `folder`, elle ganha do `album`.
  cova.poe("folder.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "folder.jpg");
  // E o `cover` ganha de todos, que é o que o Picard grava.
  cova.poe("cover.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "cover.jpg");
  // Nome que não está na lista fechada não conta.
  cova.poe("arte.jpg");
  CHECK(nu::capa_ao_lado(faixa).filename() == "cover.jpg");
}

// A CHAVE é a PASTA mais o tamanho, e não a faixa: é ella que faz um album de vinte
// faixas pedir uma conversão, e não vinte.
TEST_CASE("a chave do cache é a pasta e o tamanho, e não a faixa") {
  const std::string uma = nu::chave_do_cache("/a/A/B/01 - Um.mp3", 20, 10);
  const std::string outra = nu::chave_do_cache("/a/A/B/02 - Dous.mp3", 20, 10);
  CHECK(uma == outra);  // mesmo album: a MESMA chave
  // Album differente dá chave differente.
  CHECK(uma != nu::chave_do_cache("/a/A/C/01 - Um.mp3", 20, 10));
  // Tamanho differente tambem, que a arte tem de encher o painel novo.
  CHECK(uma != nu::chave_do_cache("/a/A/B/01 - Um.mp3", 21, 10));
  CHECK(uma != nu::chave_do_cache("/a/A/B/01 - Um.mp3", 20, 11));
}

// A GALERIA e o CACHE, com imagem de verdade. A imagem faz-se aqui: um PNG de
// dezaseis por dezaseis escripto octeto a octeto seria trabalho de mais, e por isso
// se chama o ffmpeg UMA vez e se salta o caso se elle não estiver. O que se afere não
// é o chafa: é a CONTAGEM de conversões.
TEST_CASE("a galeria converte uma vez por album e por tamanho") {
  const Cova cova;
  const std::filesystem::path imagem = cova.raiz() / "cover.png";
  const std::string commando =
      "ffmpeg -y -f lavfi -i color=c=purple:s=64x64 -frames:v 1 '" +
      imagem.string() + "' >/dev/null 2>&1";
  if (std::system(commando.c_str()) != 0 ||
      !std::filesystem::exists(imagem)) {
    WARN("sem ffmpeg: o caso do cache não corre");
    return;
  }

  nu::Galeria galeria;
  const std::filesystem::path uma = cova.raiz() / "01 - Um.mp3";
  const std::filesystem::path outra = cova.raiz() / "02 - Dous.mp3";

  const nu::CapaPintada& primeira = galeria.capa(uma, 10, 5);
  REQUIRE(primeira.achada);
  CHECK(primeira.linhas.size() == 5u);
  CHECK(galeria.quantos_renders() == 1u);

  // A MESMA faixa outra vez: cache, e conversão alguma.
  galeria.capa(uma, 10, 5);
  CHECK(galeria.quantos_renders() == 1u);
  // OUTRA faixa do mesmo album: tambem cache. É a affirmação que a chave carrega.
  galeria.capa(outra, 10, 5);
  CHECK(galeria.quantos_renders() == 1u);
  // Tamanho novo: conversão nova, que a arte tem de encher o painel novo.
  galeria.capa(uma, 12, 6);
  CHECK(galeria.quantos_renders() == 2u);
  // E de volta ao tamanho de antes: cache outra vez.
  galeria.capa(uma, 10, 5);
  CHECK(galeria.quantos_renders() == 2u);
}

// A ORDEM, provada e não sómente declarada. Os dous estão presentes, e a etiqueta
// leva arte PODRE de proposito: ganhando ella, o chafa engasgaria e `achada` viria
// falso. Vindo verdadeiro, foi o arquivo ao lado que se tomou, que é a ordem que o
// cabeçalho promette.
TEST_CASE("com capa ao lado e na etiqueta ganha a do lado") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  REQUIRE(lavra_a_etiqueta(faixa, "isto não é imagem alguma"));
  REQUIRE_FALSE(nu::arte_embutida(faixa).empty());

  const std::filesystem::path imagem = cova.raiz() / "cover.png";
  const std::string commando =
      "ffmpeg -y -f lavfi -i color=c=teal:s=64x64 -frames:v 1 '" +
      imagem.string() + "' >/dev/null 2>&1";
  if (std::system(commando.c_str()) != 0 || !std::filesystem::exists(imagem)) {
    WARN("sem ffmpeg: o caso da preferencia não corre");
    return;
  }
  nu::Galeria galeria;
  CHECK(galeria.capa(faixa, 10, 5).achada);
}

// O `.mkv` do video, e todo arquivo que a taglib não leia como MP3. Medido: ella dá
// `isValid()` verdadeiro e etiqueta NÃO nula para todos estes, que a cria a pedido;
// o que vem vazio é a lista de quadros. Sahem quietos, e é isso que segura o pintor
// contra uma excepção quando o operador põe um video no acervo.
TEST_CASE("arquivo que não é mp3 não dá capa alguma nem lança") {
  const Cova cova;
  // Um `.mkv` de VERDADE, pelos quatro octetos do cabeçalho EBML, e não prosa com
  // nome de video: foi sobre arquivo assim que a medição correu, e um arquivo de
  // texto provaria sómente que a taglib recusa texto, que é affirmação mais fraca.
  std::ofstream(cova.raiz() / "video.mkv", std::ios::binary)
      << "\x1A\x45\xDF\xA3";
  cova.poe("lixo.mp3");
  std::ofstream(cova.raiz() / "vazio.mp3", std::ios::binary);
  CHECK(nu::arte_embutida(cova.raiz() / "video.mkv").empty());
  CHECK(nu::arte_embutida(cova.raiz() / "lixo.mp3").empty());
  CHECK(nu::arte_embutida(cova.raiz() / "vazio.mp3").empty());
  // E o que nem existe sahe quieto tambem, que o pintor pergunta por faixa que o
  // acervo pode ter perdido entre a varredura e o quadro.
  CHECK(nu::arte_embutida(cova.raiz() / "nao-existe.mp3").empty());
}

TEST_CASE("album sem capa devolve a ausencia, e guarda-a") {
  const Cova cova;
  nu::Galeria galeria;
  const std::filesystem::path faixa = cova.raiz() / "01 - Um.mp3";
  const nu::CapaPintada& nada = galeria.capa(faixa, 10, 5);
  CHECK_FALSE(nada.achada);
  CHECK(nada.linhas.empty());
  CHECK(galeria.quantos_renders() == 0u);
  // E pedir outra vez não volta ao disco: a ausencia tambem está guardada. Prova-se
  // pondo a capa AGORA: estando a ausencia em cache, ella não se vê.
  cova.poe("cover.jpg");
  CHECK_FALSE(galeria.capa(faixa, 10, 5).achada);
}

// A ANALYSE do SGR, contra linhas escriptas á mão com a fórma que o chafa produz.
// Foi este parser que curou o defeito da largura, e é aqui que elle se prende.
TEST_CASE("o SGR do chafa parte-se em corridas com as suas côres") {
  // Uma linha de verdade, copiada da sahida do chafa e escripta aqui á mão.
  const std::string linha =
      "\x1b[0m\x1b[38;2;167;0;167;48;2;173;0;173m\u2580"
      "\x1b[38;2;163;0;163;48;2;170;0;170m\u2580\x1b[0m";
  const std::vector<nu::Corrida> corridas = nu::analysa_sgr(linha);
  REQUIRE(corridas.size() == 2u);
  CHECK(corridas[0].texto == "\u2580");
  CHECK(corridas[0].r_frente == 167);
  CHECK(corridas[0].g_frente == 0);
  CHECK(corridas[0].b_frente == 167);
  CHECK(corridas[0].r_fundo == 173);
  CHECK(corridas[1].texto == "\u2580");
  CHECK(corridas[1].r_frente == 163);
  CHECK(corridas[1].r_fundo == 170);

  // O `39` e o `49` apagam a côr, e é assim que o chafa diz «sem côr».
  const std::vector<nu::Corrida> apagada =
      nu::analysa_sgr("\x1b[38;2;1;2;3m a \x1b[39m\x1b[49m b ");
  REQUIRE(apagada.size() == 2u);
  CHECK(apagada[0].texto == " a ");
  CHECK(apagada[0].r_frente == 1);
  CHECK(apagada[1].texto == " b ");
  CHECK(apagada[1].r_frente == -1);
  CHECK(apagada[1].r_fundo == -1);

  // Texto sem escape algum dá UMA corrida sem côr.
  const std::vector<nu::Corrida> nua = nu::analysa_sgr("abc");
  REQUIRE(nua.size() == 1u);
  CHECK(nua[0].texto == "abc");
  CHECK(nua[0].r_frente == -1);
  // Linha vazia dá corrida alguma, e não uma corrida vazia.
  CHECK(nu::analysa_sgr("").empty());
  // Escape que não se conheça ignora-se, e o texto passa.
  const std::vector<nu::Corrida> alheio = nu::analysa_sgr("\x1b[7mx");
  REQUIRE(alheio.size() == 1u);
  CHECK(alheio[0].texto == "x");
}

// O EMBUTE da issue #83: o espelho de escripta, sem chafa e sem rede.
TEST_CASE("o embute grava a arte e o arte_embutida a relê tal e qual") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Nua.mp3";
  { std::ofstream(faixa, std::ios::binary) << "\xFF\xFB\x90\x00"; }
  const std::string arte = png_de_um_pixel();
  REQUIRE(nu::embute_arte(faixa, arte));
  CHECK(nu::arte_embutida(faixa) == arte);
  // E o mime sahiu do CONTEUDO: o PNG de guarda ha de dizer image/png.
  TagLib::MPEG::File relida(faixa.c_str());
  REQUIRE(relida.isValid());
  const auto& quadros = relida.ID3v2Tag()->frameListMap()["APIC"];
  REQUIRE_FALSE(quadros.isEmpty());
  const auto* quadro =
      dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame*>(quadros.front());
  REQUIRE(quadro != nullptr);
  CHECK(quadro->mimeType() == "image/png");
  CHECK(quadro->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover);
}

TEST_CASE("o embute preserva a etiqueta que a faixa já tinha") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Tear.mp3";
  { std::ofstream(faixa, std::ios::binary) << "\xFF\xFB\x90\x00"; }
  {
    TagLib::FileRef punho(faixa.c_str());
    REQUIRE_FALSE(punho.isNull());
    punho.tag()->setArtist(TagLib::String("Quem Já Era", TagLib::String::UTF8));
    punho.tag()->setTitle(TagLib::String("Faixa Antiga", TagLib::String::UTF8));
    REQUIRE(punho.save());
  }
  // Um JPEG de mentira basta ao sniff: os octetos de guarda e um corpo.
  const std::string jpeg = std::string("\xFF\xD8\xFF\xE0", 4) + "corpo";
  REQUIRE(nu::embute_arte(faixa, jpeg));
  TagLib::FileRef relida(faixa.c_str());
  CHECK(relida.tag()->artist().to8Bit(true) == "Quem Já Era");
  CHECK(relida.tag()->title().to8Bit(true) == "Faixa Antiga");
  CHECK(nu::arte_embutida(faixa) == jpeg);
}

TEST_CASE("octetos que não são imagem recusam-se sem tocar o arquivo") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "01 - Sã.mp3";
  { std::ofstream(faixa, std::ios::binary) << "\xFF\xFB\x90\x00"; }
  const auto tamanho = std::filesystem::file_size(faixa);
  CHECK_FALSE(nu::embute_arte(faixa, "<html>404 not found</html>"));
  CHECK(std::filesystem::file_size(faixa) == tamanho);  // byte algum entrou
  CHECK(nu::arte_embutida(faixa).empty());
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════

// OS ARGUMENTOS DO CHAFA (#94), aferidos INTEIROS. Prova pura: sem chafa, sem
// fonte e sem imagem, donde ella corre egual na machina crua.
TEST_CASE("os argumentos do chafa pedem os symbolos ricos, e nada de letras") {
  const std::vector<std::string> sem =
      nu::argumentos_do_chafa("/a/cover.jpg", 40, 20, false);
  CHECK(sem == std::vector<std::string>{
                   "chafa", "--format=symbols", "--symbols=block+half+quad",
                   "--work=9", "--size=40x20", "--animate=off",
                   "--relative=off", "--polite=on", "--colors=full", "--",
                   "/a/cover.jpg"});
  // Com sextante muda UMA cousa, e sómente ella: a lista dos symbolos.
  const std::vector<std::string> com =
      nu::argumentos_do_chafa("/a/cover.jpg", 40, 20, true);
  REQUIRE(com.size() == sem.size());
  CHECK(com[2] == "--symbols=block+half+quad+sextant");
  for (std::size_t i = 0; i < sem.size(); ++i)
    if (i != 2) CHECK(com[i] == sem[i]);

  // O CRIVO. As classes que metem letra e cifra dentro da arte ficam de fóra,
  // que é a queixa da issue: medido, `--symbols=all` sahe com 7, ©, º, Ġ, ǥ e
  // braille dentro da capa. E as duas bandeiras nullas a 24 bits tambem, que
  // bandeira que não faz nada é mentira na linha de commando.
  for (const std::vector<std::string>& lista : {sem, com})
    for (const std::string& argumento : lista)
      for (const char* proscripto :
           {"all", "ascii", "alpha", "alnum", "digit", "extra", "technical",
            "border", "--dither", "--color-space", "--stretch"})
        CHECK(argumento.find(proscripto) == std::string::npos);
}

// A PROPORÇÃO (#94). Que o chafa a guarde é promessa d'elle; aqui prende-se que
// a Casa não lhe passa nada que a quebre. Salta sem ffmpeg, como os visinhos.
TEST_CASE("a arte enche a largura e guarda a proporção, sem esticar") {
  // Conta CARACTERES, e não octetos: o quadrante come tres octetos e o
  // sextante quatro, e por octeto a capa diria cento e vinte onde tem quarenta.
  const auto collunas = [](const std::vector<nu::Corrida>& linha) {
    std::size_t conta = 0;
    for (const nu::Corrida& corrida : linha)
      for (const char letra : corrida.texto)
        if ((static_cast<unsigned char>(letra) & 0xC0) != 0x80) ++conta;
    return conta;
  };
  const Cova cova;
  const std::filesystem::path larga = cova.raiz() / "16-9.png";
  const std::filesystem::path quadrada = cova.raiz() / "1-1.png";
  const auto pinta = [](const std::string& medida,
                        const std::filesystem::path& onde) {
    return std::system(("ffmpeg -y -f lavfi -i color=c=purple:s=" + medida +
                        " -frames:v 1 '" + onde.string() + "' >/dev/null 2>&1")
                           .c_str()) == 0 &&
           std::filesystem::exists(onde);
  };
  if (!pinta("160x90", larga) || !pinta("64x64", quadrada)) {
    WARN("sem ffmpeg: o caso da proporção não corre");
    return;
  }
  // Pedidas quarenta por vinte: a 16:9 sahe 40 por DOZE (a largura enche, a
  // altura sobra) e a quadrada 40 por vinte. Esticadas, ambas dariam vinte
  // linhas; cortada, a larga daria menos que quarenta collunhas.
  const nu::CapaPintada dezaseis = nu::pinta_imagem(larga, 40, 20, false);
  REQUIRE(dezaseis.achada);
  CHECK(dezaseis.linhas.size() == 12u);
  CHECK(collunas(dezaseis.linhas.front()) == 40u);
  const nu::CapaPintada quadro = nu::pinta_imagem(quadrada, 40, 20, false);
  REQUIRE(quadro.achada);
  CHECK(quadro.linhas.size() == 20u);
  CHECK(collunas(quadro.linhas.front()) == 40u);
  // O painel apertado e o painel largo, que é o que a irmã #92 ha de dar.
  CHECK(nu::pinta_imagem(larga, 8, 5, false).linhas.size() == 3u);
  CHECK(nu::pinta_imagem(larga, 60, 31, false).linhas.size() == 17u);
  // E o que não dá capa alguma, sem lançar: tamanho zero, caminho vazio, e
  // arquivo que existe e imagem não é.
  CHECK_FALSE(nu::pinta_imagem(larga, 0, 20, false).achada);
  CHECK_FALSE(nu::pinta_imagem(larga, 40, 0, false).achada);
  CHECK_FALSE(nu::pinta_imagem({}, 40, 20, false).achada);
  cova.poe("nao-e-imagem.png");
  CHECK_FALSE(nu::pinta_imagem(cova.raiz() / "nao-e-imagem.png", 40, 20, false)
                  .achada);
}

// O SGR COM GLIFO LARGO (#94). Os symbolos ricos trazem caracteres de tres e de
// quatro octetos, e o leitor acumula octeto a octeto: prende-se aqui que nenhum
// se parte, e sobretudo o de quatro NO FIM da linha, sem repouso a seguir, que
// é onde um leitor mal feito comeria o ultimo octeto.
TEST_CASE("o SGR aceita glifo de tres e de quatro octetos sem partir corrida") {
  // ▚ (U+259A, tres octetos) e 🬀 (U+1FB00, quatro), com côr a separá-los.
  const std::vector<nu::Corrida> larga = nu::analysa_sgr(
      "\x1b[38;2;9;8;7;48;2;1;2;3m▚\x1b[38;2;70;80;90m\U0001FB00");
  REQUIRE(larga.size() == 2u);
  CHECK(larga[0].texto == "▚");
  CHECK(larga[0].texto.size() == 3u);
  CHECK(larga[0].r_frente == 9);
  CHECK(larga[0].b_fundo == 3);
  CHECK(larga[1].texto == "\U0001FB00");
  CHECK(larga[1].texto.size() == 4u);
  CHECK(larga[1].g_frente == 80);

  // Dous sextantes na MESMA corrida sahem juntos, e não partidos ao meio.
  const std::vector<nu::Corrida> juntos =
      nu::analysa_sgr("\x1b[38;2;1;1;1m\U0001FB00\U0001FB2D\x1b[0m");
  REQUIRE(juntos.size() == 1u);
  CHECK(juntos[0].texto == "\U0001FB00\U0001FB2D");
}

// O VIDEO INVERTIDO (#94), contra a linha que o chafa emitte de FACTO sobre
// imagem chapada. Copiei-a da sahida com `cat -v` e escrevi-a aqui á mão.
// NOTA DO NOME: o titulo do caso não traz colchete, e é de proposito. O
// doctest_discover_tests do CMake gera a lista dos casos entre `[==[ ]==]`, e
// um `[` no meio do nome parte a lista: o ctest recusa o projecto inteiro com
// «add_test called with incorrect number of arguments». Medi-o aqui.
TEST_CASE("o video invertido do chafa troca tinta e fundo da corrida") {
  const std::vector<nu::Corrida> chapada = nu::analysa_sgr(
      "\x1b[0m\x1b[7m\x1b[38;2;59;12;106m \x1b[0m"
      "\x1b[38;2;0;0;0;48;2;59;12;106m ");
  REQUIRE(chapada.size() == 2u);
  // A primeira vem invertida: a côr que veio por TINTA vale por FUNDO, que é o
  // que o terminal pinta. Sem a troca, a célulla sahia com o fundo do terminal.
  CHECK(chapada[0].texto == " ");
  CHECK(chapada[0].r_fundo == 59);
  CHECK(chapada[0].g_fundo == 12);
  CHECK(chapada[0].b_fundo == 106);
  CHECK(chapada[0].r_frente == -1);
  // A segunda é a mesma côr dita sem inversão, e o `ESC[0m` desfez a marca.
  CHECK(chapada[1].r_fundo == 59);
  CHECK(chapada[1].r_frente == 0);
}
