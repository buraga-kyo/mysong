// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DOS AJUSTES — testes/prova_ajustes.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o leitor e a precedencia SEM tocar em disco e sem tocar em ambiente: o
// leitor recebe texto, e o resolvedor recebe os degraus já colhidos. É d'isto
// que a prova vale: o resultado é o mesmo na machina do auctor e na crua.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "nucleo/ajustes.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("o leitor apara as pontas do valor, e conserva o branco do meio") {
  nu::Ajustes ajustes;
  const auto pares =
      nu::ler_pares("  acervo   =   /home/braga/Minhas Musicas  \n", &ajustes);
  REQUIRE(pares.size() == 1);
  CHECK(pares[0].chave == "acervo");
  CHECK(pares[0].valor == "/home/braga/Minhas Musicas");
  CHECK(ajustes.queixas.empty());
}

TEST_CASE("commentario e linha vazia não dão par algum nem queixa alguma") {
  nu::Ajustes ajustes;
  const auto pares = nu::ler_pares(
      "# só commentario\n\n   \nvolume = 70 # o resto sahe\n", &ajustes);
  REQUIRE(pares.size() == 1);
  CHECK(pares[0].valor == "70");
  CHECK(pares[0].linha == 4);
  CHECK(ajustes.queixas.empty());
}

TEST_CASE("a chave repetida vale a ultima, e a repetição vira queixa") {
  nu::Ajustes ajustes;
  const auto pares = nu::ler_pares("volume = 10\nvolume = 20\n", &ajustes);
  REQUIRE(pares.size() == 2);
  CHECK(pares[1].valor == "20");
  CHECK(ajustes.queixas.size() == 1);
}

TEST_CASE("linha sem egual, ou sem chave, vira queixa e não par") {
  nu::Ajustes ajustes;
  CHECK(nu::ler_pares("volume 70\n= 70\n", &ajustes).empty());
  CHECK(ajustes.queixas.size() == 2);
}

namespace {

// O dublê do aferidor que dá TUDO por directorio. Com elle, e com o irmão que
// não dá nada, os dous caminhos do acervo provam-se sem se creçar pasta alguma
// em disco, que é o que faz esta bateria correr egual em qualquer machina.
const nu::Aferidor tudo_vale = [](const std::filesystem::path&) { return true; };

// resolvido — corre o leitor e a escada n'um golpe, sobre o MESMO vaso, que é
// como o programa os corre.
nu::Ajustes resolvido(std::string_view arquivo, nu::Degraus degraus,
                      const nu::Aferidor& afere = tudo_vale) {
  nu::Ajustes ajustes;
  degraus.arquivo = nu::ler_pares(arquivo, &ajustes);
  nu::resolver(degraus, "/padrao", afere, &ajustes);
  return ajustes;
}

}  // namespace

TEST_CASE("a escada inteira: argumento, ambiente, arquivo, padrão") {
  nu::Degraus degraus;
  degraus.acervo_do_ambiente = "/do-ambiente";
  degraus.acervo_do_argumento = "/do-argumento";
  const nu::Ajustes quatro = resolvido("acervo = /do-arquivo\n", degraus);
  CHECK(quatro.acervo.valor == "/do-argumento");
  CHECK(quatro.acervo.origem == nu::Origem::Argumento);

  degraus.acervo_do_argumento.reset();
  const nu::Ajustes tres = resolvido("acervo = /do-arquivo\n", degraus);
  CHECK(tres.acervo.valor == "/do-ambiente");
  CHECK(tres.acervo.origem == nu::Origem::Ambiente);

  degraus.acervo_do_ambiente.reset();
  const nu::Ajustes dous = resolvido("acervo = /do-arquivo\n", degraus);
  CHECK(dous.acervo.valor == "/do-arquivo");
  CHECK(dous.acervo.origem == nu::Origem::Arquivo);

  const nu::Ajustes um = resolvido("", degraus);
  CHECK(um.acervo.valor == "/padrao");
  CHECK(um.acervo.origem == nu::Origem::Padrao);
}

namespace {

// O irmão do dublê de cima: não dá NADA por directorio. É com elle que se prova
// a recusa do acervo, e que se prova que o ambiente não passa por ella.
const nu::Aferidor nada_vale = [](const std::filesystem::path&) {
  return false;
};

}  // namespace

TEST_CASE("o numero ha de ser inteiro inteiramente consumido") {
  CHECK(nu::volume_de("70") == 70);
  CHECK(nu::volume_de("+70") == 70);
  CHECK_FALSE(nu::volume_de("70 lixo"));
  CHECK_FALSE(nu::volume_de("abc"));
  CHECK_FALSE(nu::volume_de("101"));
  CHECK_FALSE(nu::volume_de("99999999999999999999"));
  CHECK(nu::baixas_de("8"));
  CHECK_FALSE(nu::baixas_de("9"));
  CHECK_FALSE(nu::baixas_de("0"));
  CHECK_FALSE(nu::baixas_de("-3"));
  CHECK(nu::fonte_de("YouTube-Music") == nu::Fonte::YouTubeMusic);
  CHECK_FALSE(nu::fonte_de("spotfy"));
}

TEST_CASE("valor invalido cae no degrau de baixo, e deixa queixa") {
  const nu::Ajustes ajustes = resolvido(
      "volume = 500\nfonte_da_busca = spotfy\nbaixas_simultaneas = 0\n"
      "chave_de_versão_velha = 1\n", {});
  CHECK(ajustes.volume.valor == nu::VOLUME_DA_CASA);
  CHECK(ajustes.volume.origem == nu::Origem::Padrao);
  CHECK(ajustes.fonte_da_busca.origem == nu::Origem::Padrao);
  CHECK(ajustes.baixas_simultaneas.valor == nu::OBREIROS_DA_BAIXA);
  CHECK(ajustes.queixas.size() == 4);
}

TEST_CASE("o ambiente vale crú, e o acervo do arquivo afere-se") {
  nu::Degraus degraus;
  degraus.acervo_do_ambiente = "/monte-que-ainda-não-montou";
  const nu::Ajustes cru =
      resolvido("acervo = /do-arquivo\n", degraus, nada_vale);
  CHECK(cru.acervo.valor == "/monte-que-ainda-não-montou");
  CHECK(cru.acervo.origem == nu::Origem::Ambiente);
  CHECK(cru.queixas.size() == 1);
}

TEST_CASE("o diagnostico diz a origem de cada ajuste, sem escape algum") {
  nu::Degraus degraus;
  degraus.acervo_do_ambiente = "/do-ambiente";
  nu::Ajustes ajustes = resolvido(
      "volume = 70\nfonte_da_busca = spotify\nchave_velha = 1\n", degraus);
  ajustes.arquivo = "/tmp/x/mysong.conf";
  ajustes.estado = nu::EstadoDoArquivo::Lido;
  const std::string texto = nu::texto_dos_ajustes(ajustes);
  CHECK(texto.find("/do-ambiente") != std::string::npos);
  CHECK(texto.find("(ambiente)") != std::string::npos);
  CHECK(texto.find("(arquivo)") != std::string::npos);
  CHECK(texto.find("spotify") != std::string::npos);
  CHECK(texto.find("baixas_simultaneas") != std::string::npos);
  CHECK(texto.find("(padrão)") != std::string::npos);
  CHECK(texto.find("/tmp/x/mysong.conf (lido)") != std::string::npos);
  CHECK(texto.find("chave desconhecida") != std::string::npos);
  CHECK(texto.find('\x1b') == std::string::npos);
}

TEST_CASE("sem queixa não ha secção de queixas, e o ausente diz-se") {
  const nu::Ajustes ajustes = resolvido("volume = 70\n", {});
  const std::string texto = nu::texto_dos_ajustes(ajustes);
  CHECK(texto.find("queixas") == std::string::npos);
  CHECK(texto.find("(ausente, e valem os padrões)") != std::string::npos);
}

TEST_CASE("a bandeira do acervo colhe-se, e o que não é ella fica faixa") {
  std::optional<std::string> acervo;
  CHECK(nu::eh_acervo("--acervo=/tmp/x", &acervo));
  CHECK(*acervo == "/tmp/x");
  CHECK_FALSE(nu::eh_acervo("faixa.mp3", &acervo));
  CHECK_FALSE(nu::eh_acervo("--acervo", &acervo));
  CHECK(nu::eh_acervo("--acervo=", &acervo));
  CHECK(acervo->empty());
}

TEST_CASE("linha comprida de mais vira queixa, e não par") {
  nu::Ajustes ajustes;
  const std::string comprida =
      "volume = " + std::string(nu::LINHA_NO_MAXIMO, '7') + "\n";
  CHECK(nu::ler_pares(comprida, &ajustes).empty());
  CHECK(ajustes.queixas.size() == 1);
}

TEST_CASE("as queixas têm tecto, e o lixo não afoga o diagnostico") {
  nu::Ajustes ajustes;
  std::string lixo;
  for (std::size_t volta = 0; volta < nu::QUEIXAS_NO_MAXIMO * 3; ++volta)
    lixo += "linha torta sem egual\n";
  nu::ler_pares(lixo, &ajustes);
  CHECK(ajustes.queixas.size() == nu::QUEIXAS_NO_MAXIMO);
  CHECK(ajustes.queixas_de_mais == nu::QUEIXAS_NO_MAXIMO * 2);
  // E a marca do tecto vem DEPOIS da ultima queixa de linha, e não no topo.
  const std::string texto = nu::texto_dos_ajustes(ajustes);
  CHECK(texto.find("e ha mais 64 queixas") != std::string::npos);
  CHECK(texto.find("e ha mais 64 queixas") > texto.rfind("linha "));
}

TEST_CASE("o numero de pares tem tecto, que a busca da repetida é quadratica") {
  nu::Ajustes ajustes;
  std::string denso;
  for (std::size_t volta = 0; volta < nu::PARES_NO_MAXIMO * 2; ++volta)
    denso += "k" + std::to_string(volta) + " = 1\n";
  CHECK(nu::ler_pares(denso, &ajustes).size() == nu::PARES_NO_MAXIMO);
  REQUIRE(!ajustes.queixas.empty());
  CHECK(ajustes.queixas.back().find("leu-se até aqui") != std::string::npos);
}

TEST_CASE("byte nulo, e texto sem quebra no fim, não derrubam o leitor") {
  nu::Ajustes ajustes;
  const std::string cru("volume = 70\nlixo\0binario", 24);
  const auto pares = nu::ler_pares(cru, &ajustes);
  REQUIRE(pares.size() == 1);
  CHECK(pares[0].valor == "70");
  CHECK(ajustes.queixas.size() == 1);
}

TEST_CASE("a cerquilha dentro do caminho corta, que o limite é declarado") {
  nu::Ajustes ajustes;
  const auto pares = nu::ler_pares("acervo = /mnt/disco#2\n", &ajustes);
  REQUIRE(pares.size() == 1);
  CHECK(pares[0].valor == "/mnt/disco");
}
