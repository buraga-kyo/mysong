// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA TELA — testes/prova_tela_requisitos.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a tela dos requisitos SEM abrir terminal algum: pinta-se o quadro num
// écran de papel, de largura escolhida, e afere-se o que elle diz.
//
// É d'isto que a tela de erro não apodrece. Tela julgada pelo olho de quem a
// abriu prova-se uma vez, no dia em que se escreveu, e nunca mais; pintada em
// écran de papel, a asserção corre em cada ctest.
//
// DOMÍNIO ......... relatorios de dublê, e larguras de écran escolhidas aqui.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... nenhum caso abre terminal, nem consulta o systema, nem lê
//                   ambiente. A largura é parametro, e não a do terminal.
// Q.E.D. .......... afere-se em quarenta collunas a mesma tela que se afere em
//                   cem; donde a tela estreita deixa de ser esperança.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "nucleo/sonda.hpp"
#include "tui/tela_requisitos.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tl = mysong::tui;
namespace tk = mysong::tui::tokens;

namespace {

// Dublê identico ao da prova da sonda, e de proposito repetido: prova que se
// apoia em auxiliar de outra prova quebra por motivo alheio ao que afirma.
nu::Inquerito faltando(std::initializer_list<std::string_view> chaves) {
  const std::vector<std::string_view> ausentes(chaves.begin(), chaves.end());
  const auto responde = [ausentes](nu::Especie especie, std::string_view alvo) {
    for (const nu::Requisito& requisito : nu::requisitos())
      if (requisito.especie == especie && requisito.alvo == alvo)
        for (const std::string_view chave : ausentes)
          if (chave == requisito.chave) return false;
    return true;
  };
  nu::Inquerito inquerito;
  inquerito.familia_de_fonte = [responde](std::string_view alvo) {
    return responde(nu::Especie::FamiliaDeFonte, alvo);
  };
  inquerito.bibliotheca = [responde](std::string_view alvo) {
    return responde(nu::Especie::Bibliotheca, alvo);
  };
  inquerito.executavel = [responde](std::string_view alvo) {
    return responde(nu::Especie::Executavel, alvo);
  };
  return inquerito;
}

// pintar — o écran de papel: largura escolhida, altura quanto o quadro pedir.
std::string pintar(const nu::Relatorio& relatorio, int largura) {
  ftxui::Element quadro = tl::elemento_dos_requisitos(relatorio);
  // A altura vae FIXA e folgada, e não ajustada ao elemento: a altura que o
  // paragrafo pede só se sabe depois de se saber a largura em que elle reflue,
  // e pedi-la antes cortava o quadro no pé. Sobra de linhas em branco não
  // atrapalha asserção alguma; córte silencioso atrapalharia todas.
  ftxui::Screen ecran =
      ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                            ftxui::Dimension::Fixed(60));
  ftxui::Render(ecran, quadro);
  return ecran.ToString();
}

// larguras_visiveis — conta as COLLUNAS de cada linha, e não os bytes: as
// sequencias de escape não occupam célula, e caractere acentuado gasta dous
// bytes numa collunha só, e o FTXUI remata cada linha em \r\n, de sorte que o
// retorno de carro tambem se salta. Contar bytes daria falso alarme em toda
// linha que trouxesse um "ç", que são quasi todas nesta casa.
std::vector<std::size_t> larguras_visiveis(const std::string& pintura) {
  std::vector<std::size_t> larguras{0};
  for (std::size_t passo = 0; passo < pintura.size(); ++passo) {
    const unsigned char octeto = static_cast<unsigned char>(pintura[passo]);
    if (octeto == 0x1b) {  // salta a sequencia de escape até o seu remate
      while (passo < pintura.size() && pintura[passo] != 'm') ++passo;
      continue;
    }
    if (octeto == '\r') continue;  // o FTXUI remata a linha em \r\n
    if (octeto == '\n') {
      larguras.push_back(0);
      continue;
    }
    if ((octeto & 0xc0) != 0x80) ++larguras.back();  // não é continuação
  }
  return larguras;
}

}  // namespace

// A TELA ESTREITA, que é o caso que ninguem lembra e o que mais importa: tela
// de erro que quebra em quarenta collunas falha exactamente quando alguem está
// com problema. Afere-se em quarenta e em cem, e o que se exige é o mesmo.
TEST_CASE("o quadro cabe em quarenta collunas, e nada d'elle se perde") {
  const nu::Relatorio relatorio =
      nu::sondar(faltando({"fonte", "libmpv", "yt-dlp", "chafa", "ffmpeg"}));
  for (const int largura : {40, 100}) {
    const std::string pintura = pintar(relatorio, largura);
    for (const std::size_t medida : larguras_visiveis(pintura))
      CHECK(medida <= static_cast<std::size_t>(largura));
    for (const nu::Requisito& requisito : nu::requisitos())
      CHECK(pintura.find(std::string(requisito.nome.substr(0, 5))) !=
            std::string::npos);
    // Exigem-se as CAUDAS, e não sómente as cabeças: remedio cortado no pé
    // parece presente a quem procure a primeira palavra d'elle. Procuram-se
    // PALAVRAS soltas, e não frases: em quarenta collunas o paragrafo reflue, e
    // frase de varias palavras atravessa a quebra de linha, donde a busca por
    // ella falharia sobre texto que está inteiro na tela.
    CHECK(pintura.find("fc-cache") != std::string::npos);
    CHECK(pintura.find("capa)") != std::string::npos);
    CHECK(pintura.find("emulador.") != std::string::npos);
  }
}

// As côres são as da issue #2, e afferem-se pela sequencia que o écran emitte:
// impedimento em crit, aviso em warn, remedio em text_muted. O token entra por
// tui::tokens, e nunca por literal, de sorte que mudança na taboada apanhe a
// prova junto.
TEST_CASE("o impedimento veste crit, o aviso veste warn, o remedio muted") {
  const std::string pintura =
      pintar(nu::sondar(faltando({"libmpv", "chafa"})), 100);
  CHECK(pintura.find(tk::tinta(tk::crit)) != std::string::npos);
  CHECK(pintura.find(tk::tinta(tk::warn)) != std::string::npos);
  CHECK(pintura.find(tk::tinta(tk::text_muted)) != std::string::npos);
}

TEST_CASE("não havendo impedimento, o quadro não pede tecla alguma") {
  const std::string com = pintar(nu::sondar(faltando({"libmpv"})), 100);
  const std::string sem = pintar(nu::sondar(faltando({"chafa"})), 100);
  CHECK(com.find("tecle") != std::string::npos);
  CHECK(sem.find("tecle") == std::string::npos);
}

// O LIMITE da sonda ha de estar dito nos DOUS logares, e não em um: quem lê o
// quadro e quem lê o diagnostico merecem a mesma verdade.
TEST_CASE("o limite da sonda vem declarado no quadro pintado") {
  const nu::Relatorio relatorio = nu::sondar(faltando({"fonte"}));
  const std::string pintura = pintar(relatorio, 100);
  CHECK(pintura.find("emulador") != std::string::npos);
  CHECK(pintura.find("installada no systema") != std::string::npos);
}

TEST_CASE("o limite da sonda vem declarado no relatorio em texto") {
  const nu::Relatorio relatorio = nu::sondar(faltando({}));
  const std::string texto = tl::texto_do_relatorio(relatorio);
  CHECK(texto.find(std::string(tl::kLimiteDaSonda)) != std::string::npos);
}

TEST_CASE("o relatorio em texto diz todos os requisitos, presentes inclusos") {
  const std::string texto = tl::texto_do_relatorio(nu::sondar(faltando({})));
  for (const nu::Requisito& requisito : nu::requisitos())
    CHECK(texto.find(std::string(requisito.nome)) != std::string::npos);
  CHECK(texto.find("presente") != std::string::npos);
  CHECK(texto.find("FALTA") == std::string::npos);
}

// O texto do diagnostico é PURO: quem o redirija a arquivo não ha de encontrar
// escape algum, nem tela alternativa, nem côr.
TEST_CASE("o relatorio em texto não traz sequencia de escape alguma") {
  const std::string texto =
      tl::texto_do_relatorio(nu::sondar(faltando({"libmpv", "chafa"})));
  CHECK(texto.find('\x1b') == std::string::npos);
  CHECK(texto.find("FALTA") != std::string::npos);
  CHECK(texto.find("remedio") != std::string::npos);
}
